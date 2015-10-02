var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('/dev/stdin', fsio.O_WRONLY);
var buf = new Buffer(100);

function read() {

  function _read() {
    fsio.read(fd, buf, function (err, bytes) {
      if (err) throw err;
      if (bytes) console.log(bytes);
      setImmediate(_read);
    });
  }

  _read();
}

read();
