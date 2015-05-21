//
//  Grammar                                                               
//  -------                                                               
//                                                                        
//  Traverse an Ast of the input source file assembling the grammar in a  
//  more manageable form. We need lists of symbols and rules but with     
//  subtrees attached to them in various places.                          
//                                                                        
//  We're going to keep largish sets and maps of the items here all over  
//  the place, so we are going to use a flyweight pattern for storage. As 
//  we allocate these things we'll keep pointers in sets, maps and lists  
//  here and return naked pointers. When an object of this class is       
//  destroyed we'll clean up everything we allocated.                     
//

#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "AstType.H"
#include "Parser.H"
#include "ParserImpl.H"
#include "ParserData.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

//
//  Symbols                                                              
//  -------                                                              
//                                                                       
//  We have a number of relevant symbols, where a symbol is something in 
//  the grammar part of the source file. These can be terminals,         
//  nonterminals, or not part of the grammar proper. They can be scanned 
//  tokens, and so forth. What defines this is where they fit in the     
//  namespace.                                                           
//

struct Symbol final
{

    friend class Grammar;

public:

    //
    //  Identification information. 
    //

    int symbol_num = 0;
    std::string symbol_name = "";

    //
    //  Classification booleans. 
    //

    bool is_terminal = false;
    bool is_nonterminal = false;
    bool is_ignored = false;
    bool is_error = false;
    bool is_scanned = false;

    //
    //  Descriptive information. 
    //

    std::string description = "";
    int64_t location = -1;

    //
    //  Scanner data. 
    //

    Ast* regex_list_ast = nullptr;
    bool is_ast_synthesized = false;
    std::string string_value = "";
    Ast* action_ast = nullptr;
    int precedence = 100;
    bool lexeme_needed = false;
    std::string error_message = "";

    //
    //  Force these to be created in the Grammar class. 
    //

    Symbol(const Symbol&) = delete;
    Symbol(Symbol&&) = delete;
    Symbol& operator=(const Symbol&) = delete;
    Symbol& operator=(Symbol&&) = delete;

private:

    Symbol() = default;
    ~Symbol() { if (is_ast_synthesized) delete regex_list_ast; }

};

//
//  Rules                                                            
//  -----                                                            
//                                                                   
//  The core of a grammar is a collection of rules that describe the 
//  various phrases in the language.                                 
//

struct Rule final
{

    friend class Grammar;

public:

    int rule_num = 0;
    Symbol* lhs = nullptr;
    std::vector<Symbol*> rhs;
    Ast* ast_former_ast = nullptr;
    bool is_ast_synthesized = false;
    Ast* action_ast = nullptr;
    int64_t location = -1;

    //
    //  Force these to be created in the Grammar class. 
    //

    Rule(const Rule&) = delete;
    Rule(Rule&&) = delete;
    Rule& operator=(const Rule&) = delete;
    Rule& operator=(Rule&&) = delete;

private:

    Rule() = default;
    ~Rule() { if (is_ast_synthesized) delete ast_former_ast; }

};

//
//  Grammar                                                         
//  -------                                                         
//                                                                         
//  This class provides management of grammar symbols. We have lots of     
//  these so we're using a flyweight pattern. We create them here in       
//  various maps and lists and return pointers to the values. We keep this 
//  object around as long as we need those symbols and in the destructor   
//  we destoy all of them.                                                 
//

class Grammar final
{

public:

    Grammar(ParserImpl& prsi,
            ErrorHandler& errh,
            ParserData& prsd,
            Ast* root,
            int64_t debug_flags);

    ~Grammar();

    void extract();
    void save_parser_data();

    Symbol* get_symbol(const std::string& name);
    Symbol* create_symbol(const std::string& name);
    void delete_symbol(Symbol*);
    Rule* add_rule();
    void delete_rule(Rule*);

    //
    //  Global options.
    //

    int max_lookaheads = 2;
    int expected_conflicts = 0;
    bool error_recovery = true;
    bool keep_whitespace = false;
    bool case_sensitive = true;

    //
    //  Declared symbols. 
    //

    std::map<std::string, Symbol*> symbol_map;
    std::vector<Rule*> rule_list;

    Symbol* eof_symbol = nullptr;
    Symbol* error_symbol = nullptr;
    Symbol* accept_symbol = nullptr;
    Symbol* epsilon_symbol = nullptr;

    Rule* start_rule = nullptr;

    static const int line_width = 90;
    int symbol_width = 0;

    //
    //  Disable copying. 
    //

    Grammar(const Grammar&) = delete;
    Grammar(Grammar&&) = delete;
    Grammar& operator=(const Grammar&) = delete;
    Grammar& operator=(Grammar&&) = delete;

private:

