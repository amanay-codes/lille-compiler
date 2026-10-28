
#include "parser.h"
#include <stdexcept>

// Connect the parser to the existing scanner.
parser::parser(scanner* s) : scan(s) {
}

// Start parsing a Lille program.
void parser::parse() {
    parse_program();
}

// Placeholder for the program grammar production.
// FIRST and FOLLOW sets will guide the parsing logic.
void parser::parse_program() {
    throw std::logic_error(
        "Parser grammar implementation is not yet complete."
    );
}
