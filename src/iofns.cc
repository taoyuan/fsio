//
// Created by 陶源 on 15/10/8.
//

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "errors.h"
#include "helpers.h"
#include "iofns.h"

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

void fsio_attach(int fd) {
  newQForFD(fd);
}

void fsio_detach(int fd) {
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

int fsio_read(int fd, Local<Object> buffer, size_t offset, size_t length, Local<Function> callback) {
  return fsio_read(fd, buffer, offset, length, new Nan::Callback(callback));
}

int fsio_read(int fd, Local<Object> buffer, size_t offset, size_t length, Nan::Callback * callback) {
  Nan::HandleScope scope;

  char *bufferData = node::Buffer::Data(buffer);
  size_t bufferLength = node::Buffer::Length(buffer);

  if (!callback || callback->IsEmpty()) { // sync
    int rc = (int) read(fd, bufferData + offset, length);
    if (rc < 0) {
      THROW_ERRNO_ERROR();
    }
    return rc;
  }

  // async

  WriteBaton *baton = new WriteBaton();
  memset(baton, 0, sizeof(WriteBaton));
  baton->fd = fd;
  baton->buffer.Reset(buffer);
  baton->bufferData = bufferData;
  baton->bufferLength = bufferLength;
  baton->offset = offset;
  baton->length = length;
  baton->callback = callback;

  uv_work_t* req = new uv_work_t();
  req->data = baton;

  uv_queue_work(uv_default_loop(), req, __fsio_eio_read, (uv_after_work_cb) __fsio_eio_read_done);

}

void __fsio_eio_read_done(uv_work_t *req) {
  Nan::HandleScope scope;

  ReadBaton *data = static_cast<ReadBaton *>(req->data);

  Local<Value> argv[2];
  if (data->errmsg[0]) {
    argv[0] = Nan::Error(data->errmsg);
    argv[1] = Nan::Undefined();
  } else {
    argv[0] = Nan::Undefined();
    argv[1] = Nan::New(data->result);
  }

  DEBUG_LOG("read done (fd: %d, result: %d)", data->fd, data->result);
  data->callback->Call(2, argv);

  data->buffer.Reset();
  delete data->callback;
  delete data;
  delete req;
}

int fsio_write(int fd, Local<Object> buffer, size_t offset, size_t length, Nan::Callback *callback) {
  Nan::HandleScope scope;

  char *bufferData = node::Buffer::Data(buffer);
  size_t bufferLength = node::Buffer::Length(buffer);

  if (!callback || callback->IsEmpty()) { // sync
    int rc = (int) write(fd, bufferData + offset, length);
    if (rc < 0) {
      THROW_ERRNO_ERROR();
    }
    return rc;
  }

  WriteBaton *baton = new WriteBaton();
  memset(baton, 0, sizeof(WriteBaton));
  baton->fd = fd;
  baton->buffer.Reset(buffer);
  baton->bufferData = bufferData;
  baton->bufferLength = bufferLength;
  baton->offset = offset;
  baton->length = length;
  baton->callback = callback;

  QueuedWrite *queuedWrite = new QueuedWrite();
  memset(queuedWrite, 0, sizeof(QueuedWrite));
  queuedWrite->baton = baton;
  queuedWrite->req.data = queuedWrite;

  _WriteQueue *q = qForFD(fd);
  if (!q) {
    q = newQForFD(fd);
    DEBUG_LOG("There's no write queue for that file descriptor (write), create.", fd);
//    Nan::ThrowTypeError("There's no write queue for that file descriptor (write)!");
//    return -1;
  }

  q->lock();
  QueuedWrite &write_queue = q->get();
  bool empty = write_queue.empty();

  write_queue.insert_tail(queuedWrite);

  if (empty) {
    uv_queue_work(uv_default_loop(), &queuedWrite->req, __fsio_eio_write, (uv_after_work_cb) __fsio_eio_write_done);
  }
  q->unlock();
}

int fsio_write(int fd, Local<Object> buf, size_t offset, size_t length, Local<Function> callback) {
  return fsio_write(fd, buf, offset, length, new Nan::Callback(callback));
}

void __fsio_eio_write_done(uv_work_t *req) {
  Nan::HandleScope scope;

  QueuedWrite *queuedWrite = static_cast<QueuedWrite *>(req->data);
  WriteBaton *data = static_cast<WriteBaton *>(queuedWrite->baton);

  Local<Value> argv[2];
  if (data->errmsg[0]) {
    argv[0] = Nan::Error(data->errmsg);
    argv[1] = Nan::Undefined();
  } else {
    argv[0] = Nan::Undefined();
    argv[1] = Nan::New(data->result);
  }

  DEBUG_LOG("write done (fd: %d, length: %d)", data->fd, data->result);
  data->callback->Call(2, argv);

  if (!data->bufferLength && data->length && !data->errmsg[0]) {
    // We're not done with this baton, so throw it right back onto the queue.
    // Don't re-push the write in the event loop if there was an error; because same error could occur again!
    // TODO: Add a uv_poll here for unix...
    //fprintf(stderr, "Write again...\n");
    uv_queue_work(uv_default_loop(), req, __fsio_eio_write, (uv_after_work_cb) __fsio_eio_write_done);
    return;
  }

  int fd = data->fd;
  _WriteQueue *q = qForFD(fd);
  if (!q) {
    return Nan::ThrowTypeError("There's no write queue for that file descriptor (write done)!");
  }

  bool empty;
  q->lock();
  QueuedWrite &write_queue = q->get();

  // remove this one from the list
  queuedWrite->remove();

  // If there are any left, start a new thread to write the next one.
  if (!(empty = write_queue.empty())) {
    // Always pull the next work item from the head of the queue
    QueuedWrite *nextQueuedWrite = write_queue.next;
    uv_queue_work(uv_default_loop(), &nextQueuedWrite->req, __fsio_eio_write,
                  (uv_after_work_cb) __fsio_eio_write_done);
  }
  q->unlock();

  // remove if empty
  if (empty) {
    DEBUG_LOG("The write queue for fd %d is empty (write done), delete", fd);
    deleteQForFD(fd);
  }

  data->buffer.Reset();
  delete data->callback;
  delete data;
  delete queuedWrite;
}

NAN_METHOD(Open) {
  Nan::HandleScope scope;

  String::Utf8Value path(info[0]->ToString());

  int oflag;
  INT_ARG(oflag, 1);

  int rc = open(*path, oflag);
  if (rc < 0) {
    return THROW_ERRNO_ERROR();
  }

  DEBUG_LOG("opened fd -> %d (path: '%s', oflag: %d)", rc, *path, oflag);
  info.GetReturnValue().Set(Nan::New(rc));
}

NAN_METHOD(Close) {
  Nan::HandleScope scope;

  int fd;

  INT_ARG(fd, 0);

  int rc = close(fd);
  info.GetReturnValue().Set(Nan::New(rc));
}

NAN_METHOD(Poll) {
  Nan::HandleScope scope;

  int fd;
  short events, timeout;
  INT_ARG(fd, 0);
  INT_ARG(events, 1);
  INT_ARG(timeout, 2);

  struct pollfd fds;
  fds.fd = fd;
  fds.events = events;
  fds.revents = 0;

  int rc = poll(&fds, 1, timeout);

//  fprintf(stderr, "rc %d", rc);
  if (rc < 0) { // 0 no data; < 0 is error
    return THROW_ERRNO_ERROR();
  }

  info.GetReturnValue().Set(Nan::New(rc ? fds.revents : 0));
}

NAN_METHOD(Read) {
  Nan::HandleScope scope;

  int fd;

  Local<Object> buffer;
  size_t offset, length;

  INT_ARG(fd, 0);
  BUFFER_ARG(buffer, 1);
  INT_ARG(offset, 2);
  INT_ARG(length, 3);
  NAN_CALLBACK_ARG(4);

  // buffer
  size_t bufferLength = Buffer::Length(buffer);

  // offset
  if (offset > bufferLength) {
    return Nan::ThrowError("Offset is out of bounds");
  }

  // length
  if (!Buffer::IsWithinBounds(0, length, bufferLength - offset)) {
    return Nan::ThrowRangeError("Length extends beyond buffer");
  }

  int result = fsio_read(fd, buffer, offset, length, callback);

  if (callback && !callback->IsEmpty()) {
    info.GetReturnValue().SetUndefined();
  } else {
    info.GetReturnValue().Set(Nan::New(result));
  }
}

NAN_METHOD(Write) {
  Nan::HandleScope scope;

  int fd;

  Local<Object> buffer;
  size_t offset, length;

  INT_ARG(fd, 0);
  BUFFER_ARG(buffer, 1);
  INT_ARG(offset, 2);
  INT_ARG(length, 3);
  NAN_CALLBACK_ARG(4);

  // buffer
  size_t bufferLength = Buffer::Length(buffer);

  // offset
  if (offset > bufferLength) {
    return Nan::ThrowError("Offset is out of bounds");
  }

  // length
  if (!Buffer::IsWithinBounds(0, length, bufferLength - offset)) {
    return Nan::ThrowRangeError("Length extends beyond buffer");
  }

  int result = fsio_write(fd, buffer, offset, length, callback);

  if (callback && !callback->IsEmpty()) {
    info.GetReturnValue().SetUndefined();
  } else {
    info.GetReturnValue().Set(Nan::New(result));
  }
}


void initConstants(Handle<Object> target) {
  NODE_DEFINE_CONSTANT(target, O_RDONLY);
  NODE_DEFINE_CONSTANT(target, O_WRONLY);
  NODE_DEFINE_CONSTANT(target, O_RDWR);
  NODE_DEFINE_CONSTANT(target, O_ACCMODE);

  NODE_DEFINE_CONSTANT(target, O_CLOEXEC);

  /* POLL Constants */
  NODE_DEFINE_CONSTANT(target, POLLIN);
  NODE_DEFINE_CONSTANT(target, POLLPRI);
  NODE_DEFINE_CONSTANT(target, POLLOUT);
  NODE_DEFINE_CONSTANT(target, POLLRDNORM);
  NODE_DEFINE_CONSTANT(target, POLLWRNORM);
  NODE_DEFINE_CONSTANT(target, POLLRDBAND);
  NODE_DEFINE_CONSTANT(target, POLLWRBAND);

  NODE_DEFINE_CONSTANT(target, POLLERR);
  NODE_DEFINE_CONSTANT(target, POLLHUP);
  NODE_DEFINE_CONSTANT(target, POLLNVAL);
}

void InitIOFns(Handle<Object> target) {
  Nan::HandleScope scop;

  Nan::SetMethod(target, "open", Open);
  Nan::SetMethod(target, "close", Close);

  Nan::SetMethod(target, "poll", Poll);

  Nan::SetMethod(target, "read", Read);
  Nan::SetMethod(target, "write", Write);

  initConstants(target);
}
