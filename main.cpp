// From example of cppcmb
/**
 * A common example limitation for regular languages is the language a^n b^n.
 * Parses user-input, and tells if it's a^n b^n, n > 0.
 */

#include <iostream>
#include <string>
#include "CppCmb/cppcmb.hpp"

namespace pc = cppcmb;

template <char Ch>
bool is_same_char(char c) { return c == Ch; }

template <char Ch>
inline constexpr auto match = pc::one[pc::filter(is_same_char<Ch>)];

cppcmb_decl(anbn, pc::product<>);
cppcmb_decl(anbn_impl, pc::product<>);

cppcmb_def(anbn) =
      anbn_impl & pc::end
    ;

cppcmb_def(anbn_impl) = pc::pass
    | (match<'a'> & anbn_impl & match<'b'>) [pc::select<>]
    | (match<'a'> & match<'b'>) [pc::select<>]
    ;

int main() {
    auto parser = pc::parser(anbn);
    std::string line;

    while (std::getline(std::cin, line)) {
        auto res = parser.parse(line);
        if (res.is_success()) {
            std::cout << "Input matches a^n b^n!" << std::endl;
        }
        else {
            std::cout << "Input does not match a^n b^n!" << std::endl;
        }
    }

    return 0;
}
