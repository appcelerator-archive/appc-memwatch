/*
 * 2012|lloyd|http://wtfpl.org
 * 2015 Changes by Jeff Haynie <jhaynie@appcelerator> / @jhaynie to bring it
 * update to snuff with Nan 2.0 and Node 4.0. Relicensed under same original license
 */
#include "memwatch.hh"
#include "heapdiff.hh"
#include "worker.hh"
#include <iostream>

using namespace v8;
using namespace Nan;

static Nan::Callback *uponGCCallback = NULL;

NAN_METHOD(gc) {
	Nan::HandleScope scope;
	size_t deadline_in_ms = 500;
	if (info.Length() >= 1 && info[0]->IsNumber()) {
		deadline_in_ms = (int)(info[0]->Int32Value());
	}
	Nan::IdleNotification(deadline_in_ms);
	Nan::LowMemoryNotification();
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(upon_gc) {
	uponGCCallback = new Nan::Callback(info[0].As<Function>());
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(work) {
	// no op, used by the empty callback to the gc event loop
}

NAN_GC_CALLBACK(after_gc_idle) {
	// std::cout << "after_gc_idle" << std::endl;
	if (HeapDiff::InProgress()) { return; }
	// schedule our work to run in a moment, once gc has fully completed.
	v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(work);
	Callback *callback = new Callback(tpl->GetFunction());
	Worker *worker = new Worker(type, callback, uponGCCallback);
	AsyncQueueWorker(worker);
}
