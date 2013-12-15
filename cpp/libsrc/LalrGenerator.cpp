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

#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "Parser.H"
#include "ParserImpl.H"
#include "ParseAction.H"
#include "ErrorHandler.H"
#include "Grammar.H"
#include "LalrGenerator.H"
#include "ParserData.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;

//
//  A macro to find the size of an array.
//

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

//
//  ~LalrGenerator                                                           
//  --------------                                                         
//                                                                         
//  We use a flyweight pattern for some of the data structures we use here 
//  and keep lists of the stuff we allocated. Here we can free them.       
//

LalrGenerator::~LalrGenerator()
{

    for (Item* p: item_list)
    {
        delete p;
    }

    for (State* p: state_list)
    {
        delete p;
    }

}

//
//  generate                                                         
//  --------                                                         
//                                                                   
//  The external entry point. Call support functions for each phase. 
//

void LalrGenerator::generate()
{

    if ((debug_flags & DebugType::DebugProgress) != 0)
    {
        cout << "Beginning parser generation: " << prsi.elapsed_time_string() << endl;
    }

    //
    //  Compute first sets for each non-terminal. 
    //

    find_first_sets();

    if ((debug_flags & DebugType::DebugProgress) != 0)
    {
        cout << "First sets generated: " << prsi.elapsed_time_string() << endl;
    }

    if ((debug_flags & DebugType::DebugLalr) != 0)
    {
        dump_first_sets();
    }

    //
    //  Build all the LR(0) items we are going to need. After this we'll 
    //  just refer to them with naked pointers.                          
    //

    build_items();

    //
    //  We'll start with an LR(0) automaton and later graft special 
    //  lookahead states on to that. But first we have to build the 
    //  initial automaton.                                          
    //

    build_lr0_automaton();

    if ((debug_flags & DebugType::DebugProgress) != 0)
    {
        cout << "LR(0) automaton built: " 
             << state_list.size() << " states, "
             << prsi.elapsed_time_string()
             << endl;
    }

    //
    //  Now for a moment we pretend we're building an LALR(1) parser and  
    //  compute the lookahead sets. In most cases a single lookahead will 
    //  be adequate so we'll start with that and extend as necessary      
    //  later.                                                            
    //

    find_lalr1_lookaheads();

    if ((debug_flags & DebugType::DebugProgress) != 0)
    {
        cout << "Lookaheads found: " 
             << state_list.size() << " states, "
             << prsi.elapsed_time_string()
             << endl;
    }

    if ((debug_flags & DebugType::DebugLalr) != 0)
    {
        dump_automaton("LALR(1) Automaton");
    }

    //
    //  Unfortunately, conflicts are not the only kinds of errors we have 
    //  to worry about. There is a nasty error condition which will cause 
    //  this program to loop infinitely if we don't catch it now. This is 
    //  a check to make sure that doesn't happen.                         
    //

    infinite_loop_check();
    if (errh.get_error_count() > 0)
    {
        return;
    }

    //
    //  Encode our states, transitions and such into parse actions. 
    //

    encode_actions();

    //
    //  Next we take the lookaheads and build an action table. Where we    
    //  find conflicts we'll have to extend our lookahead strings. This is 
    //  where we'll discover conflicts.                                    
    //

    extend_lookaheads();
    if (errh.get_error_count() > 0)
    {
        return;
    }

    //
    //  Add error recovery to the automaton.
    //

    if (gram.error_recovery)
    {
        add_error_recovery();
    }

    if ((debug_flags & DebugType::DebugProgress) != 0)
    {
        cout << "Finished automaton generation: " << prsi.elapsed_time_string() << endl;
    }

    if ((debug_flags & DebugType::DebugLalr) != 0)
    {
        dump_automaton("LALR(k) Automaton");
    }

    //
    //  Flatten and save the parse tables. 
    //

    save_parser_data();
    if (errh.get_error_count() > 0)
    {
        return;
    }

    if ((debug_flags & DebugType::DebugProgress) != 0)
    {
        cout << "Finished saving parse tables: " << prsi.elapsed_time_string() << endl;
    }

}

//
//  find_first_sets                                                       
//  ---------------                                                       
//                                                                        
//  The `first' set of a grammar symbol is the set of terminals which may 
//  appear as the first terminal in the expansion of that symbol. The     
//  first set of a terminal is the terminal itself. The textbook method   
//  of calculating first sets is to loop over the rules, adding to the    
//  first set of the left hand side the first sets of the right hand      
//  symbols until we find one which can not produce epsilon. We continue  
//  this until the first sets stabilize.                                  
//                                                                        
//  We're going to take a slightly different approach, although the idea  
//  is the same. We'll perform three phases: In the first we find the set 
//  of nullable symbols, those which can produce epsilon. Using that set  
//  we can in a single pass determine for each symbol the other symbols   
//  to which first sets should be propagated. Finally we use a fixpoint   
//  algorithm to propagate the first sets. The advantage of this apparent 
//  extra work is that it's considerably faster, since we've eliminated a 
//  lot of looping over the rules.                                        
//

void LalrGenerator::find_first_sets()
{

    //
    //  First we find the set of nullable symbols, using a fixpoint
    //  algorithm. We keep a map from non-terminals to [rule, position]   
    //  where the non-terminal is in the position of the rule and we know 
    //  that all preceeding symbols are nullable. As we add a symbol to   
    //  the set of nullable symbols, we update this map and add it to the 
    //  workpile.                                                         
    //

    set<Symbol*> nullable_symbols;
    set<Symbol*> workpile;
    map<Symbol*, set<RuleDistance>> dependent_rules;

    for (Rule* rule: gram.rule_list)
    {

        if (rule->rhs.size() == 0)
        {
            nullable_symbols.insert(rule->lhs);
            workpile.insert(rule->lhs);
        }
        else
        {
            dependent_rules[rule->rhs[0]].insert(RuleDistance(rule, 0));;
        }

    }

    while (workpile.size() > 0)
    {

        Symbol* symbol = *workpile.begin();
        workpile.erase(symbol);

        map<Symbol*, set<RuleDistance>> new_dependent_rules;

        for (auto mp: dependent_rules)
        {

            Symbol* symbol = mp.first;

            for (RuleDistance rd: mp.second)
            {

                Rule* rule = rd.rule;
                int distance = rd.distance;
         
                while (distance < rule->rhs.size() &&
                       nullable_symbols.find(rule->rhs[distance]) != nullable_symbols.end())
                {
                    distance++;
                }
       
                if (distance >= rule->rhs.size())
                {

                    if (nullable_symbols.find(rule->lhs) == nullable_symbols.end())
                    {
                        nullable_symbols.insert(rule->lhs);
                        workpile.insert(rule->lhs);
                    }

                }
                else
                {
                    new_dependent_rules[rule->rhs[distance]].insert(RuleDistance(rule, distance));
                }

            }

        }

        dependent_rules.erase(symbol);

        for (auto mp: new_dependent_rules)
        {

            Symbol* symbol = mp.first;

            for (RuleDistance rd: mp.second)
            {

                Rule* rule = rd.rule;
                int distance = rd.distance;

                dependent_rules[symbol].insert(RuleDistance(rule, distance));
         
            }

        }

    }

    //
    //  Now we have a usable set of nullable symbols. We build a map      
    //  propagate_map, in which all elements of the first set of a domain 
    //  element should propagate to the first set of each range element.  
    //

    map<Symbol*, set<Symbol*>> propagate_map;
    for (Rule* rule: gram.rule_list)
    {

        for (Symbol* symbol: rule->rhs)
        {

            propagate_map[symbol].insert(rule->lhs);
            if (nullable_symbols.find(symbol) == nullable_symbols.end())
            {
                break;
            }

        }

    }

    //
    //  We're finally ready to find the first sets. We start with each 
    //  terminal in its own first set, and use a fixpoint algorithm to 
    //  propagate these until the first sets stabilize.                
    //

    workpile.clear();
    for (auto mp: gram.symbol_map)
    {

        Symbol* symbol = mp.second;

        if (symbol->is_terminal)
        {
            first_set[symbol].insert(symbol);
            workpile.insert(symbol);
        }

    }
            
    while (workpile.size() > 0)
    {

        Symbol* source_symbol = *workpile.begin();
        workpile.erase(source_symbol);

        for (Symbol* target_symbol: propagate_map[source_symbol])
        {

            for (Symbol* symbol: first_set[source_symbol])
            {

                if (first_set[target_symbol].find(symbol) == first_set[target_symbol].end())
                {
                    first_set[target_symbol].insert(symbol);
                    workpile.insert(target_symbol);
                }

            }

        }

    }

    for (Symbol* symbol: nullable_symbols)
    {
        first_set[symbol].insert(gram.epsilon_symbol);
    }

}

//
//  build_items                                                          
//  -----------                                                          
//                                                                      
//  An LR(0) `item' is a rule with a distinguished dot position.        
//  Conceptually the right hand side in front of the dot is the portion 
//  seen at a particular point. We're going to need to manipulate these 
//  things a lot, so we'll use a flyweight pattern, as usual.           
//

