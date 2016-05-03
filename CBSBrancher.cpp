//
// Created by sam on 27/04/16.
//

#include "CBSBrancher.h"

#include "CBSPosValChoice.hpp"

/***********************************************************************************************************************
 * CBSBrancher
 **********************************************************************************************************************/

// TODO : Qu'est-ce que ça fait si on a une min value négative.
CBSBrancher::CBSBrancher(Space &home, std::vector<CBSConstraint*> &constraints)
        : _constraints(constraints), Brancher(home) {
    /**
     * We precompute data structures needed for every CBSConstraints
     */
    struct {int nbVar; int minValue; int domSize;} params{0, INT_MAX, 0};

    for (auto& c : _constraints) {
        // Variable with the domain beginning at the minimum value
        auto varMinDomVal = std::min_element(c->_x.begin(), c->_x.end(), [](auto a, auto b) {
            return a.min() < b.min();
        });
        // Variable with the domain ending at the maximum value
        auto varMaxDomVal = std::max_element(c->_x.begin(), c->_x.end(), [](auto a, auto b) {
            return a.max() < b.max();
        });

        params.nbVar = std::max(params.nbVar, c->_x.size());
        params.minValue = std::min(params.minValue, varMinDomVal->min());
        params.domSize = std::max(params.domSize, varMaxDomVal->max() - varMinDomVal->min() + 1);
    }

    for (auto& c : _constraints)
        c->precomputeDataStruct(params.nbVar, params.domSize, params.minValue);
}

void CBSBrancher::post(Space &home, std::vector<CBSConstraint*> &constraints) {
    (void) new(home) CBSBrancher(home, constraints);
}

// TODO: COPY CTR A MIEUX FAIRE
CBSBrancher::CBSBrancher(Space &home, bool share, CBSBrancher &b)
        : Brancher(home, share, b) {
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
        for (int j = 0; j < c->_x.size(); j++)
            if (!c->_x[j].assigned())
                return true;
    return false;
}

// TODO : Refaire ca ici, c'est affreux
const Choice *CBSBrancher::choice(Space &home) {
    struct wrapper {CBSPosValDensity cbspvd; int arrayIdx;};
    std::vector<wrapper> dens;
    for (int i=0; i<_constraints.size(); i++)
        dens.push_back(wrapper{_constraints[i]->getDensity(), i});

    auto choice = std::max_element(dens.begin(), dens.end(), [](auto a, auto b) {
        return a.cbspvd.density < b.cbspvd.density;
    });

    return new CBSPosValChoice<int>(*this, 2, choice->cbspvd.pos, choice->cbspvd.val, choice->arrayIdx);
}

// TODO: A quoi sert l'archive ici
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

void cbsbranch(Space &home, std::vector<CBSConstraint*> &constraints) {
    if (home.failed()) return;

    CBSBrancher::post(home, constraints);
}