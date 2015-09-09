/*
 * 2012|lloyd|http://wtfpl.org
 * 2015 Changes by Jeff Haynie <jhaynie@appcelerator> / @jhaynie to bring it
 * update to snuff with Nan 2.0 and Node 4.0. Relicensed under same original license
 */
#ifndef __PLATFORMCOMPAT_H
#define __PLATFORMCOMPAT_H

#include <math.h> // round()

#if defined(_MSC_VER)
#include <float.h> //isinf, isnan
#include <stdlib.h> //min
#define ISINF _finite
#define ISNAN _isnan
#define FMIN __min
#define ROUND(x) floor(x + 0.5)
#else
#define ISINF isinf
#define ISNAN isnan
#define FMIN fmin
#define ROUND round
#endif

#endif