void LalrGenerator::build_items()
{

    for (Rule* rule: gram.rule_list)
    {

        //
        //  Create the items for the rule. 
        //

        Item* last = nullptr;

        for (int64_t i = 0; i <= rule->rhs.size(); i++)
        {

            Item* item = get_item();

            item->rule = rule;
            item->dot = i;
            item->next = nullptr;
            item->prev = last;

            if (last != nullptr)
            {
                last->next = item;
            }
            else
            {
                rule_item_map[rule] = item;
            }

            last = item;

        }

        //
        //  Now go back and compute first sets for each rule. 
        //

        last->first_set.insert(gram.epsilon_symbol);

        for (Item* item = last->prev; item != nullptr; item = item->prev)
        {

            item->first_set = first_set[rule->rhs[item->dot]];
            if (item->first_set.find(gram.epsilon_symbol) != item->first_set.end())
            {

                item->first_set.erase(gram.epsilon_symbol);

                for (Symbol* symbol: item->next->first_set)
                {
                    item->first_set.insert(symbol);
                }

            }

        }

    }

}

//
//  build_lr0_automaton                                                    
//  -------------------                                                    
//                                                                         
//  Now we build the LR(0) automaton. Again, we do this in phases to speed 
//  up the calculation. A simple-minded algorithm would need to find the   
//  closure of an LR(0) item set frequently, but these closure operations  
//  duplicate a lot of effort. Instead we'll find a closure set for each   
//  non-terminal. We can take the unions of many of these to find the      
//  closure of a set, given its kernel. After we've found those closure    
//  sets we'll build the automaton in the usual manner.                    
//

void LalrGenerator::build_lr0_automaton()
{

    std::map<Symbol*, ItemSet> closure_items;

    //
    //  build_closure_items                                               
    //  -------------------                                               
    //                                                                    
    //  We use a fixpoint algorithm to build the map closure_items, which 
    //  maps non-terminals to LR(0) item sets. These will be added to a   
    //  closure when the non-terminal is in the dot position of a kernel  
    //  item.                                                             
    //

    function<void(void)> build_closure_items = [&](void) -> void
    {

        map<Symbol*, set<Symbol*>> propagate_map;

        for (Rule* rule: gram.rule_list)
        {

            closure_items[rule->lhs].get().insert(rule_item_map[rule]);
            if (rule->rhs.size() > 0)
            {
                propagate_map[rule->rhs[0]].insert(rule->lhs);
            }

        }

        set<Symbol*> workpile;
        for (auto mp: gram.symbol_map)
        {
        
            Symbol* symbol = mp.second;
            if (symbol->is_nonterminal)
            {
                workpile.insert(symbol);
            }

        }

        while (workpile.size() > 0)
        {

            Symbol* symbol = *workpile.begin();
            workpile.erase(symbol);

            for (Symbol* s: propagate_map[symbol])
            {

                for (Item* item: closure_items[symbol].get())
                {

                    if (closure_items[s].get().find(item) == closure_items[s].get().end())
                    {
                        closure_items[s].get().insert(item);
                        workpile.insert(s);
                    }

                }

            }

        }

    };

    //
    //  build_lr0_closure                                             
    //  -----------------                                             
    //                                                                
    //  Given a set of kernel items add the closure items to find the 
    //  LR(0) closure.                                                
    //

    function<void(ItemSet&)> build_lr0_closure = [&](ItemSet& item_set) -> void
    {

        ItemSet raw_set = ItemSet(item_set.get());
        for (Item* kernel_item: raw_set.get())
        {

            if (kernel_item->next == nullptr)
            {
                continue;
            }

            for (Item* item: closure_items[kernel_item->rule->rhs[kernel_item->dot]].get())
            {
                item_set.get().insert(item);
            }

        }

    };

    //
    //  build_lr0_automaton                                             
    //  -------------------                                             
    //                                                                  
    //  The main function starts here.                                  
    //                                                                  
    //  Build a set of closure items so we can form lR(0) closures more 
    //  quickly.                                                        
    //

    build_closure_items();

    //
    //  Now we can build the automaton. We start with the state consisting 
    //  of `%accept ::= S,$' and expand until we have nothing left to      
    //  expand.                                                            
    //

    map<ItemSet, State*> state_map;

    start_state = get_state();
    start_state->lr0_state = start_state;
    start_state->item_set.get().insert(rule_item_map[gram.start_rule]);
    build_lr0_closure(start_state->item_set);
    state_map[start_state->item_set] = start_state;

    for (int64_t i = 0; i < state_list.size(); i++)
    {

        //
        //  Build the goto kernels. 
        //

        State* state = state_list[i];
        map<Symbol*, ItemSet> goto_kernels;

        for (Item* item: state->item_set.get())
        {
 
            if (item->next != nullptr)
            {
                goto_kernels[item->rule->rhs[item->dot]].get().insert(item->next);
            }

        }

        //
        //  Add new states for the gotos. 
        //

        for (auto mp: goto_kernels)
        {

            ItemSet item_set = ItemSet(mp.second.get());
            build_lr0_closure(item_set);

            State* goto_state = nullptr;
            if (state_map.find(item_set) != state_map.end())
            {
                goto_state = state_map[item_set];
            }
            else
            {
                goto_state = get_state();
                goto_state->lr0_state = goto_state;
                goto_state->item_set = item_set;
                state_map[goto_state->item_set] = goto_state;
            }
                
            state->lr0_goto[mp.first] = goto_state;
            goto_state->lookback_one.insert(state);

        }

    }

}

//
//  compute_lookback                                                       
//  ----------------                                                       
//                                                                         
//  Find the set of states a given distance back in the LR(0) automaton.   
//  We use a lazy algorithm for this computing as needed but saving prior  
//  results. On each call we assume anything we computed before is correct 
//  and start from there.                                                  
//                                                                         
//  We're going to do this with a fixpoint algorithm.                      
//

void LalrGenerator::compute_lookback(LalrGenerator::State* state, size_t distance)
{

    //
    //  If we've already computed this then just return. 
    //

    if (state->lookback.find(distance) != state->lookback.end())
    {
        return;
    }

    //
    //  Build a propagate map describing how states flow through the 
    //  automaton.                                                   
    //

    map<StateDistance, set<StateDistance>> propagate_map;
    set<StateDistance> workpile;

    workpile.insert(StateDistance(state, distance));

    while (workpile.size() > 0)
    {

        StateDistance sd = *workpile.begin();
        workpile.erase(sd);

        if (sd.distance == 0)         
        {
            sd.state->lookback[sd.distance].insert(sd.state);
            continue;
        }

        for (State* s: sd.state->lookback_one)
        {

            propagate_map[StateDistance(s, sd.distance - 1)].insert(
                StateDistance(sd.state, sd.distance));

            if (s->lookback.find(sd.distance - 1) == s->lookback.end())              
            {
                workpile.insert(StateDistance(s, sd.distance - 1));
            }

        }

    }

    //
    //  Everything in the source was already computed. Initialize the 
    //  workpile with those.                                          
    //

    for (auto mp: propagate_map)
    {
        workpile.insert(mp.first);
    }

    //
    //  Continue until all the sets stabilize. 
    //

    while (workpile.size() > 0)
    {

        StateDistance source = *workpile.begin();
        workpile.erase(source);

        for (StateDistance target: propagate_map[source])
        {

            for (State* s: source.state->lookback[source.distance])
            {

                if (target.state->lookback[target.distance].find(s) ==
                    target.state->lookback[target.distance].end())
                {
                    target.state->lookback[target.distance].insert(s);
                    workpile.insert(target);
                }

            }

        }

    }

}
    
//
//  compute_lhs_follow                                                  
//  ------------------                                                  
//                                                                      
//  Lhs_follow is the set of terminals that can follow a lhs in a given 
//  state.                                                              
//

void LalrGenerator::compute_lhs_follow(State* state, Symbol* lhs)
{

    //
    //  If we've already computed this then just return. 
    //

    if (state->lhs_follow.find(lhs) != state->lhs_follow.end())
    {
        return;
    }

    //
    //  Build a propagate map describing how states flow through the 
    //  automaton.                                                   
    //

    set<StateSymbol> workpile;
    map<StateSymbol, set<StateSymbol>> propagate_map;

    workpile.insert(StateSymbol(state, lhs));

    while (workpile.size() > 0)
    {

        StateSymbol ss = *workpile.begin();
        workpile.erase(ss);

        if (ss.symbol == gram.accept_symbol)
        {
            ss.state->lhs_follow[ss.symbol].insert(gram.eof_symbol);
            continue;
        }

        for (Item* item: ss.state->lr0_goto[ss.symbol]->item_set.get())
        {

            if (item->dot == 0)
            {
                continue;
            }

            for (Symbol* symbol: item->first_set)
            {

                if (symbol != gram.epsilon_symbol)
                {
                    ss.state->lhs_follow[ss.symbol].insert(symbol);
                    continue;
                }

                compute_lookback(ss.state, item->dot - 1);
                for (State* s: ss.state->lookback[item->dot - 1])
                {

                    propagate_map[StateSymbol(s, item->rule->lhs)].insert(
                        StateSymbol(ss.state, ss.symbol));

                    if (s->lhs_follow.find(item->rule->lhs) == s->lhs_follow.end())
                    {
                        workpile.insert(StateSymbol(s, item->rule->lhs));
                    }

                }

            }

        }

    }

    //
    //  Everything in the source was already computed. Initialize the 
    //  workpile with those.                                          
    //

    for (auto mp: propagate_map)
    {
        workpile.insert(mp.first);
    }

    //
    //  Continue until all the sets stabilize. 
    //

    while (workpile.size() > 0)
    {

        StateSymbol source = *workpile.begin();
        workpile.erase(source);

        for (StateSymbol target: propagate_map[source])
        {

            for (Symbol* symbol: source.state->lhs_follow[source.symbol])
            {

                if (target.state->lhs_follow[target.symbol].find(symbol) ==
                    target.state->lhs_follow[target.symbol].end())
                {
                    target.state->lhs_follow[target.symbol].insert(symbol);
                    workpile.insert(target);
                }

            }

        }

    }

}

