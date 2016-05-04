//
// Created by sam on 28/04/16.
//

#ifndef CBS_CBSPOSVALCHOICE_H
#define CBS_CBSPOSVALCHOICE_H

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>

using namespace Gecode;

// TODO: A quoi ca sert GECODE_VTABLE_EXPORT
template<class Val>
class GECODE_VTABLE_EXPORT CBSPosValChoice : public PosValChoice<Val> {
private:
    const int _arrayIdx;
public:
    CBSPosValChoice(const Brancher& b, unsigned int a, const Pos& p, const Val& n, int arrayIdx)
        : PosValChoice<Val>(b,a,p,n), _arrayIdx(arrayIdx) {}

    int arrayIdx(void) const {
        return _arrayIdx;
    }

    virtual size_t size(void) const {
        return sizeof(CBSPosValChoice<Val>);
    }

    virtual void archive(Archive& e) const {
        PosValChoice<Val>::archive(e);
        e << _arrayIdx;
    }

};

#endif //CBS_CBSPOSVALCHOICE_H
