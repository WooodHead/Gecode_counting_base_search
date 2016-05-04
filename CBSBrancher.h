//
// Created by sam on 27/04/16.
//

#ifndef CBS_ALLDIFFCBSBRANCHER_H
#define CBS_ALLDIFFCBSBRANCHER_H

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>

#include <vector>
#include "CBSConstraint.hpp"

using namespace Gecode;

class CBSBrancher : public Gecode::Brancher {
public:
    CBSBrancher(Space &home, std::vector<CBSConstraint*> &constraints,
                std::function<bool(double,double)> densityComparator);

    static void post(Space &home, std::vector<CBSConstraint*> &constraints,
                     std::function<bool(double,double)> densityComparator);

    CBSBrancher(Space &home, bool share, CBSBrancher &b);

    virtual Brancher *copy(Space &home, bool share);

    /// Does the brancher has anything left to do
    virtual bool status(const Space &home) const;

    virtual const Choice *choice(Space &home);

    virtual const Choice *choice(const Space &, Gecode::Archive &e);

    virtual ExecStatus commit(Space &home, const Choice &c, unsigned int a);

    virtual void print(const Space &home, const Choice &c, unsigned int a, std::ostream &o) const;

private:
    std::vector<CBSConstraint*> _constraints;
    std::function<bool(double,double)> _densityComparator;
};

void cbsbranch(Space &home, std::vector<CBSConstraint*> &constraints,
               std::function<bool(double, double)> densityComparator);


#endif //CBS_ALLDIFFCBSBRANCHER_H
