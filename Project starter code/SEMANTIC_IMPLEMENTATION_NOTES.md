# Lille semantic analysis implementation — CSC-4510

This build extends the user's Parser Phase project. Original starter source is preserved where not modified.

## New implementation
- `id_table.h` / `id_table.cpp`: array (vector indexed by scope level) of independent binary search trees, ordered case-insensitively by identifier name. Insert checks duplicates only within a current scope. Lookup searches current BST, then each outer BST. Printing uses inorder traversal of **all active scopes before removing the exiting scope**.
- `parser.h` / `parser.cpp`: parser now synthesizes Lille types from expressions, registers identifiers and formal parameters, checks visibility, duplicates, types, conditions, calls, return statements, ref arguments, for-index immutability, and scope name consistency. The program is scope 0; procedure and function bodies, and for loops, have nested scopes.
- `compiler.cpp`: constructs the id table and passes it to scanner and parser.
- `makefile`: rebuilds using C++20 and links all updated code.
- `Semantic_Tests`: 22 named valid/invalid test programs.

## Verified tests (Linux gcc)
- `make -B` completed.
- All original `program1` through `program9` completed with zero errors.
- All six `valid_*` semantic tests completed with zero errors.
- All sixteen `err_*` semantic tests reported a first semantic error.

## Limitations / further testing
- This is a tested reference implementation, not a guarantee that every instructor hidden semantic case passes. The project should be compiled in the HPU environment too.
- The functions rely on the starter error handler's error code messages. It reports the first semantic error; when parsing terminates early, scope dumping isn't guaranteed, consistent with the assignment instructions.
- Ref parameters require a bare modifiable identifier. Some advanced language rules not directly covered by the supplied lecture examples may need refinements when tested.
- No PAL generation was implemented or required. Offsets are omitted.

## AI assistance disclosure
This semantic-analysis implementation was generated with assistance from OpenAI ChatGPT (GPT-6) based on the student's previous compiler project, assignment instructions, and uploaded lecture slides. The assistant proposed and wrote the BST symbol table, parser integration, type checking, test cases, and build changes, and tested them in an isolated Linux environment. The student must review, understand, and verify the code and supply the actual prompts and responses/transcript if the course syllabus requires them. This statement alone does not replace that requested transcript.
