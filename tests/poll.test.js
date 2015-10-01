var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('/dev/stdin', 'r+');

var buffer = new Buffer(100);

function poll() {
  var ret = fsio.poll(fd, fsio.POLLIN, 500);
  if (ret === fsio.POLLIN) {
    var count = fs.readSync(fd, buffer, 0, buffer.length);
    console.log(buffer.slice(0, count));
  }
  process.nextTick(poll);
}

poll();
