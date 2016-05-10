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
#ifndef CBS_ALLDIFFCBSBRANCHER_H
#define CBS_ALLDIFFCBSBRANCHER_H

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>

#include <vector>
#include "CBSConstraint.hpp"

using namespace Gecode;

/**
 * Gestion of all the couting base search constraints.
 *
 * The role of the CBSBrancher is to keep track of all the constraints (via its vector of CBSConstraints). When the
 * brancher is asked for a choice, it computes the estimated solution density for all the pair (variable,value) in all
 * its constraints. It then choose one assignation (variable,value) according to its _densityComparator (for example
 * highest or lowest density).
 */
class CBSBrancher : public Gecode::Brancher {
public:
    CBSBrancher(Space &home, std::vector<CBSConstraint*> &constraints,
                std::function<bool(double,double)> densityComparator);

    static void post(Space &home, std::vector<CBSConstraint*> &constraints,
                     std::function<bool(double,double)> densityComparator);

    CBSBrancher(Space &home, bool share, CBSBrancher &b);

    virtual Brancher *copy(Space &home, bool share);
    // Does the brancher has anything left to do
    virtual bool status(const Space &home) const;
    // Choice based on all CBSConstraint in _constraints
    virtual const Choice *choice(Space &home);

    virtual const Choice *choice(const Space &, Gecode::Archive &e);

    virtual ExecStatus commit(Space &home, const Choice &c, unsigned int a);

    virtual void print(const Space &home, const Choice &c, unsigned int a, std::ostream &o) const;

private:
    // Every counting base search constraints
    using CBSConstraintVector = std::vector<CBSConstraint*, space_allocator<CBSConstraint*>>;
    CBSConstraintVector _constraints;
    // Density selection strategy for branching
    std::function<bool(double,double)> _densityComparator;
};

void cbsbranch(Space &home, std::vector<CBSConstraint*> &constraints,
               std::function<bool(double, double)> densityComparator);


#endif //CBS_ALLDIFFCBSBRANCHER_H
