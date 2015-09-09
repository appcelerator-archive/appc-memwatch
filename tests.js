const
should = require('should'),
memwatch = require('./');

describe('the library', function() {
  it('should export a couple functions', function(done) {
    should.exist(memwatch.gc);
    should(memwatch.on).be.a.function;
    should(memwatch.once).be.a.function;
    should(memwatch.removeAllListeners).be.a.function;
    should(memwatch.HeapDiff).be.an.object;
    should(memwatch.HeapDiff).be.a.function;
    done();
  });
});
describe('calling .gc()', function() {
    this.timeout(5000);
  it('should cause a stats() event to be emitted', function(done) {
    memwatch.once('stats', function(s) {
      should(s).be.an.object;
      done();
    });
    memwatch.gc();
  });
});

describe('HeapDiff', function() {
  it('should detect allocations', function(done) {
    function LeakingClass() {};
    var arr = [];
    var hd = new memwatch.HeapDiff();
    for (var i = 0; i < 100; i++) arr.push(new LeakingClass());
    var diff = hd.end();
    (Array.isArray(diff.change.details)).should.be.ok;
    diff.change.details.should.be.an.instanceOf(Array);
    // find the LeakingClass elem
    var leakingReport;
    diff.change.details.forEach(function(d) {
      if (d.what === 'LeakingClass')
        leakingReport = d;
    });
    should.exist(leakingReport);
    ((leakingReport['+'] - leakingReport['-']) > 0).should.be.ok;
    done();

  });
});

describe('HeapDiff', function() {
  it('double end should throw', function(done) {
    var hd = new memwatch.HeapDiff();
    var arr = [];
    (function() { hd.end(); }).should.not.throw();
    (function() { hd.end(); }).should.throw();
    done();
  });
});

describe('improper HeapDiff allocation', function() {
  it('should throw an exception', function(done) {
    // equivalent to "new require('memwatch').HeapDiff()"
    // see issue #30
    (function() { new (memwatch.HeapDiff()); }).should.throw();
    done();
  });
});
