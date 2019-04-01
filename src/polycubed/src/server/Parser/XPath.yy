%{
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <memory>

#include <utility>
#include "../Resources/Body/Resource.h"
#include "../Resources/Body/LeafResource.h"
#include "../Resources/Body/LeafListResource.h"
#include "../Resources/Body/ParentResource.h"
#include "../Resources/Body/ListResource.h"
#include "../Resources/Body/Service.h"
%}

%require "3.0"
%skeleton "lalr1.cc"
%define api.namespace {polycube::polycubed::Rest::Parser}
%define parser_class_name {XPathParser}
%language "C++"
%no-lines

%code requires{
#include <cstdint>
#include <unordered_map>
namespace polycube::polycubed::Rest::Parser {
  class XPathParserDriver;
}
}

%define api.value.type variant
%define parse.assert

%parse-param {std::shared_ptr<XPathParserDriver> driver}

%locations

%token END    0     "end of file"

%token NOT
%token TRUE
%token FALSE
%token OR
%token AND
%token EQ
%token NEQ
%token LT
%token LTE
%token GT
%token GTE
%token CUR
%token PAR
%token DEL
%token RO
%token RC
%token SO
%token SC
%token <std::string> ID
%token <std::string> SQSTR
%token <std::uint32_t> INT

%left AND
%left OR
%left EQ NEQ LT LTE GT GTE

%type <bool> program;
%type <bool> expr;
%type <bool> bool_expr;
%type <std::string> start_path;
%type <std::string> path;

%type <std::unordered_map<std::string, std::string>> key;
%type <std::unordered_map<std::string, std::string>> keys;
%type <std::unordered_map<std::string, std::string>> key_list;

%{
#include "../Parser/XPathParserDriver.h"
#include "../Parser/XPathScanner.h"

#undef yylex
#define yylex driver->lexer->lex
%}

%%

program : END { $$ = false; } | expr END { $$ = $1; };

expr : start_path { $$ = !$1.empty() && $1 != "0"; } | bool_expr { $$ = $1; };

bool_expr : expr EQ expr { $$ = $1 == $3; } |
            expr NEQ expr { $$ = $1 != $3; } |
            expr LT expr { $$ = $1 < $3; } |
            expr LTE expr { $$ = $1 < $3; } |
            expr GT expr { $$ = $1 < $3; } |
            expr GTE expr { $$ = $1 < $3; } |
            NOT RO expr RC { $$ = !$3; } |
            expr AND expr { $$ = $1 && $3; } |
            expr OR expr { $$ = $1 || $3; } |
            TRUE { $$ = true; } |
            FALSE { $$ = false; };

start_path : DEL ID DEL ID {
               driver->current = dynamic_cast<const Resources::Body::Resource*>(driver->current->GetService($2).get());
               driver->current_cube_name = $4;
             } path { $$ = $6; } |
             CUR {
               auto leaf = dynamic_cast<const Resources::Body::LeafResource*>(driver->current);
               $$ = leaf->ReadValue(driver->current_cube_name, driver->keys).message;
             } |
             CUR DEL path { $$ = $3; } |
             PAR { YYABORT; } |
             PAR DEL { driver->current = driver->current->Parent(); } path { $$ = $4; } |
             ID {
               auto parent = dynamic_cast<const Resources::Body::ParentResource*>(driver->current);
               if (parent != nullptr) driver->current = parent->Child($1).get();
               else YYABORT;
             } |
             ID DEL {
               auto parent = dynamic_cast<const Resources::Body::ParentResource*>(driver->current);
               if (parent != nullptr) driver->current = parent->Child($1).get();
               else YYABORT;
             } path { $$ = $4; };

path : %empty {
         auto leaf = dynamic_cast<const Resources::Body::LeafResource*>(driver->current);
         if (leaf != nullptr) $$ = leaf->ReadValue(driver->current_cube_name, driver->keys).message;
       } |
       DEL path {} | CUR path {} |
       PAR path { driver->current = driver->current->Parent(); } |
       ID path  {
         auto parent = dynamic_cast<const Resources::Body::ParentResource*>(driver->current);
         if (parent != nullptr) driver->current = parent->Child($1).get();
         else YYABORT;
       } |
       ID key_list path {
         auto parent = dynamic_cast<const Resources::Body::ParentResource*>(driver->current);
         if (parent != nullptr) {
           driver->current = parent->Child($1).get();
           auto list = dynamic_cast<const Resources::Body::ListResource*>(driver->current);
           if (list != nullptr) {
             if (!list->ValidateKeys($2)) YYABORT;
           } else {
             // TODO threat a leaflist value as a key?
             YYABORT;
           }
         } else YYABORT;
       } |
       ID nkey_list path;

key_list : key_list keys {
             $1.merge($2);
             $$ = $1;
           } | keys { $$ = $1; };

keys : SO key SC { $$ = std::move($2); };

key : ID EQ SQSTR {
        std::unordered_map<std::string, std::string> r;
        r.emplace($1, $3.substr(1, $3.length() - 2));
        $$ = r;
      };

nkey_list : nkey_list nkeys | nkeys;

nkeys : SO INT SC;

%%

namespace polycube::polycubed::Rest::Parser {
void XPathParser::error([[maybe_unused]] const XPathParser::location_type& l,
                        const std::string& m) {
    driver->error(m);
}
}