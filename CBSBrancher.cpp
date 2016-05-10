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
#include "CBSBrancher.h"

#include "CBSPosValChoice.hpp"

#include <functional>

/***********************************************************************************************************************
 * CBSBrancher
 **********************************************************************************************************************/

CBSBrancher::CBSBrancher(Space &home, std::vector<CBSConstraint*> &constraints,
                         std::function<bool(double,double)> densityComparator)
        : _constraints(constraints), _densityComparator(densityComparator), Brancher(home) {
    /**
     * Some constraints share precomputed data structures. For this reason, each constraint must gives information about
     * the domain of its variables so the precomputed data structures are usable for all constraints.
     *
     * As an exemple, we can consider the all different constraint. This constraint needs a precomputed array that spans
     * from 1 to n (n being the domain size of the variable with the largest domain in the constraint). If we want to
     * share this structure, we must take into account all counstraints.
     */
    int largestDomainSize = 0;
    int highestNumberOfVars = 0;

    for (auto& c : _constraints) {
        largestDomainSize = std::max(largestDomainSize, c->maxDomValue() - c->minDomValue() + 1);
        highestNumberOfVars = std::max(highestNumberOfVars, c->size());
    }

    for (auto& c : _constraints)
        c->precomputeDataStruct(highestNumberOfVars, largestDomainSize);
}

void CBSBrancher::post(Space &home, std::vector<CBSConstraint*> &constraints,
                       std::function<bool(double,double)> densityComparator) {
    (void) new(home) CBSBrancher(home, constraints, densityComparator);
}

CBSBrancher::CBSBrancher(Space &home, bool share, CBSBrancher &b)
        : _densityComparator(b._densityComparator), Brancher(home, share, b) {
    // We copy all constraints
    _constraints.reserve(b._constraints.size());
    for (auto& c : b._constraints)
        _constraints.push_back(c->copy(home, share, c));
}

Brancher *CBSBrancher::copy(Space &home, bool share) {
    return new(home) CBSBrancher(home, share, *this);
}

bool CBSBrancher::status(const Space &home) const {
    // To check if there's still work to do, we must ask each constraint if there are unassigned variables.
    for (auto const &c : _constraints)
        if (!c->allAssigned())
            return true;
    return false;
}

const Choice *CBSBrancher::choice(Space &home) {
    assert(status(home));
    int cIdx = 0;
    // We search for a constraint whose variables are not all assigned
    while (_constraints[cIdx]->allAssigned()) {
        cIdx++;
        assert(cIdx < _constraints.size());
    }

    // Choice for the first constraint found
    auto choice = _constraints[cIdx]->getDensity(_densityComparator);

    // We will check if there's a better choice in the other constraints
    for (int i=cIdx+1; i< _constraints.size(); i++) {
        if (!_constraints[i]->allAssigned()) {
            auto posValDensity = _constraints[i]->getDensity(_densityComparator);
            // If this choice is better than the current one...
            if (_densityComparator(posValDensity.density, choice.density)) {
                cIdx = i;
                choice = posValDensity;
            }
        }
    }
    return new CBSPosValChoice<int>(*this, 2, choice.pos, choice.val, cIdx);
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