//
//  find_lalr1_lookaheads                                                      
//  ---------------------                                                      
//                                                                       
//  Compute the LALR(1) lookahead strings. It may turn out that the      
//  grammar isn't LALR(1), in which case we'll have some conflicts to be 
//  resolved later.                                                      
//                                                                       
//  The general idea is to look for items in states and apply the first  
//  sets of those items to the symbol *before* the dot.                  
//

void LalrGenerator::find_lalr1_lookaheads()
{

    //
    //  Start by building a map describing how lookaheads propagate around 
    //  the automaton and an initial work pile with all state/item         
    //  combinations that we seed with lookaheads based on item first      
    //  sets.                                                              
    //

    set<StateItem> workpile;
    map<StateItem, set<StateItem>> propagate_map;

    State* target_state = start_state;
    Item* target_item = rule_item_map[gram.start_rule];

    while (target_item->next != nullptr)
    {
        target_state = target_state->lr0_goto[target_item->rule->rhs[target_item->dot]];
        target_item = target_item->next;
    }

    target_state->lookaheads[target_item].insert(gram.eof_symbol);
    workpile.insert(StateItem(target_state, target_item));

    for (State* source_base_state: state_list)
    {

        for (Item* source_base_item: source_base_state->item_set.get())
        {

            if (source_base_item->dot == 0)
            {
                continue;
            }

            Symbol* source_symbol = source_base_item->rule->rhs[source_base_item->dot - 1];

            State* source_state = source_base_state;
            Item* source_item = source_base_item;

            while (source_item->next != nullptr)
            {
                source_state = source_state->lr0_goto[
                                   source_item->rule->rhs[source_item->dot]];
                source_item = source_item->next;
            }

            for (State* target_base_state: source_base_state->lookback_one)
            {

                for (Item* target_base_item: target_base_state->item_set.get())
                {

                    if (target_base_item->rule->lhs != source_symbol)
                    {
                        continue;
                    }

                    State* target_state = target_base_state;
                    Item* target_item = target_base_item;

                    while (target_item->next != nullptr)
                    {
                        target_state = target_state->lr0_goto[
                                           target_item->rule->rhs[target_item->dot]];
                        target_item = target_item->next;
                    }

                    for (Symbol* symbol: source_base_item->first_set)
                    {
                        
                        if (symbol == gram.epsilon_symbol)
                        {
                            propagate_map[StateItem(source_state, source_item)].insert(
                                StateItem(target_state, target_item));
                        }
                        else
                        {
                            target_state->lookaheads[target_item].insert(symbol);
                        }

                        workpile.insert(StateItem(target_state, target_item));

                    }

                }

            }

        }

    }

    //
    //  Propagate lookaheads until the sets stabilize. 
    //

    while (workpile.size() > 0)
    {

        StateItem source = *workpile.begin();
        workpile.erase(source);

        for (StateItem target: propagate_map[source])
        {

            for (Symbol* symbol: source.state->lookaheads[source.item])
            {

                if (target.state->lookaheads[target.item].find(symbol) ==
                    target.state->lookaheads[target.item].end())
                {
                    target.state->lookaheads[target.item].insert(symbol);
                    workpile.insert(target);
                }

            }

        }

    }

}

//
//  encode_actions                                                        
//  --------------                                                        
//                                                                        
//  This procedure uses the LR(0) automaton and the lookaheads we've      
//  computed and builds a preliminary version of the action table. We say 
//  preliminary, because at this point conflicts are permitted (we'll     
//  extend lookahead strings to disambiguate later). Reduce actions are   
//  encoded with follow states, which are sets of states that can be      
//  entered after performing the reduction and reading the next token.    
//

void LalrGenerator::encode_actions()
{

    for (State* state: state_list)
    {

        //
        //  LR(0) transitions become shift actions for terminals and goto 
        //  actions for nonterminals.                                     
        //

        for (auto mp: state->lr0_goto)
        {

            auto symbol = mp.first;
            if (symbol->is_terminal)
            {
                ParseAction action;
                action.action_type = ActionShift;
                action.goto_state = mp.second->num;
                state->action_multimap[symbol].insert(action);
            }
            else if (symbol->is_nonterminal)
            {
                ParseAction action;
                action.action_type = ActionGoto;
                action.goto_state = mp.second->num;
                state->action_multimap[symbol].insert(action);
            }

        }

        //
        //  Complete items become accept actions if the left hand symbol 
        //  is accept or a reduce action otherwise.                      
        //

        for (auto mp: state->lookaheads)
        {
            
            if (mp.first->rule->lhs == gram.accept_symbol)
            {

                for (Symbol* symbol: mp.second)
                {
                    ParseAction action;
                    action.action_type = ActionAccept;
                    state->action_multimap[symbol].insert(action);
                }

            }
            else
            {

                for (Symbol* symbol: mp.second)
                {
                    ParseAction action;
                    action.action_type = ActionReduce;
                    action.rule_num = mp.first->rule->rule_num;
                    state->action_multimap[symbol].insert(action);
                }

            }

        }

    }

}

//
//  infinite_loop_check                                                   
//  -------------------                                                   
//                                                                        
//  When this procedure is called we should have found the LALR(1)        
//  lookahead sets and we're about to extend them to LALR(k). However,    
//  the method for doing that extension assumes that a few error          
//  conditions are not present in the grammar. In fact, if the error      
//  conditions are present, not only is the grammar not LALR(k) for any k 
//  things are so bad that the following procedures will loop infinitely. 
//                                                                        
//  The conditions are:                                                   
//                                                                        
//   - There is a loop in the READS relation.  That is, there is a        
//     loop on GOTO's in the LR(0) automaton in which every symbol used   
//     in the GOTO is nullable.                                           
//                                                                        
//   - There is a nonterminal which can right-most produce itself.        
//                                                                        
//  Either of these conditions will cause an infinite loop when trying to 
//  extend lookaheads.                                                    
//

