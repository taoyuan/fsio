#include <nan.h>
#include <unistd.h>
#include <fcntl.h>

#include "helpers.h"
#include "aio.h"

using namespace Nan;
using namespace v8;
using namespace node;

Nan::Persistent<FunctionTemplate> AIO::constructor_template;

class AIOReadWorker : public AsyncWorker {
public:
  AIOReadWorker(Callback *callback, AIOBaton *baton)
    : AsyncWorker(callback), baton(baton), data(NULL), length(0) {
    baton->busy = true;
  }

  ~AIOReadWorker() {
    if (data) free(data);
    data = NULL;
  }

  void Execute() {

    errno = 0;
    aiocb *aio = baton->aio;

    int rc = aio_read(baton->aio);
    if (rc) {
      SetErrorMessage(strerror(errno));
    } else while (1) {
      rc = aio_error(aio);
      if (rc == EINPROGRESS && baton->timeout) {
        struct timespec ts;
        ts.tv_sec = baton->timeout / 1000;
        ts.tv_nsec = 1000000L * (baton->timeout % 1000);
        if (aio_suspend(&aio, 1, &ts)) {
          continue;
        } else {
          rc = aio_error(aio);
        }
      }
      if (rc) {
        if (rc == EINPROGRESS) { continue; }
        DEBUG_LOG("Error during async aio on fd %d (rc %d %s)\n", aio->aio_fildes, rc, strerror(rc));
        SetErrorMessage(strerror(errno));
        break;
      } else {
        rc = (int) aio_return(aio);
        if (rc == -1) {
          DEBUG_LOG("Bad aio_return (rc %d, %s)\n", errno, strerror(errno));
          SetErrorMessage(strerror(errno));
          break;
        }
        DEBUG_LOG("Received %d bytes from gadgetfs fd %d", rc, aio->aio_fildes);
        length = (size_t) rc;
        data = static_cast<char *>(malloc(length));
        memcpy(data, (void*) (aio->aio_buf), length);
        break;
      }
    }

    baton->busy = false;
  }

  void HandleOKCallback() {
    Local<Value> argv[] = {Nan::Undefined(), NewBuffer(data, (uint32_t) length).ToLocalChecked()};
    callback->Call(2, argv);
    data = NULL;
    length = 0;
  }

private:
  AIOBaton *baton;
  char * data;
  size_t length;
};

AIO::AIO(int fd, size_t bufsize) :
  _fd(fd), _bufsize(bufsize), _rbaton(NULL), _wbaton(NULL) {

}

AIO::~AIO() {
  if (_rbaton) delete _rbaton;
  _rbaton = NULL;
  if (_wbaton) delete _wbaton;
  _wbaton = NULL;
}

NAN_METHOD(AIO::New) {
  ENTER_CONSTRUCTOR(2);

  int fd;
  size_t bufsize;
  INT_ARG(fd, 0);
  INT_ARG(bufsize, 0);

  AIO *aio = new AIO(fd, bufsize);

  aio->Wrap(info.This());
  aio->This.Reset(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(AIO::Read) {
  ENTER_METHOD(AIO, 2);

  if (that->_rbaton && that->_rbaton->busy) {
    return Nan::ThrowError("Read busy");
  }

  int timeout;
  INT_ARG(timeout, 0);
  NAN_CALLBACK_ARG(1);

  if (!that->_rbaton) {
    that->_rbaton = new AIOBaton(that->_fd, that->_bufsize);
  }

  that->_rbaton->timeout = timeout;

  AsyncQueueWorker(new AIOReadWorker(callback, that->_rbaton));
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(AIO::Write) {

}

void AIO::Init(Handle<Object> target) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("AIO").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "read", Read);
  Nan::SetPrototypeMethod(tpl, "write", Write);

  constructor_template.Reset(tpl);
  target->Set(Nan::New("AIO").ToLocalChecked(), tpl->GetFunction());
}
