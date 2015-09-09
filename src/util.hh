/*
 * 2012|lloyd|http://wtfpl.org
 * 2015 Changes by Jeff Haynie <jhaynie@appcelerator> / @jhaynie to bring it
 * update to snuff with Nan 2.0 and Node 4.0. Relicensed under same original license
 */
#ifndef __UTIL__HEADER_H
#define __UTIL__HEADER_H
#include <string>

namespace mw_util {
	// given a size in bytes, return a human readable representation of the
	// string
	std::string niceSize(int bytes);

	// given a delta in seconds, return a human redable representation
	std::string niceDelta(int seconds);
};

#endif