void LalrGenerator::infinite_loop_check()
{

    //
    //  check_reads_cycle                                       
    //  -----------------                                       
    //                                                          
    //  Check for a cycle in the READS relation. 
    //

    function<void(void)> check_reads_cycle = [&](void) -> void
    {

        set<vector<State*>> cycles;

        queue<vector<State*>> workpile;
        for (State* state: state_list)
        {
            vector<State*> path;
            path.push_back(state);
            workpile.push(path);
        }

        while (workpile.size() > 0)
        {

            vector<State*> path = workpile.front();
            workpile.pop();

            for (auto mp: path.back()->lr0_goto)
            {

                if (first_set[mp.first].find(gram.epsilon_symbol) == first_set[mp.first].end())
                {
                    continue;
                }

                if (mp.second == path.front())
                {
                    cycles.insert(path);
                }
                else
                {
                    vector<State*> new_path = path;
                    new_path.push_back(mp.second);
                    workpile.push(new_path);
                }

            }

        }

        //
        //  Format a nice message for the error handler. 
        //

        if (cycles.size() == 0)
        {
            return;
        }

        map<StateSet, vector<State*>> cycle_map;

        for (vector<State*> path: cycles)
        {
        
            StateSet state_set;
            for (State* p: path)
            {
                state_set.get().insert(p);
            }
           
            cycle_map[state_set] = path;

        }

        ostringstream short_ost;
        ostringstream long_ost;

        short_ost << "Cycle"
                  << ((cycle_map.size() > 1) ? "s" : "")
                  << " in the READS relation" << endl << endl;
                  
        long_ost << "Cycle"
                 << ((cycle_map.size() > 1) ? "s" : "")
                 << " in the READS relation" << endl << endl;
                  
        set<State*> state_set;
        for (auto mp: cycle_map)
        {

            short_ost << "    ";
            long_ost << "    ";

            for (size_t i = 0; i < mp.second.size(); i++)
            {

                state_set.insert(mp.second[i]);

                short_ost << mp.second[i]->state_name;
                long_ost << mp.second[i]->state_name;

                if (i < mp.second.size() - 1)
                {
                    short_ost << " -> ";
                    long_ost << " -> ";
                }

            }

            short_ost << endl;
            long_ost << endl;

        }
        
        short_ost << endl;
        long_ost << endl;

        for (State* state: state_list)
        {
            if (state_set.find(state) != state_set.end())
            {
                dump_state(state, long_ost, 4);
            }
        }

        errh.add_error(ErrorType::ErrorReadsCycle,
                       -1,
                       short_ost.str(),
                       long_ost.str());

    };

    //
    //  check_rm_produce                                     
    //  ----------------                                     
    //                                                       
    //  Check whether a symbol can rightmost-produce itself. 
    //

    function<void(void)> check_rm_produce = [&](void) -> void
    {

        //
        //  We're going to use a fixpoint algorithm again. First we make a 
        //  mapping from B to A where there is a rule A ::= Bd and d can   
        //  produce epsilon. Then we know that anything B can produce, A   
        //  can also produce.                                              
        //

        map<Symbol*, set<Symbol*>> propagate_map;
        for (Rule* rule: gram.rule_list)
        {

            if (rule->rhs.size() == 0)
            {
                continue;
            }

            if (rule_item_map[rule]->next->first_set.find(gram.epsilon_symbol) !=
                rule_item_map[rule]->next->first_set.end())
            {
                propagate_map[rule->rhs[0]].insert(rule->lhs);
            }

        }

        //
        //  We initialize all the produce lists. Anything in the range of 
        //  the propagate map can produce whatever is mapped to it.       
        //

        map<Symbol*, set<Symbol*>> produce;
        for (auto mp: propagate_map)
        {
            Symbol* left = mp.first;
            for (Symbol* right: mp.second)
            {
                produce[right].insert(left);
            }
        }

        //
        //  Propagate these sets until things stablilize. 
        //

        queue<Symbol*> workpile;
        for (auto mp: propagate_map)
        {
            workpile.push(mp.first);
        }

        while (workpile.size() > 0)
        {

            Symbol* left = workpile.front();
            workpile.pop();

            for (Symbol* right: propagate_map[left])
            {

                for (Symbol* symbol: produce[left])
                {
                    if (produce[right].find(symbol) == produce[right].end())
                    {
                        produce[right].insert(symbol);
                        workpile.push(right);
                    }
                }

            }
                
        }

        //
        //  If nothing can produce itself, we're home free. Otherwise we 
        //  report an error.
        //

        vector<Symbol*> bad_symbols;
        for (auto mp: produce)
        {
            if (mp.second.find(mp.first) != mp.second.end())
            {
                bad_symbols.push_back(mp.first);
            }
        }

        if (bad_symbols.size() == 0)
        {
            return;
        }

        vector<string> name_list;
        for (Symbol* s: bad_symbols)
        {
            name_list.push_back(s->symbol_name);
        }

        sort(name_list.begin(), name_list.end());

        ostringstream ost;
        ost << "The following "
            << ((name_list.size() > 1) ? "symbols" : "symbol")
            << " can produce "
            << ((name_list.size() > 1) ? "themselves" : "itself")
            << endl << endl;
                  
        ost << "    ";
        int width = 4;
        
        for (int i = 0; i < name_list.size(); i++)
        {

            if (width + gram.symbol_width > gram.line_width)
            {
                ost << endl << "    ";
                width = 4;
            }

            ost << setw(gram.symbol_width) << left << name_list[i]
                << setw(0);
            width += gram.symbol_width;

        }

        errh.add_error(ErrorType::ErrorSymbolSelfProduce,
                       -1,
                       ost.str());

    };

    //
    //  infinite_loop_check            
    //  -------------------            
    //                                 
    //  The function body begins here. 
    //

    if (gram.max_lookaheads < 2)
    {
        return;
    }

    check_reads_cycle();
    check_rm_produce();

}

//
//  extend_lookaheads                                                       
//  -----------------                                                       
//                                                                         
//  Fix up the parse actions adding lookahead states to resolve conflicts. 
//  At the end of this process we should have no unexpected conflicts. If  
//  we do we create an error message and if we don't we choose actions for 
//  all the expected conflicts.                                            
//

void LalrGenerator::extend_lookaheads()
{

    set<State*> conflict_states;
    set<StateSymbol> visited;

    //
    //  next_la                                                               
    //  -------                                                               
    //                                                                        
    //  We are given a state which can read a symbol and we return the set of 
    //  symbols that can come next.                                           
    //

    function<set<Symbol*>(StateStack&, Symbol*)> next_la =
        [&](StateStack& stack, Symbol* symbol) -> set<Symbol*>
    {

        State* state = stack.get().back();
        set<Symbol*> la;

        State* goto_state = state->lr0_goto[symbol];
        for (Item* item: goto_state->item_set.get())
        {
            for (Symbol* s: item->first_set)
            {
                la.insert(s);
            }
        }

        for (Item* item: state->item_set.get())
        {

            if (item->dot >= item->rule->rhs.size())
            {
                continue;
            }

            if (item->rule->rhs[item->dot] != symbol)
            {
                continue;
            }

            if (item->next->first_set.find(gram.epsilon_symbol) == item->next->first_set.end())
            {
                continue;
            }
       
            if (item->rule->lhs == gram.accept_symbol)
            {
                continue;
            }

            if (item->dot < stack.get().size())
            {

                StateStack next_stack = stack;
                if (item->dot > 0)
                {
                    next_stack.get().erase(next_stack.get().end() - item->dot,
                                           next_stack.get().end());
                }

                for (Symbol* s: next_la(next_stack, item->rule->lhs))
                {
                    la.insert(s);
                }

            }
            else
            {

                size_t distance = item->dot + 1 - stack.get().size();
                compute_lookback(stack.get()[0], distance);

                for (State* q: stack.get()[0]->lookback[distance])
                {   
                    compute_lhs_follow(q, item->rule->lhs);
                    for (Symbol* s: q->lhs_follow[item->rule->lhs])
                    {
                        la.insert(s);
                    }
                }

            }

        }

        if (la.find(gram.epsilon_symbol) != la.end())
        {
            la.erase(gram.epsilon_symbol);
        }

        return la;

    };

    //
    //  follow_sources                                                     
    //  --------------                                                     
    //                                                                     
    //  Follow sources returns a set of configurations which can follow a  
    //  given configuration after a transition on a symbol, some number of 
    //  reductions, and finally shift a given terminal. It recursively     
    //  looks back through the LR(0) automaton.                            
    //

    function<set<StateStack>(StateStack&, Symbol*, Symbol*)> follow_sources =
        [&](StateStack& stack, Symbol* symbol, Symbol* terminal) -> set<StateStack>
    {

        State* state = stack.get().back();

        //
        //  If we've already visited this state then return. 
        //

        if (stack.get().size() == 1)
        {

            if (visited.find(StateSymbol(state, symbol)) !=
                visited.end())
            {
                return set<StateStack>(); 
            }
            else
            {
                visited.insert(StateSymbol(state, symbol));
            }

        }

        set<StateStack> stack_set;

        //
        //  If the terminal is in the direct read set then include it in 
        //  the successor state.                                         
        //

        State* goto_state = state->lr0_goto[symbol];

        if (goto_state->lr0_goto.find(terminal) != goto_state->lr0_goto.end())
        {
            StateStack next_stack = stack;
            next_stack.get().push_back(goto_state);
            stack_set.insert(next_stack);
        }

        //
        //  Look for states and symbols in the reads relation and follow 
        //  their sources.                                               
        //

        for (auto mp: goto_state->lr0_goto)
        {

            if (first_set[mp.first].find(gram.epsilon_symbol) != first_set[mp.first].end())
            {

                StateStack next_stack = stack;
                next_stack.get().push_back(goto_state);
                for (auto s: follow_sources(next_stack, mp.first, terminal))
                {
                    stack_set.insert(s);
                }

            }

        }

        //
        //  Look for situations where we're looking at the last symbol in 
        //  a rule. When we find them we have to backtrack.               
        //

        for (Item* item: state->item_set.get())
        {

            if (item->dot != item->rule->rhs.size() - 1)
            {
                continue;
            }

            if (item->rule->rhs[item->dot] != symbol)
            {
                continue;
            }

            if (item->rule->lhs == gram.accept_symbol)
            {
                continue;
            }

            if (item->dot < stack.get().size())
            {

                StateStack next_stack = stack;
                if (item->dot > 0)
                {
                    next_stack.get().erase(next_stack.get().end() - item->dot,
                                           next_stack.get().end());
                }

                for (auto s: follow_sources(next_stack, item->rule->lhs, terminal))
                {
                    stack_set.insert(s);
                }

            }
            else
            {

                size_t distance = item->dot + 1 - stack.get().size();
                compute_lookback(stack.get()[0], distance);

                for (State* q: stack.get()[0]->lookback[distance])
                {
                    for (auto s: follow_sources(StateStack(q), item->rule->lhs, terminal))
                    {
                        stack_set.insert(s);
                    }
                }

            }

        }

        return stack_set;

    };

    //
    //  resolve_conflicts                                                      
    //  -----------------                                                      
    //                                                                         
    //  Add a lookahead state then check whether we have to keep extending the 
    //  lookahead string.                                                      
    //

    function<void(State*, Symbol*, map<ParseAction, set<StateStack>>, int)> resolve_conflicts =
        [&](State* state,
            Symbol* terminal,
            map<ParseAction, set<StateStack>> sources,
            int lookahead) -> void
    {

        //
        //  If we've exceeded the max lookahead we have an unresolvable 
        //  conflict.                                                   
        //

        if (lookahead > gram.max_lookaheads)
        {
            conflict_states.insert(state->lr0_state);
            return;
        }

        //
        //  Create a lookahead state. 
        //

        State* la_state = get_state();
        la_state->lookback_one.insert(state);
        la_state->lr0_state = state->lr0_state;
        state->la_goto_map[terminal] = la_state;
        la_state->la_symbol = terminal;

        //
        //  Find symbols which can be read in the lookahead state. 
        //

        for (auto mp: sources)
        {
            for (auto stack: mp.second)
            {
                for (Symbol* symbol: next_la(stack, terminal))
                {
                    la_state->action_multimap[symbol].insert(mp.first);
                }
            }
        }

        //
        //  If we've still got conflicts we extend the automaton again. 
        //

        for (auto mp: la_state->action_multimap)
        {

            if (mp.second.size() < 2)
            {
                continue;
            }

            map<ParseAction, set<StateStack>> new_sources;
            for (auto a: mp.second)
            {

                for (auto stack: sources[a])
                {

                    visited.clear();
                    for (auto s: follow_sources(stack, terminal, mp.first))
                    {
                        new_sources[a].insert(s);
                    }

                }

            }

            resolve_conflicts(la_state, mp.first, new_sources, lookahead + 1);

        }

    };
    
    //
    //  extend_lookaheads                                                
    //  -----------------                                                
    //                                                                   
    //  The function body begins here. Start by finding conflicts in the 
    //  LALR(1) automaton.                                               
    //

    for (size_t i = 0; i < state_list.size(); i++)
    {

        State* state = state_list[i];

        if (state->lr0_state != state)
        {
            continue;
        }

        for (auto mp: state->action_multimap)
        {

            if (mp.second.size() < 2)
            {
                continue;
            }

            map<ParseAction, set<StateStack>> sources;

            for (ParseAction action: mp.second)
            {

                if (action.action_type == ParseActionType::ActionShift)
                {
                    sources[action].insert(StateStack(state));
                }    
                else if (action.action_type == ParseActionType::ActionReduce)
                {

                    size_t distance = gram.rule_list[action.rule_num]->rhs.size();
                    Symbol* lhs = gram.rule_list[action.rule_num]->lhs;

                    compute_lookback(state, distance);
                    for (State* p: state->lookback[distance])
                    {

                        visited.clear();
                        for (auto s: follow_sources(StateStack(p), lhs, mp.first))
                        {
                            sources[action].insert(s);
                        }

                    }

                }

            }

            resolve_conflicts(state, mp.first, sources, 2);

        }

    }

    //
    //  If we still have conflicts and haven't predicted them in advance 
    //  then format an error message.                                    
    //

    if (conflict_states.size() > gram.expected_conflicts)
    {

        ostringstream short_ost;
        ostringstream long_ost;

        short_ost << "The following state"
                  << ((conflict_states.size() > 1) ? "s" : "")
                  << " had conflicts: ";
                  
        long_ost << "The following state"
                 << ((conflict_states.size() > 1) ? "s" : "")
                 << " had conflicts" << endl << endl;
                  
        short_ost << "    ";

        int count = conflict_states.size();
        for (State* s: conflict_states)
        {

            short_ost << s->state_name;
            count--;

            if (count > 1)
            {
                short_ost << ", ";
            }

        }

        short_ost << endl;

        for (State* state: state_list)
        {
            if (conflict_states.find(state) != conflict_states.end())
            {
                dump_state(state, long_ost, 2);
            }
        }

        errh.add_error(ErrorType::ErrorLalrConflict,
                       -1,
                       short_ost.str(),
                       long_ost.str());

        return;

    }

    //
    //  Up to this point we kept LaShift actions in a parallel map in case 
    //  our algorithms needed to access a conflict state. Now we replace   
    //  the conflicts we were able to resolve with the LaShift actions.    
    //

    for (State* state: state_list)
    {

        for (auto mp: state->la_goto_map)
        {

            ParseAction action;
            action.action_type = ParseActionType::ActionLaShift;
            action.goto_state = mp.second->num;

            state->action_multimap.erase(mp.first);
            state->action_multimap[mp.first].insert(action);

        }

    }

    //
    //  Break conflicts in the sequence shift, accept, reduce with lowest rule 
    //  number. This is kind of arbitrary. I hope it isn't used.               
    //

    for (State* state: conflict_states)
    {

        for (auto mp: gram.symbol_map)
        {

            if (state->action_multimap[mp.second].size() < 2)
            {
                continue;
            }

            ParseAction action;

            int64_t rule_num = gram.rule_list.size() + 1;
            for (ParseAction a: state->action_multimap[mp.second])
            {

                if (a.action_type == ParseActionType::ActionReduce &&    
                    a.rule_num < rule_num)
                {
                    action = a;
                    rule_num = a.rule_num;
                }


            }

            for (ParseAction a: state->action_multimap[mp.second])
            {

                if (a.action_type == ParseActionType::ActionAccept)
                {
                    action = a;
                }

            }

            for (ParseAction a: state->action_multimap[mp.second])
            {

                if (a.action_type == ParseActionType::ActionShift)
                {
                    action = a;
                }

            }

            state->action_multimap.erase(mp.second);
            state->action_multimap[mp.second].insert(action);

        }

    }

    //
    //  We're down to a unique action per symbol so copy the action to a 
    //  single-valued map.                                               
    //

    for (State* state: state_list)
    {

        for (auto mp: state->action_multimap)
        {
            state->action_map[mp.first] = *mp.second.begin();
        }

        state->action_multimap.clear();

    }

}

