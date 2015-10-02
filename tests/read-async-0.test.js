var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('/dev/stdin', fsio.O_WRONLY);
var buf = new Buffer(0);

fsio.read(fd, buf, function (err, bytes) {
  console.log('ok');
  fs.closeSync(fd);
});
