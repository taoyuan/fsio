//
// Created by 陶源 on 15/9/29.
//

#ifndef FSIO_SOCKET_H
#define FSIO_SOCKET_H

#include <node.h>

#include <nan.h>

class Socket : public node::ObjectWrap {

public:
    static void Init(v8::Handle<v8::Object> target);

    static NAN_METHOD(New);
    static NAN_METHOD(Start);
    static NAN_METHOD(Stop);
    static NAN_METHOD(Write);

private:
    Socket(int fd);
    ~Socket();

    void start();
    void stop();

    void write_(char* data, int length);

    void poll();

    void emitErrnoError();

    static void PollCloseCallback(uv_poll_t* handle);
    static void PollCallback(uv_poll_t* handle, int status, int events);

private:
    Nan::Persistent<v8::Object> This;

    int _fd;
    uv_poll_t _pollHandle;

    static Nan::Persistent<v8::FunctionTemplate> constructor_template;
};

#endif //FSIO_SOCKET_H