//
//  add_error_recovery                                                     
//  ------------------                                                     
//                                                                         
//  During error recovery we want to parse with a powerset automaton where 
//  each state represents a set of LR(0) states. When an error is          
//  discovered we enter the set of all LR(0) states. Then we parse by      
//  considering the union of all the actions in the underlying states. If  
//  they agree we perform the action. If they disagree we fall back to the 
//  set of states which could be entered after parsing the next token.     
//                                                                         
//  All that is computed statically, here. We create recovery states that  
//  represent sets of LR(0) states and augment reduce actions with those   
//  states.                                                                
//

void LalrGenerator::add_error_recovery()
{

    //
    //  find_after_shift                                                
    //  ----------------                                                
    //                                                                  
    //  In recovery mode reduce actions might fail due to insufficient  
    //  stack elements. In that case we want to restart in some set of  
    //  states that *might* have been correct. Here we compute a set of 
    //  states which we might go into after shifting the current token. 
    //

    function<void(void)> find_after_shift = [&](void) -> void
    {

        //
        //  We're going to use a fixpoint algorithm. In the first phase we 
        //  compute a `propagate_map' that describes how lookaheads flow   
        //  around the automaton.                                          
        //

        map<StateSymbol, set<StateSymbol>> propagate_map;

        for (State* state: state_list)
        {

            for (auto mp: state->action_map)
            {
        
                Symbol* symbol = mp.first;
                ParseAction action = mp.second;

                switch (action.action_type)
                {

                    case ParseActionType::ActionLaShift:
                    {

                        if (state->lr0_state != state)
                        {
                            propagate_map[StateSymbol(state, symbol)].insert(
                                StateSymbol(*state->lookback_one.begin(), state->la_symbol));
                        }

                        break;

                    }

                    case ParseActionType::ActionShift:
                    {

                        State* goto_state = state_list[action.goto_state];
                        state->after_shift[symbol].insert(goto_state);

                        if (state->lr0_state != state)
                        {
                            propagate_map[StateSymbol(state, symbol)].insert(
                                StateSymbol(*state->lookback_one.begin(), state->la_symbol));
                        }

                        break;

                    }

                    case ParseActionType::ActionReduce:
                    {

                        Rule* rule = gram.rule_list[action.rule_num];

                        Symbol* la_symbol = symbol;
                        for (State* s = state;
                             s != s->lr0_state;
                             s = *s->lookback_one.begin())
                        {
                            la_symbol = s->la_symbol;
                        }

                        compute_lookback(state->lr0_state, rule->rhs.size());
                        for (State* s: state->lr0_state->lookback[rule->rhs.size()])
                        {

                            ParseAction action = s->action_map[rule->lhs];
                            State* goto_state = state_list[action.goto_state];

                            propagate_map[StateSymbol(goto_state, la_symbol)].insert(
                                StateSymbol(state, symbol));

                            if (state->lr0_state != state)
                            {
                                propagate_map[StateSymbol(state, symbol)].insert(
                                    StateSymbol(*state->lookback_one.begin(), state->la_symbol));
                            }

                        }

                        break;

                    }

                }

            }

        }

        //
        //  Now we propagate states until the sets stabilize. 
        //

        set<StateSymbol> workpile;

        for (auto mp: propagate_map)
        {
            workpile.insert(mp.first);
        }

        while (workpile.size() > 0)
        {

            StateSymbol source = *workpile.begin();
            workpile.erase(source);

            for (StateSymbol target: propagate_map[source])
            {

                for (State* state: source.state->after_shift[source.symbol])
                {

                    if (target.state->after_shift[target.symbol].find(state) ==
                        target.state->after_shift[target.symbol].end())
                    {
                        target.state->after_shift[target.symbol].insert(state);
                        workpile.insert(target);
                    }

                }

            }

        }
                    
    };

    //
    //  add_fallback_states                                              
    //  -------------------                                              
    //                                                                   
    //  Add a powerset state for restarting and powerset states for each 
    //  of the reduce actions.                                           
    //

    function<void(void)> add_fallback_states = [&](void) -> void
    {

        //
        //  If a powerset state resolves to a single base state we can 
        //  move back to the normal automaton.                         
        //

        for (State* state: state_list)
        {
            state->base_states.insert(state);
            StateSet state_set(state->base_states);
            state_set_map[state_set] = state;
        }

        //
        //  Create a restart state which is the set of all LR(0) states. 
        //

        restart_state = get_state();

        for (State* state: state_list)
        {
            if (state->lr0_state == state)
            {
                restart_state->base_states.insert(state);
            }
        }

        StateSet state_set(restart_state->base_states);
        state_set_map[state_set] = restart_state;

        //
        //  For each reduce action make a fallback state of the set of 
        //  states which could be entered after shifting the symbol.   
        //

        for (State* state: state_list)
        {
            
            vector<Symbol*> domain;
            for (auto mp: state->action_map)
            {
                domain.push_back(mp.first);
            }

            for (Symbol* symbol: domain)
            {
          
                ParseAction action = state->action_map[symbol];

                if (action.action_type != ParseActionType::ActionReduce)
                {
                    continue;
                }

                StateSet state_set(state->after_shift[symbol]);
                State* fallback_state = nullptr;

                if (symbol == gram.eof_symbol)
                {
                    fallback_state = restart_state;
                }
                else if (state_set_map.find(state_set) != state_set_map.end())
                {
                    fallback_state = state_set_map[state_set];
                }
                else
                {
                    fallback_state = get_state();
                    fallback_state->base_states = state_set.get();
                    state_set_map[state_set] = fallback_state;
                }

                action.fallback_state = fallback_state->num;
                state->action_map[symbol] = action;

            }

        }

    };

    //
    //  expand_powerset_states                                           
    //  ----------------------                                           
    //                                                                   
    //  For each powerset set create an action map. We have to merge the 
    //  actions for each symbol and synthesize a new action.             
    //

    function<void(void)> expand_powerset_states = [&](void) -> void
    {

        //
        //  Expand the powerset states by merging action tables, possibly 
        //  adding new states. At some point we will have processed all   
        //  the states.                                                   
        //

        for (int state_num = restart_state->num;
             state_num < state_list.size();
             state_num++)
        {

            State* state = state_list[state_num];

            for (auto mp: gram.symbol_map)
            {

                Symbol* symbol = mp.second;

                //
                //  Separate the actions by type. 
                //

                set<State*> fallback_set;
                set<ParseAction> shift_set;
                set<ParseAction> reduce_set;
                bool accept_found = false;

                for (State* base_state: state->base_states)
                {
                
                    if (base_state->action_map.find(symbol) == base_state->action_map.end())
                    {
                        continue;
                    }

                    ParseAction action = base_state->action_map[symbol];
                    
                    switch (action.action_type)
                    {

                        case ParseActionType::ActionLaShift:
                        {
                            
                            for (State* s: base_state->after_shift[symbol])
                            {
                                fallback_set.insert(s);
                            }

                            break;

                        }

                        case ParseActionType::ActionShift:
                        {
                             
                            if (base_state->lr0_state == base_state)
                            {
                                shift_set.insert(action);
                            }
                            else
                            {
                                fallback_set.insert(state_list[action.goto_state]);
                            }

                            break;

                        }

                        case ParseActionType::ActionReduce:
                        {

                            if (base_state->lr0_state == base_state)
                            {
                                reduce_set.insert(action);
                            }
                            else
                            {
                                for (State* s: base_state->after_shift[symbol])
                                {
                                    fallback_set.insert(s);
                                }
                            }

                            break;

                        }

                        case ParseActionType::ActionGoto:
                        {
                            shift_set.insert(action);
                            break;
                        }

                        case ParseActionType::ActionAccept:
                        {           
                            accept_found = true;
                            break;
                        }

                    }

                }

                //
                //  Now we have to resolve the sets of actions into a single 
                //  action.                                                  
                //

                if (accept_found)
                {
                    ParseAction action;
                    action.action_type = ParseActionType::ActionAccept;
                    state->action_map[symbol] = action;
                    continue;
                }

                if (state == restart_state)
                {
                    reduce_set.clear();
                }

                int total_size = shift_set.size() + reduce_set.size() + fallback_set.size();
                
                if (total_size == 0)
                {
                    continue;
                }

                //
                //  If all the actions are shift we can merge into a shift. 
                //

                if (shift_set.size() == total_size)
                {

                    StateSet state_set;
                    for (ParseAction action: shift_set)
                    {
                        state_set.get().insert(state_list[action.goto_state]);
                    }

                    State* goto_state = nullptr;
                    if (state_set_map.find(state_set) == state_set_map.end())
                    {
                        goto_state = get_state();
                        goto_state->base_states = state_set.get();
                        state_set_map[state_set] = goto_state;
                    }
                    else
                    {
                        goto_state = state_set_map[state_set];
                    }

                    ParseAction action;
       
                    if (symbol->is_terminal)
                    {
                        action.action_type = ParseActionType::ActionShift;
                    }
                    else
                    {
                        action.action_type = ParseActionType::ActionGoto;
                    }

                    action.goto_state = goto_state->num;
                    state->action_map[symbol] = action;

                    continue;
               
                }

                //
                //  If all the actions are reduce and by the same rule we can 
                //  merge them.                                               
                //

                if (reduce_set.size() == total_size)
                {

                    set<int64_t> rule_num_set;
                    StateSet state_set;

                    for (ParseAction action: reduce_set)
                    {
                        rule_num_set.insert(action.rule_num);
                        for (State* s: state_list[action.fallback_state]->base_states)
                        {
                            state_set.get().insert(s);
                        }
                    }

                    if (rule_num_set.size() == 1)
                    {

                        State* fallback_state = nullptr;

                        if (state_set_map.find(state_set) != state_set_map.end())
                        {
                            fallback_state = state_set_map[state_set];
                        }
                        else
                        {
                            fallback_state = get_state();
                            fallback_state->base_states = state_set.get();
                            state_set_map[state_set] = fallback_state;
                        }
                    
                        ParseAction action;
                        action.action_type = ParseActionType::ActionReduce;
                        action.rule_num = *rule_num_set.begin();
                        action.fallback_state = fallback_state->num;
                        state->action_map[symbol] = action;

                        continue;

                    }

                }

                //
                //  We have an incompatible mixture. Merge them into a 
                //  restart.                                           
                //

                for (ParseAction action: shift_set)
                {
                    fallback_set.insert(state_list[action.goto_state]);
                }

                for (ParseAction action: reduce_set)
                {
                    for (State* s: state_list[action.fallback_state]->base_states)
                    {
                        fallback_set.insert(s);
                    }
                }
            
                StateSet state_set(fallback_set);

                if (state_set_map.find(state_set) == state_set_map.end())
                {
                    State* state = get_state();
                    state->base_states = state_set.get();
                    state_set_map[state_set] = state;
                }

                ParseAction action;
                action.action_type = ParseActionType::ActionRestart;
                action.goto_state = state_set_map[state_set]->num;
                state->action_map[symbol] = action;

            }
                
        }

    };

    //
    //  add_error_recovery             
    //  ------------------             
    //                                 
    //  The main function starts here. 
    //

    find_after_shift();
    add_fallback_states();
    expand_powerset_states();

}

