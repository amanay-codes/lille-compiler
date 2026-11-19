/*
 * id_table.h -- Original author: Michael Oudshoorn (2020-2023).
 * Reda Benhaimoud | CSC-4510 | Semantic analysis: array-of-BST symbol table,
 * nested scopes, parameter signatures, and inorder scope dumps.
 */
#ifndef ID_TABLE_H_
#define ID_TABLE_H_
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "error_handler.h"

class id_table {
public:
    enum class Type { integer, real, string, boolean, procedure, function, program, unknown };
    enum class Kind { variable, constant, value_param, ref_param, for_ident, procedure, function, program };
    struct Parameter { Type type; bool by_ref; std::string name; int line = 0, position = 0; };
    struct Entry {
        std::string name;
        Type type = Type::unknown;
        Kind kind = Kind::variable;
        int level = 0, line = 0, position = 0;
        std::string constant_value;
        std::vector<Parameter> parameters;
        Type return_type = Type::unknown;
    };

    explicit id_table(error_handler* err);
    void enter_scope();
    void exit_scope();
    int level() const { return static_cast<int>(scopes.size()) - 1; }
    bool insert(Entry entry);
    Entry* lookup_local(const std::string& name);
    Entry* lookup(const std::string& name);
    void dump_id_table(bool dump_all = true) const;
    static std::string type_name(Type type);
    static std::string kind_name(Kind kind);

private:
    struct Node {
        Entry value;
        std::unique_ptr<Node> left, right;
        explicit Node(Entry e) : value(std::move(e)) {}
    };
    // One separate binary-search tree root per active scope level.
    std::vector<std::unique_ptr<Node>> scopes;
    error_handler* error;
    static std::string normalize(std::string name);
    static Entry* search(Node* root, const std::string& key);
    static bool add(std::unique_ptr<Node>& root, Entry value);
    static void inorder(const Node* root);
};
#endif
