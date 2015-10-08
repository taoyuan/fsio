var fs = require('fs');
var fsio = require('../../');

var fd = fsio.openSync('/dev/stdin', fsio.O_RDONLY);
var aio = fsio.aio(fd);

aio.poll(function (err, buffer) {
  if (err) return console.error(err);
  console.log(buffer);
});
