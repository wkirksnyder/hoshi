//
//  ParserStatic                                                           
//  ------------                                                           
//                                                                         
//  We want to support multiple client languages in Hoshi but for the most 
//  part those languages just support C. We collect in this class static,  
//  C-friendly functions that access the C++ library. This will make the   
//  actual wrapper functions easier to write.                              
//                                                                         
//  These should all be trivial and inline.                                
//

#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "Parser.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

//
//  ExceptionType                                                        
//  -------------                                                        
//                                                                       
//  We'll have to pass exceptions back to the language-specific clients. 
//  This flag tells the receiver the kind of exception.                  
//

enum ExceptionType : int
{
    ExceptionMinimum = 0,
    ExceptionMaximum = 3,
    ExceptionNull    = 0,
    ExceptionGrammar = 1,
    ExceptionSource  = 2,
    ExceptionUnknown = 3
};

//
//  Callbacks                                                             
//  ---------                                                             
//                                                                        
//  Return values can generally just be primitive types. When we want to  
//  return something more, like a string or exception, we have to get     
//  fancier. That's going to vary a lot by client language so we're going 
//  to require the client to provide a callback for those.                
//

typedef std::function<void(const std::string&)> StringResult;
typedef std::function<void(int, const std::string&)> ExceptionHandler;

//
//  ParserStatic                   
//  ------------                   
//                                 
//  The ParserStatic class proper. 
//

class ParserStatic
{
public:

    //
    //  parser_new_parser
    //  -----------------
    //  
    //  Construct a new Parser and return a pointer as a pointer-sized integer.
    //  We're pretty open to memory leaks here when called from garbage-collected
    //  languages (most higher-level languages). They'll have to explicitly free
    //  what they create. Hopefully we'll be able to wrap that in a class so the
    //  final client doesn't have to keep track of it.
    //
    
    static ptrdiff_t parser_new_parser()
    {
        return parser_handle_in(new Parser());
    }

    //
    //  parser_clone_parser
    //  -------------------
    //  
    //  Copy new Parser and return a pointer as a pointer-sized integer. This is
    //  basically a call to the C++ copy constructor.
    //
    
    static ptrdiff_t parser_clone_parser(ptrdiff_t parser_handle)
    {
        return parser_handle_in(new Parser(*parser_handle_out(parser_handle)));
    }

    //
    //  parser_delete_parser
    //  --------------------
    //  
    //  Delete a parser. This just calls C++ delete. For garbage collected languages
    //  this should probably be in the finalizer.
    //
    
    static void parser_delete_parser(ptrdiff_t parser_handle)
    {
        delete parser_handle_out(parser_handle);
    }

    //
    //  parser_is_grammar_loaded
    //  ------------------------
    //  
    //  Check whether the parser has a grammer loaded.
    //

