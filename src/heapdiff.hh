/*
 * 2012|lloyd|http://wtfpl.org
 * 2015 Changes by Jeff Haynie <jhaynie@appcelerator> / @jhaynie to bring it
 * update to snuff with Nan 2.0 and Node 4.0. Relicensed under same original license
 */
#ifndef HEAPDIFF_H_INCLUDE_
#define HEAPDIFF_H_INCLUDE_

#include <nan.h>
#include <v8-profiler.h>

using namespace Nan;

class HeapDiff : public node::ObjectWrap {
public:
	static NAN_MODULE_INIT (Init);
	static bool InProgress();

private:
	HeapDiff();
	~HeapDiff();

	static NAN_METHOD(New);
	static NAN_METHOD(End);

	static Persistent<v8::Function> constructor;
	const v8::HeapSnapshot * before;
	const v8::HeapSnapshot * after;
	bool ended;
};

#endif
