/*
 * 2012|lloyd|http://wtfpl.org
 * 2015 Changes by Jeff Haynie <jhaynie@appcelerator> / @jhaynie to bring it
 * update to snuff with Nan 2.0 and Node 4.0. Relicensed under same original license
 */
#include <map>
#include <string>
#include <set>
#include <vector>
#include "heapdiff.hh"
#include "util.hh"

using namespace std;

Nan::Persistent<v8::Function> HeapDiff::constructor;

static bool inProgress = false;
static time_t startTime;

#if (NODE_MODULE_VERSION < NODE_0_12_MODULE_VERSION)
	#define NanTakeHeapSnapshot v8::HeapProfiler::TakeSnapshot(Nan::New<v8::String>("HeapDiff").ToLocalChecked())
#elif NODE_MODULE_VERSION <= NODE_0_12_MODULE_VERSION
	#define NanTakeHeapSnapshot v8::Isolate::GetCurrent()->GetHeapProfiler()->TakeHeapSnapshot(Nan::New<v8::String>("HeapDiff").ToLocalChecked())
#else
	#define NanTakeHeapSnapshot v8::Isolate::GetCurrent()->GetHeapProfiler()->TakeHeapSnapshot()
#endif

#if NODE_MODULE_VERSION < NODE_0_12_MODULE_VERSION
	#define GetShallowSize GetSelfSize
#endif


HeapDiff::HeapDiff () : before(NULL), after(NULL), ended(false) {
}

HeapDiff::~HeapDiff () {
	if (before) {
		((v8::HeapSnapshot *) before)->Delete();
		before = NULL;
	}

	if (after) {
		((v8::HeapSnapshot *) after)->Delete();
		after = NULL;
	}
}

bool HeapDiff::InProgress () {
	return inProgress;
}


