#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>

#include "CBSBrancher.h"
#include "AllDiffCBS.h"

using namespace Gecode;

class DummyProblem : public IntMaximizeSpace {
protected:
    IntVarArray l1, l2;
    IntVar maximize_this;
public:
    DummyProblem(void)
            : l1(*this, 3, 0, 9),
              l2(*this, 3, 0, 9),
              maximize_this(*this, 0, 100000) {
        distinct(*this, l1);
        distinct(*this, l2);

        rel(*this, maximize_this == 10 * l1[0] + 9 * l1[1] + 8 * l1[2]
                                    + 10 * l2[0] + 9 * l2[1] + 8 * l2[2]);

        std::vector<CBSConstraint*> constraints{
                new AllDiffCBS(*this, l1),
                new AllDiffCBS(*this, l2)
        };

        cbsbrancher(*this, constraints);
    }

    DummyProblem(bool share, DummyProblem &s)
            : IntMaximizeSpace(share, s) {
        l1.update(*this, share, s.l1);
        l2.update(*this, share, s.l2);
        maximize_this.update(*this, share, s.maximize_this);
    }

    virtual Space *copy(bool share) {
        return new DummyProblem(share, *this);
    }

    void print(void) const {
        std::cout << l1 << " " << l2 << std::endl;
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