//
//  save_parser_data                                                       
//  ----------------                                                       
//                                                                         
//  Take the graph form of parsing tables and flatten them into arrays.    
//  The primary technique used here is attributed to Zeigler but described 
//  pretty well by Tarjan and Yao.                                         
//

void LalrGenerator::save_parser_data()
{

    //
    //  renumber_symbols                                                   
    //  ----------------                                                   
    //                                                                     
    //  Assign each symbol a number to be used in the action table. We get 
    //  better compression if the most-used symbols get lower symbol       
    //  numbers.                                                           
    //

    function<void(void)> renumber_symbols = [&](void) -> void
    {

        map<Symbol*, int> action_count;
        for (State* state: state_list)
        {

            for (auto mp: state->action_map)
            {

                if (action_count.find(mp.first) == action_count.end())
                {
                    action_count[mp.first] = 0;
                }

                action_count[mp.first] += 1;

            }

        }
       
        vector<Symbol*> symbol_list;
        for (auto mp: gram.symbol_map)
        {
            symbol_list.push_back(mp.second);
        }

        sort(symbol_list.begin(), symbol_list.end(),
             [&](Symbol* left, Symbol* right) -> bool
             {

                 if (action_count.find(left) != action_count.end() &&
                     action_count.find(right) == action_count.end())
                 {
                     return true;
                 }

                 if (action_count.find(left) == action_count.end() &&
                     action_count.find(right) != action_count.end())
                 {
                     return false;
                 }

                 if (action_count.find(left) == action_count.end() ||
                     action_count.find(right) == action_count.end())
                 {
                     return left < right;
                 }

                 if (left->is_terminal && !right->is_terminal)
                 {
                     return true;
                 }

                 if (!left->is_terminal && right->is_terminal)
                 {
                     return false;
                 }

                 if (left->is_nonterminal && !right->is_nonterminal)
                 {
                     return true;
                 }

                 if (!left->is_nonterminal && right->is_nonterminal)
                 {
                     return false;
                 }

                 if (!left->is_terminal && !left->is_nonterminal)
                 {
                     return left < right;
                 }

                 return action_count[left] > action_count[right];

             });

        for (int i = 0; i < symbol_list.size(); i++)
        {
            symbol_list[i]->symbol_num = i;
        }

    };

    //
    //  allocate_bits                              
    //  -------------                              
    //                                             
    //  Allocate a field into a set of data words. 
    //

    auto allocate_bits =
        [&](int* bits_used, int max_words, int bits, int& offset, int64_t& mask, int& shift) -> void
    {

        const int bits_per_word = sizeof(int64_t) * CHAR_BIT;

        for (offset = 0;
             offset < max_words && bits_used[offset] + bits > bits_per_word;
             offset++);

        if (offset >= max_words)
        {
            errh.add_error(ErrorType::ErrorWordOverflow,
                           -1,
                           "Grammar too complex to encode.");
            offset = 0;
        }

        mask = (1 << bits) - 1;
        shift = bits_used[offset];
        bits_used[offset] += bits;

    };

    //
    //  save_parser_data                                                  
    //  ----------------                                                  
    //                                                                    
    //  The function body begins here. Renumber the symbols in descending 
    //  order of use.                                                     
    //

    renumber_symbols();

    //
    //  See how many bits we need for each field. 
    //

    int64_t symbol_num_bits;
    for (symbol_num_bits = 1;
         1 << symbol_num_bits <= gram.symbol_map.size();
         symbol_num_bits++);

    int64_t action_type_bits;
    for (action_type_bits = 1;
         1 << action_type_bits <= ParseActionType::ActionError;
         action_type_bits++);

    int64_t state_num_bits;
    for (state_num_bits = 1;
         1 << state_num_bits <= state_list.size();
         state_num_bits++);

    int64_t rule_num_bits;
    for (rule_num_bits = 1;
         1 << rule_num_bits <= gram.rule_list.size();
         rule_num_bits++);

    //
    //  Allocate the values we need into bit fields. 
    //

    prsd.start_state = start_state->num;
    prsd.restart_state = restart_state->num;
    prsd.num_offsets = 0;
    int bits_used[8];

    for (int i = 0; i < LENGTH(bits_used); i++)
    {
        bits_used[i] = 0;
    }
    
    allocate_bits(bits_used,
                  LENGTH(bits_used),
                  symbol_num_bits,
                  prsd.symbol_num_offset,
                  prsd.symbol_num_mask,
                  prsd.symbol_num_shift);

    allocate_bits(bits_used,
                  LENGTH(bits_used),
                  action_type_bits,
                  prsd.action_type_offset,
                  prsd.action_type_mask,
                  prsd.action_type_shift);

    allocate_bits(bits_used,
                  LENGTH(bits_used),
                  rule_num_bits,
                  prsd.rule_num_offset,
                  prsd.rule_num_mask,
                  prsd.rule_num_shift);

    allocate_bits(bits_used,
                  LENGTH(bits_used),
                  state_num_bits,
                  prsd.state_num_offset,
                  prsd.state_num_mask,
                  prsd.state_num_shift);

    allocate_bits(bits_used,
                  LENGTH(bits_used),
                  state_num_bits,
                  prsd.fallback_num_offset,
                  prsd.fallback_num_mask,
                  prsd.fallback_num_shift);

    for (prsd.num_offsets = 0;
         prsd.num_offsets < LENGTH(bits_used) && bits_used[prsd.num_offsets] > 0;
         prsd.num_offsets++);               

    //
    //  We would like to place states in descending order by the number of 
    //  actions.                                                           
    //

    vector<State*> desc_state_list;
    for (State* state: state_list)
    {
        desc_state_list.push_back(state);
    }

    sort(desc_state_list.begin(), desc_state_list.end(),
         [](State* left, State* right) -> bool
         {
             return left->action_map.size() > right->action_map.size();
         });

    set<int64_t> used_indices;
    vector<int64_t> checked_data;
    vector<int64_t> checked_index(desc_state_list.size(), -1);

    for (State* state: desc_state_list)
    {

        vector<Symbol*> symbol_list;
        for (auto mp: state->action_map)
        {
            symbol_list.push_back(mp.first);
        }

        sort(symbol_list.begin(), symbol_list.end(),
             [](Symbol* left, Symbol* right) -> bool
             {
                 return left->symbol_num < right->symbol_num;
             });  

        //
        //  Find an unused offset into checked information. 
        //

        for (int64_t i = 0; ; i+= prsd.num_offsets)
        {

            if (used_indices.find(i) != used_indices.end())
            {
                continue;
            }

            bool found = true;

            for (Symbol* symbol: symbol_list)
            {

                if (i + symbol->symbol_num * prsd.num_offsets >= checked_data.size())
                {
                    break;
                }

                if (checked_data[i + symbol->symbol_num * prsd.num_offsets] >= 0)
                {
                    found = false;
                    break;
                }

            }

            if (found)
            {
                checked_index[state->num] = i;
                used_indices.insert(i);
                break;
            }

        }

        //
        //  Extend the checked information to handle this row. 
        //

        while (checked_data.size() <= checked_index[state->num] +
                                      gram.symbol_map.size() * prsd.num_offsets)
        {
            checked_data.push_back(-1);
        }

        //
        //  Encode each action into checked information. 
        //

        for (Symbol* symbol: symbol_list)
        {

            ParseAction action = state->action_map[symbol];
          
            for (int i = 0; i < prsd.num_offsets; i++)
            {
                checked_data[checked_index[state->num] +
                             symbol->symbol_num * prsd.num_offsets + i] = 0;
            }

            checked_data[checked_index[state->num] +
                         symbol->symbol_num * prsd.num_offsets +
                         prsd.symbol_num_offset] |=
                             symbol->symbol_num << prsd.symbol_num_shift;
                            
            checked_data[checked_index[state->num] +
                         symbol->symbol_num * prsd.num_offsets +
                         prsd.action_type_offset] |=
                             action.action_type << prsd.action_type_shift;
                            
            if (action.rule_num >= 0)
            {
                checked_data[checked_index[state->num] +
                             symbol->symbol_num * prsd.num_offsets +
                             prsd.rule_num_offset] |=
                                 action.rule_num << prsd.rule_num_shift;
            }
                            
            if (action.goto_state >= 0)
            {
                checked_data[checked_index[state->num] +
                             symbol->symbol_num * prsd.num_offsets +
                             prsd.state_num_offset] |=
                                 action.goto_state << prsd.state_num_shift;
            }

            if (action.fallback_state >= 0)
            {
                checked_data[checked_index[state->num] +
                             symbol->symbol_num * prsd.num_offsets +
                             prsd.fallback_num_offset] |=
                                 action.fallback_state << prsd.fallback_num_shift;
            }

        }

    }

    prsd.checked_data_count = checked_data.size();
    prsd.checked_data = new int64_t[checked_data.size()];
    memcpy(static_cast<void *>(prsd.checked_data),
           static_cast<void *>(checked_data.data()),
           checked_data.size() * sizeof(int64_t));

    prsd.checked_index_count = checked_index.size();
    prsd.checked_index = new int64_t[checked_index.size()];
    memcpy(static_cast<void *>(prsd.checked_index),
           static_cast<void *>(checked_index.data()),
           checked_index.size() * sizeof(int64_t));

}

