//
// Created by 陶源 on 15/9/27.
//

#include <nan.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "helpers.h"
#include "fsio.h"

struct _WriteQueue {
    const int _fd; // the fd that is associated with this write queue
    QueuedWrite _write_queue;
    uv_mutex_t _write_queue_mutex;
    _WriteQueue *_next;

    _WriteQueue(const int fd) : _fd(fd), _write_queue(), _next(NULL) {
      uv_mutex_init(&_write_queue_mutex);
    }

    void lock() { uv_mutex_lock(&_write_queue_mutex); };

    void unlock() { uv_mutex_unlock(&_write_queue_mutex); };

    QueuedWrite &get() { return _write_queue; }
};


static _WriteQueue *write_queues = NULL;

static _WriteQueue *qForFD(const int fd) {
  _WriteQueue *q = write_queues;
  while (q != NULL) {
    if (q->_fd == fd) {
      return q;
    }
    q = q->_next;
  }
  return NULL;
};

static _WriteQueue *newQForFD(const int fd) {
  _WriteQueue *q = qForFD(fd);

  if (q == NULL) {
    if (write_queues == NULL) {
      write_queues = new _WriteQueue(fd);
      return write_queues;
    } else {
      q = write_queues;
      while (q->_next != NULL) {
        q = q->_next;
      }
      q->_next = new _WriteQueue(fd);
      return q->_next;
    }
  }

  return q;
};

static void deleteQForFD(const int fd) {
  if (write_queues == NULL)
    return;

  _WriteQueue *q = write_queues;
  if (write_queues->_fd == fd) {
    write_queues = write_queues->_next;
    delete q;

    return;
  }

  while (q->_next != NULL) {
    if (q->_next->_fd == fd) {
      _WriteQueue *out_q = q->_next;
      q->_next = q->_next->_next;
      delete out_q;

      return;
    }
    q = q->_next;
  }

  // It wasn't found...
};

void attach(int fd) {
  newQForFD(fd);
}

void detach(int fd) {
  _WriteQueue *q = qForFD(fd);
  if (q) {
    q->lock();
    QueuedWrite &write_queue = q->get();
    while (!write_queue.empty()) {
      QueuedWrite *del_q = write_queue.next;
      del_q->baton->buffer.Reset();
      del_q->remove();
    }
    q->unlock();

    deleteQForFD(fd);
  }
}


void write_async(int fd, Local<Object> buffer, size_t offset, size_t len, Local<Function> callback) {
  Nan::HandleScope scope;

  char *bufferData = node::Buffer::Data(buffer);
  size_t bufferLength = node::Buffer::Length(buffer);

  WriteBaton *baton = new WriteBaton();
  memset(baton, 0, sizeof(WriteBaton));
  baton->fd = fd;
  baton->buffer.Reset(buffer);
  baton->bufferData = bufferData;
  baton->bufferLength = bufferLength;
  baton->offset = 0;
  baton->callback = new Nan::Callback(callback);

  QueuedWrite *queuedWrite = new QueuedWrite();
  memset(queuedWrite, 0, sizeof(QueuedWrite));
  queuedWrite->baton = baton;
  queuedWrite->req.data = queuedWrite;

  _WriteQueue *q = qForFD(fd);
  if (!q) {
    return Nan::ThrowTypeError("There's no write queue for that file descriptor (write)!");
  }

  q->lock();
  QueuedWrite &write_queue = q->get();
  bool empty = write_queue.empty();

  write_queue.insert_tail(queuedWrite);

  if (empty) {
    uv_queue_work(uv_default_loop(), &queuedWrite->req, EIO_Write, (uv_after_work_cb) EIO_AfterWrite);
  }
  q->unlock();
}

