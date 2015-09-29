//
// Created by 陶源 on 15/9/27.
//

#ifndef FSIO_H
#define FSIO_H

//#define DEBUG

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

    int _write(char *data, size_t length);

    void poll();

    void throwErrnoError();

    static void PollCloseCallback(uv_poll_t* handle);
    static void PollCallback(uv_poll_t* handle, int status, int events);

private:
    Nan::Persistent<v8::Object> This;

    int _fd;
    uv_poll_t _poll_handle;
    Nan::Callback* _callback;

    static Nan::Persistent<v8::FunctionTemplate> constructor_template;
};

#ifdef DEBUG
#define DEBUG_HEADER fprintf(stderr, "fsio [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__);
  #define DEBUG_FOOTER fprintf(stderr, "\n");
  #define DEBUG_LOG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
#else
#define DEBUG_LOG(...)
#endif


#endif //FSIO_H