    ParserImpl& prsi;
    ErrorHandler& errh;
    ParserData& prsd;
    Ast* root;
    int64_t debug_flags;

    //
    //  We will perform something like a depth first search on the Ast. 
    //  This structure is passed along to communicate among the nodes.  
    //

    struct Context
    {

        std::set<int> processed_set;    
        bool first = false;
        bool last = false;

        Symbol* symbol = nullptr;

        Rule* rule = nullptr;
        Symbol* lhs = nullptr;
        Symbol* rhs_term = nullptr;
        bool left_assoc = false;
        Ast* ast_former_ast = nullptr;
        Ast* action_ast = nullptr;

        std::string lexeme = "";
        int64_t location = -1;
        int integer_value = 0;
        std::string string_value = "";
        bool bool_value = false;

    };

    //
    //  The generic node handler and specific handlers for various node 
    //  types.                                                          
    //

    static void (*extract_handler[])(Grammar& gram, Ast* root, Context& ctx);
    static const char* extract_handler_name[];

    static void handle_extract(Grammar& gram, Ast* root, Context& ctx);
    static void handle_error(Grammar& gram, Ast* root, Context& ctx);
    static void handle_list(Grammar& gram, Ast* root, Context& ctx);
    static void handle_lookaheads(Grammar& gram, Ast* root, Context& ctx);
    static void handle_error_recovery(Grammar& gram, Ast* root, Context& ctx);
    static void handle_conflicts(Grammar& gram, Ast* root, Context& ctx);
    static void handle_keep_whitespace(Grammar& gram, Ast* root, Context& ctx);
    static void handle_case_sensitive(Grammar& gram, Ast* root, Context& ctx);
    static void handle_token_declaration(Grammar& gram, Ast* root, Context& ctx);
    static void handle_token_option_list(Grammar& gram, Ast* root, Context& ctx);
    static void handle_token_template(Grammar& gram, Ast* root, Context& ctx);
    static void handle_token_description(Grammar& gram, Ast* root, Context& ctx);
    static void handle_token_regex_list(Grammar& gram, Ast* root, Context& ctx);
    static void handle_token_precedence(Grammar& gram, Ast* root, Context& ctx);
    static void handle_token_action(Grammar& gram, Ast* root, Context& ctx);
    static void handle_token_lexeme(Grammar& gram, Ast* root, Context& ctx);
    static void handle_token_ignore(Grammar& gram, Ast* root, Context& ctx);
    static void handle_token_error(Grammar& gram, Ast* root, Context& ctx);
    static void handle_rule(Grammar& gram, Ast* root, Context& ctx);
    static void handle_rule_rhs(Grammar& gram, Ast* root, Context& ctx);
    static void handle_optional(Grammar& gram, Ast* root, Context& ctx);
    static void handle_zero_closure(Grammar& gram, Ast* root, Context& ctx);
    static void handle_one_closure(Grammar& gram, Ast* root, Context& ctx);
    static void handle_group(Grammar& gram, Ast* root, Context& ctx);
    static void handle_rule_precedence(Grammar& gram, Ast* root, Context& ctx);
    static void handle_rule_precedence_spec(Grammar& gram, Ast* root, Context& ctx);
    static void handle_rule_left_assoc(Grammar& gram, Ast* root, Context& ctx);
    static void handle_rule_right_assoc(Grammar& gram, Ast* root, Context& ctx);
    static void handle_rule_operator_spec(Grammar& gram, Ast* root, Context& ctx);
    static void handle_terminal_reference(Grammar& gram, Ast* root, Context& ctx);
    static void handle_nonterminal_reference(Grammar& gram, Ast* root, Context& ctx);
    static void handle_empty(Grammar& gram, Ast* root, Context& ctx);
    static void handle_identifier(Grammar& gram, Ast* root, Context& ctx);
    static void handle_integer(Grammar& gram, Ast* root, Context& ctx);
    static void handle_string(Grammar& gram, Ast* root, Context& ctx);
    static void handle_triple_string(Grammar& gram, Ast* root, Context& ctx);
    static void handle_true(Grammar& gram, Ast* root, Context& ctx);
    static void handle_false(Grammar& gram, Ast* root, Context& ctx);

    //
    //  Debugging utilities.
    //

    void dump_grammar(std::ostream& os = std::cout, int indent = 0) const;
    void dump_tokens(std::ostream& os = std::cout, int indent = 0) const;
    void dump_nonterminals(std::ostream& os = std::cout, int indent = 0) const;
    void dump_rules(std::ostream& os = std::cout, int indent = 0) const;

};

} // namespace hoshi

#endif // GRAMMAR_H
