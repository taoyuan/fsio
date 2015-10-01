var fs = require('fs');
var fsio = require('../');

var fd = fsio.openSync('fixture/data', fsio.O_RDWR | fsio.O_CLOEXEC);
var buf = new Buffer(20);

var socket = fsio.Socket(fd);

var bytesRead = socket.readSync(buf);

console.log(buf.slice(0, bytesRead));

fs.closeSync(fd);
