//
// Created by 陶源 on 15/9/27.
//

#ifndef FSIO_H
#define FSIO_H

#define ERROR_STRING_SIZE 1024

#define ENABLE_ASYNC

NAN_METHOD(Read);

NAN_METHOD(Write);
void EIO_Write(uv_work_t* req);
void EIO_AfterWrite(uv_work_t* req);

struct WriteBaton {
public:
    int fd;
    char* bufferData;
    size_t bufferLength;
    size_t offset;
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

#endif //FSIO_H
