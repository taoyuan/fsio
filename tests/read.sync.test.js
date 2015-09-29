var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('fixture/data', 'r');
var buf = new Buffer(20);

var socket = fsio.Socket(fd);

var bytesRead = socket.readSync(buf);

console.log(buf.slice(0, bytesRead));

fs.closeSync(fd);
