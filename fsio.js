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
