var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('in', 'r+');
var buf = new Buffer('hello12345');

fsio.write(fd, buf, function () {
  console.log(arguments);
  fs.closeSync(fd);
});
