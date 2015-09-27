var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('in', 'r+');
var buf = new Buffer(20);

console.log(fsio.readSync(fd, buf));

console.log(buf.toString());

fs.closeSync(fd);
