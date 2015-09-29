'use strict';

var fs = require('fs');
var fsio = require('../');

var fd = fs.openSync('/dev/stdin', 'r');
var buf = new Buffer(10);

var poller = new fsio.Poller(fd, read);

function read(err) {
  if (err) throw err;

  fs.read(fd, buf, 0, buf.length, null, function (err, bytesRead) {
    afterRead(err, bytesRead);
  });

  function afterRead(err, bytesRead) {
    if (bytesRead === 0) {
      return poller.start();
    }
    console.log(bytesRead, buf);
    read();
  }
}

poller.start();
