//
//  Parser                                                                 
//  ------                                                                 
//                                                                         
//  This is the public interface to the Hoshi parser generator and parser. 
//  This is the only header file needed to use Hoshi.                      
//                                                                         
//  We've used the pimpl idiom to hide most of the implementation details  
//  in other files.                                                        
//

#include <mutex>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "Parser.H"
#include "ParserImpl.H"
#include "ParserEngine.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;

//
//  Missing argument flag. 
//

map<string, int> Parser::kind_map_missing;
string Parser::string_missing{""};

//
//  Copy control                                                           
//  ------------                                                           
//                                                                         
//  Parser is implemented with the pimpl idiom, so nothing really is kept  
//  on the Parser object except a pointer to the implementation. We assume 
//  that can be copied and arrange the operators accordingly.              
//

Parser::Parser()
{
    initialize();
    impl = new ParserImpl();
}

Parser::~Parser()
{
    delete impl;
}

Parser::Parser(const Parser& rhs)
{
    impl = new ParserImpl(*rhs.impl);
}

Parser::Parser(Parser&& rhs) noexcept
{
    swap(impl, rhs.impl);    
}

Parser& Parser::operator=(const Parser& rhs)
{

    if (&rhs != this)
    {
        ParserImpl* new_impl = new ParserImpl(*rhs.impl);
        delete impl;
        impl = new_impl;
    }

    return *this;

}

Parser& Parser::operator=(Parser&& rhs) noexcept
{
    swap(impl, rhs.impl);    
    return *this;
}

//
//  initialize                                                            
//  ----------                                                            
//                                                                        
//  Drive the initialize process for all the classes in Hoshi. The client 
//  has to create a Parser before it can do pretty much anything but is   
//  unlikely to create all that many, and if he does he will probably go  
//  through an expensive generation process on each one. We add a little  
//  bit to the expense of that to guarantee that each class is statically 
//  initialized appropritately.                                           
//

void Parser::initialize()
{

    static volatile bool finished = false;
    static mutex initialize_mutex;

    if (finished)
    {
        return;
    }

    lock_guard<mutex> initialize_guard(initialize_mutex);

    if (finished)
    {
        return;
    }

    ParserEngine::initialize();
    ParserImpl::initialize();

    finished = true;

}

//
//  State Queries                                                        
//  -------------                                                        
//                                                                       
//  Some of our accessors throw exceptions if the object is in the wrong 
//  state. These predicates give the client the ability to check in      
//  advance.                                                             
//

bool Parser::is_grammar_loaded()
{
    return impl->is_grammar_loaded();
}

bool Parser::is_grammar_failed()
{
    return impl->is_grammar_failed();
}

bool Parser::is_parse_successful()
{
    return impl->is_parse_successful();
}

bool Parser::is_parse_failed()
{
    return impl->is_parse_failed();
}

//
//  set_ast_kind_map                                                      
//  ----------------                                                      
//                                                                        
//  For the bootstrap runs we need to create a skeletal parser which only 
//  contains the Ast kind map. We want to be able to see ast kinds, but   
//  we're going to generate the Ast outside the Hoshi Parser.             
//

void Parser::set_ast_kind_map(const std::map<std::string, int>& ast_kind_map)
{
    impl->set_ast_kind_map(ast_kind_map);
}

//
//  generate                                                               
//  --------                                                               
//                                                                         
//  Generate a parser from an Ast. During bootstrapping we will get an Ast 
//  from a non-hoshi parser. We can use that Ast to generate a Hoshi       
//  parser.                                                                
//

void Parser::generate(Ast* ast,
                      const Source& src,
                      const map<string, int>& ast_kind_map,
                      const int64_t debug_flags)   
{
    impl->generate(ast, src, ast_kind_map, debug_flags);    
}

//
//  generate_ast                                                          
//  ------------                                                          
//                                                                        
//  Generate an Ast from the bootstrapped Hoshi parser. This is only used 
//  to validate the bootstrap process.                                    
//

Ast* Parser::generate_ast(const Source& src,
                          const int64_t debug_flags)   
{
    return impl->generate_ast(src, debug_flags);    
}

//
//  generate                               
//  --------                               
//                                         
//  Generate a parser from a grammar file. 
//

