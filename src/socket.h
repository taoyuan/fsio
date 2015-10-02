#ifndef FSIO_SOCKET_H
#define FSIO_SOCKET_H

#include <nan.h>

class Socket : public node::ObjectWrap {

public:
    static void Init(Handle<Object> target);

    static NAN_METHOD(New);
    static NAN_METHOD(Start);
    static NAN_METHOD(Stop);
    static NAN_METHOD(Read);
    static NAN_METHOD(Write);

private:
    Socket(int fd);
    ~Socket();

    void start();
    void stop();

    int _read(char *data, size_t length);
    int _write(char *data, size_t length);

    void poll();

    static void PollCloseCallback(uv_poll_t* handle);
    static void PollCallback(uv_poll_t* handle, int status, int events);

private:
    Nan::Persistent<v8::Object> This;

    int _fd;
    uv_poll_t _poll_handle;
    Nan::Callback* _callback;

    static Nan::Persistent<v8::FunctionTemplate> constructor_template;
};

#endif
