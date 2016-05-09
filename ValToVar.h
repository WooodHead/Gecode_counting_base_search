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
#ifndef GECODE_COUTING_BASE_SEARCH_VALTOVAR_H
#define GECODE_COUTING_BASE_SEARCH_VALTOVAR_H

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>

#include <set>

#include "utils.hpp"

using namespace Gecode;

using IntAlloc = space_allocator<int>;
using stdSet = std::set<int, std::less<int>, IntAlloc>;
using stdSetArray = std::vector<stdSet, IntAlloc>;

/**
 * TODO: Add description here
 */
class ValToVarHandle : public LocalHandle {
protected:
    class ValToVar : public LocalObject {
    public:
        stdSetArray _data;
        int _min; // offset

        ValToVar(Space &home, const IntVarArgs &x);

        ValToVar(Space& home, bool share, ValToVar & l);

        virtual LocalObject* copy(Space& home, bool share);

        virtual size_t dispose(Space&);
    };

public:
    ValToVarHandle(Space& home, const IntVarArgs& x);

    ValToVarHandle(const ValToVarHandle & li);

    ValToVarHandle & operator =(const ValToVarHandle & li);
    /// Get the possible variables for a value
    stdSet *get(int val) const;
    /// Remove a variable from a value
    void remove(int val, int var);
    /// Assign a value to a variable
    void assignVar(int val, int var);
};

/**
 * TODO: Add description here
 */
class ValToVarPropagator : public NaryPropagator<Int::IntView,PC_GEN_NONE> {
private:
    ValToVarHandle _valToVarH;
protected:
    using NaryPropagator<Int::IntView,PC_GEN_NONE>::x;
    /// Advisor with index and change information
    class ValToVarAdvisor : public Advisor {
    protected:
        /// Index and mark information
        int _info;
    public:
        /// Constructor for creation
        ValToVarAdvisor(Space& home, Propagator& p, Council<ValToVarAdvisor>& c, int i);
        /// Constructor for cloning \a a
        ValToVarAdvisor(Space& home, bool share, ValToVarAdvisor & a);
        /// Mark advisor as modified
        void mark(void);
        /// Mark advisor as unmodified
        void unmark(void);
        /// Whether advisor's view has been marked
        bool marked(void) const;
        /// Get index of view
        int idx(void) const;
    };
    /// The advisor council
    Council<ValToVarAdvisor> c;
    /// Constructor for cloning \a p
    ValToVarPropagator(Space& home, bool share, ValToVarPropagator & p);
public:
    /// Constructor for creation
    ValToVarPropagator(Home home, ViewArray<Int::IntView>& x, ValToVarHandle vtvH);
    /// Copy propagator during cloning
    virtual Propagator* copy(Space& home, bool share);
    /// Cost function (crazy so that propagator is likely to run last)
    virtual PropCost cost(const Space& home, const ModEventDelta& med) const;
    /// Give advice to propagator
    virtual ExecStatus advise(Space& home, Advisor& a, const Delta& d);
    /// Perform propagation
    virtual ExecStatus propagate(Space& home, const ModEventDelta& med);
    /// Delete propagator and return its size
    virtual size_t dispose(Space& home);
    /// Post activity recorder propagator
    static ExecStatus post(Home home, ViewArray<Int::IntView>& x, ValToVarHandle sharedInt);
};

#endif //GECODE_COUTING_BASE_SEARCH_VALTOVAR_H
