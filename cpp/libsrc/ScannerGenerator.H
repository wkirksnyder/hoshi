//
//  ScannerGenerator                                                       
//  ----------------                                                       
//                                                                         
//  Create the scanner part of the parser from token information in the    
//  grammar source. Each token type has a regular expression or is used as 
//  a literal in the grammar. From these we generate a DFA that can be     
//  called by the parser to scan the input source.                         
//                                                                         
//  The theory behind all this is fairly well known. My favorite reference 
//  is Introduction to AUtomata Theory, Languages and Computation by       
//  Hopcraft and Ullman. The 1979 first edition may still be the best one. 
//

#ifndef SCANNER_GENERATOR_H
#define SCANNER_GENERATOR_H

#include <cstdint>
#include <cstring>
#include <string>
#include <queue>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "AstType.H"
#include "OpcodeType.H"
#include "Parser.H"
#include "ParserImpl.H"
#include "ParserData.H"
#include "ErrorHandler.H"
#include "Grammar.H"
#include "CodeGenerator.H"
#include "ActionGenerator.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

class ScannerGenerator final
{

    struct State;

public:

    ScannerGenerator(ParserImpl& prsi,
                     ErrorHandler& errh,
                     Grammar& gram,
                     CodeGenerator& code,   
                     ActionGenerator& actg,
                     ParserData& prsd,
                     int64_t debug_flags)
        : prsi(prsi), errh(errh), gram(gram), code(code), actg(actg), prsd(prsd),
          debug_flags(debug_flags) {}

    ~ScannerGenerator();

    void generate();
    void save_parser_data();

private:

    ParserImpl& prsi;
    ErrorHandler& errh;
    Grammar& gram;
    CodeGenerator& code;
    ActionGenerator& actg;
    ParserData& prsd;
    int64_t debug_flags;

    //
    //  AcceptAction                                                     
    //  ------------                                                     
    //                                                                   
    //  This information is needed when we hit an accepting state in the 
    //  automaton.                                                       
    //

    struct AcceptAction
    {

        Symbol* token = nullptr;
        Ast* guard_ast = nullptr;
        
        AcceptAction(Symbol* token, Ast* guard_ast)
            : token(token), guard_ast(guard_ast) {}

        bool operator<(const AcceptAction& rhs) const
        {
            if (token->precedence > rhs.token->precedence) return true;
            if (token->precedence < rhs.token->precedence) return false;
            if (token < rhs.token) return true;
            if (token > rhs.token) return false;
            return guard_ast < rhs.guard_ast;
        } 

        bool operator>(const AcceptAction& rhs) const 
        {
            return rhs < *this;
        } 

        bool operator==(const AcceptAction& rhs)
        {
            return token == rhs.token &&
                   guard_ast == rhs.guard_ast;
        }   

    };   

    //
    //  Transition                                                         
    //  ----------                                                         
    //                                                                     
    //  Transitions for either a DFA or a NFA with e-moves. We want to 
    //  keep ordered sets of these around so we provide the minimal        
    //  support for that.                                                  
    //

    struct Transition
    {

        char32_t range_start = 0;
        char32_t range_end = 0;
        State* target_state = nullptr;

        Transition(char32_t range_start, char32_t range_end, State* target_state)
            : range_start(range_start), range_end(range_end), target_state(target_state) {}

        bool operator<(const Transition& rhs) const
        {
            if (range_start < rhs.range_start) return true;
            if (range_start > rhs.range_start) return false;
            if (range_end < rhs.range_end) return true;
            if (range_end > rhs.range_end) return false;
            return target_state < rhs.target_state;
        } 

        bool operator>(const Transition& rhs) const 
        {
            return rhs < *this;
        } 

        bool operator==(const Transition& rhs)
        {
            return range_start == rhs.range_start &&
                   range_end == rhs.range_end &&
                   target_state == rhs.target_state;
        }   

    };

    //
    //  State                                                              
    //  -----                                                              
    //                                                                     
    //  A state for either a deterministic or non-deterministic finite     
    //  automaton. Most of what we store is transitions. For DFA's the     
    //  transitions must be disjoint ranges and there can be no            
    //  epsilon-moves. For NFA's both these conditions are relaxed.        
    //                                                                     
    //  Note that we are doing a lot of set manipulations on these things  
    //  so we just store them as naked pointers. We maintain a vector of   
    //  the pointers in this class and free everything in that destructor. 
    //  This seems to be called the flyweight pattern in GOF.              
    //

    struct State
    {
        std::set<State*> e_moves;
        std::set<Transition> transitions;
        std::set<AcceptAction> accept_actions;
    };

