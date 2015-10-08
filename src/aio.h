#ifndef FSIO_AIO_H
#define FSIO_AIO_H

#include <nan.h>
#include <aio.h>
#include <unistd.h>
#include <fcntl.h>

struct AIOBaton {
public:
  aiocb *aio;
  int timeout;
  bool busy;

  AIOBaton(int fd, size_t size) {
    busy = false;
    aio = new aiocb;
    memset(aio, 0, sizeof(struct aiocb));
    aio->aio_fildes = fd;
    aio->aio_sigevent.sigev_notify = SIGEV_NONE;
    aio->aio_nbytes = size;
    aio->aio_buf = malloc(size);
  }

  ~AIOBaton() {
    if (busy) aio_cancel(aio->aio_fildes, aio);
    if (aio->aio_fildes) close(aio->aio_fildes);
    if (aio->aio_buf) free((void *) (aio->aio_buf));
    aio->aio_buf = NULL;
    delete (aio);
    aio = NULL;
    busy = false;
  }
};


class AIO : public Nan::ObjectWrap {

public:
  static void Init(Handle<Object> target);

  // AIO(fd, buffer_size)
  static NAN_METHOD(New);

  // read(timeout, callback)
  static NAN_METHOD(Read);

  // write(buffer, offset, length, callback)
  static NAN_METHOD(Write);

private:
  int _fd;
  size_t _bufsize;
  char *_buf;
  AIOBaton *_rbaton;
  AIOBaton *_wbaton;

private:
  AIO(int fd, size_t bufsize);

  ~AIO();

private:
  static Nan::Persistent<FunctionTemplate> constructor_template;
  Nan::Persistent<Object> This;
};


#endif //FSIO_AIO_H
