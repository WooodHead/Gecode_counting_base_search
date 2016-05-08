/*
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ValToVar.h"

/***********************************************************************************************************************
 * ValToVarHandle::ValToVar
 **********************************************************************************************************************/

ValToVarHandle::ValToVar::ValToVar(Space &home, const IntVarArgs &x)
        : LocalObject(home), _data(stdSetArray::allocator_type(home)) {
    _min = minDomVal(x);
    int maxDomValue = maxDomVal(x);
    int spanningDomainSize = maxDomValue - _min + 1;

    assert(spanningDomainSize > 0);
    _data.resize((unsigned long)spanningDomainSize, stdSet(stdSet::key_compare(), stdSet::allocator_type(home)));

    for (int i=0; i<x.size(); ++i) {
        for (int j=x[i].min(); j<=x[i].max(); j++) {
            int val = j - _min;
            _data[val].insert(i);
        }
    }
}

ValToVarHandle::ValToVar::ValToVar(Space& home, bool share, ValToVar & l)
        : LocalObject(home,share,l),
          _data(stdSetArray::allocator_type(home)),
          _min(l._min) {
    _data.resize(l._data.size(), stdSet(stdSet::key_compare(), stdSet::allocator_type(home)));
    for (int i=0; i<_data.size(); i++) {
        _data[i] = l._data[i];
    }
}

LocalObject* ValToVarHandle::ValToVar::copy(Space& home, bool share) {
    return new (home) ValToVar(home,share,*this);
}

size_t ValToVarHandle::ValToVar::dispose(Space&) {
    return sizeof(*this);
}


/***********************************************************************************************************************
 * ValToVarHandle
 **********************************************************************************************************************/

ValToVarHandle::ValToVarHandle(Space& home, const IntVarArgs& x)
        : LocalHandle(new (home) ValToVar(home,x)) {}

ValToVarHandle::ValToVarHandle(const ValToVarHandle & li)
        : LocalHandle(li) {}

ValToVarHandle &ValToVarHandle::operator =(const ValToVarHandle & li) {
    return static_cast<ValToVarHandle &>(LocalHandle::operator =(li));
}

stdSet *ValToVarHandle::get(int val) const {
    ValToVar *lio = static_cast<ValToVar *>(object());
    int offset = lio->_min;
    return &lio->_data[val - offset];
}

void ValToVarHandle::remove(int val, int var) {
    ValToVar *lio = static_cast<ValToVar *>(object());
    int offset = lio->_min;
    lio->_data[val - offset].erase(var);
}


/***********************************************************************************************************************
 * ValToVarPropagator::ValToVarAdvisor
 **********************************************************************************************************************/

ValToVarPropagator::ValToVarAdvisor::ValToVarAdvisor(Space& home, Propagator& p, Council<ValToVarAdvisor>& c, int i)
        : Advisor(home,p,c), _info(i << 1) {}

ValToVarPropagator::ValToVarAdvisor::ValToVarAdvisor(Space& home, bool share, ValToVarAdvisor & a)
        : Advisor(home,share,a), _info(a._info) {}

void ValToVarPropagator::ValToVarAdvisor::mark(void) {
    _info |= 1;
}

void ValToVarPropagator::ValToVarAdvisor::unmark(void) {
    _info &= ~1;
}

bool ValToVarPropagator::ValToVarAdvisor::marked(void) const {
    return ( _info & 1) != 0;
}

int ValToVarPropagator::ValToVarAdvisor::idx(void) const {
    return _info >> 1;
}


/***********************************************************************************************************************
 * ValToVarPropagator
 **********************************************************************************************************************/

ValToVarPropagator::ValToVarPropagator(Space& home, bool share, ValToVarPropagator & p)
        : NaryPropagator<Int::IntView,PC_GEN_NONE>(home,share,p), _valToVarH(p._valToVarH) {
    c.update(home, share, p.c);
    _valToVarH.update(home, share, p._valToVarH);
}

ValToVarPropagator::ValToVarPropagator(Home home, ViewArray<Int::IntView>& x, ValToVarHandle sharedInt)
        : NaryPropagator<Int::IntView,PC_GEN_NONE>(home,x), c(home), _valToVarH(sharedInt) {
    home.notice(*this,AP_DISPOSE);
    for (int i=x.size(); i--; )
        if (!x[i].assigned())
            x[i].subscribe(home,*new (home) ValToVarAdvisor(home,*this,c,i));
}

Propagator* ValToVarPropagator::copy(Space& home, bool share) {
    return new (home) ValToVarPropagator(home, share, *this);
}

PropCost ValToVarPropagator::cost(const Space& home, const ModEventDelta& med) const {
    return PropCost::crazy(PropCost::HI,1000);
}

ExecStatus ValToVarPropagator::advise(Space& home, Advisor& a, const Delta& d) {
    ValToVarAdvisor & ad = static_cast<ValToVarAdvisor &>(a);
    ad.mark();
    int varIdx = ad.idx();
    if (!x[varIdx].any(d)) {
        int min = x[varIdx].min(d);
        int max = x[varIdx].max(d);
        for (int val=min; val<=max; val++) {
            _valToVarH.remove(val, varIdx);
        }
    }
    return ES_NOFIX;
}

ExecStatus ValToVarPropagator::propagate(Space& home, const ModEventDelta& med) {
    for (Advisors<ValToVarAdvisor> as(c); as(); ++as) {
        int i = as.advisor().idx();
        if (as.advisor().marked()) {
            as.advisor().unmark();
            if (x[i].assigned())
                as.advisor().dispose(home,c);
        } else {
            assert(!x[i].assigned());
        }
    }
    return c.empty() ? home.ES_SUBSUMED(*this) : ES_FIX;
}

size_t ValToVarPropagator::dispose(Space& home) {
    // Delete access to activity information
    home.ignore(*this,AP_DISPOSE);
    // Cancel remaining advisors
    for (Advisors<ValToVarAdvisor> as(c); as(); ++as) {
        x[as.advisor().idx()].cancel(home,as.advisor());
    }
    c.dispose(home);
    (void) NaryPropagator<Int::IntView,PC_GEN_NONE>::dispose(home);
    return sizeof(*this);
}

ExecStatus ValToVarPropagator::post(Home home, ViewArray<Int::IntView>& x, ValToVarHandle sharedInt) {
    (void) new (home) ValToVarPropagator(home,x,sharedInt);
    return ES_OK;
}

