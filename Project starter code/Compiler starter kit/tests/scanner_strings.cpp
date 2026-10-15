#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "scanner.h"

void require(bool condition, const string& message)
{
    if (!condition)
        throw runtime_error(message);
}

// Keep test inputs outside the project and delete each after its scanner closes.
class input_file {
public:
    filesystem::path path;
    explicit input_file(const string& source)
    {
        static unsigned sequence = 0;
        path = filesystem::temp_directory_path() /
            ("lille-strings-" + to_string(chrono::steady_clock::now().time_since_epoch().count())
             + "-" + to_string(sequence++) + ".lil");
        ofstream output(path);
        output << source;
        require(output.good(), "Cannot create test input");
    }
    ~input_file() { filesystem::remove(path); }
};

void check_string(scanner& scan, const string& expected, int line, int position)
{
    token* value = scan.get_token();
    require(value->get_sym() == symbol::strng, "Expected a string token");
    require(value->get_string_value() == expected, "Incorrect string contents");
    require(value->get_line_number() == line && value->get_pos_on_line() == position,
            "Incorrect string location");
}

int main()
{
    try
    {
        {
            input_file input("  \"hello\";\"\" \"Say \"\"hello\"\"\" \"\"\"\" \"-- + :=\"\n\"last\"");
            error_handler errors(input.path.string());
            scanner scan(input.path.string(), nullptr, &errors);
            check_string(scan, "hello", 1, 2);
            require(scan.get_token()->get_sym() == symbol::semicolon_sym,
                    "Character immediately after closing quote was skipped");
            check_string(scan, "", 1, 10);
            check_string(scan, "Say \"hello\"", 1, 13);
            check_string(scan, "\"", 1, 29);
            check_string(scan, "-- + :=", 1, 34);
            check_string(scan, "last", 2, 0);
            require(scan.get_token()->get_sym() == symbol::end_of_program, "Expected EOF");
            require(errors.error_count() == 0, "Valid strings reported errors");
        }
        // Missing closing quotes, including a doubled quote at the end of a line.
        for (const string& source : {string("\"broken"), string("\""),
                                     string("\"broken\"\""), string("\"broken\r")})
        {
            input_file input(source + "\n7");
            error_handler errors(input.path.string());
            scanner scan(input.path.string(), nullptr, &errors);
            require(scan.get_token()->get_sym() == symbol::strng, "Expected recovery token");
            require(errors.error_count() == 1, "Expected one unterminated-string error");
            token* next = scan.get_token();
            require(next->get_sym() == symbol::integer && next->get_integer_value() == 7
                    && next->get_line_number() == 2 && next->get_pos_on_line() == 0,
                    "Recovery skipped or swallowed the next line");
            require(scan.get_token()->get_sym() == symbol::end_of_program, "Expected EOF");
        }
        {
            input_file input("\"unfinished");
            error_handler errors(input.path.string());
            scanner scan(input.path.string(), nullptr, &errors);
            scan.get_token();
            require(errors.error_count() == 1, "Expected error at EOF");
            require(scan.get_token()->get_sym() == symbol::end_of_program, "EOF recovery failed");
        }
        cout << "String scanner tests passed.\n";
    }
    catch (const exception& error)
    {
        cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
