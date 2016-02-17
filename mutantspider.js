
"use strict"

var fs = require('fs');

// write the error message to stdout and then tell make to stop with an error by causing it to
// execute the $(error ) function - then exit this node process (with an error exit code, but make
// ignores the exit code)
module.exports.make_assert = (err, str) => {
  if (err) {
    console.error(str || '' + err);
    console.log('$(error )');
    process.exit(1);
  }
};

module.exports.assert = (err, str) => {
  if (err) {
    console.error(str || '' + err);
    process.exit(1);
  }
};

// read the given file (name) into a string, and once it has been thusly read, call the given
// "start" function, passing it the string
module.exports.read_file_and_start = (file_name, first_pass_make, start_func) => {

  let _assert = first_pass_make ? module.exports.make_assert : module.exports.assert;

  fs.stat(file_name, (error, stats) => {
    module.exports.make_assert(error);
    fs.open(file_name, 'r', (error, fd) => {
      _assert(error);
      let buffer = new Buffer(stats.size);
      fs.read(fd, buffer, 0, stats.size, null, (error, bytesRead, buffer) => {
        fs.close(fd);
        _assert(error);
        _assert(bytesRead != stats.size, 'incomplete read of file: ' + file_name + ', only read ' + bytesRead + ' bytes of ' + stats.size);
        start_func(buffer.toString());
      });
    });
  });

};

// the root directory named used in the s3 buckets for items we put in there
module.exports.root_dir = 'code/';
