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

#ifndef EDITOR_H
#define EDITOR_H

#include "ErrorHandler.H"
#include "ParserImpl.H"
#include "Grammar.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

//
//  Editor                                                           
//  ------                                                           
//                                                                   
//  This class doesn't provide much in the way of storage. We're just 
//  editing stuff stored elsewhere.                                  
//

class Editor
{

public:

    Editor(ParserImpl& prsi, ErrorHandler& errh, Grammar& gram, int64_t debug_flags)
        : prsi(prsi), errh(errh), gram(gram), debug_flags(debug_flags) {}

    void generate();

    //
    //  Disable copying. 
    //

    Editor(const Editor&) = delete;
    Editor(Editor&&) = delete;
    Editor& operator=(const Editor&) = delete;
    Editor& operator=(Editor&&) = delete;

private:

    ParserImpl& prsi;
    ErrorHandler& errh;
    Grammar& gram;
    int64_t debug_flags;

    std::set<Symbol*> useless_symbols;
    std::set<Rule*> useless_rules;

    void find_unused_terminals();
    void find_undefined_nonterminals();
    void find_unused_nonterminals();
    void find_useless_nonterminals();
    void find_useless_rules();

};

} // namespace hoshi

#endif // EDITOR_H
