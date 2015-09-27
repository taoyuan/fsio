//
// Created by 陶源 on 15/9/27.
//

#include <nan.h>
#include <unistd.h>
#include "fsio.h"
#include "poller.h"


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

  char * buf = nullptr;

  // file descriptor
  int fd = info[0]->ToInt32()->Int32Value();

  // buffer
  v8::Local<v8::Object> buffer = info[1]->ToObject();
  char* bufferData = node::Buffer::Data(buffer);
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

  // callback
//  if(!info[3]->IsFunction()) {
//    Nan::ThrowTypeError("Forth argument must be a function");
//    info.GetReturnValue().SetUndefined();
//  }
//  v8::Local<v8::Function> callback = info[3].As<v8::Function>();

  uint32_t result = (uint32_t) read(fd, buf, len);

  info.GetReturnValue().Set(Nan::New(result));
}

NAN_METHOD(Write) {
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

  char * buf = nullptr;

  // file descriptor
  int fd = info[0]->ToInt32()->Int32Value();

  // buffer
  v8::Local<v8::Object> buffer = info[1]->ToObject();
  char* bufferData = node::Buffer::Data(buffer);
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

  // callback
//  if(!info[3]->IsFunction()) {
//    Nan::ThrowTypeError("Forth argument must be a function");
//    info.GetReturnValue().SetUndefined();
//  }
//  v8::Local<v8::Function> callback = info[3].As<v8::Function>();

  uint32_t result = (uint32_t) write(fd, buf, len);

  info.GetReturnValue().Set(Nan::New(result));
}

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
