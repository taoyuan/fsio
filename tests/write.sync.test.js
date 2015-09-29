var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('in', 'r+');
var buf = new Buffer('hello12345');

console.log(fsio.writeSync(fd, buf));

fs.closeSync(fd);