//
//  Debugging facilities                                                  
//  --------------------                                                  
//                                                                        
//  I find parser generation pretty challenging. The only way I can get   
//  through it is to print out a lot of intermediate information. I'm     
//  going to pull those routines here to avoid cluttering the rest of the 
//  logic. I'm fine with doing extra work in these routines to get a      
//  nicer log.                                                            
//

void LalrGenerator::dump_first_sets(ostream& os, int indent)
{

    prsi.log_heading("First Sets: " + prsi.elapsed_time_string(), os, indent);

    vector<Symbol*> symbol_list;
    for (auto mp: gram.symbol_map)
    {
        symbol_list.push_back(mp.second);
    }

    sort(symbol_list.begin(), symbol_list.end(),
         [] (Symbol* left, Symbol* right) { return left->symbol_name < right->symbol_name; });
    
    for (Symbol* symbol: symbol_list)
    {

        if (!symbol->is_terminal && !symbol->is_nonterminal)
        {
            continue;
        }

        vector<string> name_list;
        for (Symbol* s: first_set[symbol])
        {
            name_list.push_back(s->symbol_name);
        }

        sort(name_list.begin(), name_list.end());

        os << setw(indent) << setfill(' ') << "" << setw(0) 
           << "  " << setw(gram.symbol_width) << left << symbol->symbol_name;

        int width = gram.symbol_width + 2;
        for (string name: name_list)
        {

            if (width + name.length() > gram.line_width - indent)
            {

                os << setw(indent) << setfill(' ') << "" << setw(0) 
                   << endl << setw(gram.symbol_width + 2) << " ";

                width = gram.symbol_width + 2;

            }

            os << name << " ";
            width = width + name.length() + 1;

        }

        os << endl;

    }

}

//
//  dump_automaton                                  
//  --------------                                  
//                                                  
//  Dump the LR(0) automaton and associated tables. 
//

void LalrGenerator::dump_automaton(const string& title, ostream& os, int indent) const
{

    if (title.length() == 0)
    {
        prsi.log_heading("Automaton: " + prsi.elapsed_time_string(), os, indent);
    }
    else
    {
        prsi.log_heading(title + ": " + prsi.elapsed_time_string(), os, indent);
    }

    for (State* state: state_list)
    {
        dump_state(state, os, indent);
    }

}

//
//  dump_state
//  ----------
//                                                  
//  Dump the LR(0) automaton and associated tables. 
//

