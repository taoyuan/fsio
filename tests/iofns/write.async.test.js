var fs = require('fs');
var fsio = require('../../');

var fd = fsio.openSync('/dev/stdout', fsio.O_WRONLY);

for (var i = 0; i < 100; i++) {
  fsio.write(fd, new Buffer(i + ' '), function (err, result) {
    if (err) throw err;
  });
}

