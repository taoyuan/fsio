//
// Created by 陶源 on 15/10/8.
//

#ifndef FSIO_ERRORS_H
#define FSIO_ERRORS_H

#include <nan.h>
#include <errno.h>

using namespace v8;

inline static void THROW_ERRNO_ERROR() {
  Nan::HandleScope scope;

  Local<Object> globalObj = Nan::GetCurrentContext()->Global();
  Local<Function> errorConstructor = Local<Function>::Cast(globalObj->Get(Nan::New("Error").ToLocalChecked()));

  Local<Value> constructorArgs[1] = {
    Nan::New(strerror(errno)).ToLocalChecked()
  };

  Local<Value> error = errorConstructor->NewInstance(1, constructorArgs);

  Nan::ThrowError(error);

//  Local<Value> argv[2] = {
//    Nan::New("error").ToLocalChecked(),
//    error
//  };
//
//  Nan::MakeCallback(Nan::New<Object>(this->This), Nan::New("emit").ToLocalChecked(), 2, argv);
}


#endif //FSIO_ERRORS_H
