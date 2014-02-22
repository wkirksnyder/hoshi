#line 2 "u:\\hoshi\\raw\\LalrGenerator.cpp"
//
//  LalrGenerator                                                         
//  -------------                                                         
//                                                                        
//  Generate an LALR(k) parser from the symbols, rules, etc. in the       
//  grammar. We do this through a number of more manageable phases:       
//                                                                        
//   - Precompute first sets and items.                                   
//                                                                        
//   - Build the LR(0) automaton.                                         
//                                                                        
//   - Find the lookaheads that make the LR(0) automaton into LALR(1).    
//                                                                        
//   - Extend the lookaheads (adding lookahead states) until we have an   
//     LALR(k) automaton.                                                 
//                                                                        
//   - Add `fallback' states to the automaton representing sets of basic  
//     states. These are used in error recovery.                          
//                                                                        
//   - Flatten and save the parse table.
//                                                                        
//  There's a lot of background theory related to this which is not       
//  covered in the comments. For general LALR parsing my favorite         
//  reference is Compilers: Principles, Techniques and Tools by Aho,      
//  Sethi, Ullman and Lam (a.k.a. The Dragon Book). For the extension of  
//  LALR(1) to LALR(k) see Philippe Charles' Ph.D. thesis. For error      
//  recovery see Kirk Snyder's Ph.D. thesis. For parse table flattening   
//  and compression see Storing a Sparse Table by Tarjan and Yao in CACM. 
//

#ifndef LALR_GENERATOR_H
#define LALR_GENERATOR_H

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
#include "Parser.H"
#include "ParserImpl.H"
#include "ErrorHandler.H"
#include "Grammar.H"
#include "ParserData.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

class LalrGenerator
{

    class State;
    class Item;

public:

    LalrGenerator(ParserImpl& prsi,
                  ErrorHandler& errh,
                  Grammar& gram,
                  ParserData& prsd,
                  int64_t debug_flags)
        : prsi(prsi), errh(errh), gram(gram), prsd(prsd), debug_flags(debug_flags) {}

    ~LalrGenerator();

    void generate();

    LalrGenerator(const LalrGenerator&) = delete;
    LalrGenerator(LalrGenerator&&) = delete;
    LalrGenerator& operator=(const LalrGenerator&) = delete;
    LalrGenerator& operator=(LalrGenerator&&) = delete;

private:

    ParserImpl& prsi;
    ErrorHandler& errh;
    Grammar& gram;
    ParserData& prsd;
    int64_t debug_flags;

    //
    //  StateDistance                                                
    //  -------------                                                
    //                                                                   
    //  A class with a state and a distance usable as a map key.         
    //

    struct StateDistance final
    {

        StateDistance(State* state, int distance) : state(state), distance(distance) {}

        bool operator<(const StateDistance& rhs) const
        {

            if (state < rhs.state)
            {
                return true;
            }

            if (state > rhs.state)
            {
                return false;
            }

            return distance < rhs.distance;

        } 

        bool operator>(const StateDistance& rhs) const 
        {
            return rhs < *this;
        } 

        bool operator==(const StateDistance& rhs)
        {
            return state == rhs.state && distance == rhs.distance;
        }   

        State* state;
        int distance;

    };

    //
    //  StateItem                                                
    //  ---------
    //                                                                 
    //  A class with a state and an item usable as a map key.
    //

    struct StateItem final
    {

        StateItem(State* state, Item* item) : state(state), item(item) {}

        bool operator<(const StateItem& rhs) const
        {

            if (state < rhs.state)
            {
                return true;
            }

            if (state > rhs.state)
            {
                return false;
            }

            return item < rhs.item;

        } 

        bool operator>(const StateItem& rhs) const 
        {
            return rhs < *this;
        } 

        bool operator==(const StateItem& rhs)
        {
            return state == rhs.state && item == rhs.item;
        }   

        State* state;
        Item* item;

    };

    //
    //  StateSymbol                                                
    //  -----------
    //                                                                 
    //  A class with a state and a symbol usable as a map key.
    //

    struct StateSymbol final
    {

        StateSymbol(State* state, Symbol* symbol) : state(state), symbol(symbol) {}

        bool operator<(const StateSymbol& rhs) const
        {

            if (state < rhs.state)
            {
                return true;
            }

            if (state > rhs.state)
            {
                return false;
            }

            return symbol < rhs.symbol;

        } 

        bool operator>(const StateSymbol& rhs) const 
        {
            return rhs < *this;
        } 

        bool operator==(const StateSymbol& rhs)
        {
            return state == rhs.state && symbol == rhs.symbol;
        }   

        State* state;
        Symbol* symbol;

    };

    //
    //  RuleDistance                                                
    //  ------------                                                
    //                                                                   
    //  A class with a rule and a distance usable as a map key.         
    //

    struct RuleDistance final
    {

        RuleDistance(Rule* rule, int distance) : rule(rule), distance(distance) {}

        bool operator<(const RuleDistance& rhs) const
        {

            if (rule < rhs.rule)
            {
                return true;
            }

            if (rule > rhs.rule)
            {
                return false;
            }

            return distance < rhs.distance;

        } 

        bool operator>(const RuleDistance& rhs) const 
        {
            return rhs < *this;
        } 

        bool operator==(const RuleDistance& rhs)
        {
            return rule == rhs.rule && distance == rhs.distance;
        }   

        Rule* rule;
        int distance;

    };

