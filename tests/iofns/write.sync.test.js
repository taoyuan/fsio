var fs = require('fs');
var fsio = require('../../');

var fd = fsio.openSync('/dev/stdout', fsio.O_WRONLY);
var result = fsio.writeSync(fd, new Buffer('hello12345\n'));
console.log('wrote', result);
