#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>

#include <numeric>

using namespace Gecode;

class AllDiffCBSBrancher : public Brancher {
protected:
    ViewArray<Int::IntView> x;
public:
    AllDiffCBSBrancher(Space &home, ViewArray<Int::IntView> &x)
            : Brancher(home), x(x), mincFactors(x), liangBaiFactors(x), densities(x) { }

    static void post(Space &home, ViewArray<Int::IntView> &x) {
        (void) new(home) AllDiffCBSBrancher(home, x);
    }

    AllDiffCBSBrancher(Space &home, bool share, AllDiffCBSBrancher &b)
            : Brancher(home, share, b), mincFactors(b.mincFactors),
              liangBaiFactors(b.liangBaiFactors), densities(b.densities) {
        x.update(home, share, b.x);
    }

    virtual Brancher *copy(Space &home, bool share) {
        return new(home) AllDiffCBSBrancher(home, share, *this);
    }

    /// Does the brancher has anything left to do
    virtual bool status(const Space &home) const {
        for (int i = 0; i < x.size(); i++)
            if (!x[i].assigned())
                return true;
        return false;
    }

    virtual const Choice *choice(Space &home) {
        // Minc and Brégman and Liang and Bai upper bound.
        struct UB { double minc; double liangBai; };
        int idx = 0;
        UB ub = std::accumulate(x.begin(), x.end(), UB{1, 1}, [&](UB acc, auto elem) {
            return UB{acc.minc * mincFactors.get(elem.size()),
                      acc.liangBai * liangBaiFactors.get(idx++, elem.size())};
        });

        // densities is shared between all instances of this brancher is all spaces. We have to clear its content before
        // using it.
        densities.clearMatrix();

        auto upperBoundUpdate = [&](UB &_ub, int index, int oldDomSize, int newDomSize) {
            _ub.minc *= mincFactors.get(newDomSize) / mincFactors.get(oldDomSize);
            _ub.liangBai *= liangBaiFactors.get(index, newDomSize) / liangBaiFactors.get(index, oldDomSize);
        };

        struct { int varidx; int val; double density; } next_assignment{0, 0, -1};
        for (int i = 0; i < x.size(); i++) {
            if (x[i].assigned()) {
                densities.set(i, x[i].val(), 1); // We have 100% chance taking the variable assigned value.
            } else {
                auto varUB = ub;
                upperBoundUpdate(varUB, i, x[i].size(), 1); // Assignation of the variable
                // We calculate the density for every value assignment for the variable
                double normalisationCte = 0;
                for (IntVarValues val(x[i]); val(); ++val) {
                    auto localUB = varUB;
                    // We update the upper bound for every variable affected by the assignation.
                    for (int j = 0; j < x.size(); j++)
                        if (x[j].in(val.val()) && j != i)
                            upperBoundUpdate(localUB, j, x[j].size(), x[j].size() - 1);
                    auto lowerUB = std::min(localUB.minc, sqrt(localUB.liangBai));
                    densities.set(i, val.val(), lowerUB);
                    normalisationCte += lowerUB;
                }
                // Normalisation
                for (IntVarValues val(x[i]); val(); ++val) {
                    auto d = densities.get(i, val.val()) / normalisationCte;
                    densities.set(i, val.val(), d);
                    // We keep track of the pair (var,val) with the highest density
                    if (d > next_assignment.density)
                        next_assignment = {i, val.val(), d};
                }
            }
        }

        return new PosValChoice<int>(*this, 2, next_assignment.varidx, next_assignment.val);
    }

    virtual const Choice *choice(const Space &, Archive &e) {
        int pos, val;
        e >> pos >> val;
        return new PosValChoice<int>(*this, 2, pos, val);
    }

    virtual ExecStatus commit(Space &home, const Choice &c, unsigned int a) {
        const PosValChoice<int> &pv = static_cast<const PosValChoice<int> &>(c);
        int pos = pv.pos().pos, val = pv.val();
        if (a == 0)
            return me_failed(x[pos].eq(home, val)) ? ES_FAILED : ES_OK;
        else
            return me_failed(x[pos].nq(home, val)) ? ES_FAILED : ES_OK;

    }

    virtual void print(const Space &home, const Choice &c, unsigned int a, std::ostream &o) const {
        const PosValChoice<int> &pv = static_cast<const PosValChoice<int> &>(c);
        int pos = pv.pos().pos, val = pv.val();
        if (a == 0)
            o << "x[" << pos << "] = " << val;
        else
            o << "x[" << pos << "] != " << val;
    }

private:
    /* Factors precomputed for every value in the domain of x. Thoses factors are used to compute the Minc and Brégman
     * upper bound for the permanent. */
    class MincFactors {
    public:
        MincFactors() { }

        MincFactors(const ViewArray<Int::IntView> &x) {
            auto var = std::max_element(x.begin(), x.end(), [](auto a, auto b) {
                return a.size() < b.size();
            });
            largestDomainSize = var->size();

            mincFactors = heap.alloc<double>(largestDomainSize);
            precomputeMincFactors(largestDomainSize);
        }

        MincFactors(const MincFactors &mf)
                : largestDomainSize(mf.largestDomainSize), mincFactors(mf.mincFactors) { }

        double get(int domSize) {
            assert(domSize <= largestDomainSize);
            return mincFactors[domSize - 1];
        }

