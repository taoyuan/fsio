// Copyright (C) 2013 Robert Giseburt <giseburt@gmail.com>
// serialport_poller.cpp Written as a part of https://github.com/voodootikigod/node-fsio
// License to use this is the same as that of node-fsio.

#include <nan.h>
#include "poller.h"

using namespace v8;
using namespace node;

Nan::Persistent<v8::Function> fsio_constructor;

FsioPoller::FsioPoller() : ObjectWrap() { };

FsioPoller::~FsioPoller() {
  // printf("~FsioPoller\n");
  delete callback_;
};

void _fsioReadable(uv_poll_t *req, int status, int events) {
  FsioPoller *sp = (FsioPoller *) req->data;
  // We can stop polling until we have read all of the data...
  sp->_stop();
  sp->callCallback(status);
}

void FsioPoller::callCallback(int status) {
  Nan::HandleScope scope;
  // uv_work_t* req = new uv_work_t;

  // Call the callback to go read more data...

  v8::Local<v8::Value> argv[1];
  if (status != 0) {
    // error handling changed in libuv, see:
    // https://github.com/joyent/libuv/commit/3ee4d3f183331
#ifdef UV_ERRNO_H_
    const char *err_string = uv_strerror(status);
#else
    uv_err_t errno = uv_last_error(uv_default_loop());
    const char* err_string = uv_strerror(errno);
#endif
    snprintf(this->errorString, sizeof(this->errorString), "Error %s on polling", err_string);
    argv[0] = v8::Exception::Error(Nan::New(this->errorString).ToLocalChecked());
  } else {
    argv[0] = Nan::Undefined();
  }

  callback_->Call(1, argv);
}


void FsioPoller::Init(Handle<Object> target) {
  Nan::HandleScope scope;

  // Prepare constructor template
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->SetClassName(Nan::New("FsioPoller").ToLocalChecked());
  ctor->InstanceTemplate()->SetInternalFieldCount(1);


  // Prototype

  // FsioPoller.start()
  Nan::SetPrototypeMethod(ctor, "start", Start);
  // FsioPoller.close()
  Nan::SetPrototypeMethod(ctor, "close", Close);

  fsio_constructor.Reset(ctor->GetFunction());

  Nan::Set(target, Nan::New("FsioPoller").ToLocalChecked(), ctor->GetFunction());
}

NAN_METHOD(FsioPoller::New) {
  Nan::HandleScope scope;

  if (!info[0]->IsInt32()) {
    Nan::ThrowTypeError("First argument must be an fd");
    info.GetReturnValue().SetUndefined();
  }

  if (!info[1]->IsFunction()) {
    Nan::ThrowTypeError("Third argument must be a function");
    info.GetReturnValue().SetUndefined();
  }

  FsioPoller *obj = new FsioPoller();
  obj->fd_ = info[0]->ToInt32()->Int32Value();
  obj->callback_ = new Nan::Callback(info[1].As<v8::Function>());
  // obj->callCallback();

  obj->Wrap(info.This());

  obj->poll_handle_.data = obj;
/*int r = */uv_poll_init(uv_default_loop(), &obj->poll_handle_, obj->fd_);

  uv_poll_start(&obj->poll_handle_, UV_READABLE, _fsioReadable);

  info.GetReturnValue().Set(info.This());
}

void FsioPoller::_start() {
  uv_poll_start(&poll_handle_, UV_READABLE, _fsioReadable);
}

void FsioPoller::_stop() {
  uv_poll_stop(&poll_handle_);
}


NAN_METHOD(FsioPoller::Start) {
  Nan::HandleScope scope;

  FsioPoller *obj = ObjectWrap::Unwrap<FsioPoller>(info.This());
  obj->_start();

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(FsioPoller::Close) {
  Nan::HandleScope scope;

  FsioPoller *obj = ObjectWrap::Unwrap<FsioPoller>(info.This());
  obj->_stop();

  // DO SOMETHING!

  info.GetReturnValue().SetUndefined();
}
