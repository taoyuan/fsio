#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <node_buffer.h>
#include <nan.h>
#include "Socket.h"

using namespace v8;

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
  Nan::SetPrototypeMethod(ctor, "write", Write);

  target->Set(Nan::New("Socket").ToLocalChecked(), ctor->GetFunction());
}

Socket::Socket(int fd) : _fd(fd),
  node::ObjectWrap() {

  uv_poll_init(uv_default_loop(), &this->_pollHandle, this->_fd);

  this->_pollHandle.data = this;
}

Socket::~Socket() {
  uv_close((uv_handle_t*)&this->_pollHandle, (uv_close_cb)Socket::PollCloseCallback);
}

void Socket::start() {
  uv_poll_start(&this->_pollHandle, UV_READABLE, Socket::PollCallback);
}

void Socket::poll() {
  Nan::HandleScope scope;

  int length = 0;
  char data[1024];

  length = (int) read(this->_fd, data, sizeof(data));

  if (length > 0) {
    Local<Value> argv[2] = {
      Nan::New("data").ToLocalChecked(),
      Nan::CopyBuffer(data, (uint32_t) length).ToLocalChecked()
    };

    Nan::MakeCallback(Nan::New<Object>(this->This), Nan::New("emit").ToLocalChecked(), 2, argv);
  }
}

void Socket::stop() {
  uv_poll_stop(&this->_pollHandle);
}

void Socket::write_(char* data, int length) {
  if (write(this->_fd, data, length) < 0) {
    this->emitErrnoError();
  }
}

void Socket::emitErrnoError() {
  Nan::HandleScope scope;

  Local<Object> globalObj = Nan::GetCurrentContext()->Global();
  Local<Function> errorConstructor = Local<Function>::Cast(globalObj->Get(Nan::New("Error").ToLocalChecked()));

  Local<Value> constructorArgs[1] = {
    Nan::New(strerror(errno)).ToLocalChecked()
  };

  Local<Value> error = errorConstructor->NewInstance(1, constructorArgs);

  Local<Value> argv[2] = {
    Nan::New("error").ToLocalChecked(),
    error
  };

  Nan::MakeCallback(Nan::New<Object>(this->This), Nan::New("emit").ToLocalChecked(), 2, argv);
}

NAN_METHOD(Socket::New) {
  Nan::HandleScope scope;

  if (!info[0]->IsInt32() && !info[0]->IsUint32()) {
    return Nan::ThrowTypeError("fd is required");
  }
  int fd = info[0]->Int32Value();

  Socket* p = new Socket(fd);
  p->Wrap(info.This());
  p->This.Reset(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Socket::Start) {
  Nan::HandleScope scope;

  Socket* p = node::ObjectWrap::Unwrap<Socket>(info.This());

  p->start();

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Socket::Stop) {
  Nan::HandleScope scope;

  Socket* p = node::ObjectWrap::Unwrap<Socket>(info.This());

  p->stop();

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Socket::Write) {
  Nan::HandleScope scope;
  Socket* p = node::ObjectWrap::Unwrap<Socket>(info.This());

  if (info.Length() > 0) {
    Local<Value> arg0 = info[0];
    if (arg0->IsObject()) {

      p->write_(node::Buffer::Data(arg0), node::Buffer::Length(arg0));
    }
  }

  info.GetReturnValue().SetUndefined();
}


void Socket::PollCloseCallback(uv_poll_t* handle) {
  delete handle;
}

void Socket::PollCallback(uv_poll_t* handle, int status, int events) {
  Socket *p = (Socket*)handle->data;

  p->poll();
}