void LalrGenerator::dump_state(State* state, ostream& os, int indent) const
{

    //
    //  dump_item                               
    //  ---------                               
    //                                          
    //  Dump an LR(0) item.
    //

    function<void(Item*)> dump_item = [&](Item* item) -> void
    {

        Rule* rule = item->rule;
        os << setw(indent) << setfill(' ') << "" << setw(0) 
           << "  " << setw(gram.symbol_width) << left << rule->lhs->symbol_name
           << setw(0) << "::= ";

        int width = gram.symbol_width + 6;

        for (int i = 0; i <= rule->rhs.size(); i++)
        {

            if (i < rule->rhs.size() &&
                width + rule->rhs[i]->symbol_name.length() > gram.line_width - indent)
            {

                os << setw(indent) << setfill(' ') << "" << setw(0) 
                   << endl << setw(gram.symbol_width + 6) << " ";

                width = gram.symbol_width + 6;

            }

            if (item->dot == i)
            {
                os << ". ";
                width += 2;
            }

            if (i < rule->rhs.size())
            {
                os << rule->rhs[i]->symbol_name << " ";
                width = width + rule->rhs[i]->symbol_name.length() + 1;
            }

        }

        os << endl;

    };

    //
    //  dump_lookaheads                               
    //  ---------------                               
    //                                                
    //  Dump the lookaheads associated with the item. 
    //

    function<void(set<Symbol*>)> dump_lookaheads = 
        [&](set<Symbol*> lookaheads) -> void
    {

        if (lookaheads.size() == 0)
        {
            return;
        }

        os << setw(indent) << setfill(' ') << "" << setw(0)
           << setw(gram.symbol_width + 2) << ""
           << setw(0) << "/   ";

        int width = indent + gram.symbol_width + 6;

        vector<string> name_list;
        for (Symbol* symbol: lookaheads)
        {
            name_list.push_back(symbol->symbol_name);
        }

        sort(name_list.begin(), name_list.end());

        for (string name: name_list)
        {

            if (width + name.length() > gram.line_width - indent)
            {

                os << endl << setw(indent) << setfill(' ') << "" << setw(0) 
                   << setw(gram.symbol_width + 6) << "";

                width = indent + gram.symbol_width + 6;

            }

            os << name << " ";
            width = width + name.length() + 1;

        }

        os << endl;

    };

    //
    //  dump_state                                                       
    //  ----------                                                       
    //                                                                   
    //  Dump a single state on the console. This is the workhorse of the 
    //  state dumping mechanism.                                         
    //

    os << setw(indent) << setfill(' ') << "" << setw(0)
       << "State " << state->state_name << endl 
       << setw(indent) << setfill(' ') << "" << setw(0) 
       << setw(gram.line_width - indent) << setfill('-') << ""
       << setw(0) << setfill(' ')
       << endl;
    
    //
    //  Dump the incoming transitions. 
    //

    if (state->lookback_one.size() > 0)
    {

        size_t to_print = state->lookback_one.size();
        string label = (to_print > 1) ? "Incoming transitions:" : "Incoming transition:";

        os << setw(indent) << setfill(' ') << "" << setw(0)
           << label << " ";

        vector<State*> state_list;
        for (State* s: state->lookback_one)
        {
            state_list.push_back(s);
        }

        sort(state_list.begin(), state_list.end(),
             [] (State* left, State* right) { return left->num < right->num; });
        
        int width = label.length() + 1;
        for (State* s: state_list)
        {

            if (width + s->state_name.length() + 2 > gram.line_width - indent)
            {
                os << endl << setw(indent + label.length() + 1) << setfill(' ') << "" << setw(0);
                width = label.length() + 1;
            }

            os << s->state_name;
            width += s->state_name.length();
            to_print--;

            if (to_print > 0)
            {
                os << ", ";
                width += 2;
            }

        }

        os << endl << endl;

    }
    
    //
    //  If this is a Lookahead state dump out the lookahead symbol. 
    //

    if (state->lr0_state != nullptr && state != state->lr0_state)
    {

        function<void(State*)> dump_predecessor = [&](State* state) -> void
        {

            if (state == state->lr0_state)
            {
                return;
            }
      
            dump_predecessor(*state->lookback_one.begin());

            os << state->la_symbol->symbol_name << " ";

        };

        os << setw(indent) << setfill(' ') << "" << setw(0)
           << "Lookahead: ";

        dump_predecessor(state);

        os << endl << endl;

    }

    //
    //  If this is a powerset state then dump out the base states. 
    //

    if (state->base_states.size() > 1 ||
        (state->base_states.size() == 1 &&
         state != *state->base_states.begin()))
    {

        size_t to_print = state->base_states.size();
        string label = (to_print > 1) ? "Base states:" : "Base state:";

        os << setw(indent) << setfill(' ') << "" << setw(0)
           << label << " ";

        vector<State*> state_list;
        for (State* s: state->base_states)
        {
            state_list.push_back(s);
        }

        sort(state_list.begin(), state_list.end(),
             [] (State* left, State* right) { return left->num < right->num; });
        
        int width = label.length() + 1;
        for (State* s: state_list)
        {

            if (width + s->state_name.length() + 2 > gram.line_width - indent)
            {
                os << endl << setw(indent + label.length() + 1) << setfill(' ') << "" << setw(0);
                width = label.length() + 1;
            }

            os << s->state_name;
            width += s->state_name.length();
            to_print--;

            if (to_print > 0)
            {
                os << ", ";
                width += 2;
            }

        }

        os << endl << endl;

    }

    //
    //  Dump the set of LR(0) items. 
    //

    if (state->item_set.get().size() > 0)
    {

        os << setw(indent) << setfill(' ') << "" << setw(0)
           << "Item set:" << endl;

        vector<Item*> item_list;
        for (Item* s: state->item_set.get())
        {
            item_list.push_back(s);
        }

        sort(item_list.begin(), item_list.end(),
             [] (Item* left, Item* right) { return left->num < right->num; });
        
        for (Item* item: item_list)
        {
            dump_item(item);
            dump_lookaheads(state->lookaheads[item]);
        }

        os << endl;

    }

    //
    //  Dump the parse actions. 
    //

    if (state->action_multimap.size() > 0 ||
        state->action_map.size())
    {

        os << setw(indent) << setfill(' ') << "" << setw(0)
           << "Actions:" << endl;

        vector<Symbol*> symbol_list;
        for (auto mp: state->action_multimap)
        {
            symbol_list.push_back(mp.first);
        }

        for (auto mp: state->action_map)
        {
            symbol_list.push_back(mp.first);
        }

        sort(symbol_list.begin(), symbol_list.end(),
             [] (Symbol* left, Symbol* right) { return left->symbol_name < right->symbol_name; });
        
        for (Symbol* symbol: symbol_list)
        {

            set<ParseAction> action_set;
            for (ParseAction action: state->action_multimap[symbol])
            {
                action_set.insert(action);
            }
            
            if (state->action_map.find(symbol) != state->action_map.end())
            {
                action_set.insert(state->action_map[symbol]);
            }

            for (ParseAction action: action_set)
            {

                os << setw(indent) << setfill(' ') << "" << setw(0);
                os << "  " << setw(gram.symbol_width) << left << symbol->symbol_name;

                switch (action.action_type)
                {

                    case ParseActionType::ActionLaShift:
                    {
                        os << "LaShift: " << state_list[action.goto_state]->state_name << endl;
                        break;
                    }

                    case ParseActionType::ActionShift:
                    {
                        os << "Shift: " << state_list[action.goto_state]->state_name << endl;
                        break;
                    }

                    case ParseActionType::ActionReduce:
                    {

                        Rule *rule = gram.rule_list[action.rule_num];
                        os << "Reduce: " << rule->lhs->symbol_name << " " << "::=";

                        if (rule->rhs.size() == 0)
                        {
                            os << " " << gram.epsilon_symbol->symbol_name;
                        }
                        else
                        {

                            int prefix_width = gram.symbol_width + 2 +
                                               string("Reduce:  ::=").length() +
                                               rule->lhs->symbol_name.length();
                            int width = prefix_width;

                            for (int i = 0; i < rule->rhs.size(); i++)
                            {

                                if (width + rule->rhs[i]->symbol_name.length() + 1 > gram.line_width - indent)
                                {
                                    os << endl << setw(prefix_width) << setfill(' ') << "" << setw(0);
                                    width = prefix_width;
                                }

                                os << " " << rule->rhs[i]->symbol_name;
                                width = width + rule->rhs[i]->symbol_name.length() + 1;

                            }

                        }

                        os << endl;

                        break;

                    }

                    case ParseActionType::ActionGoto:
                    {
                        os << "Goto: " << state_list[action.goto_state]->state_name << endl;
                        break;
                    }

                    case ParseActionType::ActionRestart:
                    {
                        os << "Restart: " << state_list[action.goto_state]->state_name << endl;
                        break;
                    }

                    case ParseActionType::ActionAccept:
                    {
                        os << "Accept" << endl;
                        break;
                    }

                    case ParseActionType::ActionError:
                    {
                        os << "Error" << endl;
                        break;
                    }

                }

                if (action.fallback_state >= 0)
                {
                    os << setw(indent + gram.symbol_width + 2) << setfill(' ') 
                       << "" << setw(0)
                       << "Fallback: " << action.fallback_state << endl;
                }

            }

        }

        os << endl;

    }

    os << endl;
        
}

} // namespace hoshi

