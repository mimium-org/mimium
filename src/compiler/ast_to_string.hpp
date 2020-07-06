/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast_new.hpp"

namespace mimium {
class AstStringifier{
    public:
    private:

};

struct TermStringVisitor{
    void operator()(newast::Number ast);

    private:
    std::stringstream output;
};

}