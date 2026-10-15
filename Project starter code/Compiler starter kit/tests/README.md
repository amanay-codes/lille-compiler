Run from the Compiler starter kit directory with a C++20 compiler:

```sh
g++ -std=c++20 -I . tests/scanner_strings.cpp scanner.cpp token.cpp symbol.cpp error_handler.cpp lille_exception.cpp -o scanner_strings_test
./scanner_strings_test
```

On Windows PowerShell, run `./scanner_strings_test.exe` instead.

The test checks string values, doubled quotes, empty strings, token locations,
adjacent punctuation, line boundaries, and recovery from unterminated strings.
Five expected diagnostics are printed by the invalid-input cases. Success ends
with `String scanner tests passed.` and exit code zero. Inputs are temporary files;
the test does not modify the supplied sample programs.
