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
#ifndef CBS_ALLDIFFCBS_H
#define CBS_ALLDIFFCBS_H

#include <complex>

#include "CBSConstraint.hpp"
#include "ValToVar.h"


/**
 * All different couting base search constraint.
 *
 * The implementation is based on the paper "Counting-Based Search: Branching Heuristics for Constraint Satisfaction
 * Problems" by Gilles Pesant, Claude-Guy Quimper and Alessandro Zanarini. It uses two precomputed data structures
 * for calculating densities upper bounds (Minc and Brégman and Liang and Bai).
 */
class AllDiffCBS : public CBSConstraint {
public:
    AllDiffCBS(Space &home, const IntVarArgs &x);

    AllDiffCBS(Space &home, bool share, AllDiffCBS &c);

    CBSConstraint *copy(Space &home, bool share, CBSConstraint &c) override;

    CBSPosValDensity getDensity(std::function<bool(double,double)> comparator) const override;

    void precomputeDataStruct(int nbVar, int largestDomainSize) override;

private:
    /**
     * Factors precomputed for every value in the domain of x. Thoses factors are used to compute the Minc and Brégman
     * upper bound for the permanent.
     */
    class MincFactors {
    public:
        MincFactors();

        MincFactors(int largestDomainSize);

        MincFactors(const MincFactors &mf);

        double get(int domSize);

    private:
        // Recursive function for precomputing _mincFactors from 1..n
        double precomputeMincFactors(int n);

    private:
        double *mincFactors;
        int largestDomainSize;
    };

    /**
     * Factors precomputed for every index and domain size in x. Thoses factors are used to compute the Liang and Bai
     * upper bound for the permanent
     */
    class LiangBaiFactors {
    public:
        LiangBaiFactors();

        LiangBaiFactors(int nbVar, int largestDomainSize);

        LiangBaiFactors(const LiangBaiFactors &lb);

        double get(int index, int domSize);

    private:
        void precomputeLiangBaiFactors();

    private:
        double **liangBaiFactors;
        int nbVar;
        int largestDomainSize;
    };

private:
    ValToVarHandle _valToVarH;
    static MincFactors _mincFactors;
    static LiangBaiFactors _liangBaiFactors;
};

#endif //CBS_ALLDIFFCBS_H
