//
// Created by 陶源 on 15/10/1.
//

#include <nan.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include "iofns.h"
#include "helpers.h"

void __fsio_eio_read(uv_work_t *req) {
  ReadBaton *data = static_cast<ReadBaton *>(req->data);

  errno = 0;
  data->result = 0;

  if (!data->length) { // read 0
    DEBUG_LOG("read (fd: %d, length: 0)", data->fd);
    data->result = (int) read(data->fd, 0, 0);
    DEBUG_LOG("read (fd: %d, result: %d)", data->fd, data->result);
    if (data->result < 0) {
      snprintf(data->errmsg, sizeof(data->errmsg), "Error %s calling read(...)", strerror(errno));
    }
    return;
  }

  struct pollfd fds;
  fds.fd = data->fd;
  fds.events = POLLIN;

//  DEBUG_LOG("poll (fd: %d)", data->fd);
  int ret =  poll(&fds, 1, 500);
  if (!ret || !(fds.revents & POLLIN)) {
    if (ret < 0) {
      snprintf(data->errmsg, sizeof(data->errmsg), "Error %s calling read(...)", strerror(errno));
      DEBUG_LOG("poll (fd: %d, error: %s)", data->fd, data->errmsg);
    }
//    DEBUG_LOG("poll finish with no data (fd: %d)", data->fd);
    return;
  }

//  DEBUG_LOG("read (fd: %d, length: %d)", data->fd, data->length);
  if ((data->result = (int) read(data->fd, data->bufferData + data->offset, data->length)) < 0) {
    snprintf(data->errmsg, sizeof(data->errmsg), "Error %s calling read(...)", strerror(errno));
    DEBUG_LOG("poll (fd: %d, error: %s)", data->fd, data->errmsg);
  }

  DEBUG_LOG("read (fd: %d, result: %d)", data->fd, data->result);

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

  DEBUG_LOG("write (fd: %d, length: %d)", data->fd, data->length);
  // We carefully *DON'T* break out of this loop.
  do {
    if ((data->result = (int) write(data->fd, data->bufferData + data->offset, data->length)) == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;

      // The write call might be interrupted, if it is we just try again immediately.
      if (errno != EINTR) {
        snprintf(data->errmsg, sizeof(data->errmsg), "Error %s calling write(...)", strerror(errno));
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
