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

exports.O_RDONLY = binding.O_RDONLY;
exports.O_WRONLY = binding.O_WRONLY;
exports.O_RDWR = binding.O_RDWR;
exports.O_ACCMODE = binding.O_ACCMODE;

exports.O_CLOEXEC = binding.O_CLOEXEC;

var keys = Object.keys(binding);
keys.forEach(function (k) {
  if (typeof binding[k] !== 'function' && !(k in exports)) {
    exports[k] = binding[k];
  }
});