    std::vector<State*> allocated_states;
    State* get_new_state()
    {
        State* state = new State();
        allocated_states.push_back(state);
        return state;
    }

    //
    //  StateSet                                                       
    //  --------                                                       
    //                                                                 
    //  We will need a lot of sets of states, and in fact need to use  
    //  those states as map keys. This is a wrapper around the library 
    //  set<> suitable to use as a key. We don't care that much about  
    //  order, but we have to provide the facilities map<> needs.      
    //

    struct StateSet final
    {
    public:

        StateSet() : data(new std::set<State*>()) {}

        std::set<State*>& get() { return *data.get(); }

        bool operator<(const StateSet& rhs) const
        {

            auto left = data->begin();
            auto right = rhs.data->begin();

            while (left != data->end() || right != rhs.data->end())
            {

                if (left == data->end())
                {
                    return true;
                }
                else if (right == rhs.data->end())
                {
                    return false;
                }

                if (*left < *right)
                {
                    return true;
                }
                else if (*left > *right)
                {
                    return false;
                }
                 
                left++;
                right++;

            }

            return false;

        } 

        bool operator>(const StateSet& rhs) const 
        {
            return rhs < *this;
        } 

        bool operator==(const StateSet& rhs)
        {

            auto left = data->begin();
            auto right = rhs.data->begin();

            while (left != data->end() || right != rhs.data->end())
            {

                if (left == data->end() || right == rhs.data->end())
                {
                    return false;
                }

                if (*left != *right)
                {
                    return false;
                }
                 
                left++;
                right++;

            }

            return true;

        }   

    private:

        std::shared_ptr<std::set<State*>> data;

    };

    //
    //  Context                                                         
    //  -------                                                         
    //                                                                  
    //  We will perform something like a depth first search on the Ast. 
    //  This structure is passed along to communicate among the nodes.  
    //

    struct Context
    {
        State* start_state = nullptr;
        State* final_state = nullptr;
        char32_t character = 0;
    };

    //
    //  NFA creation stuff                                           
    //  ------------------                                           
    //                                                                
    //  We're going to create the NFA by traversing the Ast creating 
    //  sub-automata and merging them.                                
    //

    void construct_nfa();

    State* nfa_start_state = nullptr;

    static void (*build_nfa_handler[])(ScannerGenerator& scan, Ast* root, Context& ctx);
    static const char* build_nfa_handler_name[];

    static void handle_build_nfa(ScannerGenerator& scan,
                                 Ast* root,
                                 Context& ctx);

    static void handle_error(ScannerGenerator& scan,
                             Ast* root,
                             Context& ctx);
    
    static void handle_regex(ScannerGenerator& scan,
                             Ast* root,
                             Context& ctx);
    
    static void handle_regex_or(ScannerGenerator& scan,
                                Ast* root,
                                Context& ctx);
    
    static void handle_regex_list(ScannerGenerator& scan,
                                  Ast* root,
                                  Context& ctx);
    
    static void handle_regex_optional(ScannerGenerator& scan,
                                      Ast* root,
                                      Context& ctx);
    
    static void handle_regex_zero_closure(ScannerGenerator& scan,
                                          Ast* root,
                                          Context& ctx);
    
    static void handle_regex_one_closure(ScannerGenerator& scan,
                                         Ast* root,
                                         Context& ctx);
    
    static void handle_regex_char(ScannerGenerator& scan,
                                  Ast* root,
                                  Context& ctx);
    
    static void handle_regex_wildcard(ScannerGenerator& scan,
                                      Ast* root,
                                      Context& ctx);
    
    static void handle_regex_whitespace(ScannerGenerator& scan,
                                        Ast* root,
                                        Context& ctx);
    
    static void handle_regex_not_whitespace(ScannerGenerator& scan,
                                            Ast* root,
                                            Context& ctx);
    
    static void handle_regex_digits(ScannerGenerator& scan,
                                    Ast* root,
                                    Context& ctx);
    
    static void handle_regex_not_digits(ScannerGenerator& scan,
                                        Ast* root,
                                        Context& ctx);
    
    static void handle_regex_escape(ScannerGenerator& scan,
                                    Ast* root,
                                    Context& ctx);
    
    static void handle_regex_alt_newline(ScannerGenerator& scan,
                                         Ast* root,
                                         Context& ctx);
    
    static void handle_regex_newline(ScannerGenerator& scan,
                                     Ast* root,
                                     Context& ctx);
    
    static void handle_regex_cr(ScannerGenerator& scan,
                                Ast* root,
                                Context& ctx);
    
    static void handle_regex_v_bar(ScannerGenerator& scan,
                                   Ast* root,
                                   Context& ctx);
    
