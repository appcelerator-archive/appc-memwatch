/*
 * 2012|lloyd|http://wtfpl.org
 * 2015 Changes by Jeff Haynie <jhaynie@appcelerator> / @jhaynie to bring it
 * update to snuff with Nan 2.0 and Node 4.0. Relicensed under same original license
 */
#ifndef MEMWATCH_H
#define MEMWATCH_H

#include <nan.h>

NAN_METHOD(gc);
NAN_METHOD(upon_gc);

NAN_GC_CALLBACK(after_gc_idle);

#endif
