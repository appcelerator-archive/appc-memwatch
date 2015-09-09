/*
 * 2012|lloyd|http://wtfpl.org
 * 2015 Changes by Jeff Haynie <jhaynie@appcelerator> / @jhaynie to bring it
 * update to snuff with Nan 2.0 and Node 4.0. Relicensed under same original license
 */
#include "platformcompat.hh"
#include "util.hh"

#include <sstream>

#include <stdlib.h> // abs()

std::string mw_util::niceSize(int bytes) {
    std::stringstream ss;

    if (abs(bytes) > 1024 * 1024) {
        ss << ROUND(bytes / (((double) 1024 * 1024 ) / 100)) / (double) 100 << " mb";
    } else if (abs(bytes) > 1024) {
        ss << ROUND(bytes / (((double) 1024 ) / 100)) / (double) 100 << " kb";
    } else {
        ss << bytes << " bytes";
    }

    return ss.str();
}

std::string mw_util::niceDelta(int seconds)  {
    std::stringstream ss;

    if (seconds > (60*60)) {
        ss << (seconds / (60*60)) << "h ";
        seconds %= (60*60);
    }

    if (seconds > (60)) {
        ss << (seconds / (60)) << "m ";
        seconds %= (60);
    }

    ss << seconds << "s";

    return ss.str();
}
