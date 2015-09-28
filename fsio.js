'use strict';

var binding = require('./build/Release/fsio');

var noop = function () {

};

var fsio = module.exports = {};

fsio.Poller = binding.FsioPoller;

fsio.readSync = function (fd, buffer, offset, length) {
  offset = offset || 0;
  length = length || buffer.length;
  return binding.read(fd, buffer, offset, length);
};

fsio.write = function (fd, buffer, offset, length, cb) {
  if (typeof offset === 'function') {
    cb = offset;
    offset = null;
  } else if (typeof length === 'function') {
    cb = length;
    length = null;
  }
  offset = offset || 0;
  length = length || buffer.length;
  cb = cb || noop;

  return binding.write(fd, buffer, offset, length, cb);
};


fsio.writeSync = function (fd, buffer, offset, length) {
  offset = offset || 0;
  length = length || buffer.length;
  return binding.write(fd, buffer, offset, length);
};


