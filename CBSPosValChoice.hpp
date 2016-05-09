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
