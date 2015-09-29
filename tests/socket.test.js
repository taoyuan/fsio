var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('/dev/stdin', 'r+');

var socket = new fsio.Socket(fd);

socket.on('data', function (data) {
  console.log(data);
});
socket.on('error', function (err) {
  console.error(err);
});

socket.start();
//socket.write(new Buffer('hello world'));
console.log('started');

//setTimeout(function () {
//  fs.closeSync(fd);
//}, 1000);

