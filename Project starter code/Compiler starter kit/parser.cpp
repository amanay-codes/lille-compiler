#include "parser.h"
#include <stdexcept>

parser::parser(scanner* scanner_ptr, error_handler* error_ptr)
    : scan(scanner_ptr), errors(error_ptr) {
    if (!scan || !errors)
        throw std::invalid_argument("Parser requires a scanner and error handler");
}

bool parser::have(S expected) const { return scan->have(expected); }

bool parser::one_of(std::initializer_list<S> choices) const {
    for (S choice : choices)
        if (have(choice)) return true;
    return false;
}

// The starter scanner reports an error but does not advance on mismatch.
// Stop after that first error, as permitted by the Parser phase rubric.
void parser::must_be(S expected) {
    bool matched = have(expected);
    scan->must_be(expected);
    if (!matched) throw parsing_stopped{};
}

void parser::syntax_error(int code) {
    errors->flag(scan->this_token(), code);
    throw parsing_stopped{};
}

bool parser::starts_statement() const {
    return one_of({S::identifier, S::exit_sym, S::return_sym, S::read_sym,
                   S::write_sym, S::writeln_sym, S::null_sym, S::if_sym,
                   S::loop_sym, S::for_sym, S::while_sym});
}

bool parser::starts_expression() const {
    return one_of({S::identifier, S::integer, S::real_num, S::strng,
                   S::true_sym, S::false_sym, S::left_paren_sym,
                   S::not_sym, S::odd_sym, S::plus_sym, S::minus_sym});
}

bool parser::parse() {
    try {
        scan->get_token(); // Prime the scanner's single lookahead token.
        program();
        must_be(S::end_of_program); // Reject anything after the final ';'.
        return true;
    } catch (const parsing_stopped&) {
        return false;
    }
}

// <prog> ::= program <ident> is <block> ;
void parser::program() {
    must_be(S::program_sym);
    must_be(S::identifier);
    must_be(S::is_sym);
    block();
    must_be(S::semicolon_sym);
}

// <block> ::= { <declaration> }* begin <statement_list> end [ <ident> ]
// FIRST(declaration) = {IDENT, PROCEDURE, FUNCTION}; BEGIN follows the list.
void parser::block() {
    while (one_of({S::identifier, S::procedure_sym, S::function_sym}))
        declaration();
    must_be(S::begin_sym);
    statement_list();
    must_be(S::end_sym);
    if (have(S::identifier)) must_be(S::identifier);
}

// <ident_list> ::= <ident> { , <ident> }*
void parser::ident_list() {
    must_be(S::identifier);
    while (have(S::comma_sym)) {
        must_be(S::comma_sym);
        must_be(S::identifier);
    }
}

// <type> ::= integer | real | string | boolean
void parser::type() {
    if (one_of({S::integer_sym, S::real_sym, S::string_sym, S::boolean_sym}))
        must_be(scan->this_token()->get_sym());
    else
        syntax_error(96);
}

// <declaration> has variable/constant, procedure and function alternatives.
void parser::declaration() {
    if (have(S::identifier)) {
        ident_list();
        must_be(S::colon_sym);
        if (have(S::constant_sym)) must_be(S::constant_sym);
        type();
        // The grammar allows an OPTIONAL literal initializer.
        // Whether a constant requires one (or a variable forbids one)
        // is a static-semantic rule, not a Parser-phase syntax rule.
        if (have(S::becomes_sym)) {
            must_be(S::becomes_sym);
            if (one_of({S::integer, S::real_num, S::strng,
                        S::true_sym, S::false_sym}))
                must_be(scan->this_token()->get_sym());
            else
                syntax_error(92);
        }
        must_be(S::semicolon_sym);
    } else if (one_of({S::procedure_sym, S::function_sym})) {
        bool function = have(S::function_sym);
        must_be(function ? S::function_sym : S::procedure_sym);
        must_be(S::identifier);
        if (have(S::left_paren_sym)) {
            must_be(S::left_paren_sym);
            param_list(); // A present parameter list cannot be empty.
            must_be(S::right_paren_sym);
        }
        if (function) {
            must_be(S::return_sym);
            type();
        }
        must_be(S::is_sym);
        block();
        must_be(S::semicolon_sym);
    } else {
        syntax_error(106);
    }
}

// <param_list> ::= <param> { ; <param> }*
void parser::param_list() {
    param();
    while (have(S::semicolon_sym)) {
        must_be(S::semicolon_sym);
        param();
    }
}

// <param> ::= <ident_list> : <param_kind> <type>
void parser::param() {
    ident_list();
    must_be(S::colon_sym);
    if (one_of({S::value_sym, S::ref_sym}))
        must_be(scan->this_token()->get_sym());
    else
        syntax_error(94);
    type();
}

// <statement_list> ::= <statement> ; { <statement> ; }*
// FIRST(statement) is required once: an empty statement list is illegal.
void parser::statement_list() {
    if (!starts_statement()) syntax_error(80);
    do {
        statement();
        must_be(S::semicolon_sym);
    } while (starts_statement());
    // Only these tokens can follow a completed statement list.
    if (!one_of({S::end_sym, S::elsif_sym, S::else_sym}))
        syntax_error(79);
}

