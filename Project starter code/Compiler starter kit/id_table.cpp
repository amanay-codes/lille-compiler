/*
 * id_table.cpp -- Original author: Michael Oudshoorn (2020-2023).
 * Reda Benhaimoud | CSC-4510 | One BST per active scope, nearest-visible
 * lookup, duplicate checking, and required inorder printing on scope exit.
 */
#include "id_table.h"
#include <algorithm>
#include <cctype>
#include <utility>
using namespace std;

id_table::id_table(error_handler* err) : error(err) {}
string id_table::normalize(string name) {
    transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return tolower(c); });
    return name;
}
id_table::Entry* id_table::search(Node* root, const string& key) {
    while (root) {
        string existing = normalize(root->value.name);
        if (key == existing) return &root->value;
        root = key < existing ? root->left.get() : root->right.get();
    }
    return nullptr;
}
bool id_table::add(unique_ptr<Node>& root, Entry value) {
    if (!root) { root = make_unique<Node>(move(value)); return true; }
    auto key = normalize(value.name), current = normalize(root->value.name);
    if (key == current) return false;
    return add(key < current ? root->left : root->right, move(value));
}
void id_table::enter_scope() { scopes.emplace_back(nullptr); }
void id_table::exit_scope() {
    if (scopes.empty()) return;
    // Critical: print all active scopes INCLUDING the one about to be removed.
    dump_id_table(true);
    scopes.pop_back();
}
bool id_table::insert(Entry entry) {
    if (scopes.empty()) enter_scope();
    entry.level = level();
    return add(scopes.back(), move(entry));
}
id_table::Entry* id_table::lookup_local(const string& name) {
    return scopes.empty() ? nullptr : search(scopes.back().get(), normalize(name));
}
id_table::Entry* id_table::lookup(const string& name) {
    string key = normalize(name);
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
        if (auto* found = search(it->get(), key)) return found;
    return nullptr;
}
string id_table::type_name(Type t) {
    switch (t) {
        case Type::integer: return "INTEGER"; case Type::real: return "REAL";
        case Type::string: return "STRING"; case Type::boolean: return "BOOLEAN";
        case Type::procedure: return "PROCEDURE"; case Type::function: return "FUNCTION";
        case Type::program: return "PROGRAM"; default: return "UNKNOWN";
    }
}
string id_table::kind_name(Kind k) {
    switch (k) {
        case Kind::variable: return "VARIABLE"; case Kind::constant: return "CONSTANT";
        case Kind::value_param: return "VALUE PARAM"; case Kind::ref_param: return "REF PARAM";
        case Kind::for_ident: return "FOR INDEX"; case Kind::procedure: return "PROCEDURE";
        case Kind::function: return "FUNCTION"; default: return "PROGRAM";
    }
}
void id_table::inorder(const Node* root) {
    if (!root) return;
    inorder(root->left.get());
    const auto& e = root->value;
    cout << "  Token Name: " << e.name << " Type: " << type_name(e.type)
         << " Kind: " << kind_name(e.kind) << " Level: " << e.level
         << " Line: " << e.line << " Position: " << e.position;
    if (!e.constant_value.empty()) cout << " Constant: " << e.constant_value;
    if (e.kind == Kind::function || e.kind == Kind::procedure) {
        cout << " Params: " << e.parameters.size();
        for (auto& p : e.parameters)
            cout << " [" << p.name << ":" << (p.by_ref ? "REF " : "VALUE ") << type_name(p.type) << "]";
        if (e.kind == Kind::function) cout << " Returns: " << type_name(e.return_type);
    }
    cout << '\n';
    inorder(root->right.get());
}
void id_table::dump_id_table(bool dump_all) const {
    cout << "\n========== SYMBOL TABLE (inorder BSTs) ==========\n";
    if (scopes.empty()) cout << "(No active scopes)\n";
    int begin = dump_all ? 0 : max(0, level());
    for (int i = begin; i <= level(); ++i) {
        cout << "Scope level " << i << "\n";
        if (scopes[i]) inorder(scopes[i].get()); else cout << "  (empty)\n";
    }
    cout << "=================================================\n";
}