    static void handle_regex_star(ScannerGenerator& scan,
                                  Ast* root,
                                  Context& ctx);
    
    static void handle_regex_plus(ScannerGenerator& scan,
                                  Ast* root,
                                  Context& ctx);
    
    static void handle_regex_question(ScannerGenerator& scan,
                                      Ast* root,
                                      Context& ctx);
    
    static void handle_regex_period(ScannerGenerator& scan,
                                    Ast* root,
                                    Context& ctx);
    
    static void handle_regex_dollar(ScannerGenerator& scan,
                                    Ast* root,
                                    Context& ctx);
    
    static void handle_regex_space(ScannerGenerator& scan,
                                   Ast* root,
                                   Context& ctx);
    
    static void handle_regex_left_paren(ScannerGenerator& scan,
                                        Ast* root,
                                        Context& ctx);
    
    static void handle_regex_right_paren(ScannerGenerator& scan,
                                         Ast* root,
                                         Context& ctx);
    
    static void handle_regex_left_bracket(ScannerGenerator& scan,
                                          Ast* root,
                                          Context& ctx);
    
    static void handle_regex_right_bracket(ScannerGenerator& scan,
                                           Ast* root,
                                           Context& ctx);
    
    static void handle_regex_left_brace(ScannerGenerator& scan,
                                        Ast* root,
                                        Context& ctx);
    
    static void handle_regex_right_brace(ScannerGenerator& scan,
                                         Ast* root,
                                         Context& ctx);
    
    static void handle_charset(ScannerGenerator& scan,
                               Ast* root,
                               Context& ctx);
    
    static void handle_charset_invert(ScannerGenerator& scan,
                                      Ast* root,
                                      Context& ctx);
    
    static void handle_charset_range(ScannerGenerator& scan,
                                     Ast* root,
                                     Context& ctx);
    
    static void handle_charset_char(ScannerGenerator& scan,
                                    Ast* root,
                                    Context& ctx);
    
    static void handle_charset_whitespace(ScannerGenerator& scan,
                                          Ast* root,
                                          Context& ctx);
    
    static void handle_charset_not_whitespace(ScannerGenerator& scan,
                                              Ast* root,
                                              Context& ctx);
    
    static void handle_charset_digits(ScannerGenerator& scan,
                                      Ast* root,
                                      Context& ctx);
    
    static void handle_charset_not_digits(ScannerGenerator& scan,
                                          Ast* root,
                                          Context& ctx);
    
    static void handle_charset_escape(ScannerGenerator& scan,
                                      Ast* root,
                                      Context& ctx);
    
    static void handle_charset_alt_newline(ScannerGenerator& scan,
                                           Ast* root,
                                           Context& ctx);
    
    static void handle_charset_newline(ScannerGenerator& scan,
                                       Ast* root,
                                       Context& ctx);
    
    static void handle_charset_cr(ScannerGenerator& scan,
                                  Ast* root,
                                  Context& ctx);
    
    static void handle_charset_caret(ScannerGenerator& scan,
                                     Ast* root,
                                     Context& ctx);
    
    static void handle_charset_dash(ScannerGenerator& scan,
                                    Ast* root,
                                    Context& ctx);
    
    static void handle_charset_dollar(ScannerGenerator& scan,
                                      Ast* root,
                                      Context& ctx);
    
    static void handle_charset_left_bracket(ScannerGenerator& scan,
                                            Ast* root,
                                            Context& ctx);
    
    static void handle_charset_right_bracket(ScannerGenerator& scan,
                                             Ast* root,
                                             Context& ctx);
    
    //
    //  DFA creation stuff                                                
    //  ------------------                                                
    //                                                                    
    //  We create the DFA from the NFA in the conventional manner. 
    //  Hopcraft and Ullman's automata book has an exhaustive description 
    //  of the process.                                                   
    //

    void nfa_to_dfa();
    State* dfa_start_state;

    std::map<StateSet, State*> nfa_to_dfa_map;
    std::map<State*, StateSet> dfa_to_nfa_map;
    std::queue<State*> workpile;

    void find_e_closure(StateSet& closure);
    void collapse_dfa_state(State* state);
    State* get_dfa_state(StateSet& nfa_state_set);

    //
    //  Optimization and Code Generation stuff 
    //

    ICodeLabel* scan_label = nullptr;
    void minimize_dfa();
    void create_vmcode();

    //
    //  Debugging code. 
    //

    void dump_automaton(State* start_state,
                        std::ostream& os = std::cout,
                        int indent = 0) const;

};

} // namespace hoshi

#endif // SCANNER_GENERATOR_H






