/**
 *  Main author:
 *      Samuel Gagnon
 *
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
#ifndef CBS_CBSCONSTRAINT_H
#define CBS_CBSCONSTRAINT_H

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>

#include <functional>

using namespace Gecode;

struct CBSPosValDensity {
    int pos;
    int val;
    double density;
};

/**
 * Base class for all counting base search constraints.
 *
 * Its role is to give a generic interface for manipulating all cbs constraints in CBSBrancher.
 */
class CBSConstraint {
protected:
    ViewArray<Int::IntView> _x;
public:
    CBSConstraint(Space &home, const IntVarArgs &x) {
        ViewArray<Int::IntView> y(home, x);
        _x = y;
    }

    CBSConstraint(Space &home, bool share, CBSConstraint &c) {
        _x.update(home, share, c._x);
    }

    virtual CBSConstraint* copy(Space &home, bool share, CBSConstraint &c) = 0;

    virtual CBSPosValDensity getDensity(std::function<bool(double,double)> comparator) const = 0;

    virtual void precomputeDataStruct(int nbVar, int largestDomainSize) { }

    ExecStatus commit(Space &home, const Choice &c, unsigned int a) {
        const PosValChoice<int> &pvi = static_cast<const PosValChoice<int> &>(c);

        int pos = pvi.pos().pos, val = pvi.val();

        if (a == 0)
            return me_failed(_x[pos].eq(home, val)) ? ES_FAILED : ES_OK;
        else
            return me_failed(_x[pos].nq(home, val)) ? ES_FAILED : ES_OK;
    }

public:
    int size() const {
        return _x.size();
    }

    bool allAssigned() const {
        return _x.assigned();
    }

    // Return the minimum domain value of all the variables in the constraint
    int minDomValue() const {
        auto v = std::min_element(_x.begin(), _x.end(), [](auto a, auto b) {
            return a.min() < b.min();
        });
        return v->min();
    }

    // Return the maximum domain value of all the variables in the constraint
    int maxDomValue() const {
        auto v = std::max_element(_x.begin(), _x.end(), [](auto a, auto b) {
            return a.max() < b.max();
        });
        return v->max();
    }
};

#endif //CBS_CBSCONSTRAINT_H