    //
    //  PointerSet                                                         
    //  ----------                                                         
    //                                                                     
    //  This is not a great class, but if I use it with discipline it will 
    //  do. It creates a set of pointer types that can be used as the      
    //  domain type in a map. We're going to use a lot of these so they    
    //  will share a pointer to the underlying set. That means we have to  
    //  build it *before* inserting into a set or map, then never touch it 
    //  again. If we want to modify it then do a deep copy and change the  
    //  copy.                                                              
    //

    template<typename Ptr> class PointerSet
    {
    public:

        PointerSet() : data(new std::set<Ptr>()) {}

        explicit PointerSet(const std::set<Ptr>& init) :
            data(new std::set<Ptr>(init)) {}

        std::set<Ptr>& get() { return *data.get(); }

        bool operator<(const PointerSet& rhs) const
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

        bool operator>(const PointerSet& rhs) const 
        {
            return rhs < *this;
        } 

        bool operator==(const PointerSet& rhs) const
        {

            auto left = data->begin();
            auto right = rhs.data->begin();

            while (left != data->end() || right != rhs.data->end())
            {

                if (left == data->end() || right == rhs.data->end() || *left != *right)
                {
                    return false;
                }
                 
                left++;
                right++;

            }

            return true;

        }   

    private:

        std::shared_ptr<std::set<Ptr>> data;

    };

    //
    //  StateStack                                                      
    //  ----------                                                      
    //                                                                  
    //  This is a relatively thin wrapper around a vector of states. We 
    //  need to keep sets of these and this implements the facilities   
    //  required by set.                                              
    //

    class StateStack final
    {
    public:

        StateStack() {}
        explicit StateStack(State* state) { data.clear(); data.push_back(state); }

        std::vector<State*>& get() { return data; }

        bool operator<(const StateStack& rhs) const
        {

            auto left = data.begin();
            auto right = rhs.data.begin();

            while (left != data.end() || right != rhs.data.end())
            {

                if (left == data.end())
                {
                    return true;
                }
                else if (right == rhs.data.end())
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

        bool operator>(const StateStack& rhs) const 
        {
            return rhs < *this;
        } 

        bool operator==(const StateStack& rhs) const
        {

            auto left = data.begin();
            auto right = rhs.data.begin();

            while (left != data.end() || right != rhs.data.end())
            {

                if (left == data.end() || right == rhs.data.end() || *left != *right)
                {
                    return false;
                }
                 
                left++;
                right++;

            }

            return true;

        }   

    private:

        std::vector<State*> data;

    };

    //
    //  Item                                                             
    //  ----                                                             
    //                                                                   
    //  An LR(0) item. The next pointer indicates the item with the dot  
    //  position advanced by one. It will be null in the last rule item. 
    //

    struct Item final
    {

        int64_t num = 0;
        Rule* rule = nullptr;
        size_t dot = 0;
        std::set<Symbol*> first_set;
        Item* prev = nullptr;
        Item* next = nullptr;

        Item(const Item&) = delete;
        Item(Item&&) = delete;
        Item& operator=(const Item&) = delete;
        Item& operator=(Item&&) = delete;

        Item() = default;

    };

    std::vector<Item*> item_list;
    Item* get_item()
    {
        Item* item = new Item();
        item->num = item_list.size();
        item_list.push_back(item);
        return item;
    }

    typedef PointerSet<Item*> ItemSet;

    std::map<Rule*, Item*> rule_item_map;

    //
    //  State                        
    //  -----                        
    //                                  
    //  A state in the automaton. 
    //

    struct State final
    {

        int64_t num = 0;
        std::string state_name = "";
        State* lr0_state = nullptr;

        ItemSet item_set;
        std::map<Symbol*, State*> lr0_goto; 
        std::set<State*> lookback_one;
        std::map<size_t, std::set<State*>> lookback;
        std::map<Item*, std::set<Symbol*>> lookaheads;
        std::map<Symbol*, std::set<Symbol*>> lhs_follow;
        std::map<Symbol*, std::set<ParseAction>> action_multimap;
        std::map<Symbol*, ParseAction> action_map;
        std::map<Symbol*, State*> la_goto_map;
        Symbol* la_symbol = nullptr;
        std::map<Symbol*, std::set<State*>> after_shift;
        std::set<State*> base_states;

        State(const State&) = delete;
        State(State&&) = delete;
        State& operator=(const State&) = delete;
        State& operator=(State&&) = delete;

        State() = default;

    };

    std::vector<State*> state_list;
    State* get_state()
    {

        State* state = new State();
        state->num = state_list.size();
        state->state_name = std::to_string(state_list.size());
        state_list.push_back(state);

        return state;

    }

    typedef PointerSet<State*> StateSet;

    State* start_state = nullptr;
    State* restart_state = nullptr;
    std::map<StateSet, State*> state_set_map;
    std::map<Symbol*, std::set<Symbol*>> first_set;

    //
    //  This will proceed in phases. Most of these are major phases. 
    //

    void find_first_sets();
    void build_items();
    void build_lr0_automaton();
    void compute_lookback(State* state, size_t distance);
    void compute_lhs_follow(State* state, Symbol* lhs);
    void find_lalr1_lookaheads();
    void encode_actions();
    void infinite_loop_check();
    void extend_lookaheads();
    void add_error_recovery();
    void save_parser_data();
    
    //
    //  Debugging facilities. 
    //

    void dump_first_sets(std::ostream& os = std::cout,
                         int indent = 0);

    void dump_automaton(const std::string& title = "",
                        std::ostream& os = std::cout,
                        int indent = 0) const;

    void dump_state(State* state,
                    std::ostream& os = std::cout,
                    int indent = 0) const;

};

} // namespace hoshi

#endif // LALR_GENERATOR_H
