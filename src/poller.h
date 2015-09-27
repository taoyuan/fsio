#ifndef FSIO_POLLER_H
#define FSIO_POLLER_H

#include <nan.h>
#include "fsio.h"

class FsioPoller : public node::ObjectWrap {
public:
    static void Init(v8::Handle<v8::Object> target);

    void callCallback(int status);

    void _start();

    void _stop();

private:
    FsioPoller();

    ~FsioPoller();

    static NAN_METHOD(New);

    static NAN_METHOD(Close);

    static NAN_METHOD(Start);

    uv_poll_t poll_handle_;
    int fd_;
    char errorString[ERROR_STRING_SIZE];

    Nan::Callback *callback_;
};

#endif
