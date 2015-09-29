'use strict';

var events = require('events');
var binding = require('./build/Release/fsio');

var fsio = module.exports = {};

fsio.Socket = binding.Socket;
// NOT USE util.inherits
inherits(fsio.Socket, events.EventEmitter);

// extend prototype
function inherits(target, source) {
  for (var k in source.prototype) {
    target.prototype[k] = source.prototype[k];
  }
}