// An identifier statement may be an assignment, a call, OR an identifier
// alone, as specified by <simple_statement> in syntax.pdf.
void parser::statement() {
    if (have(S::identifier)) {
        must_be(S::identifier);
        if (have(S::becomes_sym)) {
            must_be(S::becomes_sym);
            expr();
        } else if (have(S::left_paren_sym)) {
            call_arguments();
        }
    } else if (have(S::exit_sym)) {
        must_be(S::exit_sym);
        if (have(S::when_sym)) {
            must_be(S::when_sym);
            expr();
        }
    } else if (have(S::return_sym)) {
        must_be(S::return_sym);
        if (starts_expression()) expr();
    } else if (have(S::read_sym)) {
        must_be(S::read_sym);
        bool parenthesized = have(S::left_paren_sym);
        if (parenthesized) must_be(S::left_paren_sym);
        ident_list();
        if (parenthesized) must_be(S::right_paren_sym);
    } else if (have(S::write_sym)) {
        must_be(S::write_sym);
        bool parenthesized = have(S::left_paren_sym);
        if (parenthesized) must_be(S::left_paren_sym);
        expression_list(); // WRITE always needs at least one expression.
        if (parenthesized) must_be(S::right_paren_sym);
    } else if (have(S::writeln_sym)) {
        must_be(S::writeln_sym);
        bool parenthesized = have(S::left_paren_sym);
        if (parenthesized) must_be(S::left_paren_sym);
        if (starts_expression()) expression_list(); // WRITELN may be empty.
        if (parenthesized) must_be(S::right_paren_sym);
    } else if (have(S::null_sym)) {
        must_be(S::null_sym);
    } else if (have(S::if_sym)) {
        if_statement();
    } else if (have(S::while_sym)) {
        while_statement();
    } else if (have(S::for_sym)) {
        for_statement();
    } else if (have(S::loop_sym)) {
        loop_statement();
    } else {
        syntax_error(80);
    }
}

// Parenthesized call arguments: ( <expr> { , <expr> }* )
void parser::call_arguments() {
    must_be(S::left_paren_sym);
    expression_list();
    must_be(S::right_paren_sym);
}

void parser::expression_list() {
    expr();
    while (have(S::comma_sym)) {
        must_be(S::comma_sym);
        expr();
    }
}

// if expr then statement_list {elsif expr then statement_list}*
// [else statement_list] end if
void parser::if_statement() {
    must_be(S::if_sym);
    expr();
    must_be(S::then_sym);
    statement_list();
    while (have(S::elsif_sym)) {
        must_be(S::elsif_sym);
        expr();
        must_be(S::then_sym);
        statement_list();
    }
    if (have(S::else_sym)) {
        must_be(S::else_sym);
        statement_list();
    }
    must_be(S::end_sym);
    must_be(S::if_sym);
}

// <loop_statement> ::= loop <statement_list> end loop
void parser::loop_statement() {
    must_be(S::loop_sym);
    statement_list();
    must_be(S::end_sym);
    must_be(S::loop_sym);
}

void parser::while_statement() {
    must_be(S::while_sym);
    expr();
    loop_statement();
}

void parser::for_statement() {
    must_be(S::for_sym);
    must_be(S::identifier);
    must_be(S::in_sym);
    if (have(S::reverse_sym)) must_be(S::reverse_sym);
    range();
    loop_statement();
}

// <range> ::= <simple_expr> .. <simple_expr>
void parser::range() {
    simple_expr();
    must_be(S::range_sym);
    simple_expr();
}

// <expr> ::= <simple_expr> [ <relop> <simple_expr> ]
//          | <simple_expr> in <range>
void parser::expr() {
    simple_expr();
    if (one_of({S::less_than_sym, S::greater_than_sym, S::equals_sym,
                S::not_equals_sym, S::less_or_equal_sym,
                S::greater_or_equal_sym})) {
        must_be(scan->this_token()->get_sym());
        simple_expr();
    } else if (have(S::in_sym)) {
        must_be(S::in_sym);
        range();
    }
}

// <simple_expr> ::= <expr2> { & <expr2> }*
void parser::simple_expr() {
    expr2();
    while (have(S::ampersand_sym)) {
        must_be(S::ampersand_sym);
        expr2();
    }
}

// <expr2> ::= <term> { ( + | - | or ) <term> }*
void parser::expr2() {
    term();
    while (one_of({S::plus_sym, S::minus_sym, S::or_sym})) {
        must_be(scan->this_token()->get_sym());
        term();
    }
}

// <term> ::= <factor> { ( * | / | and ) <factor> }*
void parser::term() {
    factor();
    while (one_of({S::asterisk_sym, S::slash_sym, S::and_sym})) {
        must_be(scan->this_token()->get_sym());
        factor();
    }
}

// <factor> ::= <primary> [ ** <primary> ] | [ + | - ] <primary>
void parser::factor() {
    if (one_of({S::plus_sym, S::minus_sym})) {
        must_be(scan->this_token()->get_sym());
        primary();
    } else {
        primary();
        if (have(S::power_sym)) {
            must_be(S::power_sym);
            primary();
        }
    }
}

// <primary> ::= not <expr> | odd <expr> | ( <simple_expr> )
//             | IDENT [ ( <expr> { , <expr> }* ) ]
//             | <number> | <string> | <bool>
void parser::primary() {
    if (one_of({S::not_sym, S::odd_sym})) {
        must_be(scan->this_token()->get_sym());
        expr();
    } else if (have(S::left_paren_sym)) {
        must_be(S::left_paren_sym);
        simple_expr();
        must_be(S::right_paren_sym);
    } else if (have(S::identifier)) {
        must_be(S::identifier);
        if (have(S::left_paren_sym)) call_arguments();
    } else if (one_of({S::integer, S::real_num, S::strng,
                       S::true_sym, S::false_sym})) {
        must_be(scan->this_token()->get_sym());
    } else {
        syntax_error(84);
    }
}
