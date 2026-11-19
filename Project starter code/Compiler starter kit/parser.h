/* Reda Benhaimoud | CSC-4510 | Recursive-descent parsing with Lille static
 * semantics: declarations, nested BST scopes, type inference and call checks. */
#ifndef PARSER_H_
#define PARSER_H_
#include "scanner.h"
#include "error_handler.h"
#include "id_table.h"
#include <initializer_list>
#include <string>
#include <vector>

class parser {
public:
    parser(scanner* scanner_ptr, error_handler* error_ptr, id_table* table_ptr);
    bool parse();
private:
    using S = symbol::symbol_type;
    using T = id_table::Type;
    using K = id_table::Kind;
    using Entry = id_table::Entry;
    struct parsing_stopped {};
    struct Actual { T type; bool modifiable; std::string identifier; };
    scanner* scan;
    error_handler* errors;
    id_table* table;
    std::string active_subprogram;
    T active_return = T::unknown;
    bool saw_return = false;
    int loop_depth = 0;
    bool plain_identifier = false; // lvalue status of the last parsed expression
    bool have(S expected) const;
    bool one_of(std::initializer_list<S> choices) const;
    void must_be(S expected);
    void syntax_error(int code);
    void semantic_error(int code, token* where = nullptr);
    token current() const;
    bool starts_statement() const;
    bool starts_expression() const;
    static bool numeric(T t);
    static bool compatible(T target, T source);
    void require(T observed, T expected, int code, token* at = nullptr);
    Entry* find(const token& tok);
    void declare(Entry entry, const token& at);
    T parse_type();
    std::vector<token> ident_list();
    std::vector<id_table::Parameter> param_list();
    void param(std::vector<id_table::Parameter>& params);
    void program();
    void block(const std::string& expected_end = "");
    void declaration();
    void statement_list();
    void statement();
    void if_statement();
    void loop_statement();
    void while_statement();
    void for_statement();
    std::vector<Actual> call_arguments();
    void check_call(Entry* target, const std::vector<Actual>& args, bool expression, const token& at);
    void expression_list();
    void range();
    T expr();
    T simple_expr();
    T expr2();
    T term();
    T factor();
    T primary();
};
#endif
