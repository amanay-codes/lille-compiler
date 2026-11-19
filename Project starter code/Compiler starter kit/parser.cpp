// Reda Benhaimoud | CSC-4510 | Warning cleanup: clarify the exponentiation type-check conditional.
/* Reda Benhaimoud | CSC-4510 | Semantic-analysis extension of the working
 * LL(1) Lille parser: scope-aware declarations, type checks, and call validation.
 * Existing scanner and professor-supplied diagnostic codes are reused. */
#include "parser.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>
using namespace std;

parser::parser(scanner* sc, error_handler* er, id_table* ids) : scan(sc), errors(er), table(ids) {
    if (!scan || !errors || !table) throw invalid_argument("Parser requires scanner, error handler, and symbol table");
}
bool parser::have(S s) const { return scan->have(s); }
bool parser::one_of(initializer_list<S> choices) const { for (S s : choices) if (have(s)) return true; return false; }
void parser::must_be(S s) {
    bool ok = have(s); scan->must_be(s);
    if (!ok) throw parsing_stopped{};
}
void parser::syntax_error(int code) { errors->flag(scan->this_token(), code); throw parsing_stopped{}; }
void parser::semantic_error(int code, token* where) {
    errors->flag(where ? where : scan->this_token(), code);
    throw parsing_stopped{}; // one diagnostic is sufficient for this phase
}
token parser::current() const { return *scan->this_token(); }
bool parser::starts_statement() const {
    return one_of({S::identifier, S::exit_sym, S::return_sym, S::read_sym,
        S::write_sym, S::writeln_sym, S::null_sym, S::if_sym,
        S::loop_sym, S::for_sym, S::while_sym});
}
bool parser::starts_expression() const {
    return one_of({S::identifier,S::integer,S::real_num,S::strng,S::true_sym,S::false_sym,
        S::left_paren_sym,S::not_sym,S::odd_sym,S::plus_sym,S::minus_sym});
}
bool parser::numeric(T t) { return t == T::integer || t == T::real; }
bool parser::compatible(T target, T source) {
    return target == source || (target == T::real && source == T::integer);
}
void parser::require(T observed, T expected, int code, token* at) {
    if (observed != expected) semantic_error(code, at);
}
parser::Entry* parser::find(const token& tok) {
    auto* result = table->lookup(const_cast<token&>(tok).get_identifier_value());
    if (!result) { token copy = tok; semantic_error(81, &copy); }
    return result;
}
void parser::declare(Entry entry, const token& at) {
    entry.line = const_cast<token&>(at).get_line_number();
    entry.position = const_cast<token&>(at).get_pos_on_line();
    if (!table->insert(move(entry))) { token copy = at; semantic_error(82, &copy); }
}
bool parser::parse() {
    try {
        scan->get_token();
        program();
        must_be(S::end_of_program);
        return true;
    } catch (const parsing_stopped&) { return false; }
}
// Program scope is level 0; nested procedures/functions/for loops get new BSTs.
void parser::program() {
    must_be(S::program_sym);
    token program_id = current(); must_be(S::identifier);
    must_be(S::is_sym);
    table->enter_scope();
    Entry program_entry; program_entry.name = program_id.get_identifier_value();
    program_entry.type = T::program; program_entry.kind = K::program;
    declare(program_entry, program_id);
    block(program_entry.name);
    must_be(S::semicolon_sym);
    table->exit_scope();
}
void parser::block(const string& expected_end) {
    while (one_of({S::identifier,S::procedure_sym,S::function_sym})) declaration();
    must_be(S::begin_sym);
    statement_list();
    must_be(S::end_sym);
    if (have(S::identifier)) {
        token closing = current(); must_be(S::identifier);
        string lhs = expected_end, rhs = closing.get_identifier_value();
        transform(lhs.begin(),lhs.end(),lhs.begin(),[](unsigned char c){return tolower(c);});
        transform(rhs.begin(),rhs.end(),rhs.begin(),[](unsigned char c){return tolower(c);});
        if (!expected_end.empty() && lhs != rhs) semantic_error(107,&closing);
    }
}
vector<token> parser::ident_list() {
    vector<token> names;
    names.push_back(current()); must_be(S::identifier);
    while (have(S::comma_sym)) {
        must_be(S::comma_sym);
        names.push_back(current()); must_be(S::identifier);
    }
    return names;
}
parser::T parser::parse_type() {
    if (have(S::integer_sym)) {must_be(S::integer_sym);return T::integer;}
    if (have(S::real_sym)) {must_be(S::real_sym);return T::real;}
    if (have(S::string_sym)) {must_be(S::string_sym);return T::string;}
    if (have(S::boolean_sym)) {must_be(S::boolean_sym);return T::boolean;}
    syntax_error(96);return T::unknown;
}
void parser::param(vector<id_table::Parameter>& params) {
    auto names = ident_list(); must_be(S::colon_sym);
    bool by_ref = have(S::ref_sym);
    if (by_ref) must_be(S::ref_sym);
    else if (have(S::value_sym)) must_be(S::value_sym);
    else syntax_error(94);
    T typ = parse_type();
    for (auto& n : names) params.push_back({typ,by_ref,n.get_identifier_value(),n.get_line_number(),n.get_pos_on_line()});
}
vector<id_table::Parameter> parser::param_list() {
    vector<id_table::Parameter> params;
    param(params);
    while (have(S::semicolon_sym)) {must_be(S::semicolon_sym);param(params);}
    return params;
}
void parser::declaration() {
    if (have(S::identifier)) {
        auto names = ident_list(); must_be(S::colon_sym);
        bool constant = have(S::constant_sym);
        if (constant) must_be(S::constant_sym);
        T declared = parse_type();
        bool initialized = have(S::becomes_sym);
        T literal_type = T::unknown;
        string literal;
        if (initialized) {
            must_be(S::becomes_sym);
            token val = current();
            if (have(S::integer)) {literal_type=T::integer;literal=val.to_string();must_be(S::integer);}
            else if (have(S::real_num)) {literal_type=T::real;literal=val.to_string();must_be(S::real_num);}
            else if (have(S::strng)) {literal_type=T::string;literal=val.get_string_value();must_be(S::strng);}
            else if (have(S::true_sym)||have(S::false_sym)) {literal_type=T::boolean;literal=have(S::true_sym)?"true":"false";must_be(current().get_sym());}
            else syntax_error(112);
        }
        must_be(S::semicolon_sym);
        if (constant && !initialized) {token n=names.front();semantic_error(110,&n);}
        if (!constant && initialized) {token n=names.front();semantic_error(91,&n);}
        if (initialized && !compatible(declared,literal_type)) {token n=names.front();semantic_error(111,&n);}
        for (auto& n : names) {
            Entry e; e.name=n.get_identifier_value(); e.type=declared;
            e.kind=constant?K::constant:K::variable; e.constant_value=constant?literal:"";
            declare(e,n);
        }
    } else if (one_of({S::procedure_sym,S::function_sym})) {
        bool is_func=have(S::function_sym);
        must_be(is_func?S::function_sym:S::procedure_sym);
        token name=current();must_be(S::identifier);
        vector<id_table::Parameter> params;
        if (have(S::left_paren_sym)) {
            must_be(S::left_paren_sym);params=param_list();must_be(S::right_paren_sym);
        }
        T return_type=T::unknown;
        if (is_func) {must_be(S::return_sym);return_type=parse_type();}
        for (auto& p:params) if (is_func && p.by_ref) semantic_error(123,&name);
        Entry e;e.name=name.get_identifier_value(); e.type=is_func?T::function:T::procedure;
        e.kind=is_func?K::function:K::procedure; e.parameters=params;e.return_type=return_type;
        declare(e,name); // Insert in enclosing scope BEFORE entering body scope.
        must_be(S::is_sym);
        string previous=active_subprogram;T previous_type=active_return;bool prev_return=saw_return;
        active_subprogram=e.name;active_return=return_type;saw_return=false;
        table->enter_scope();
        for (auto& p:params) {
            Entry pe;pe.name=p.name;pe.type=p.type;pe.kind=p.by_ref?K::ref_param:K::value_param;
            token formal=name;
            pe.line=p.line;pe.position=p.position;
            // Param declarations appear in the procedure's own scope BST.
            if (!table->insert(pe)) semantic_error(82,&formal);
        }
        block(e.name); must_be(S::semicolon_sym);
        if (is_func && !saw_return) semantic_error(109,&name);
        table->exit_scope();
        active_subprogram=previous;active_return=previous_type;saw_return=prev_return;
    } else syntax_error(106);
}
void parser::statement_list() {
    if (!starts_statement()) syntax_error(80);
    do {statement();must_be(S::semicolon_sym);} while(starts_statement());
    if (!one_of({S::end_sym,S::elsif_sym,S::else_sym})) syntax_error(79);
}
vector<parser::Actual> parser::call_arguments() {
    vector<Actual> args;
    must_be(S::left_paren_sym);
    do {
        bool lone_id=have(S::identifier);
        token origin=current();
        T ty=expr();
        // The expression parser tracks whether the actual argument is exactly
        // one modifiable identifier (as REF parameters require).
        args.push_back({ty, lone_id && plain_identifier,
                        lone_id ? origin.get_identifier_value() : ""});
        if (!have(S::comma_sym)) break;
        must_be(S::comma_sym);
    } while (true);
    must_be(S::right_paren_sym);
    return args;
}
void parser::check_call(Entry* target,const vector<Actual>& args,bool expression,const token& at) {
    token loc=at;
    if (expression && target->kind!=K::function) semantic_error(121,&loc);
    if (!expression && target->kind!=K::procedure) semantic_error(90,&loc);
    if (args.size()!=target->parameters.size()) semantic_error(97,&loc);
    for (size_t i=0;i<args.size();++i) {
        const auto& formal=target->parameters[i];
        if (!compatible(formal.type,args[i].type)) semantic_error(98,&loc);
        if (formal.by_ref) {
            if (formal.type!=args[i].type || !args[i].modifiable) semantic_error(99,&loc);
        }
    }
}
void parser::expression_list() {
    expr();while(have(S::comma_sym)){must_be(S::comma_sym);expr();}
}
void parser::statement() {
    if (have(S::identifier)) {
        token name=current();Entry* entry=find(name);must_be(S::identifier);
        if (have(S::becomes_sym)) {
            if (!(entry->kind==K::variable||entry->kind==K::ref_param)) semantic_error(85,&name);
            must_be(S::becomes_sym);T rhs=expr();
            if (!compatible(entry->type,rhs)) semantic_error(93,&name);
        } else if (have(S::left_paren_sym)) {auto args=call_arguments();check_call(entry,args,false,name);}
        else {check_call(entry,{},false,name);}
    } else if (have(S::exit_sym)) {
        token at=current();must_be(S::exit_sym);
        if (!loop_depth) semantic_error(89,&at);
        if (have(S::when_sym)){must_be(S::when_sym);require(expr(),T::boolean,103,&at);}
    } else if (have(S::return_sym)) {
        token at=current();must_be(S::return_sym);
        if (active_subprogram.empty()) semantic_error(88,&at);
        bool has_value=starts_expression();
        if (active_return==T::unknown) {
            if (has_value) semantic_error(87,&at);
        } else {
            if (!has_value) semantic_error(87,&at);
            if (!compatible(active_return,expr())) semantic_error(87,&at);
        }
        saw_return=true;
    } else if (have(S::read_sym)) {
        must_be(S::read_sym);
        bool paren=have(S::left_paren_sym);if(paren)must_be(S::left_paren_sym);
        auto names=ident_list();
        for(auto& n:names){auto* e=find(n);if(!(e->kind==K::variable||e->kind==K::ref_param))semantic_error(85,&n);}
        if(paren)must_be(S::right_paren_sym);
    } else if (have(S::write_sym)||have(S::writeln_sym)) {
        bool newline=have(S::writeln_sym);must_be(newline?S::writeln_sym:S::write_sym);
        bool paren=have(S::left_paren_sym);if(paren)must_be(S::left_paren_sym);
        if (!newline||starts_expression())expression_list();
        if(paren)must_be(S::right_paren_sym);
    } else if(have(S::null_sym))must_be(S::null_sym);
    else if(have(S::if_sym))if_statement();
    else if(have(S::while_sym))while_statement();
    else if(have(S::for_sym))for_statement();
    else if(have(S::loop_sym))loop_statement();
    else syntax_error(80);
}
void parser::if_statement() {
    token at=current();must_be(S::if_sym);require(expr(),T::boolean,103,&at);
    must_be(S::then_sym);statement_list();
    while(have(S::elsif_sym)) {at=current();must_be(S::elsif_sym);require(expr(),T::boolean,103,&at);must_be(S::then_sym);statement_list();}
    if(have(S::else_sym)){must_be(S::else_sym);statement_list();}
    must_be(S::end_sym);must_be(S::if_sym);
}
void parser::loop_statement() {
    must_be(S::loop_sym);++loop_depth;statement_list();--loop_depth;
    must_be(S::end_sym);must_be(S::loop_sym);
}
void parser::while_statement() {
    token at=current();must_be(S::while_sym);require(expr(),T::boolean,103,&at);loop_statement();
}
void parser::for_statement() {
    must_be(S::for_sym);token id=current();must_be(S::identifier);must_be(S::in_sym);
    if(have(S::reverse_sym))must_be(S::reverse_sym);
    // The for identifier is local to the loop, shadows an outer declaration,
    // and is immutable during the loop body.
    table->enter_scope();
    Entry e;e.name=id.get_identifier_value();e.kind=K::for_ident;e.type=T::integer;declare(e,id);
    range();loop_statement();table->exit_scope();
}
void parser::range() {
    T left=simple_expr();must_be(S::range_sym);T right=simple_expr();
    if(left!=T::integer||right!=T::integer) semantic_error(104);
}
parser::T parser::expr() {
    T left=simple_expr();
    if(one_of({S::less_than_sym,S::greater_than_sym,S::equals_sym,S::not_equals_sym,S::less_or_equal_sym,S::greater_or_equal_sym})) {
        token op=current();S operation=op.get_sym();must_be(operation);T right=simple_expr();
        if(!((numeric(left)&&numeric(right))||(left==right))) semantic_error(114,&op);
        plain_identifier=false;
        return T::boolean;
    }
    if(have(S::in_sym)) {must_be(S::in_sym);if(left!=T::integer)semantic_error(104);range();plain_identifier=false;return T::boolean;}
    return left;
}
parser::T parser::simple_expr() {
    T left=expr2();
    while(have(S::ampersand_sym)) {token at=current();must_be(S::ampersand_sym);T right=expr2();
        if(left!=T::string||right!=T::string) semantic_error(115,&at);
        left=T::string;plain_identifier=false;}
    return left;
}
parser::T parser::expr2() {
    T left=term();
    while(one_of({S::plus_sym,S::minus_sym,S::or_sym})) {
        token op=current();S kind=op.get_sym();must_be(kind);T right=term();
        plain_identifier=false;
        if(kind==S::or_sym) {if(left!=T::boolean||right!=T::boolean)semantic_error(117,&op);left=T::boolean;}
        else {if(!numeric(left)||!numeric(right))semantic_error(116,&op);left=(left==T::real||right==T::real)?T::real:T::integer;}
    }
    return left;
}
parser::T parser::term() {
    T left=factor();
    while(one_of({S::asterisk_sym,S::slash_sym,S::and_sym})) {
        token op=current();S kind=op.get_sym();must_be(kind);T right=factor();
        plain_identifier=false;
        if(kind==S::and_sym){if(left!=T::boolean||right!=T::boolean)semantic_error(117,&op);left=T::boolean;}
        else{if(!numeric(left)||!numeric(right))semantic_error(116,&op);left=(left==T::real||right==T::real)?T::real:T::integer;}
    }
    return left;
}
parser::T parser::factor() {
    if(one_of({S::plus_sym,S::minus_sym})) {
        token op=current();must_be(op.get_sym());T result=primary();plain_identifier=false;if(!numeric(result))semantic_error(116,&op);return result;
    }
    T left=primary();
    if(have(S::power_sym)){token at=current();must_be(S::power_sym);T right=primary();
        if (!numeric(left) || right != T::integer) {
            semantic_error(119, &at);
        }
        plain_identifier = false;}
    return left;
}
parser::T parser::primary() {
    if(have(S::not_sym)) {token at=current();must_be(S::not_sym);T result=expr();require(result,T::boolean,117,&at);plain_identifier=false;return T::boolean;}
    if(have(S::odd_sym)) {token at=current();must_be(S::odd_sym);T result=expr();require(result,T::integer,119,&at);plain_identifier=false;return T::boolean;}
    if(have(S::left_paren_sym)) {must_be(S::left_paren_sym);T result=simple_expr();must_be(S::right_paren_sym);plain_identifier=false;return result;}
    if(have(S::identifier)) {
        token name=current();must_be(S::identifier);
        // Predefined conversion functions are part of the Lille language.
        string key=name.get_identifier_value();transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return tolower(c);});
        if(key=="int2real"||key=="real2int"||key=="int2string"||key=="real2string") {
            if(!have(S::left_paren_sym))semantic_error(121,&name);
            auto args=call_arguments();
            if(args.size()!=1)semantic_error(97,&name);
            T required=(key=="int2real"||key=="int2string")?T::integer:T::real;
            if(args[0].type!=required)semantic_error(98,&name);
            plain_identifier=false;
            return (key=="int2real")?T::real:(key=="real2int")?T::integer:T::string;
        }
        Entry* e=find(name);
        if(have(S::left_paren_sym)){auto args=call_arguments();check_call(e,args,true,name);plain_identifier=false;return e->return_type;}
        if(e->kind==K::function) {check_call(e,{},true,name);plain_identifier=false;return e->return_type;}
        if(e->kind==K::procedure||e->kind==K::program) semantic_error(91,&name);
        plain_identifier=(e->kind==K::variable||e->kind==K::ref_param);
        return e->type;
    }
    if(have(S::integer)){must_be(S::integer);plain_identifier=false;return T::integer;}
    if(have(S::real_num)){must_be(S::real_num);plain_identifier=false;return T::real;}
    if(have(S::strng)){must_be(S::strng);plain_identifier=false;return T::string;}
    if(have(S::true_sym)||have(S::false_sym)){must_be(current().get_sym());plain_identifier=false;return T::boolean;}
    syntax_error(84);return T::unknown;
}
