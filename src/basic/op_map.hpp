#include <unordered_map>


using tokentype = mmmpsr::MimiumParser::token::yytokentype;

static std::unordered_map<std::string, tokentype> op_map {
    {"+",mmmpsr::MimiumParser::token::ADD},
    {"-",mmmpsr::MimiumParser::token::SUB},
    {"*",mmmpsr::MimiumParser::token::MUL},
    {"/",mmmpsr::MimiumParser::token::DIV},
    {"^",mmmpsr::MimiumParser::token::EXPONENT},
    {"%",mmmpsr::MimiumParser::token::MOD},
    {"&",mmmpsr::MimiumParser::token::AND},
    {"|",mmmpsr::MimiumParser::token::OR},
    {"&&",mmmpsr::MimiumParser::token::BITAND},
    {"||",mmmpsr::MimiumParser::token::BITAND},
    {"==",mmmpsr::MimiumParser::token::EQ},
    {"!=",mmmpsr::MimiumParser::token::NEQ}
};