NAN_MODULE_INIT (HeapDiff::Init) {
	// Prepare constructor template
	v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
	tpl->SetClassName(Nan::New<v8::String>("HeapDiff").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	SetPrototypeMethod(tpl, "end", End);

	constructor.Reset(tpl->GetFunction());
	Set(target, Nan::New<v8::String>("HeapDiff").ToLocalChecked(), tpl->GetFunction());
}

NAN_METHOD (HeapDiff::New) {
	if (info.IsConstructCall()) {
		HeapDiff* obj = new HeapDiff();
		obj->Wrap(info.This());
		// take a snapshot and save a pointer to it
		inProgress = true;
		startTime = time(NULL);
		obj->before = NanTakeHeapSnapshot;
		inProgress = false;
		info.GetReturnValue().Set(info.This());
	} else {
		v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
		info.GetReturnValue().Set(cons->NewInstance());
	}
}

static string handleToStr (const v8::Handle<v8::Value> & str) {
	v8::String::Utf8Value utfString(str->ToString());
	return *utfString;
}

static void buildIDSet (set<uint64_t> * seen, const v8::HeapGraphNode* cur, int & s) {
	Nan::HandleScope scope;

	// cycle detection
	if (seen->find(cur->GetId()) != seen->end()) {
		return;
	}
	// always ignore HeapDiff related memory
	if (cur->GetType() == v8::HeapGraphNode::kObject &&
		handleToStr(cur->GetName()).compare("HeapDiff") == 0) {
		return;
	}

	// update memory usage as we go
	s += cur->GetShallowSize();

	seen->insert(cur->GetId());

	for (int i=0; i < cur->GetChildrenCount(); i++) {
		buildIDSet(seen, cur->GetChild(i)->GetToNode(), s);
	}
}

typedef set<uint64_t> idset;

// why doesn't STL work?
// XXX: improve this algorithm
static void setDiff(idset a, idset b, vector<uint64_t> &c) {
	for (idset::iterator i = a.begin(); i != a.end(); i++) {
		if (b.find(*i) == b.end()) c.push_back(*i);
	}
}

class example
{
public:
	v8::HeapGraphEdge::Type context;
	v8::HeapGraphNode::Type type;
	std::string name;
	std::string value;
	std::string heap_value;
	int self_size;
	int retained_size;
	int retainers;

	example() : context(v8::HeapGraphEdge::kHidden),
				type(v8::HeapGraphNode::kHidden),
				self_size(0), retained_size(0), retainers(0) { };
};

class change
{
public:
	long int size;
	long int added;
	long int released;
	std::vector<example> examples;

	change() : size(0), added(0), released(0) { }
};

typedef std::map<std::string, change>changeset;

static void manageChange(changeset & changes, const v8::HeapGraphNode * node, bool added) {
	std::string type;

	switch(node->GetType()) {
		case v8::HeapGraphNode::kArray:
			type.append("Array");
			break;
		case v8::HeapGraphNode::kString:
			type.append("String");
			break;
		case v8::HeapGraphNode::kObject:
			type.append(handleToStr(node->GetName()));
			break;
		case v8::HeapGraphNode::kCode:
			type.append("Code");
			break;
		case v8::HeapGraphNode::kClosure:
			type.append("Closure");
			break;
		case v8::HeapGraphNode::kRegExp:
			type.append("RegExp");
			break;
		case v8::HeapGraphNode::kHeapNumber:
			type.append("Number");
			break;
		case v8::HeapGraphNode::kNative:
			type.append("Native");
			break;
		case v8::HeapGraphNode::kHidden:
		default:
			return;
	}

	if (changes.find(type) == changes.end()) {
		changes[type] = change();
	}

	changeset::iterator i = changes.find(type);

	i->second.size += node->GetShallowSize() * (added ? 1 : -1);
	if (added) {
		i->second.added++;
	} else {
		i->second.released++;
	}

	// XXX: example

	return;
}

NAN_METHOD (HeapDiff::End) {
	HeapDiff *t = Unwrap<HeapDiff>( info.This() );
	// How shall we deal with double .end()ing?  The only reasonable
	// approach seems to be an exception, cause nothing else makes
	// sense.
	if (t->ended) {
		return Nan::ThrowError(Nan::New<v8::String>("attempt to end() a HeapDiff that was already ended").ToLocalChecked());
	}

	t->ended = true;
	inProgress = true;
	t->after = NanTakeHeapSnapshot;
	inProgress = false;

	Nan::HandleScope scope;
	int s, diffBytes;

	v8::Local<v8::Object> comparison = Nan::New<v8::Object>();

	// first let's append summary information
	v8::Local<v8::Object> b = Nan::New<v8::Object>();
	b->Set(Nan::New<v8::String>("nodes").ToLocalChecked(), Nan::New<v8::Number>(t->before->GetNodesCount()));
	//b->Set(NanNew("time"), s_startTime);
	comparison->Set(Nan::New<v8::String>("before").ToLocalChecked(), b);

	v8::Local<v8::Object> a = Nan::New<v8::Object>();
	a->Set(Nan::New<v8::String>("nodes").ToLocalChecked(), Nan::New<v8::Number>(t->after->GetNodesCount()));
	//a->Set(NanNew("time"), time(NULL));
	comparison->Set(Nan::New<v8::String>("after").ToLocalChecked(), a);

	// now let's get allocations by name
	set<uint64_t> beforeIDs, afterIDs;
	s = 0;
	buildIDSet(&beforeIDs, t->before->GetRoot(), s);
	b->Set(Nan::New<v8::String>("size_bytes").ToLocalChecked(), Nan::New<v8::Number>(s));
	b->Set(Nan::New<v8::String>("size").ToLocalChecked(), Nan::New<v8::String>(mw_util::niceSize(s).c_str()).ToLocalChecked());

	diffBytes = s;
	s = 0;
	buildIDSet(&afterIDs, t->after->GetRoot(), s);
	a->Set(Nan::New<v8::String>("size_bytes").ToLocalChecked(), Nan::New<v8::Number>(s));
	a->Set(Nan::New<v8::String>("size").ToLocalChecked(), Nan::New<v8::String>(mw_util::niceSize(s).c_str()).ToLocalChecked());

	diffBytes = s - diffBytes;

	v8::Local<v8::Object> c = Nan::New<v8::Object>();
	c->Set(Nan::New<v8::String>("size_bytes").ToLocalChecked(), Nan::New<v8::Number>(diffBytes));
	c->Set(Nan::New<v8::String>("size").ToLocalChecked(), Nan::New<v8::String>(mw_util::niceSize(diffBytes).c_str()).ToLocalChecked());
	comparison->Set(Nan::New<v8::String>("change").ToLocalChecked(), c);

	// before - after will reveal nodes released (memory freed)
	vector<uint64_t> changedIDs;
	setDiff(beforeIDs, afterIDs, changedIDs);
	c->Set(Nan::New<v8::String>("freed_nodes").ToLocalChecked(), Nan::New<v8::Number>(changedIDs.size()));

	// here's where we'll collect all the summary information
	changeset changes;

	// for each of these nodes, let's aggregate the change information
	for (unsigned long i = 0; i < changedIDs.size(); i++) {
		const v8::HeapGraphNode * n = t->before->GetNodeById(changedIDs[i]);
		manageChange(changes, n, false);
	}

	changedIDs.clear();

	// after - before will reveal nodes added (memory allocated)
	setDiff(afterIDs, beforeIDs, changedIDs);

	c->Set(Nan::New<v8::String>("allocated_nodes").ToLocalChecked(), Nan::New<v8::Number>(changedIDs.size()));

	for (unsigned long i = 0; i < changedIDs.size(); i++) {
		const v8::HeapGraphNode * n = t->after->GetNodeById(changedIDs[i]);
		manageChange(changes, n, true);
	}

	v8::Local<v8::Array> changeDetails = Nan::New<v8::Array>();

	for (changeset::iterator i = changes.begin(); i != changes.end(); i++) {
		v8::Local<v8::Object> d = Nan::New<v8::Object>();
		d->Set(Nan::New<v8::String>("what").ToLocalChecked(), Nan::New<v8::String>(i->first.c_str()).ToLocalChecked());
		d->Set(Nan::New<v8::String>("size_bytes").ToLocalChecked(), Nan::New<v8::Number>(i->second.size));
		d->Set(Nan::New<v8::String>("size").ToLocalChecked(), Nan::New<v8::String>(mw_util::niceSize(i->second.size).c_str()).ToLocalChecked());
		d->Set(Nan::New<v8::String>("+").ToLocalChecked(), Nan::New<v8::Number>(i->second.added));
		d->Set(Nan::New<v8::String>("-").ToLocalChecked(), Nan::New<v8::Number>(i->second.released));
		changeDetails->Set(changeDetails->Length(), d);
	}

	c->Set(Nan::New<v8::String>("details").ToLocalChecked(), changeDetails);

	// free early, free often.  I mean, after all, this process we're in is
	// probably having memory problems.  We want to help her.
	((v8::HeapSnapshot *) t->before)->Delete();
	t->before = NULL;
	((v8::HeapSnapshot *) t->after)->Delete();
	t->after = NULL;

	info.GetReturnValue().Set(comparison);
}
