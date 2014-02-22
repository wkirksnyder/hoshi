#line 82 "u:\\hoshi\\raw\\Editor.cpp"
//
//  Editor                                                                 
//  ------                                                                 
//                                                                         
//  Perform a light editing and fix-up phase following extraction of the   
//  grammar. The goal here is to detect and remove nonsense like unused    
//  symbols and rules. For more serious stuff like a necessary symbol with 
//  no definition we stop the process. If we make it through this step the 
//  grammar should be clean enough to try building the parsing automaton.  
//

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include "ErrorHandler.H"
#include "ParserImpl.H"
#include "Grammar.H"
#include "Editor.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;

//
//  generate
//  --------                                                             
//                                                                   
//  An external entry point. Call support functions for each of the 
//  conditions we are checking.                                      
//

void Editor::generate()
{

    //
    //  Perform the edits. 
    //

    find_unused_terminals();
    find_undefined_nonterminals();
    find_unused_nonterminals();
    find_useless_nonterminals();
    find_useless_rules();

    //
    //  Clean the useless symbols and rules out of the grammar. 
    //

    for (Symbol* symbol: useless_symbols)
    {
        gram.delete_symbol(symbol);
    }

    for (Rule* rule: useless_rules)
    {
        gram.delete_rule(rule);
    }

}

//
//  find_unused_terminals                                                  
//  ---------------------                                                  
//                                                                         
//  An unused terminal is probably an incomplete grammar. The user has     
//  entered the definition of a terminal but hasn't entered the rules that 
//  use it yet. We're going to call that a warning situation.              
//

void Editor::find_unused_terminals()
{

    //
    //  Create a set of all the terminals. 
    //

    set<Symbol*> unused_terminals;
    for (auto mp: gram.symbol_map) 
    {
    
        Symbol* symbol = mp.second;
        if (symbol->is_terminal)
        {
            unused_terminals.insert(symbol);
        }

    }

    //
    //  Remove the terminals that are used in a rule and the special 
    //  symbols.                                                     
    //

    for (Rule* rule: gram.rule_list)
    {

        for (Symbol* rhs_term: rule->rhs)
        {
            unused_terminals.erase(rhs_term);
        }

    }

    unused_terminals.erase(gram.epsilon_symbol);
    unused_terminals.erase(gram.error_symbol);
    unused_terminals.erase(gram.eof_symbol);

    //
    //  If there is anything left we have unused terminals. 
    //

    if (unused_terminals.size() > 0)
    {

        ostringstream ost;

        ost << "The following terminal"
            << ((unused_terminals.size() > 1) ? "s are " : " is ")
            << "unused:" << endl << endl;

        vector<string> name_list;
        for (Symbol* symbol: unused_terminals)
        {
            name_list.push_back(symbol->symbol_name);
        }

        sort(name_list.begin(), name_list.end());

        int column_count = gram.line_width / gram.symbol_width;
        int column_num = column_count;
        for (string& name: name_list)
        {

            if (column_num >= column_count)
            {
                ost << endl << "  ";
                column_num = 0;
            }

            ost << setw(gram.symbol_width) << left << name;
            column_num++;

        }
        
        errh.add_error(ErrorType::ErrorUnusedTerm, -1, ost.str());

        for (Symbol* symbol: unused_terminals)
        {
            useless_symbols.insert(symbol);
        }

    }

}

//
//  find_undefined_nonterminals                                            
//  ---------------------------                                            
//                                                                         
//  An undefined nonterminal is also a sign of an incomplete grammar, but  
//  it's more serious. It means a symbol is used in the right hand side of 
//  a rule but isn't defined anywhere. We call that an error.              
//

