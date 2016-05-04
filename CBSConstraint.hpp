//
// Created by sam on 29/04/16.
//

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

    virtual CBSPosValDensity getDensity(std::function<bool(double,double)> comparator) = 0;

    virtual void precomputeDataStruct(int nbVar, int largestDomainSize, int minValue) = 0;

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
        for (int i=0; i<_x.size(); i++)
            if (!_x[i].assigned())
                return false;
        return true;
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
