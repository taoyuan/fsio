//
// Created by 陶源 on 15/9/27.
//

#include <nan.h>
#include "Socket.h"
#include "fsio.h"

extern "C" {
void init(v8::Handle<v8::Object> target) {
  Nan::HandleScope scop;
  Socket::Init(target);
}
}

NODE_MODULE(fsio, init);
