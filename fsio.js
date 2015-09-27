'use strict';

var binding = require('./build/Release/fsio');

var fsio = module.exports = {};

fsio.Poller = binding.FsioPoller;

fsio.readSync = function (fd, buffer, offset, length) {
  if (arguments.length < 3) {
    offset = 0;
  }
  if (arguments.length < 4) {
    length = buffer.length;
  }
  return binding.read(fd, buffer, offset, length);
};

fsio.writeSync = function (fd, buffer, offset, length) {
  if (arguments.length < 3) {
    offset = 0;
  }
  if (arguments.length < 4) {
    length = buffer.length;
  }
  return binding.write(fd, buffer, offset, length);
};


