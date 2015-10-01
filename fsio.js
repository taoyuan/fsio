'use strict';

var util = require('util');
var events = require('events');
var binding = require('./build/Release/fsio');

var nop = function () {};

function Socket(fd, cb) {
  if (!(this instanceof Socket)) {
    return new Socket(fd, cb);
  }
  cb = cb || nop;
  this._socket = new binding.Socket(fd, cb);
  this.fd = fd;
}

util.inherits(Socket, events.EventEmitter);

Socket.prototype.start = function () {
  this._socket.start();
};

Socket.prototype.stop = function () {
  this._socket.stop();
};

Socket.prototype.readSync = function (buffer, offset, length) {
  offset = offset || 0;
  length = length || buffer.length;
  return this._socket.read(buffer, offset, length);
};

Socket.prototype.write = function (buffer, offset, length, cb) {
  if (typeof offset === 'function') {
    cb = offset;
    offset = null;
  } else if (typeof length === 'function') {
    cb = length;
    length = null;
  }
  offset = offset || 0;
  length = length || buffer.length;
  cb = cb || nop;
  return this._socket.write(buffer, offset, length, cb);
};

Socket.prototype.writeSync = function (buffer, offset, length) {
  offset = offset || 0;
  length = length || buffer.length;
  return this._socket.write(buffer, offset, length);
};


exports.Socket = Socket;

exports.openSync = function (path, flag) {
  return binding.open(path, flag);
};

exports.closeSync = function (path) {
  return binding.close(path);
};

// Open
exports.O_RDONLY = binding.O_RDONLY;
exports.O_WRONLY = binding.O_WRONLY;
exports.O_RDWR = binding.O_RDWR;
exports.O_ACCMODE = binding.O_ACCMODE;

exports.O_CLOEXEC = binding.O_CLOEXEC;

// POLL
exports.POLLIN = binding.POLLIN;
exports.POLLPRI = binding.POLLPRI;
exports.POLLOUT = binding.POLLOUT;
exports.POLLRDNORM = binding.POLLRDNORM;
exports.POLLWRNORM = binding.POLLWRNORM;
exports.POLLRDBAND = binding.POLLRDBAND;
exports.POLLWRBAND = binding.POLLWRBAND;

exports.POLLEXTEND = binding.POLLEXTEND;
exports.POLLATTRIB = binding.POLLATTRIB;
exports.POLLNLINK = binding.POLLNLINK;
exports.POLLWRITE = binding.POLLWRITE;

exports.POLLERR = binding.POLLERR;
exports.POLLHUP = binding.POLLHUP;
exports.POLLNVAL = binding.POLLNVAL;

var keys = Object.keys(binding);
keys.forEach(function (k) {
  if (typeof binding[k] !== 'function' && !(k in exports)) {
    exports[k] = binding[k];
  }
});

exports.poll = function (fd, events, timeout) {

};
