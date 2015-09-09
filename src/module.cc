/*
 * 2012|lloyd|http://wtfpl.org
 * 2015 Changes by Jeff Haynie <jhaynie@appcelerator> / @jhaynie to bring it
 * update to snuff with Nan 2.0 and Node 4.0. Relicensed under same original license
 */
#include "memwatch.hh"
#include "heapdiff.hh"

using v8::FunctionTemplate;

NAN_MODULE_INIT(InitAll) {
	Nan::Set(target, Nan::New("gc").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(gc)).ToLocalChecked());
	Nan::Set(target, Nan::New("upon_gc").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(upon_gc)).ToLocalChecked());
	Nan::AddGCPrologueCallback(after_gc_idle);
	HeapDiff::Init(target);
}

NODE_MODULE(memwatch, InitAll)
