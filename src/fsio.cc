//
// Created by 陶源 on 15/9/27.
//

#include <nan.h>
#include <unistd.h>
#include <errno.h>
#include <sstream>
#include "fsio.h"
#include "poller.h"

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

NAN_METHOD(Read) {
  Nan::HandleScope scope;

  if (info.Length() < 2) {
    Nan::ThrowTypeError("fd and buffer are required");
    info.GetReturnValue().SetUndefined();
  }
  if (!info[0]->IsInt32()) {
    Nan::ThrowTypeError("fd must be a file descriptor");
    info.GetReturnValue().SetUndefined();
  }
  if (!node::Buffer::HasInstance(info[1])) {
    Nan::ThrowTypeError("Second argument needs to be a buffer");
    info.GetReturnValue().SetUndefined();
  }

  char *buf = NULL;

  // file descriptor
  int fd = info[0]->ToInt32()->Int32Value();

  // buffer
  v8::Local<v8::Object> buffer = info[1]->ToObject();
  char *bufferData = node::Buffer::Data(buffer);
  size_t bufferLength = node::Buffer::Length(buffer);

  // offset
  size_t offset = info[2]->Uint32Value();
  if (offset >= bufferLength) {
    Nan::ThrowError("Offset is out of bounds");
    info.GetReturnValue().SetUndefined();
  }

  // length
  size_t len = info[3]->ToInt32()->Uint32Value();
  if (!node::Buffer::IsWithinBounds(offset, len, bufferLength)) {
    Nan::ThrowRangeError("Length extends beyond buffer");
  }

  buf = bufferData + offset;

  uint32_t result = (uint32_t) read(fd, buf, len);
  info.GetReturnValue().Set(Nan::New(result));
}

NAN_METHOD(Write) {
  Nan::HandleScope scope;

  if (info.Length() < 2) {
    return Nan::ThrowTypeError("fd and buffer are required");
  }
  if (!info[0]->IsInt32()) {
    return Nan::ThrowTypeError("fd must be a file descriptor");
  }
  if (!node::Buffer::HasInstance(info[1])) {
    return Nan::ThrowTypeError("Second argument needs to be a buffer");
  }

  char *buf = NULL;

  // file descriptor
  int fd = info[0]->ToInt32()->Int32Value();

  // buffer
  v8::Local<v8::Object> buffer = info[1]->ToObject();
  char *bufferData = node::Buffer::Data(buffer);
  size_t bufferLength = node::Buffer::Length(buffer);

  // offset
  size_t offset = info[2]->Uint32Value();
  if (offset >= bufferLength) {
    return Nan::ThrowError("Offset is out of bounds");
  }

  // length
  size_t len = info[3]->ToInt32()->Uint32Value();
  if (!node::Buffer::IsWithinBounds(offset, len, bufferLength)) {
    return Nan::ThrowRangeError("Length extends beyond buffer");
  }

  buf = bufferData + offset;

  // callback

#ifdef ENABLE_ASYNC
  if (info.Length() > 4 && info[4]->IsFunction()) { // async
    Nan::Callback *callback = new Nan::Callback(info[4].As<v8::Function>());

    WriteBaton *baton = new WriteBaton();
    memset(baton, 0, sizeof(WriteBaton));
    baton->fd = fd;
//    NanAssignPersistent<v8::Object>(baton->buffer, buffer);
    baton->buffer.Reset(buffer);
    baton->bufferData = bufferData;
    baton->bufferLength = bufferLength;
    baton->offset = 0;
    baton->callback = callback;

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

    info.GetReturnValue().SetUndefined();
  } else { // sync
#endif
    uint32_t result = (uint32_t) write(fd, buf, len);

    if (result < 0) {
      using namespace std;
      stringstream s;
      s << "Fail on write  " << errno << "  " << strerror(errno) << ".";
      return Nan::ThrowError(s.str().c_str());
    }

    info.GetReturnValue().Set(Nan::New(result));
#ifdef ENABLE_ASYNC
  }
#endif
}

#ifdef ENABLE_ASYNC


///////////////////////////////////////
// UNIX version
///////////////////////////////////////


void EIO_Write(uv_work_t *req) {
  QueuedWrite *queuedWrite = static_cast<QueuedWrite *>(req->data);
  WriteBaton *data = queuedWrite->baton;

  data->result = 0;
  errno = 0;

  // We carefully *DON'T* break out of this loop.
  do {
    if ((data->result = write(data->fd, data->bufferData + data->offset, data->bufferLength - data->offset)) == -1) {
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

  v8::Local<v8::Value> argv[2];
  if (data->errorString[0]) {
    argv[0] = v8::Exception::Error(Nan::New(data->errorString).ToLocalChecked());
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

  // dispose buffer
  data->buffer.Reset();

  delete data->callback;
  delete data;
  delete queuedWrite;
}

#endif

extern "C" {
void init(v8::Handle<v8::Object> target) {
  Nan::HandleScope scop;

  Nan::SetMethod(target, "read", Read);
  Nan::SetMethod(target, "write", Write);

#ifndef WIN32
  FsioPoller::Init(target);
#endif
}
}

NODE_MODULE(fsio, init);
