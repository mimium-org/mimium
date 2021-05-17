/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <utility>

#include "basic/mir.hpp"
#include "compiler/type_infer_visitor.hpp"

namespace mimium {

using optvalptr = std::optional<mir::valueptr>;

mir::valueptr generateMir(LAst::expr const& expr, TypeEnvH const& typeenv);

}  // namespace mimium