void EIO_Write(uv_work_t *req) {
  QueuedWrite *queuedWrite = static_cast<QueuedWrite *>(req->data);
  WriteBaton *data = queuedWrite->baton;

  data->result = 0;
  errno = 0;

  if (!data->bufferLength) {
    data->result = (int) write(data->fd, NULL, 0);
    return;
  }

  // We carefully *DON'T* break out of this loop.
  do {
    if ((data->result = (int) write(data->fd, data->bufferData + data->offset, data->bufferLength - data->offset)) ==
        -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;

      // The write call might be interrupted, if it is we just try again immediately.
      if (errno != EINTR) {
        snprintf(data->errorString, sizeof(data->errorString), "Error %s calling write(...)", strerror(errno));
        return;
      }

      // try again...
      continue;
    }
      // there wasn't an error, do the math on what we actually wrote...
    else {
      data->offset += data->result;
    }

    // if we get there, we really don't want to loop
    // break;
  } while (data->bufferLength > data->offset);
}

void EIO_AfterWrite(uv_work_t *req) {
  Nan::HandleScope scope;

  QueuedWrite *queuedWrite = static_cast<QueuedWrite *>(req->data);
  WriteBaton *data = static_cast<WriteBaton *>(queuedWrite->baton);

  Local<Value> argv[2];
  if (data->errorString[0]) {
    argv[0] = Nan::Error(data->errorString);
    argv[1] = Nan::Undefined();
  } else {
    argv[0] = Nan::Undefined();
    argv[1] = Nan::New(data->result);
  }
  data->callback->Call(2, argv);

  if (data->offset < data->bufferLength && !data->errorString[0]) {
    // We're not done with this baton, so throw it right back onto the queue.
    // Don't re-push the write in the event loop if there was an error; because same error could occur again!
    // TODO: Add a uv_poll here for unix...
    //fprintf(stderr, "Write again...\n");
    uv_queue_work(uv_default_loop(), req, EIO_Write, (uv_after_work_cb) EIO_AfterWrite);
    return;
  }

  int fd = data->fd;
  _WriteQueue *q = qForFD(fd);
  if (!q) {
    return Nan::ThrowTypeError("There's no write queue for that file descriptor (after write)!");
  }

  q->lock();
  QueuedWrite &write_queue = q->get();

  // remove this one from the list
  queuedWrite->remove();

  // If there are any left, start a new thread to write the next one.
  if (!write_queue.empty()) {
    // Always pull the next work item from the head of the queue
    QueuedWrite *nextQueuedWrite = write_queue.next;
    uv_queue_work(uv_default_loop(), &nextQueuedWrite->req, EIO_Write, (uv_after_work_cb) EIO_AfterWrite);
  }
  q->unlock();

  data->buffer.Reset();
  delete data->callback;
  delete data;
  delete queuedWrite;
}

NAN_METHOD(Open) {
  Nan::HandleScope scope;

  char *path = NULL;
  int oflag;

  STRING_ARG(path, 0);
  INT_ARG(oflag, 1);

  int rc = open(path, oflag);
  if (rc < 0) {
    throwErrnoError();
  }
  info.GetReturnValue().Set(Nan::New(rc));
}

NAN_METHOD(Close) {
  Nan::HandleScope scope;

  int fd;

  INT_ARG(fd, 0);

  int rc = close(fd);
  info.GetReturnValue().Set(Nan::New(rc));
}

void initConstants(Handle<Object> target) {
  NODE_DEFINE_CONSTANT(target, O_RDONLY);
  NODE_DEFINE_CONSTANT(target, O_WRONLY);
  NODE_DEFINE_CONSTANT(target, O_RDWR);
  NODE_DEFINE_CONSTANT(target, O_ACCMODE);

  NODE_DEFINE_CONSTANT(target, O_CLOEXEC);
}


extern "C" {
void init(v8::Handle<v8::Object> target) {
  Nan::HandleScope scop;

  Nan::SetMethod(target, "open", Open);
  Nan::SetMethod(target, "close", Close);

  Socket::Init(target);

  initConstants(target);
}
}

NODE_MODULE(fsio, init);
