var fs = require('fs');
var fsio = require('../../');

var fd = fsio.openSync('/dev/stdout', fsio.O_WRONLY);
var aio = fsio.aio(fd);

for (var i = 0; i < 100; i++) {
  var buf = new Buffer(i.toString());
  aio.write(buf, function () {
    //console.log('ok', i);
  });
}


