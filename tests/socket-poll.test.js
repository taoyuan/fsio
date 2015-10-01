var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('/dev/stdin', 'r+');

var socket = new fsio.Socket(fd, function (data) {
  if (data.length === 1) {
    socket.stop();
    fs.closeSync(fd);
    console.log('closed');
    return;
  }
  console.log(data);

});

socket.start();

console.log('started');
