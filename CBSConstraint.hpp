//
// Created by sam on 29/04/16.
//

#ifndef CBS_CBSCONSTRAINT_H
#define CBS_CBSCONSTRAINT_H

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>

using namespace Gecode;

struct CBSPosValDensity {
    int pos;
    int val;
    double density;
};

// TODO: Trouver si c'est possible de donner View et val en meme temps...
//template<typename View, typename Val>
class CBSConstraint {
public:
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

    virtual CBSPosValDensity getDensity() = 0;

    virtual void precomputeDataStruct(int nbVar, int largestDomainSize, int minValue) = 0;

    ExecStatus commit(Space &home, const Choice &c, unsigned int a) {
        const PosValChoice<int> &pvi = static_cast<const PosValChoice<int> &>(c);

        int pos = pvi.pos().pos, val = pvi.val();

        if (a == 0)
            return me_failed(_x[pos].eq(home, val)) ? ES_FAILED : ES_OK;
        else
            return me_failed(_x[pos].nq(home, val)) ? ES_FAILED : ES_OK;
    }
};

#endif //CBS_CBSCONSTRAINT_H
