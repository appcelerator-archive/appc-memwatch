var memwatch = require('bindings')('memwatch'),
	events = require('events'),
	emitter = new events.EventEmitter();

module.exports = emitter;
module.exports.gc = memwatch.gc;
module.exports.HeapDiff = memwatch.HeapDiff;

memwatch.upon_gc(function(event, data) {
	if (module.exports.listeners(event).length) {
		module.exports.emit(event, data);
	}
});
