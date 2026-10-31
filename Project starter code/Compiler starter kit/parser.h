#ifndef PARSER_H_
#define PARSER_H_

#include "scanner.h"
#include "error_handler.h"
#include <initializer_list>

// LL(1) recursive-descent parser for the syntax of lille.
// Semantic checks (types, scopes, matching END names) belong to the next phase.
class parser {
public:
    parser(scanner* scanner_ptr, error_handler* error_ptr);
    bool parse(); // Returns false after the first syntax error.

private:
    using S = symbol::symbol_type;
    struct parsing_stopped {};

    scanner* scan;
    error_handler* errors;

    bool have(S expected) const;
    bool one_of(std::initializer_list<S> choices) const;
    void must_be(S expected);
    void syntax_error(int code);
    bool starts_statement() const;
    bool starts_expression() const;

    void program();
    void block();
    void declaration();
    void ident_list();
    void type();
    void param_list();
    void param();
    void statement_list();
    void statement();
    void if_statement();
    void loop_statement();
    void while_statement();
    void for_statement();
    void call_arguments();
    void expression_list();
    void range();
    void expr();
    void simple_expr();
    void expr2();
    void term();
    void factor();
    void primary();
};

#endif // PARSER_H_
