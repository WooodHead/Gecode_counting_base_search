//
// Created by sam on 27/04/16.
//

#include "CBSBrancher.h"

#include "CBSPosValChoice.hpp"

#include <functional>

/***********************************************************************************************************************
 * CBSBrancher
 **********************************************************************************************************************/

// TODO : Qu'est-ce que ça fait si on a une min value négative.
CBSBrancher::CBSBrancher(Space &home, std::vector<CBSConstraint*> &constraints,
                         std::function<bool(double,double)> densityComparator)
        : _constraints(constraints), _densityComparator(densityComparator), Brancher(home) {
    /**
     * We precompute data structures needed for every CBSConstraints
     */
    struct {int nbVar; int minValue; int domSize;} params{0, INT_MAX, 0};

    for (auto& c : _constraints) {
        params.nbVar = std::max(params.nbVar, c->size());
        params.minValue = std::min(params.minValue, c->minDomValue());
        params.domSize = std::max(params.domSize, c->maxDomValue() - c->minDomValue() + 1);
    }

    for (auto& c : _constraints)
        c->precomputeDataStruct(params.nbVar, params.domSize, params.minValue);
}

void CBSBrancher::post(Space &home, std::vector<CBSConstraint*> &constraints,
                       std::function<bool(double,double)> densityComparator) {
    (void) new(home) CBSBrancher(home, constraints, densityComparator);
}

CBSBrancher::CBSBrancher(Space &home, bool share, CBSBrancher &b)
        : _densityComparator(b._densityComparator), Brancher(home, share, b) {
    _constraints.reserve(b._constraints.size());
    for (auto& c : b._constraints) {
        _constraints.push_back(c->copy(home, share, *c));
    }
}

Brancher *CBSBrancher::copy(Space &home, bool share) {
    return new(home) CBSBrancher(home, share, *this);
}

/// Does the brancher has anything left to do
bool CBSBrancher::status(const Space &home) const {
    for (auto const &c : _constraints)
        if (!c->allAssigned())
            return true;
    return false;
}

const Choice *CBSBrancher::choice(Space &home) {
    int cIdx = 0;
    CBSPosValDensity c = _constraints[cIdx]->getDensity(_densityComparator);
    for (int i=1; i< _constraints.size(); i++) {
        auto posValDensity = _constraints[i]->getDensity(_densityComparator);
        if (_densityComparator(posValDensity.density, c.density)) {
            cIdx = i;
            c = posValDensity;
        }
    }
    return new CBSPosValChoice<int>(*this, 2, c.pos, c.val, cIdx);
}

const Choice *CBSBrancher::choice(const Space &, Gecode::Archive &e) {
    int pos, val, arrayIdx;
    e >> pos >> val >> arrayIdx;
    return new CBSPosValChoice<int>(*this, 2, pos, val, arrayIdx);
}

Gecode::ExecStatus CBSBrancher::commit(Space &home, const Choice &c, unsigned int a) {
    const CBSPosValChoice<int> &pvi = static_cast<const CBSPosValChoice<int> &>(c);
    int pos = pvi.pos().pos, val = pvi.val(), arrayIdx = pvi.arrayIdx();

    _constraints[arrayIdx]->commit(home, c, a);
}

void CBSBrancher::print(const Space &home, const Choice &c, unsigned int a, std::ostream &o) const {
    const CBSPosValChoice<int> &pv = static_cast<const CBSPosValChoice<int> &>(c);
    int pos = pv.pos().pos, val = pv.val();
    if (a == 0)
        o << "x[" << pos << "] = " << val;
    else
        o << "x[" << pos << "] != " << val;
}


/***********************************************************************************************************************
 * Method to create brancher
 **********************************************************************************************************************/

void cbsbranch(Space &home, std::vector<CBSConstraint *> &constraints,
               std::function<bool(double, double)> densityComparator) {
    if (home.failed()) return;
    CBSBrancher::post(home, constraints, densityComparator);
}