    static bool parser_is_grammar_loaded(ptrdiff_t this_handle, 
                                         ExceptionHandler exception_handler)
    {
        
        try
        {
            return parser_handle_out(this_handle)->is_grammar_loaded();
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
        return false;
        
    }

    //
    //  parser_is_grammar_failed
    //  ------------------------
    //  
    //  Check whether the parser has a failed grammar.
    //

    static bool parser_is_grammar_failed(ptrdiff_t this_handle, 
                                         ExceptionHandler exception_handler)
    {
        
        try
        {
            return parser_handle_out(this_handle)->is_grammar_failed();
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
        return false;
        
    }

    //
    //  parser_is_source_loaded
    //  -----------------------
    //  
    //  Check whether the parser has a source loaded.
    //

    static bool parser_is_source_loaded(ptrdiff_t this_handle, 
                                        ExceptionHandler exception_handler)
    {
        
        try
        {
            return parser_handle_out(this_handle)->is_source_loaded();
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
        return false;
        
    }

    //
    //  parser_is_source_failed
    //  -----------------------
    //  
    //  Check whether the parser has a grammer failed.
    //

    static bool parser_is_source_failed(ptrdiff_t this_handle, 
                                        ExceptionHandler exception_handler)
    {
        
        try
        {
            return parser_handle_out(this_handle)->is_source_failed();
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
        return false;
        
    }

    //
    //  parser_generate
    //  ---------------
    //  
    //  Generate a parser from a grammar file.
    //

    static void parser_generate(ptrdiff_t this_handle, 
                                ExceptionHandler exception_handler, 
                                const std::string& source, 
                                std::map<std::string, int>& kind_map, 
                                int64_t debug_flags)
    {
        
        try
        {
            parser_handle_out(this_handle)->generate(source, 
                                                     kind_map, 
                                                     debug_flags);
        }
        catch (hoshi::GrammarError& e)
        {
            exception_handler(ExceptionType::ExceptionGrammar, e.what());
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
    }

    //
    //  parser_generate_1
    //  -----------------
    //  
    //  Generate a parser from a grammar file. This version does not include a kind
    //  map for languages that do not have a switch on integer feature.
    //
    
    static void parser_generate_1(ptrdiff_t this_handle, 
                                  ExceptionHandler exception_handler, 
                                  const std::string& source, 
                                  int64_t debug_flags)
    {
        
        try
        {
            parser_handle_out(this_handle)->generate(source,
                                                     std::map<std::string, int>(),
                                                     debug_flags);
        }
        catch (hoshi::GrammarError& e)
        {
            exception_handler(ExceptionType::ExceptionGrammar, e.what());
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
    }

    //
    //  parser_parse
    //  ------------
    //  
    //  Parse a source string saving the Ast and error messages.
    //

    static void parser_parse(ptrdiff_t this_handle, 
                             ExceptionHandler exception_handler, 
                             const std::string& source, 
                             int64_t debug_flags)
    {
        
        try
        {
            parser_handle_out(this_handle)->parse(source, debug_flags);
        }
        catch (hoshi::SourceError& e)
        {
            exception_handler(ExceptionType::ExceptionSource, e.what());
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
    }

    //
    //  parser_get_encoded_ast
    //  ----------------------
    //  
    //  Return the Ast encoded as a string. We use this method to pass entire
    //  trees back to the caller to facilitate interlanguage calls.
    //

    static void parser_get_encoded_ast(ptrdiff_t this_handle, 
                                       ExceptionHandler exception_handler, 
                                       StringResult string_result)
    {
        
        try
        {
            string_result(parser_handle_out(this_handle)->get_encoded_ast());
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
    }

    //
    //  parser_get_error_count
    //  ----------------------
    //  
    //  Return the number of error messages over the error threshhold.
    //

    static int parser_get_error_count(ptrdiff_t this_handle, 
                                      ExceptionHandler exception_handler)
    {
        
        try
        {
            return parser_handle_out(this_handle)->get_error_count();
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
        return 0;
        
    }

    //
    //  parser_get_warning_count
    //  ------------------------
    //  
    //  Return the number of error messages under the error threshhold.
    //

    static int parser_get_warning_count(ptrdiff_t this_handle, 
                                        ExceptionHandler exception_handler)
    {
        
        try
        {
            return parser_handle_out(this_handle)->get_warning_count();
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
        return 0;
        
    }

    //
    //  parser_add_error
    //  ----------------
    //  
    //  Add another error to the message list. This is provided so that clients
    //  can use the parser message handler for all errors, not just parsing
    //  errors.
    //

    static void parser_add_error(ptrdiff_t this_handle, 
                                 ExceptionHandler exception_handler, 
                                 ErrorType error_type, 
                                 int64_t location, 
                                 const std::string& short_message, 
                                 const std::string& long_message)
    {
        
        try
        {
            parser_handle_out(this_handle)->add_error(error_type, 
                                                      location, 
                                                      short_message, 
                                                      long_message);
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
    }

    //
    //  parser_get_encoded_error_messages
    //  ---------------------------------
    //  
    //  Return the error messages encoded as a string. We use this method to
    //  pass entire trees back to the caller to facilitate interlanguage calls.
    //

    static void parser_get_encoded_error_messages(ptrdiff_t this_handle, 
                                                  ExceptionHandler exception_handler, 
                                                  StringResult string_result)
    {
        
        try
        {
            string_result(parser_handle_out(this_handle)->get_encoded_error_messages());
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
    }

    //
    //  parser_get_source_list
    //  ----------------------
    //  
    //  Return a source list with embedded messages.
    //

    static void parser_get_source_list(ptrdiff_t this_handle, 
                                       ExceptionHandler exception_handler, 
                                       StringResult string_result, 
                                       const std::string& source, 
                                       int indent)
    {
        
        try
        {
            string_result(parser_handle_out(this_handle)->get_source_list(
                    source, 
                    indent));
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
    }

    //
    //  parser_encode
    //  -------------
    //  
    //  Create a string encoding of a Parser.
    //

    static void parser_encode(ptrdiff_t this_handle, 
                              ExceptionHandler exception_handler, 
                              StringResult string_result)
    {
        
        try
        {
            string_result(parser_handle_out(this_handle)->encode());
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
    }

    //
    //  parser_decode
    //  -------------
    //  
    //  Decode a previously created string into a parser
    //

    static void parser_decode(ptrdiff_t this_handle, 
                              ExceptionHandler exception_handler, 
                              const std::string& str)
    {
        
        try
        {
            parser_handle_out(this_handle)->decode(str);
        }
        catch (std::exception& e)
        {
            exception_handler(ExceptionType::ExceptionUnknown, e.what());
        }
        catch (...)
        {
            exception_handler(ExceptionType::ExceptionUnknown, "Unknown exception");
        }
        
    }

private:

    //
    //  Simple Translators                                               
    //  ------------------                                               
    //                                                                   
    //  We need simple conversion routines for types that change in this 
    //  layer.                                                           
    //

    static ptrdiff_t parser_handle_in(Parser* parser_ptr)
    {
        return reinterpret_cast<ptrdiff_t>(parser_ptr);
    }

    static Parser* parser_handle_out(ptrdiff_t parser_handle)
    {
        return reinterpret_cast<Parser*>(parser_handle);
    }

};

} // namespace hoshi
