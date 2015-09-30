#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>
#include "fsio.h"
#include "helpers.h"

using namespace v8;
using namespace node;

Nan::Persistent<FunctionTemplate> Socket::constructor_template;

void Socket::Init(Handle<Object> target) {
  Nan::HandleScope scope;

  // Prepare constructor template
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  constructor_template.Reset(ctor);

  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(Nan::New("Socket").ToLocalChecked());

  // Prototype
  Nan::SetPrototypeMethod(ctor, "start", Start);
  Nan::SetPrototypeMethod(ctor, "stop", Stop);
  Nan::SetPrototypeMethod(ctor, "read", Read);
  Nan::SetPrototypeMethod(ctor, "write", Write);

  target->Set(Nan::New("Socket").ToLocalChecked(), ctor->GetFunction());
}

Socket::Socket(int fd): _fd(fd),
  node::ObjectWrap() {

  uv_poll_init(uv_default_loop(), &_poll_handle, _fd);
  _poll_handle.data = this;

  attach(_fd);
}

Socket::~Socket() {
  delete _callback;
  detach(_fd);
  uv_close((uv_handle_t *) &_poll_handle, (uv_close_cb) Socket::PollCloseCallback);
}

void Socket::start() {
  uv_poll_start(&this->_poll_handle, UV_READABLE, Socket::PollCallback);
}

void Socket::poll() {
  Nan::HandleScope scope;

  int length = 0;
  char data[1024];

  length = (int) read(this->_fd, data, sizeof(data));

  if (!_callback->IsEmpty() && length > 0) {
    Local<Value> argv[1] = {
      Nan::CopyBuffer(data, (uint32_t) length).ToLocalChecked()
    };

    Nan::TryCatch tc;
    _callback->Call(1, argv);
    if (tc.HasCaught()) {
      Nan::FatalException(tc);
    }
  }
}

void Socket::stop() {
  uv_poll_stop(&this->_poll_handle);
}

int Socket::_read(char *data, size_t length) {
  int result = (int) read(this->_fd, data, length);
  if (result < 0) {
    throwErrnoError();
  }
  return result;
}

int Socket::_write(char *data, size_t length) {
  int result = (int) write(this->_fd, data, length);
  if (result < 0) {
    throwErrnoError();
  }
  return result;
}

NAN_METHOD(Socket::New) {
  ENTER_CONSTRUCTOR(1);

  int fd;
  INT_ARG(fd, 0);
  CALLBACK_ARG(1);

  Socket *p = new Socket(fd);
  p->_callback = new Nan::Callback(callback);

  p->Wrap(info.This());
  p->This.Reset(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Socket::Start) {
  ENTER_METHOD(Socket, 0)

  Socket *p = node::ObjectWrap::Unwrap<Socket>(info.This());

  p->start();

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Socket::Stop) {
  ENTER_METHOD(Socket, 0)

  Socket *p = node::ObjectWrap::Unwrap<Socket>(info.This());

  p->stop();

  info.GetReturnValue().SetUndefined();
}


NAN_METHOD(Socket::Read) {
  ENTER_METHOD(Socket, 3)

  char *buf = NULL;

  Local<Object> buffer;
  size_t offset, length;

  BUFFER_ARG(buffer, 0)
  INT_ARG(offset, 1)
  INT_ARG(length, 2)

  // buffer
  char *bufferData = node::Buffer::Data(buffer);
  size_t bufferLength = node::Buffer::Length(buffer);

  // offset
  if (offset > bufferLength) {
    return Nan::ThrowError("Offset is out of bounds");
  }

  // length
  if (!Buffer::IsWithinBounds(0, length, bufferLength - offset)) {
    return Nan::ThrowRangeError("Length extends beyond buffer");
  }

  buf = bufferData + offset;

  DEBUG_LOG("Reading {offset:%d, length:%d, buffer:%p}", offset, length, buf);

  Socket *p = node::ObjectWrap::Unwrap<Socket>(info.This());

  int result = p->_read(length ? buf : 0, length);

  info.GetReturnValue().Set(Nan::New(result));
}

NAN_METHOD(Socket::Write) {
  ENTER_METHOD(Socket, 3)

  char *buf = NULL;

  Local<Object> buffer;
  size_t offset, length;

  BUFFER_ARG(buffer, 0)
  INT_ARG(offset, 1)
  INT_ARG(length, 2)
  CALLBACK_ARG(3);

  // buffer
  char *bufferData = node::Buffer::Data(buffer);
  size_t bufferLength = node::Buffer::Length(buffer);

  // offset
  if (offset > bufferLength) {
    return Nan::ThrowError("Offset is out of bounds");
  }

  // length
  if (!Buffer::IsWithinBounds(0, length, bufferLength - offset)) {
    return Nan::ThrowRangeError("Length extends beyond buffer");
  }

  buf = bufferData + offset;

  DEBUG_LOG("Writing {offset:%d, length:%d, buffer:%p}", offset, length, buf);

  Socket *p = node::ObjectWrap::Unwrap<Socket>(info.This());

  if (has_callback) {
    DEBUG_LOG("Write in async");
    write_async(that->_fd, buffer, offset, length, callback);
    info.GetReturnValue().SetUndefined();
  } else {
    DEBUG_LOG("Write in sync");
    int result = p->_write(length ? buf : 0, length);
    info.GetReturnValue().Set(Nan::New(result));
  }
}


void Socket::PollCloseCallback(uv_poll_t *handle) {
  delete handle;
}

void Socket::PollCallback(uv_poll_t *handle, int status, int events) {
  Socket *p = (Socket *) handle->data;

  p->poll();
}
