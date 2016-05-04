//
// Created by sam on 29/04/16.
//

#include "AllDiffCBS.h"

/***********************************************************************************************************************
 * AllDiffCBS
 **********************************************************************************************************************/

// Static variables declaration
AllDiffCBS::MincFactors AllDiffCBS::mincFactors;
AllDiffCBS::LiangBaiFactors AllDiffCBS::liangBaiFactors;
AllDiffCBS::DensityMatrix AllDiffCBS::densityMatrix;


AllDiffCBS::AllDiffCBS(Space &home, const IntVarArgs &x)
        : CBSConstraint(home, x) { }

AllDiffCBS::AllDiffCBS(Space &home, bool share, AllDiffCBS &c)
        : CBSConstraint(home, share, c) { }

CBSConstraint *AllDiffCBS::copy(Space &home, bool share, CBSConstraint &c) {
    return new AllDiffCBS(home, share, *this);
}

CBSPosValDensity AllDiffCBS::getDensity(std::function<bool(double,double)> comparator) {
    assert(!_x.assigned());

    // Minc and Brégman and Liang and Bai upper bound.
    struct UB { double minc; double liangBai; };
    int idx = 0;
    UB ub = std::accumulate(_x.begin(), _x.end(), UB{1, 1}, [&](UB acc, auto elem) {
        return UB{acc.minc * mincFactors.get(elem.size()),
                  acc.liangBai * liangBaiFactors.get(idx++, elem.size())};
    });

    // densityMatrix is shared between all instances of this brancher is all spaces. We have to clear its content before
    // using it.
    densityMatrix.clearMatrix();

    // Function for updating both upper bounds when a domain change.
    auto upperBoundUpdate = [&](UB &_ub, int index, int oldDomSize, int newDomSize) {
        _ub.minc *= mincFactors.get(newDomSize) / mincFactors.get(oldDomSize);
        _ub.liangBai *= liangBaiFactors.get(index, newDomSize) / liangBaiFactors.get(index, oldDomSize);
    };

    struct { int pos; int val; double density; } next_assignment;
    bool first_assignment = true;
    for (int i = 0; i < _x.size(); i++) {
        if (_x[i].assigned()) {
            densityMatrix.set(i, _x[i].val(), 1); // We have 100% chance taking the variable assigned value.
        } else {
            auto varUB = ub;
            upperBoundUpdate(varUB, i, _x[i].size(), 1); // Assignation of the variable
            // We calculate the density for every value assignment for the variable
            double normalisationCte = 0;
            for (Gecode::IntVarValues val((IntVar)_x[i]); val(); ++val) {
                auto localUB = varUB;
                // We update the upper bound for every variable affected by the assignation.
                for (int j = 0; j < _x.size(); j++) {
                    if (_x[j].in(val.val()) && j != i) {
                        upperBoundUpdate(localUB, j, _x[j].size(), _x[j].size() - 1);
                    }
                }
                auto lowerUB = std::min(localUB.minc, sqrt(localUB.liangBai));
                densityMatrix.set(i, val.val(), lowerUB);
                normalisationCte += lowerUB;
            }
            // Normalisation
            for (Gecode::IntVarValues val((IntVar)_x[i]); val(); ++val) {
                auto d = densityMatrix.get(i, val.val()) / normalisationCte;
                densityMatrix.set(i, val.val(), d);
                // We keep track of the pair (var,val) that we want to return
                if (comparator(d, next_assignment.density) || first_assignment) {
                    next_assignment = {i, val.val(), d};
                    first_assignment = false;
                }
            }
        }
    }

    return CBSPosValDensity{next_assignment.pos, next_assignment.val, next_assignment.density};
}

//template<typename View, typename Val>
void AllDiffCBS::precomputeDataStruct(int nbVar, int largestDomainSize, int minValue) {
    mincFactors = MincFactors(largestDomainSize);
    liangBaiFactors = LiangBaiFactors(nbVar, largestDomainSize);
    densityMatrix = DensityMatrix(nbVar, largestDomainSize, minValue);
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

/***********************************************************************************************************************
 * DensityMatrix
 **********************************************************************************************************************/

AllDiffCBS::DensityMatrix::DensityMatrix() { }

AllDiffCBS::DensityMatrix::DensityMatrix(int nbVar, int largestDomainSize, int minValue)
        : nbVar(nbVar), largestDomainSize(largestDomainSize), minValue(minValue) {
    // We allocate space only once : we can reuse it for each computation.
    densities = heap.alloc<double *>(nbVar);
    for (int i = 0; i < nbVar; i++)
        densities[i] = heap.alloc<double>(largestDomainSize);
}

AllDiffCBS::DensityMatrix::DensityMatrix(const DensityMatrix &dm)
        : densities(dm.densities), nbVar(dm.nbVar), largestDomainSize(dm.largestDomainSize), minValue(dm.minValue) { }

void AllDiffCBS::DensityMatrix::clearMatrix() {
    for (int i = 0; i < nbVar; i++)
        for (int j = 0; j < largestDomainSize; j++)
            densities[i][j] = 0;
}

double AllDiffCBS::DensityMatrix::get(int variablePos, int value) const {
    assert(variablePos < nbVar);
    assert(value - minValue < largestDomainSize);
    return densities[variablePos][value - minValue];
}

void AllDiffCBS::DensityMatrix::set(int variablePos, int value, double density) {
    assert(variablePos < nbVar);
    assert(value - minValue < largestDomainSize);
    densities[variablePos][value - minValue] = density;
}
