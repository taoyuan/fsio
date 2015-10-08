//
// Created by 陶源 on 15/10/8.
//

#ifndef FSIO_SOCKET_H
#define FSIO_SOCKET_H

#include <nan.h>

class Socket : public Nan::ObjectWrap {

public:
  static void Init(Handle<Object> target);

  static NAN_METHOD(New);

  static NAN_METHOD(Start);

  static NAN_METHOD(Stop);

  static NAN_METHOD(Read);

  static NAN_METHOD(Write);

private:
  int _fd;
  uv_poll_t _poll_handle;
  Nan::Callback *_callback;

private:
  Socket(int fd);

  ~Socket();

  void start();

  void stop();

  int _read(char *data, size_t length);

  int _write(char *data, size_t length);

  void poll();

  static void PollCloseCallback(uv_poll_t *handle);

  static void PollCallback(uv_poll_t *handle, int status, int events);

private:
  static Nan::Persistent<FunctionTemplate> constructor_template;
  Nan::Persistent<Object> This;
};


#endif //FSIO_SOCKET_H
