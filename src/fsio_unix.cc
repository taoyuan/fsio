//
// Created by 陶源 on 15/10/1.
//

#include <nan.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include "fsio.h"
#include "helpers.h"

void __fsio_eio_read(uv_work_t *req) {
  ReadBaton *data = static_cast<ReadBaton *>(req->data);

  errno = 0;
  data->result = 0;

  if (!data->length) { // read 0
    DEBUG_LOG("--- read 0 begin");
    data->result = (int) read(data->fd, 0, 0);
    DEBUG_LOG("--- read 0 complete");
    if (data->result < 0) {
      snprintf(data->errorString, sizeof(data->errorString), "Error %s calling read(...)", strerror(errno));
    }
    return;
  }

  struct pollfd fds;
  fds.fd = data->fd;
  fds.events = POLLIN;

  if (!poll(&fds, 1, 500) || !(fds.revents & POLLIN)) {
    return;
  }

  if ((data->result = (int) read(data->fd, data->bufferData + data->offset, data->length)) < 0) {
    snprintf(data->errorString, sizeof(data->errorString), "Error %s calling read(...)", strerror(errno));
    return;
  }

}

void __fsio_eio_write(uv_work_t *req) {
  QueuedWrite *queuedWrite = static_cast<QueuedWrite *>(req->data);
  WriteBaton *data = queuedWrite->baton;

  data->result = 0;
  errno = 0;

  if (!data->length) {
    DEBUG_LOG("write(%d, 0, 0)", data->fd);
    data->result = (int) write(data->fd, 0, 0);
    return;
  }

  // We carefully *DON'T* break out of this loop.
  do {
    if ((data->result = (int) write(data->fd, data->bufferData + data->offset, data->length)) == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;

      // The write call might be interrupted, if it is we just try again immediately.
      if (errno != EINTR) {
        snprintf(data->errorString, sizeof(data->errorString), "Error %s calling write(...)", strerror(errno));
        return;
      }

      // try again...
      continue;
    }
      // there wasn't an error, do the math on what we actually wrote...
    else {
      data->length -= data->result;
    }

    // if we get there, we really don't want to loop
    // break;
  } while (data->length > 0);
}
