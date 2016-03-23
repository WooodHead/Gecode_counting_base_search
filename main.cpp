#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>

#include <set>

using namespace Gecode;

// TODO: Il faut remplacer cette fonction par quelque chose de plus sensé
template<class T>
void domainSize(const T& x, int& dom, int& min) {
    // We find the largest variable domain size in the ViewArray to precompute
    // Bregman and Minc factors
    min = x[0].min(); // Do I need to assert that there's at least one element here?
    int max = x[0].max();
    for (int i=1; i<x.size(); i++) {
        if (x[i].min() < min) {
            min = x[i].min();
        }
        if (x[i].max() > max) {
            max = x[i].max();
        }
    }
    dom =  max - min + 1;
}

/// LocalHandle for sharing data structures between the Propagator and the Brancher
class LI : public LocalHandle {
    // TODO: Trouver une solution pour mettre ca en dehors de la classe......
    typedef space_allocator<int> IntAlloc;
    typedef std::set<int, std::less<int>, IntAlloc > Set;
    typedef std::vector<Set, IntAlloc> SetArray;
protected:
    class LIO : public LocalObject {
    public:
        SetArray _data;
        int _min; // offset
        LIO(Space& home, const IntVarArgs& x) : LocalObject(home), _data(SetArray::allocator_type(home)) {
            int dom;
            domainSize(x, dom, _min);
            _data.resize(dom, Set(Set::key_compare(), Set::allocator_type(home)));
            for (int i=0; i<x.size(); ++i) {
                for (int j=x[i].min(); j<=x[i].max(); j++) {
                    int val = j - _min;
                    _data[val].insert(i);
                }
            }
        }
        LIO(Space& home, bool share, LIO& l)
                : LocalObject(home,share,l),
                  _data(l._data.begin(), l._data.end(), SetArray::allocator_type(home)) {}
        virtual LocalObject* copy(Space& home, bool share) {
            return new (home) LIO(home,share,*this);
        }
        virtual size_t dispose(Space&) { return sizeof(*this); }
    };
public:
    LI(Space& home, const IntVarArgs& x)
            : LocalHandle(new (home) LIO(home,x)) {}
    LI(const LI& li)
            : LocalHandle(li) {}
    LI& operator =(const LI& li) {
        return static_cast<LI&>(LocalHandle::operator =(li));
    }
    Set get(int val) const {
        LIO *lio = static_cast<LIO*>(object());
        int offset = lio->_min;
        return lio->_data[val - offset];
    }
    void remove(int val, int var) {
        LIO *lio = static_cast<LIO*>(object());
        int offset = lio->_min;
        lio->_data[val - offset].erase(var);
    }
};

