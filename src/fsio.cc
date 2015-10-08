//
// Created by 陶源 on 15/9/27.
//

#include <nan.h>

#include "fsio.h"
#include "iofns.h"
#include "socket.h"
#include "aio.h"

NAN_MODULE_INIT(InitAll) {
  InitIOFns(target);
  Socket::Init(target);
  AIO::Init(target);
}

NODE_MODULE(fsio, InitAll)