void Editor::find_undefined_nonterminals()
{

    //
    //  Create a set of all the nonterminals. 
    //

    set<Symbol*> undefined_nonterminals;
    for (auto mp: gram.symbol_map) 
    {
    
        Symbol* symbol = mp.second;
        if (symbol->is_nonterminal)
        {
            undefined_nonterminals.insert(symbol);
        }

    }

    //
    //  Remove the nonterminals that are the lhs of a rule. 
    //

    for (Rule* rule: gram.rule_list)
    {
        undefined_nonterminals.erase(rule->lhs);
    }

    //
    //  If there is anything left we have undefined nonterminals. 
    //

    if (undefined_nonterminals.size() > 0)
    {

        ostringstream ost;

        ost << "The following nonterminal"
            << ((undefined_nonterminals.size() > 1) ? "s are " : " is ")
            << "undefined:" << endl;

        vector<string> name_list;
        for (Symbol* symbol: undefined_nonterminals)
        {
            name_list.push_back(symbol->symbol_name);
        }

        sort(name_list.begin(), name_list.end());

        int column_count = gram.line_width / gram.symbol_width;
        int column_num = column_count;
        for (string& name: name_list)
        {

            if (column_num >= column_count)
            {
                ost << endl << "  ";
                column_num = 0;
            }

            ost << setw(gram.symbol_width) << left << name;
            column_num++;

        }
        
        errh.add_error(ErrorType::ErrorUndefinedNonterm, -1, ost.str());

    }

}

//
//  find_unused_nonterminals                                                  
//  ------------------------                                                  
//                                                                         
//  An unused nonterminal is probably an incomplete grammar. The user has     
//  entered the definition of a nonterminal but hasn't entered the rules that 
//  use it yet. We're going to call that a warning situation.              
//

void Editor::find_unused_nonterminals()
{

    //
    //  Create a set of all the nonterminals. 
    //

    set<Symbol*> unused_nonterminals;
    for (auto mp: gram.symbol_map) 
    {
    
        Symbol* symbol = mp.second;
        if (symbol->is_nonterminal)
        {
            unused_nonterminals.insert(symbol);
        }

    }

    //
    //  Remove the nonterminals that are used in a rule.
    //

    for (Rule* rule: gram.rule_list)
    {

        unused_nonterminals.erase(rule->lhs);

        for (Symbol* rhs_term: rule->rhs)
        {
            unused_nonterminals.erase(rhs_term);
        }

    }

    //
    //  If there is anything left we have unused nonterminals. 
    //

    if (unused_nonterminals.size() > 0)
    {

        ostringstream ost;

        ost << "The following nonterminal"
            << ((unused_nonterminals.size() > 1) ? "s are " : " is ")
            << "unused:" << endl;

        vector<string> name_list;
        for (Symbol* symbol: unused_nonterminals)
        {
            name_list.push_back(symbol->symbol_name);
        }

        sort(name_list.begin(), name_list.end());

        int column_count = gram.line_width / gram.symbol_width;
        int column_num = column_count;
        for (string& name: name_list)
        {

            if (column_num >= column_count)
            {
                ost << endl << "  ";
                column_num = 0;
            }

            ost << setw(gram.symbol_width) << left << name;
            column_num++;

        }
        
        errh.add_error(ErrorType::ErrorUnusedNonterm, -1, ost.str());

        for (Symbol* symbol: unused_nonterminals)
        {
            useless_symbols.insert(symbol);
        }

    }

}

//
//  find_useless_nonterminals                                    
//  -------------------------                                    
//                                                               
//  A useless nonterminal is one that can not produce a string a 
//  terminals. This is an error.                                 
//

