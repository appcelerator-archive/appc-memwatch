/**
 * don't build on win32
 */

if (process.platform==='win32') {
	process.exit(0);
}

var spawn = require('child_process').spawn,
	fs = require('fs'),
	path = require('path'),
	from = path.join(__dirname, 'binding'),
	to = path.join(__dirname, 'binding.gyp');


fs.renameSync(from, to);
var child = spawn('node-gyp', ['rebuild'], {stdio:'inherit'});
child.on('exit', function (ex) {
	fs.renameSync(to, from);
	process.exit(ex);
});
