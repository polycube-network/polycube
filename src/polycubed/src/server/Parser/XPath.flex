%{
#include <cstdint>
#include <string>
#include <memory>

#include "../Parser/XPathScanner.h"

using token = polycube::polycubed::Rest::Parser::XPathParser::token;
using token_type = polycube::polycubed::Rest::Parser::XPathParser::token_type;
#define yyterminate() return(token::END)
#define YY_NO_UNISTD_H
#define YY_USER_ACTION yylloc->columns(yyleng);
%}

%option nounput noinput nomain noyywrap nodefault
%option yyclass="XPathScanner"

NOT     not
TRUE    true\(\)
FALSE   false\(\)
OR      or
AND     and
EQ      =
NEQ     !=
LT      <
LTE     <=
GT      >
GTE     >=

CURRENT current\(\)|.
PARENT  ..
PATH    \/
ID      [a-zA-Z_][a-zA-Z0-9_.-]*
SQSRT   \'[^\']*\'
INTEGER [1-9][0-9]*
WS      [ \t]+

%%

%{
  yylloc->step();
%}

NOT     {return token::NOT;}
TRUE    {return token::TRUE;}
FALSE   {return token::FALSE;}
OR      {return token::OR;}
AND     {return token::AND;}
EQ      {return token::EQ;}
NEQ     {return token::NEQ;}
LT      {return token::LT;}
LTE     {return token::LTE;}
GT      {return token::GT;}
GTE     {return token::GTE;}
CURRENT {return token::CUR;}
PARENT  {return token::PAR;}
PATH    {return token::DEL;}
\(      {return token::RO;}
\)      {return token::RC;}
\[      {return token::SO;}
\]      {return token::SC;}
ID      {
          yylval->build<std::string>() = yytext;
          return token::ID;
        }
SQSRT   {
          yylval->build<std::string>() = yytext;
          return token::SQSTR;
        }
INTEGER {
          yylval->build<std::uint32_t>() = std::stoul(yytext, nullptr, 10);
          return token::INT;
        }

{WS}|.  {}
%%
namespace polycube::polycubed::Rest::Parser {
XPathScanner::XPathScanner(std::istream* in)
    : yyFlexLexer(in, 0){}

void XPathScanner::set_debug(bool b)
{
    yy_flex_debug = b;
}

#ifdef yylex
#undef yylex
#endif
}
