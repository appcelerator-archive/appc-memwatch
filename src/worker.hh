/*
 * 2012|lloyd|http://wtfpl.org
 * 2015 Changes by Jeff Haynie <jhaynie@appcelerator> / @jhaynie to bring it
 * update to snuff with Nan 2.0 and Node 4.0. Relicensed under same original license
 */
#ifndef WORKER_H_INCLUDE_
#define WORKER_H_INCLUDE_

#include <nan.h>

class Worker : public Nan::AsyncWorker {
public:
	Worker(v8::GCType, Nan::Callback *, Nan::Callback *);
	~Worker();
private:
	void Execute ();
	void WorkComplete();
	size_t heapUsage;
	v8::GCType type;
	Nan::Callback *gcCallback;
};

#endif
