var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('/dev/stdout', 'w');
var buf = new Buffer('hello12345');

var socket = fsio.Socket(fd);

socket.write(buf, function () {
  console.log('done');
  fs.closeSync(fd);
});


