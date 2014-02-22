#line 66 "u:\\hoshi\\raw\\LibraryToken.cpp"
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

#include <cstdint>
#include <string>
#include "Parser.H"
#include "LibraryToken.H"

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
//  Built-in Tokens                                                       
//  ---------------                                                       
//                                                                        
//  List of the built-in tokens. Note that we have to keep them in sorted 
//  order.                                                                
//

LibraryToken LibraryToken::library_token_list[] =
{

    {
      "c_comment",              // name
      "/\\* ([^*] | (\\*+ ([^*/])))* \\*+/",
                                // regex_string
      "",                       // description
      100,                      // precedence
      false,                    // lexeme_needed
      true,                     // is_ignored
      ""                        // error_message
    },

    {
      "cpp_comment",            // name
      "{slash_prefix_comment} | {c_comment}",
                                // regex_string
      "",                       // description
      100,                      // precedence
      false,                    // lexeme_needed
      true,                     // is_ignored
      ""                        // error_message
    },

    {
      "float",                  // name
      "[0-9]+\\.[0-9]+([eE][+\\-]?[1-9][0-9]*)?",
                                // regex_string
      "",                       // description
      100,                      // precedence
      true,                     // lexeme_needed
      false,                    // is_ignored
      ""                        // error_message
    },

    {
      "identifier",             // name
      "[A-Za-z][A-Za-z0-9_]*",  // regex_string
      "",                       // description
      50,                       // precedence
      true,                     // lexeme_needed
      false,                    // is_ignored
      ""                        // error_message
    },

    {
      "integer",                // name
      "[0-9]+",                 // regex_string
      "",                       // description
      100,                      // precedence
      true,                     // lexeme_needed
      false,                    // is_ignored
      ""                        // error_message
    },

    {
      "number",                 // name
      "{integer} | {float}",    // regex_string
      "",                       // description
      100,                      // precedence
      true,                     // lexeme_needed
      false,                    // is_ignored
      ""                        // error_message
    },

    {
      "pascal_comment",         // name
      "\\(\\* ([^*] | (\\*+ ([^*)])))* \\*+\\)",
                                // regex_string
      "",                       // description
      100,                      // precedence
      false,                    // lexeme_needed
      true,                     // is_ignored
      ""                        // error_message
    },

    {
      "slash_prefix_comment",   // name
      "// [^\\n]*",             // regex_string
      "",                       // description
      100,                      // precedence
      false,                    // lexeme_needed
      true,                     // is_ignored
      ""                        // error_message
    },

    {
      "whitespace",             // name
      "\\s+",                   // regex_string
      "",                       // description
      100,                      // precedence
      false,                    // lexeme_needed
      true,                     // is_ignored
      ""                        // error_message
    }

};

//
//  get_library_token                          
//  -----------------                          
//                                             
//  Look up a library token based on its name. 
//

LibraryToken* LibraryToken::get_library_token(const string& name)
{

    int min = 0;
    int max = LENGTH(library_token_list) - 1;
    int mid;

    while (min <= max)
    {

        mid = min + (max - min) / 2;

        if (library_token_list[mid].name < name)
        {
            min = mid + 1;
        }
        else if (library_token_list[mid].name > name)
        {
            max = mid - 1;
        }
        else
        {
            break;
        }

    }

    if (min <= max)
    {
        return library_token_list + mid;
    }
    else
    {
        return nullptr;
    }

}

} // namespace hoshi


