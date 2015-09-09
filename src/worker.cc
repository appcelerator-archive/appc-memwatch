/*
 * 2012|lloyd|http://wtfpl.org
 * 2015 Changes by Jeff Haynie <jhaynie@appcelerator> / @jhaynie to bring it
 * update to snuff with Nan 2.0 and Node 4.0. Relicensed under same original license
 */
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>

#include <math.h> // for pow
#include <time.h> // for time
#include "platformcompat.hh"
#include "util.hh"
#include "worker.hh"

static const unsigned int RECENT_PERIOD = 10;
static const unsigned int ANCIENT_PERIOD = 120;

static struct {
	// counts of different types of gc events
	unsigned int gc_full;
	unsigned int gc_inc;
	unsigned int gc_compact;

	// last base heap size as measured *right* after GC
	unsigned int last_base;

	// the estimated "base memory" usage of the javascript heap
	// over the RECENT_PERIOD number of GC runs
	unsigned int base_recent;

	// the estimated "base memory" usage of the javascript heap
	// over the ANCIENT_PERIOD number of GC runs
	unsigned int base_ancient;

	// the most extreme values we've seen for base heap size
	unsigned int base_max;
	unsigned int base_min;

	// leak detection!

	// the period from which this leak analysis starts
	time_t leak_time_start;
	// the base memory for the detection period
	time_t leak_base_start;
	// the number of consecutive compactions for which we've grown
	unsigned int consecutive_growth;
} s_stats;

Worker::Worker(v8::GCType type_, Nan::Callback *callback_, Nan::Callback *gcCallback_) : Nan::AsyncWorker(callback_), heapUsage(0), type(type_), gcCallback(gcCallback_) {
	v8::HeapStatistics hs;
	Nan::GetHeapStatistics(&hs);
	this->heapUsage = hs.used_heap_size();
	if (!s_stats.last_base) {
		s_stats.last_base = this->heapUsage;
	}
}

Worker::~Worker() {
}

void Worker::Execute () {
	// no op
}

void Worker::WorkComplete() {
	Nan::HandleScope scope;

	// std::cout << "gc" << std::endl;

	// record the type of GC event that occured
	if (this->type == v8::kGCTypeMarkSweepCompact) {
		s_stats.gc_full++;
	} else {
		s_stats.gc_inc++;
	}

	// std::cout << "gc - type=" << this->type << ", last_base=" << s_stats.last_base << ", heapUsage=" << this->heapUsage << std::endl;

	// leak detection code.  has the heap usage grown?
	if (s_stats.last_base < this->heapUsage) {
		if (s_stats.consecutive_growth == 0) {
			s_stats.leak_time_start = time(NULL);
			s_stats.leak_base_start = this->heapUsage;
		}

		s_stats.consecutive_growth++;

		// consecutive growth over 5 GCs suggests a leak
		if (s_stats.consecutive_growth >= 5) {
			// reset to zero
			s_stats.consecutive_growth = 0;

			// emit a leak report!
			if (gcCallback) {
				size_t growth = this->heapUsage - s_stats.leak_base_start;
				int now = time(NULL);
				int delta = now - s_stats.leak_time_start;

				v8::Local<v8::Object> leakReport = Nan::New<v8::Object>();
				leakReport->Set(Nan::New<v8::String>("growth").ToLocalChecked(), Nan::New<v8::Number>(growth));

				std::stringstream ss;
				ss << "heap growth over 5 consecutive GCs ("
					<< mw_util::niceDelta(delta) << ") - "
					<< (delta ? mw_util::niceSize(growth / ((double) delta / (60.0 * 60.0))) :
						mw_util::niceSize(growth / 60.0 * 60.0)) << "/hr";

				leakReport->Set(Nan::New<v8::String>("reason").ToLocalChecked(), Nan::New<v8::String>(ss.str().c_str()).ToLocalChecked());

				v8::Local<v8::Value> argv[] = {
					Nan::New<v8::String>("leak").ToLocalChecked(),
					leakReport
				};
				gcCallback->Call(2, argv);
			}
		}
	} else {
		s_stats.consecutive_growth = 0;
	}

	// update last_base
	s_stats.last_base = this->heapUsage;

	// update compaction count
	s_stats.gc_compact++;

	// the first ten compactions we'll use a different algorithm to
	// dampen out wider memory fluctuation at startup
	if (s_stats.gc_compact < RECENT_PERIOD) {
		double decay = pow(s_stats.gc_compact / RECENT_PERIOD, 2.5);
		decay *= s_stats.gc_compact;
		if (ISINF(decay) || ISNAN(decay)) decay = 0;
		s_stats.base_recent = ((s_stats.base_recent * decay) +
							   s_stats.last_base) / (decay + 1);

		decay = pow(s_stats.gc_compact / RECENT_PERIOD, 2.4);
		decay *= s_stats.gc_compact;
		s_stats.base_ancient = ((s_stats.base_ancient * decay) +
								s_stats.last_base) /  (1 + decay);

	} else {
		s_stats.base_recent = ((s_stats.base_recent * (RECENT_PERIOD - 1)) +
							   s_stats.last_base) / RECENT_PERIOD;
		double decay = FMIN(ANCIENT_PERIOD, s_stats.gc_compact);
		s_stats.base_ancient = ((s_stats.base_ancient * (decay - 1)) +
								s_stats.last_base) / decay;
	}

	// only record min/max after 3 gcs to let initial instability settle
	if (s_stats.gc_compact >= 3) {
		if (!s_stats.base_min || s_stats.base_min > s_stats.last_base) {
			s_stats.base_min = s_stats.last_base;
		}

		if (!s_stats.base_max || s_stats.base_max < s_stats.last_base) {
			s_stats.base_max = s_stats.last_base;
		}
	}

	// if there are any listeners, it's time to emit!
	if (1 /*gcCallback && this->type == v8::kGCTypeMarkSweepCompact*/) {
		v8::Local<v8::Object> statsReport = Nan::New<v8::Object>();
		double ut = 0.0;
		if (s_stats.base_ancient) {
			ut = (double) ROUND(((double) (s_stats.base_recent - s_stats.base_ancient) /
								  (double) s_stats.base_ancient) * 1000.0) / 10.0;
		}
		statsReport->Set(Nan::New<v8::String>("num_full_gc").ToLocalChecked(), Nan::New<v8::Number>(s_stats.gc_full));
		statsReport->Set(Nan::New<v8::String>("num_inc_gc").ToLocalChecked(), Nan::New<v8::Number>(s_stats.gc_inc));
		statsReport->Set(Nan::New<v8::String>("heap_compactions").ToLocalChecked(), Nan::New<v8::Number>(s_stats.gc_compact));
		statsReport->Set(Nan::New<v8::String>("usage_trend").ToLocalChecked(), Nan::New<v8::Number>(ut));
		statsReport->Set(Nan::New<v8::String>("estimated_base").ToLocalChecked(), Nan::New<v8::Number>(s_stats.base_recent));
		statsReport->Set(Nan::New<v8::String>("current_base").ToLocalChecked(), Nan::New<v8::Number>(s_stats.last_base));
		statsReport->Set(Nan::New<v8::String>("min").ToLocalChecked(), Nan::New<v8::Number>(s_stats.base_min));
		statsReport->Set(Nan::New<v8::String>("max").ToLocalChecked(), Nan::New<v8::Number>(s_stats.base_max));

		v8::Local<v8::Value> argv[] = {
			Nan::New<v8::String>("stats").ToLocalChecked(),
			statsReport
		};
		gcCallback->Call(2, argv);
	}
}
