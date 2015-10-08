//
// Created by 陶源 on 15/10/8.
//

#ifndef FSIO_IOFNS_H
#define FSIO_IOFNS_H

#include <nan.h>

using namespace v8;
using namespace node;

#define ERROR_STRING_SIZE 1024

struct ReadBaton {
public:
  int fd;
  char *bufferData;
  size_t bufferLength;
  size_t offset;
  size_t length;
  Nan::Persistent<v8::Object> buffer;
  Nan::Callback *callback;
  int result;
  char errmsg[ERROR_STRING_SIZE];
};

struct WriteBaton {
public:
  int fd;
  char *bufferData;
  size_t bufferLength;
  size_t offset;
  size_t length;
  Nan::Persistent<v8::Object> buffer;
  Nan::Callback *callback;
  int result;
  char errmsg[ERROR_STRING_SIZE];
};

struct QueuedWrite {
public:
  uv_work_t req;
  QueuedWrite *prev;
  QueuedWrite *next;
  WriteBaton *baton;

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


void fsio_attach(int fd);

void fsio_detach(int fd);

int fsio_read(int fd, Local<Object> buffer, size_t offset, size_t length, Local<Function> callback);

void __fsio_eio_read(uv_work_t *req);

void __fsio_eio_after_read(uv_work_t *req);

int fsio_write(int fd, Local<Object> buf, size_t offset, size_t length, Local<Function> callback);

void __fsio_eio_write(uv_work_t *req);

void __fsio_eio_after_write(uv_work_t *req);


NAN_METHOD(Open);

NAN_METHOD(Close);

NAN_METHOD(Poll);

NAN_METHOD(Read);

void InitIOFns(Handle<Object> target);

#endif //FSIO_IOFNS_H