    private:
        /* Recursive function for precomputing mincFactors from 1..n */
        double precomputeMincFactors(int n) {
            if (n == 1) {
                mincFactors[0] = 1;
                return 1;
            } else {
                double fact = n * precomputeMincFactors(n - 1);
                mincFactors[n - 1] = pow(fact, 1.0 / n);
                return fact;
            }
        }

    private:
        double *mincFactors;
        int largestDomainSize;
    } mincFactors;

    /* Factors precomputed for every index and domain size in x. Thoses factors are used to compute the Liang and Bai
     * upper bound for the permanent */
    class LiangBaiFactors {
    public:
        LiangBaiFactors() { }

        LiangBaiFactors(const ViewArray<Int::IntView> &x) {
            auto var = std::max_element(x.begin(), x.end(), [](auto a, auto b) {
                return a.size() < b.size();
            });
            largestDomainSize = var->size();
            nbVars = x.size();

            liangBaiFactors = heap.alloc<double *>(nbVars);
            for (int i = 0; i < nbVars; i++)
                liangBaiFactors[i] = heap.alloc<double>(largestDomainSize);

            precomputeLiangBaiFactors();
        }

        LiangBaiFactors(const LiangBaiFactors &lb)
                : nbVars(lb.nbVars), largestDomainSize(lb.largestDomainSize), liangBaiFactors(lb.liangBaiFactors) { }

        double get(int index, int domSize) {
            assert(index < nbVars);
            assert(domSize <= largestDomainSize);
            return liangBaiFactors[index][domSize - 1];
        }

    private:
        void precomputeLiangBaiFactors() {
            for (int i = 1; i <= nbVars; i++) {
                double b = std::ceil(i / 2.0);
                for (int j = 1; j <= largestDomainSize; j++) {
                    double a = std::ceil((j + 1) / 2.0);
                    double q = std::min(a, b);
                    liangBaiFactors[i - 1][j - 1] = q * (j - q + 1);
                }
            }
        }

    private:
        double **liangBaiFactors;
        int nbVars;
        int largestDomainSize;
    } liangBaiFactors;

    /* Matrix for storing densities calculation.
     * densities is shared between all instances of this brancher in all spaces. This way, we can reuse it to save the
     * time needed for the allocation */
    class DensityMatrix {
    public:
        DensityMatrix(const ViewArray<Int::IntView> &x) {
            // Variable with the domain beginning at the minimum value
            auto varMinDomVal = std::min_element(x.begin(), x.end(), [](auto a, auto b) {
                return a.min() < b.min();
            });
            // Variable with the domain ending at the maximum value
            auto varMaxDomVal = std::min_element(x.begin(), x.end(), [](auto a, auto b) {
                return a.max() < b.max();
            });

            nbVar = x.size();
            minValue = varMinDomVal->min();
            domSize = varMaxDomVal->max() - minValue + 1;

            // We allocate space only once : we can reuse it for each computation.
            densities = heap.alloc<double *>(nbVar);
            for (int i = 0; i < nbVar; i++)
                densities[i] = heap.alloc<double>(domSize);
        }

        DensityMatrix(const DensityMatrix &dm)
                : densities(dm.densities), nbVar(dm.nbVar), domSize(dm.domSize), minValue(dm.minValue) { }

    public:
        void clearMatrix() {
            for (int i = 0; i < nbVar; i++)
                for (int j = 0; j < domSize; j++)
                    densities[i][j] = 0;
        }

        double get(int variablePos, int value) const {
            assert(variablePos < nbVar);
            assert(value - minValue < domSize);
            return densities[variablePos][value - minValue];
        }

        void set(int variablePos, int value, double density) {
            assert(variablePos < nbVar);
            assert(value - minValue < domSize);
            densities[variablePos][value - minValue] = density;
        }

    private:
        double **densities;
        int nbVar;
        int domSize;
        int minValue;
    } densities;
};

void alldiffcbsbrancher(Space &home, const IntVarArgs &x) {
    if (home.failed()) return;

    ViewArray<Int::IntView> y(home, x);
    AllDiffCBSBrancher::post(home, y);
}

class DummyProblem : public IntMaximizeSpace {
protected:
    IntVarArray l;
    IntVar maximize_this;
public:
    DummyProblem(void)
            : l(*this, 3, 0, 9), maximize_this(*this, 0, 100000) {
        distinct(*this, l);
        IntVar a(l[0]), b(l[1]), c(l[2]);
        rel(*this, maximize_this == 10 * l[0] + 9 * l[1] + 8 * l[2]);

        alldiffcbsbrancher(*this, l);
    }

    DummyProblem(bool share, DummyProblem &s)
            : IntMaximizeSpace(share, s) {
        l.update(*this, share, s.l);
        maximize_this.update(*this, share, s.maximize_this);
    }

    virtual Space *copy(bool share) {
        return new DummyProblem(share, *this);
    }

    void print(void) const {
        std::cout << l << std::endl;
    }

    virtual IntVar cost(void) const {
        return maximize_this;
    }
};

int main(int argc, char *argv[]) {
    DummyProblem *m = new DummyProblem;
    BAB<DummyProblem> e(m);
//  delete m;
    while (DummyProblem *s = e.next()) {
        s->print();
        delete s;
    }

    return 0;
}