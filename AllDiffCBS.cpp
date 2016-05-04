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
#include "AllDiffCBS.h"

/***********************************************************************************************************************
 * AllDiffCBS
 **********************************************************************************************************************/

// Static variables declaration
AllDiffCBS::MincFactors AllDiffCBS::mincFactors;
AllDiffCBS::LiangBaiFactors AllDiffCBS::liangBaiFactors;

AllDiffCBS::AllDiffCBS(Space &home, const IntVarArgs &x)
        : CBSConstraint(home, x) { }

AllDiffCBS::AllDiffCBS(Space &home, bool share, AllDiffCBS &c)
        : CBSConstraint(home, share, c) { }

CBSConstraint *AllDiffCBS::copy(Space &home, bool share, CBSConstraint &c) {
    return new AllDiffCBS(home, share, *this);
}

CBSPosValDensity AllDiffCBS::getDensity(std::function<bool(double,double)> comparator) const {
    assert(!_x.assigned());

    // Minc and Brégman and Liang and Bai upper bound.
    struct UB { double minc; double liangBai; };
    int idx = 0;
    UB ub = std::accumulate(_x.begin(), _x.end(), UB{1, 1}, [&](UB acc, auto elem) {
        return UB{acc.minc * mincFactors.get(elem.size()),
                  acc.liangBai * liangBaiFactors.get(idx++, elem.size())};
    });

    // Function for updating both upper bounds when a domain change.
    auto upperBoundUpdate = [&](UB &_ub, int index, int oldDomSize, int newDomSize) {
        _ub.minc *= mincFactors.get(newDomSize) / mincFactors.get(oldDomSize);
        _ub.liangBai *= liangBaiFactors.get(index, newDomSize) / liangBaiFactors.get(index, oldDomSize);
    };

    // Vector that span the domain of every variables for keeping densities. There's no need to set the vector to zero
    // at each iteration.
    std::vector<double> densities((unsigned long)_x.size());
    auto minDomVal = minDomValue(); // TODO: Regarder si il n'y aurait pas une meilleur façon de faire ça

    struct { int pos; int val; double density; } choice;
    bool first_choice = true;
    for (int i = 0; i < _x.size(); i++) {
        if (!_x[i].assigned()) {
            auto varUB = ub;
            upperBoundUpdate(varUB, i, _x[i].size(), 1); // Assignation of the variable
            double normalization = 0; // Normalization constant for keeping all densities values between 0 and 1
            // We calculate the density for every value assignment for the variable
            for (Gecode::IntVarValues val((IntVar)_x[i]); val(); ++val) {
                double *density = &densities[val.val() - minDomVal];
                auto localUB = varUB;
                // We update the upper bound for every variable affected by the assignation.
                for (int j = 0; j < _x.size(); j++) {
                    if (_x[j].in(val.val()) && j != i) {
                        upperBoundUpdate(localUB, j, _x[j].size(), _x[j].size() - 1);
                    }
                }
                auto lowerUB = std::min(localUB.minc, sqrt(localUB.liangBai));
                *density = lowerUB;
                normalization += lowerUB;
            }

            // Normalisation and choice selection
            for (Gecode::IntVarValues val((IntVar)_x[i]); val(); ++val) {
                double *density = &densities[val.val() - minDomVal];
                *density /= normalization;
                // Is this new density a better choice than our current one?
                if (comparator(*density, choice.density) || first_choice) {
                    choice = {i, val.val(), *density};
                    first_choice = false;
                }
            }
        }
    }

    return CBSPosValDensity{choice.pos, choice.val, choice.density};
}

void AllDiffCBS::precomputeDataStruct(int nbVar, int largestDomainSize) {
    mincFactors = MincFactors(largestDomainSize);
    liangBaiFactors = LiangBaiFactors(nbVar, largestDomainSize);
}


/***********************************************************************************************************************
 * MincFactors
 **********************************************************************************************************************/

AllDiffCBS::MincFactors::MincFactors() { }

AllDiffCBS::MincFactors::MincFactors(int largestDomainSize)
        : largestDomainSize(largestDomainSize) {
    mincFactors = heap.alloc<double>(largestDomainSize);
    precomputeMincFactors(largestDomainSize);
}

AllDiffCBS::MincFactors::MincFactors(const MincFactors &mf)
        : largestDomainSize(mf.largestDomainSize), mincFactors(mf.mincFactors) { }

double AllDiffCBS::MincFactors::get(int domSize) {
    assert(domSize <= largestDomainSize);
    return mincFactors[domSize - 1];
}

double AllDiffCBS::MincFactors::precomputeMincFactors(int n) {
    if (n == 1) {
        mincFactors[0] = 1;
        return 1;
    } else {
        double fact = n * precomputeMincFactors(n - 1);
        mincFactors[n - 1] = pow(fact, 1.0 / n);
        return fact;
    }
}


/***********************************************************************************************************************
 * LiangBaiFactors
 **********************************************************************************************************************/

AllDiffCBS::LiangBaiFactors::LiangBaiFactors() { }

AllDiffCBS::LiangBaiFactors::LiangBaiFactors(int nbVar, int largestDomainSize)
        : nbVar(nbVar), largestDomainSize(largestDomainSize) {
    liangBaiFactors = heap.alloc<double *>(nbVar);
    for (int i = 0; i < nbVar; i++)
        liangBaiFactors[i] = heap.alloc<double>(largestDomainSize);

    precomputeLiangBaiFactors();
}

AllDiffCBS::LiangBaiFactors::LiangBaiFactors(const LiangBaiFactors &lb)
        : nbVar(lb.nbVar), largestDomainSize(lb.largestDomainSize), liangBaiFactors(lb.liangBaiFactors) { }

double AllDiffCBS::LiangBaiFactors::get(int index, int domSize) {
    assert(index < nbVar);
    assert(domSize <= largestDomainSize);
    return liangBaiFactors[index][domSize - 1];
}

void AllDiffCBS::LiangBaiFactors::precomputeLiangBaiFactors() {
    for (int i = 1; i <= nbVar; i++) {
        double b = std::ceil(i / 2.0);
        for (int j = 1; j <= largestDomainSize; j++) {
            double a = std::ceil((j + 1) / 2.0);
            double q = std::min(a, b);
            liangBaiFactors[i - 1][j - 1] = q * (j - q + 1);
        }
    }
}
