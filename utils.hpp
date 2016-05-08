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
#ifndef GECODE_COUTING_BASE_SEARCH_UTILS_H
#define GECODE_COUTING_BASE_SEARCH_UTILS_H

#include <algorithm>

// TODO: Remplacer ça... Il doit avoir une manière de faire ça dans Gecode sans ces fonctions.

// Return the maximum domain value of all the variables in T
template<class T>
int minDomVal(const T &x) {
    auto v = std::min_element(x.begin(), x.end(), [](auto a, auto b) {
        return a.min() < b.min();
    });
    return v->min();
}

// Return the minimum domain value of all the variables in T
template<class T>
int maxDomVal(const T &x) {
    auto v = std::max_element(x.begin(), x.end(), [](auto a, auto b) {
        return a.max() < b.max();
    });
    return v->max();
}

#endif //GECODE_COUTING_BASE_SEARCH_UTILS_H
