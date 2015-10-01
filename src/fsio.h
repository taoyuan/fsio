//
// Created by 陶源 on 15/9/27.
//

#ifndef FSIO_H
#define FSIO_H

#include <node.h>

using namespace v8;
using namespace node;

//#define DEBUG

#define ERROR_STRING_SIZE 1024

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

void attach(int fd);
void detach(int fd);

NAN_METHOD(Open);
NAN_METHOD(Close);

NAN_METHOD(Poll);

void write_async(int fd, Local<Object> buf, size_t offset, size_t length, Local<Function> callback);
void EIO_Write(uv_work_t* req);
void EIO_AfterWrite(uv_work_t* req);



struct WriteBaton {
public:
    int fd;
    char* bufferData;
    size_t bufferLength;
    size_t offset;
    size_t length;
    Nan::Persistent<v8::Object> buffer;
    Nan::Callback* callback;
    int result;
    char errorString[ERROR_STRING_SIZE];
};

struct QueuedWrite {
public:
    uv_work_t req;
    QueuedWrite *prev;
    QueuedWrite *next;
    WriteBaton* baton;

    QueuedWrite() {
      prev = this;
      next = this;

      baton = 0;
    };

    ~QueuedWrite() {
      remove();
    };

    void remove() {
      prev->next = next;
      next->prev = prev;

      next = this;
      prev = this;
    };

    void insert_tail(QueuedWrite *qw) {
      qw->next = this;
      qw->prev = this->prev;
      qw->prev->next = qw;
      this->prev = qw;
    };

    bool empty() {
      return next == this;
    };

};



#ifdef DEBUG
#define DEBUG_HEADER fprintf(stderr, "fsio [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__);
  #define DEBUG_FOOTER fprintf(stderr, "\n");
  #define DEBUG_LOG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
#else
#define DEBUG_LOG(...)
#endif


#endif //FSIO_H
