//
//  LibraryToken                                                          
//  ------------                                                          
//                                                                        
//  We want to provide a list of pre-defined tokens. These can be used    
//  without declaration, included in regex strings or used as templates   
//  for client token definitions.                                         
//                                                                        
//  Most DSL's use quite similar definitions for literals. By providing a 
//  library we may be able to avoid these in grammar files most of the    
//  time. It a very convenient shortcut.                                  
//

#ifndef LIBRARY_TOKEN_H
#define LIBRARY_TOKEN_H

#include <cstdint>
#include <string>
#include "Parser.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

class LibraryToken final
{
public:

    LibraryToken(const std::string& name,
                 const std::string& regex_string,
                 const std::string& description,
                 int precedence,
                 bool lexeme_needed,
                 bool is_ignored,
                 const std::string& error_message)
        : name(name), regex_string(regex_string), description(description), precedence(precedence),
          lexeme_needed(lexeme_needed), is_ignored(is_ignored), error_message(error_message) {}

    std::string name = "";
    std::string regex_string = "";
    std::string description = "";
    int precedence = 100;
    bool lexeme_needed = false;
    bool is_ignored = false;
    std::string error_message = "";

    static LibraryToken library_token_list[];
    static LibraryToken* get_library_token(const std::string& name);

};

} // namespace hoshi

#endif // LIBRARY_TOKEN_H