void Parser::generate(const Source& src,
                      const map<string, int>& ast_kind_map,
                      const int64_t debug_flags)   
{
    impl->generate(src, ast_kind_map, debug_flags);    
}

//
//  parse                                              
//  -----                                              
//                                                     
//  Use the generated parser to parse a source string. 
//

void Parser::parse(const Source& src, const int64_t debug_flags)
{
    impl->parse(src, debug_flags);
}

//
//  get_ast_kind_map                                                     
//  ----------------                                                     
//                                                                       
//  Get the current ast_kind_map. The client can use this to find out if 
//  he forgot to define any important kind strings.                      
//

map<string, int> Parser::get_ast_kind_map() const
{
    return impl->get_ast_kind_map();
}

//
//  get_ast                                        
//  -------                                        
//                                                 
//  Return the result Ast from a successful parse. 
//

Ast* Parser::get_ast()
{
    return impl->get_ast();
}

//
//  dump_ast                                              
//  --------                                              
//                                                    
//  For debugging we'll provide a nice dump function. 
//

void Parser::dump_ast(Ast* root, ostream& os, int indent) const
{
    impl->dump_ast(root, os, indent);
}

//
//  get_kind                                                          
//  --------                                                          
//                                                                    
//  Get the integer code for a given string.                          
//

int Parser::get_kind(const string& kind_str) const
{
    return impl->get_kind(kind_str);
}

//
//  get_kind_force                                                          
//  --------------                                                          
//                                                                    
//  Get the integer code for a given string. If it doesn't exist then 
//  install it.                                                       
//

int Parser::get_kind_force(const string& kind_str)
{
    return impl->get_kind_force(kind_str);
}

//
//  get_kind_string                       
//  ---------------                       
//                                        
//  Get the text name for a numeric code. 
//

string Parser::get_kind_string(int kind) const
{
    return impl->get_kind_string(kind);
}

//
//  get_kind_string                       
//  ---------------                       
//                                        
//  Get the text name for an Ast
//

string Parser::get_kind_string(const Ast* root) const
{
    return impl->get_kind_string(root);
}

//
//  add_error                                    
//  ---------                                    
//                                               
//  Create a new message and add it to the list. 
//

void Parser::add_error(ErrorType error_type,
                       int64_t location,
                       const string& short_message,
                       const string& long_message)
{

    if (&long_message != &string_missing)
    {
        impl->add_error(error_type, location, short_message, long_message);
    }
    else
    {
        impl->add_error(error_type, location, short_message);
    }

}

//
//  get_error_count                                          
//  ---------------                                          
//                                                           
//  Return the number of messages over the error threshhold. 
//

int Parser::get_error_count() const
{
    return impl->get_error_count();
}

//
//  get_warning_count                                         
//  -----------------                                         
//                                                            
//  Return the number of messages below the error threshhold. 
//

int Parser::get_warning_count() const
{
    return impl->get_warning_count();
}

//
//  get_error_messages                                   
//  ------------------                                   
//                                                       
//  Return the list of error messages in location order. 
//

vector<ErrorMessage> Parser::get_error_messages()
{
    return impl->get_error_messages();
}

//
//  dump_source                         
//  -----------                         
//                                      
//  List the source and error messages. 
//

void Parser::dump_source(const Source& src,
                         ostream& os,
                         int indent) const
{
    impl->dump_source(src, os, indent);
}

//
//  export_cpp                                                            
//  ----------                                                            
//                                                                        
//  Create a serialized version of the parser data in a string and export 
//  as a C++ source file.                                                 
//

void Parser::export_cpp(std::string file_name, std::string identifier) const
{
    return impl->export_cpp(file_name, identifier);
}

//
//  encode                                                                
//  ------                                                                
//                                                                        
//  Encode the generated parser as a string so that it can be reloaded in 
//  a subsequent run. Not sure this will be useful by clients, but it's   
//  essential to bootstrapping.                                           
//

string Parser::encode() const
{
    return impl->encode();
}

//
//  decode                                                   
//  ------                                                   
//                                                           
//  Decode the string version of a parser into a new parser. 
//

void Parser::decode(const string& str,
                    const map<string, int>& ast_kind_map)
{
    impl->decode(str, ast_kind_map);
}

} // namespace hoshi