/// Propagator for recording activity information
template<class View>
class DummyPropagator : public NaryPropagator<View,PC_GEN_NONE> {
    // TODO: Trouver une solution pour mettre ca en dehors de la classe......
    typedef space_allocator<int> IntAlloc;
    typedef std::set<int, std::less<int>, IntAlloc > Set;
    typedef std::vector<Set, IntAlloc> SetArray;
public:
    LI _sharedInt;
protected:
    using NaryPropagator<View,PC_GEN_NONE>::x;
    /// Advisor with index and change information
    class DummyAdvisor : public Advisor {
    protected:
        /// Index and mark information
        int _info;
    public:
        /// Constructor for creation
        DummyAdvisor(Space& home, Propagator& p, Council<DummyAdvisor>& c, int i)
                : Advisor(home,p,c), _info(i << 1) {}
        /// Constructor for cloning \a a
        DummyAdvisor(Space& home, bool share, DummyAdvisor& a)
                : Advisor(home,share,a), _info(a._info) {}
        /// Mark advisor as modified
        void mark(void) {
            _info |= 1;
        }
        /// Mark advisor as unmodified
        void unmark(void) {
            _info &= ~1;
        }
        /// Whether advisor's view has been marked
        bool marked(void) const {
            return ( _info & 1) != 0;
        }
        /// Get index of view
        int idx(void) const {
            return _info >> 1;
        }
    };
    /// The advisor council
    Council<DummyAdvisor> c;
    /// Constructor for cloning \a p
    DummyPropagator(Space& home, bool share, DummyPropagator<View>& p)
            : NaryPropagator<View,PC_GEN_NONE>(home,share,p), _sharedInt(p._sharedInt) {
        c.update(home, share, p.c);
    }
public:
    /// Constructor for creation
    DummyPropagator(Home home, ViewArray<View>& x, LI sharedInt)
            : NaryPropagator<View,PC_GEN_NONE>(home,x), c(home), _sharedInt(sharedInt) {
        home.notice(*this,AP_DISPOSE);
        for (int i=x.size(); i--; )
            if (!x[i].assigned())
                x[i].subscribe(home,*new (home) DummyAdvisor(home,*this,c,i));
    }
    /// Copy propagator during cloning
    virtual Propagator* copy(Space& home, bool share) {
        return new (home) DummyPropagator<View>(home, share, *this);
    }
    /// Cost function (crazy so that propagator is likely to run last)
    virtual PropCost cost(const Space& home, const ModEventDelta& med) const {
        return PropCost::crazy(PropCost::HI,1000);
    }
    /// Give advice to propagator
    virtual ExecStatus advise(Space& home, Advisor& a, const Delta& d) {
        DummyAdvisor& ad = static_cast<DummyAdvisor&>(a);
        ad.mark();
        int varIdx = ad.idx();
        if (!x[varIdx].any(d)) {
            int min = x[varIdx].min(d);
            int max = x[varIdx].max(d);
            for (int val=min; val<=max; val++) {
//        std::cout << "remove " << val << " for " << varIdx << std::endl;
                _sharedInt.remove(val, varIdx);
            }
        }
        return ES_NOFIX;
    }
    /// Perform propagation
    virtual ExecStatus propagate(Space& home, const ModEventDelta& med) {
        for (Advisors<DummyAdvisor> as(c); as(); ++as) {
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
    /// Delete propagator and return its size
    virtual size_t dispose(Space& home) {
        // Delete access to activity information
        home.ignore(*this,AP_DISPOSE);
        // Cancel remaining advisors
        for (Advisors<DummyAdvisor> as(c); as(); ++as) {
            x[as.advisor().idx()].cancel(home,as.advisor());
        }
        c.dispose(home);
        (void) NaryPropagator<View,PC_GEN_NONE>::dispose(home);
        return sizeof(*this);
    }
    /// Post activity recorder propagator
    static ExecStatus post(Home home, ViewArray<View>& x, LI sharedInt) {
        (void) new (home) DummyPropagator<View>(home,x,sharedInt);
        return ES_OK;
    }
};

class DummyBrancher : public Brancher {
    // TODO: Trouver une solution pour mettre ca en dehors de la classe......
    typedef space_allocator<int> IntAlloc;
    typedef std::set<int, std::less<int>, IntAlloc > Set;
    typedef std::vector<Set, IntAlloc> SetArray;
public:
    LI _sharedInt;
protected:
    ViewArray<Int::IntView> x;
public:
    /// Default constructor
    DummyBrancher(Space& home, ViewArray<Int::IntView>& x, LI sharedInt)
            : Brancher(home), x(x), _sharedInt(sharedInt) {
        // TODO: Call redondant
        int dom, min;
        domainSize(x, dom, min);
        mincFactors = home.alloc<double>(dom);
        precomputeMincFactors(dom);
    }
    /// Post brancher
    static void post(Space& home, ViewArray<Int::IntView>& x, LI sharedInt) {
        (void) new (home) DummyBrancher(home,x,sharedInt);
    }
    /// Copy constructor
    DummyBrancher(Space& home, bool share, DummyBrancher& b)
            : Brancher(home,share,b), _sharedInt(b._sharedInt) {
        x.update(home,share,b.x);
    }
    /// Copy
    virtual Brancher* copy(Space& home, bool share) {
        return new (home) DummyBrancher(home,share,*this);
    }
    /// Does the brancher has anything left to do
    virtual bool status(const Space& home) const {
        for (int i = 0; i < x.size(); i++)
            if (!x[i].assigned())
                return true;
        return false;
    }
    /// Make choice
    virtual Choice* choice(Space& home) {
        for (int i=0; true; i++)
            if (!x[i].assigned())
                return new PosValChoice<int>(*this,2,i,x[i].min());
    }
    /// Make choice with archive
    virtual Choice* choice(const Space&, Archive& e) {
        int pos, val;
        e >> pos >> val;
        return new PosValChoice<int>(*this,2,pos,val);
    }
    /// Commit to a choice
    virtual ExecStatus commit(Space& home, const Choice& c, unsigned int a) {
        for (int i=0; i<10; i++) {
            std::cout << "valToVar " << i << ": ";
            Set ss(_sharedInt.get(i));
            for (Set::iterator it = ss.begin(); it != ss.end(); ++it) {
                std::cout << *it << ", ";
            }
            std::cout << std::endl;
        }

        const PosValChoice<int>& pv = static_cast<const PosValChoice<int>&>(c);
        int pos=pv.pos().pos, val=pv.val();
        if (a == 0)
            return me_failed(x[pos].eq(home,val)) ? ES_FAILED : ES_OK;
        else
            return me_failed(x[pos].nq(home,val)) ? ES_FAILED : ES_OK;

    }
    /// Print Choice
    virtual void print(const Space& home, const Choice& c,
                       unsigned int a, std::ostream& o) const {
        const PosValChoice<int>& pv = static_cast<const PosValChoice<int>&>(c);
        int pos=pv.pos().pos, val=pv.val();
        if (a == 0)
            o << "x[" << pos << "] = " << val;
        else
            o << "x[" << pos << "] != " << val;
    }

private:
    double precomputeMincFactors(int n) {
        if (n==1) {
            mincFactors[0] = 1;
            return 1;
        } else {
            double fact = n * precomputeMincFactors(n-1);
            mincFactors[n-1] = pow(fact, 1.0/n);
            return fact;
        }
    }

private:
    double* mincFactors;

};

void dummybrancher(Space& home, const IntVarArgs& x) {
    if (home.failed()) return;


    LI sharedInt(home, x);

    ViewArray<Int::IntView> y(home,x);
    // TODO: Call redondant
    int dom, min;
    domainSize(y, dom, min);

    DummyPropagator<Int::IntView>::post(home,y,sharedInt);
    DummyBrancher::post(home,y,sharedInt);
}

class DummyProblem : public IntMaximizeSpace {
protected:
    IntVarArray l;
    IntVar maximize_this;
public:
    DummyProblem(void)
            : l(*this, 3, 0, 9) , maximize_this(*this,0,100000) {
        distinct(*this, l);
        IntVar a(l[0]), b(l[1]), c(l[2]);
        rel(*this, maximize_this == 10*l[0] + 9*l[1] + 8*l[2]);

        dummybrancher(*this, l);
    }

    DummyProblem(bool share, DummyProblem & s)
            : IntMaximizeSpace(share, s) {
        l.update(*this, share, s.l);
        maximize_this.update(*this, share, s.maximize_this);
    }

    virtual Space* copy(bool share) {
        return new DummyProblem(share,*this);
    }

    void print(void) const {
        std::cout << l << std::endl;
    }

    virtual IntVar cost(void) const {
        return maximize_this;
    }
};

int main(int argc, char* argv[]) {
    DummyProblem * m = new DummyProblem;
    BAB<DummyProblem> e(m);
//  delete m;
    while (DummyProblem * s = e.next()) {
        s->print(); delete s;
    }

    return 0;
}