void Editor::find_useless_nonterminals()
{

    //
    //  Create a set of all the nonterminals. 
    //

    set<Symbol*> useless_nonterminals;
    for (auto mp: gram.symbol_map) 
    {
    
        Symbol* symbol = mp.second;
        if (symbol->is_nonterminal)
        {
            useless_nonterminals.insert(symbol);
        }

    }

    //
    //  Using a fixpoint algorithm remove all the nonterminals that can 
    //  produce a string of terminals.                                  
    //

    bool any_changes = true;
    while (any_changes)
    {

        any_changes = false;
        for (Rule* rule: gram.rule_list)
        {

            if (useless_nonterminals.find(rule->lhs) == useless_nonterminals.end())
            {
                continue;
            }

            bool found = false;
            for (Symbol* symbol: rule->rhs)
            {

                if (useless_nonterminals.find(symbol) != useless_nonterminals.end())
                {
                    found = true;
                    break;
                }
            
            }

            if (!found)
            {
                useless_nonterminals.erase(rule->lhs);
                any_changes = true;
            }

        }

    }

    //
    //  If there is anything left we have useless nonterminals. 
    //

    if (useless_nonterminals.size() > 0)
    {

        ostringstream ost;

        ost << "The following nonterminal"
            << ((useless_nonterminals.size() > 1) ? "s are " : " is ")
            << "useless:" << endl;

        vector<string> name_list;
        for (Symbol* symbol: useless_nonterminals)
        {
            name_list.push_back(symbol->symbol_name);
        }

        sort(name_list.begin(), name_list.end());

        int column_count = gram.line_width / gram.symbol_width;
        int column_num = column_count;
        for (string& name: name_list)
        {

            if (column_num >= column_count)
            {
                ost << endl << "  ";
                column_num = 0;
            }

            ost << setw(gram.symbol_width) << left << name;
            column_num++;

        }
        
        errh.add_error(ErrorType::ErrorUselessNonterm, -1, ost.str());

    }

}

//
//  find_useless_rules                                                   
//  ------------------                                                   
//                                                                       
//  A useless rule is one that can not be reached from the start symbol. 
//  These should generate a warning.                                     
//

void Editor::find_useless_rules()
{

    //
    //  Initially all terminals and the start symbol are reachable. 
    //

    set<Symbol*> reachable_symbols;
    for (auto mp: gram.symbol_map) 
    {
    
        Symbol* symbol = mp.second;
        if (symbol->is_terminal &&
            useless_symbols.find(symbol) == useless_symbols.end())
        {
            reachable_symbols.insert(symbol);
        }

    }
    
    reachable_symbols.insert(gram.accept_symbol);

    //
    //  Use a fixpoint algorithm to find all the symbols reachable from 
    //  the start symbol.                                               
    //

    bool any_changes = true;
    while (any_changes)
    {

        any_changes = false;
    
        for (Rule* rule: gram.rule_list)
        {

            if (reachable_symbols.find(rule->lhs) == reachable_symbols.end())
            {
                continue;
            }

            for (Symbol* symbol: rule->rhs)
            {

                if (reachable_symbols.find(symbol) == reachable_symbols.end())
                {
                    reachable_symbols.insert(symbol);
                    any_changes = true;
                }
            
            }

        }

    }

    //
    //  Find rules with an unreachable symbol on the left. 
    //

    set<Rule*> rule_set;
    for (Rule* rule: gram.rule_list)
    {
        if (reachable_symbols.find(rule->lhs) == reachable_symbols.end())
        {
            rule_set.insert(rule);
        }
    }

    //
    //  Flag the useless rules.
    //

    if (rule_set.size() > 0)
    {

        ostringstream ost;

        ost << "The following rules"
            << ((reachable_symbols.size() > 1) ? "s are " : " is ")
            << "useless:" << endl;

        for (Rule* rule: gram.rule_list)
        {

            if (rule_set.find(rule) == rule_set.end())
            {
                continue;
            }

            ost << "  " 
                << setw(gram.symbol_width) << left << rule->lhs->symbol_name 
                << setw(0) << "::= ";

            int width = gram.symbol_width + 6;

            for (Symbol* symbol: rule->rhs)
            {
             
                if (width + symbol->symbol_name.length() > gram.line_width)
                {
                    ost << endl << setw(gram.symbol_width + 6) << "" << setw(0);
                    width = gram.symbol_width + 6;
                }

                ost << symbol->symbol_name << " ";
                width = width + symbol->symbol_name.length() + 1;

            }

            ost << endl;

        }
        
        errh.add_error(ErrorType::ErrorUselessRule, -1, ost.str());

        for (Rule* rule: rule_set)
        {
            useless_rules.insert(rule);
        }

    }

}

} // namespace hoshi

