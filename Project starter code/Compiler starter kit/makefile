# Reda Benhaimoud | CSC-4510 | Build semantic analyzer and array-of-BST symbol table.
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra
OBJS = compiler.o error_handler.o lille_exception.o scanner.o parser.o id_table.o symbol.o token.o

all: compiler
	@echo Compilation complete.

compiler: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

compiler.o: compiler.cpp parser.h scanner.h id_table.h
parser.o: parser.cpp parser.h id_table.h scanner.h
id_table.o: id_table.cpp id_table.h
scanner.o: scanner.cpp scanner.h
error_handler.o: error_handler.cpp error_handler.h
lille_exception.o: lille_exception.cpp lille_exception.h
symbol.o: symbol.cpp symbol.h
token.o: token.cpp token.h

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) compiler

.PHONY: all clean
