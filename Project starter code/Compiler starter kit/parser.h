
#ifndef PARSER_H_
#define PARSER_H_

#include "scanner.h"

// Recursive-descent parser for the Lille programming language.
// Each grammar production will eventually have its own function.
class parser {
private:
    scanner* scan;

    // Main Lille grammar production.
    void parse_program();

public:
    // Use the existing scanner to obtain input tokens.
    explicit parser(scanner* s);

    // Begin syntax analysis.
    void parse();
};

#endif // PARSER_H_
