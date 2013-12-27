//
//  PythonWrapper                                                          
//  -------------                                                          
//                                                                         
//  This is glue code to use Hoshi from Python. Python must call C and     
//  each function we wish to access in Python must be here. From this file 
//  we call a language-independent static C++ class (ParserStatic) with    
//  the method we want, and that will in turn make the call into C++.      
//                                                                         
//  These levels of forwarding simplify the coding. With inline functions  
//  the cost should be negligible.                                         
//

#include <iostream>
#include <sstream>
#include "Parser.H"
#include "ParserStatic.H"

#ifdef _WIN64
#include <Windows.h>
#define EXTERN _declspec(dllexport)
#else
#define EXTERN
#endif

using namespace std;
using namespace hoshi;

//
//  StringResultStruct & ExceptionStruct                                
//  ------------------------------------                                
//                                                                      
//  These are places to stash results too big to return as primitives.  
//  When the eventual called function wants to return these we save the 
//  output here and allow the python client to query them.              
//

struct StringResultStruct
{
    string result_string;
};

struct ExceptionStruct
{
    int exception_type;
    string exception_string;
};

//
//  string_result_out                                                     
//  -----------------                                                     
//                                                                        
//  We need to plant callbacks in the ParserStatic class to handle string 
//  return values. This is essentially a currying function to create such 
//  a thing from a location provided in python.                           
//

static StringResult string_result_out(void** result_handle)
{

    return [result_handle](const std::string& what) -> void
    {
        StringResultStruct* result = new StringResultStruct();
        result->result_string = what;
        *reinterpret_cast<StringResultStruct**>(result_handle) = result;
    };

}
        
//
//  exception_handler_out                                                 
//  ---------------------                                                 
//                                                                        
//  We need to plant callbacks in the ParserStatic class to handle thrown 
//  exceptions. This is essentially a currying function to create such a  
//  thing from a location provided in python.                             
//

static ExceptionHandler exception_handler_out(void** exception_handle)
{

    return [exception_handle](int exception_type, const std::string& what) -> void
    {
        ExceptionStruct* exception_ptr = new ExceptionStruct();
        exception_ptr->exception_type = exception_type;
        exception_ptr->exception_string = what;
        *reinterpret_cast<ExceptionStruct**>(exception_handle) = exception_ptr;
    };

}
        
//
//  py_get_exception_type
//  ---------------------
//  
//  Fetch the exception type for Python given a handle to the exception.
//

extern "C" EXTERN
int64_t py_get_exception_type(void** exception_ptr)
{

    if (exception_ptr == nullptr || *exception_ptr == nullptr)
    {
        return -1;    
    }

    ExceptionStruct* struct_ptr = *reinterpret_cast<ExceptionStruct**>(exception_ptr);

    return struct_ptr->exception_type;

}

//
//  py_get_exception_length
//  -----------------------
//  
//  Fetch the length of the exception string for Python given a handle to the
//  exception.
//

extern "C" EXTERN
int64_t py_get_exception_length(void** exception_ptr)
{

    if (exception_ptr == nullptr || *exception_ptr == nullptr)
    {
        return -1;    
    }

    ExceptionStruct* struct_ptr = *reinterpret_cast<ExceptionStruct**>(exception_ptr);

    return struct_ptr->exception_string.length() + 1;

}

//
//  py_get_exception_string
//  -----------------------
//  
//  Fetch the text of the exception string for Python given a handle to the
//  exception. Delete the exception as a by-product.
//

extern "C" EXTERN
void py_get_exception_string(void** exception_ptr, char* string_ptr)
{

    if (exception_ptr == nullptr || *exception_ptr == nullptr)
    {
        return;    
    }

    ExceptionStruct* struct_ptr = *reinterpret_cast<ExceptionStruct**>(exception_ptr);

    memcpy(reinterpret_cast<void*>(string_ptr),
           reinterpret_cast<void*>(const_cast<char*>(struct_ptr->exception_string.data())),
           struct_ptr->exception_string.length());

    string_ptr[struct_ptr->exception_string.length()] = '\0';

    delete struct_ptr;
    *reinterpret_cast<ExceptionStruct**>(exception_ptr) = nullptr;
    
}

//
//  py_get_string_length
//  --------------------
//  
//  Fetch the length of the result string for Python given a handle to the
//  string.
//

extern "C" EXTERN
int64_t py_get_string_length(void** result_ptr)
{

    if (result_ptr == nullptr || *result_ptr == nullptr)
    {
        return -1;    
    }

    StringResultStruct* struct_ptr = *reinterpret_cast<StringResultStruct**>(result_ptr);

    return struct_ptr->result_string.length() + 1;

}

//
//  py_get_string_string
//  --------------------
//  
//  Fetch the text of the result string for Python given a handle to the string.
//  Delete the string as a by-product.
//

extern "C" EXTERN
void py_get_string_string(void** result_ptr, char* string_ptr)
{

    if (result_ptr == nullptr || *result_ptr == nullptr)
    {
        return;    
    }

    StringResultStruct* struct_ptr = *reinterpret_cast<StringResultStruct**>(result_ptr);

    memcpy(reinterpret_cast<void*>(string_ptr),
           reinterpret_cast<void*>(const_cast<char*>(struct_ptr->result_string.data())),
           struct_ptr->result_string.length());

    string_ptr[struct_ptr->result_string.length()] = '\0';

    delete struct_ptr;
    *reinterpret_cast<StringResultStruct**>(result_ptr) = nullptr;
    
}

//
//  py_parser_new_parser
//  --------------------
//  
//  Construct a new Parser and return a pointer as a pointer-sized integer.
//  We're pretty open to memory leaks here when called from garbage-collected
//  languages (most higher-level languages). They'll have to explicitly free
//  what they create. Hopefully we'll be able to wrap that in a class so the
//  final client doesn't have to keep track of it.
//

extern "C" EXTERN
ptrdiff_t py_parser_new_parser()
{
    return ParserStatic::parser_new_parser();
}

//
//  py_parser_clone_parser
//  ----------------------
//  
//  Copy new Parser and return a pointer as a pointer-sized integer. This is
//  basically a call to the C++ copy constructor.
//

extern "C" EXTERN
ptrdiff_t py_parser_clone_parser(ptrdiff_t parser_handle)
{
    return ParserStatic::parser_clone_parser(parser_handle);
}

//
//  py_parser_delete_parser
//  -----------------------
//  
//  Delete a parser. This just calls C++ delete. For garbage collected languages
//  this should probably be in the finalizer.
//

extern "C" EXTERN
void py_parser_delete_parser(ptrdiff_t parser_handle)
{
    ParserStatic::parser_delete_parser(parser_handle);
}

//
//  py_parser_is_grammar_loaded
//  ---------------------------
//  
//  Check whether the parser has a grammer loaded.
//

extern "C" EXTERN
bool py_parser_is_grammar_loaded(ptrdiff_t this_handle, void** exception_ptr)
{
    return ParserStatic::parser_is_grammar_loaded(this_handle, 
                                                  exception_handler_out(exception_ptr));
}

//
//  py_parser_is_grammar_failed
//  ---------------------------
//  
//  Check whether the parser has a failed grammar.
//

extern "C" EXTERN
bool py_parser_is_grammar_failed(ptrdiff_t this_handle, void** exception_ptr)
{
    return ParserStatic::parser_is_grammar_failed(this_handle, 
                                                  exception_handler_out(exception_ptr));
}

//
//  py_parser_is_source_loaded
//  --------------------------
//  
//  Check whether the parser has a source loaded.
//

extern "C" EXTERN
bool py_parser_is_source_loaded(ptrdiff_t this_handle, void** exception_ptr)
{
    return ParserStatic::parser_is_source_loaded(this_handle, 
                                                 exception_handler_out(exception_ptr));
}

//
//  py_parser_is_source_failed
//  --------------------------
//  
//  Check whether the parser has a grammer failed.
//

extern "C" EXTERN
bool py_parser_is_source_failed(ptrdiff_t this_handle, void** exception_ptr)
{
    return ParserStatic::parser_is_source_failed(this_handle, 
                                                 exception_handler_out(exception_ptr));
}

//
//  py_parser_generate_1
//  --------------------
//  
//  Generate a parser from a grammar file. This version does not include a kind
//  map for languages that do not have a switch on integer feature.
//

extern "C" EXTERN
void py_parser_generate_1(ptrdiff_t this_handle, 
                          void** exception_ptr, 
                          char* source, 
                          int64_t debug_flags)
{
    
    ParserStatic::parser_generate_1(this_handle, 
                                    exception_handler_out(exception_ptr), 
                                    source, 
                                    debug_flags);
    
}

//
//  py_parser_parse
//  ---------------
//  
//  Parse a source string saving the Ast and error messages.
//

extern "C" EXTERN
void py_parser_parse(ptrdiff_t this_handle, 
                     void** exception_ptr, 
                     char* source, 
                     int64_t debug_flags)
{
    
    ParserStatic::parser_parse(this_handle, 
                               exception_handler_out(exception_ptr), 
                               source, 
                               debug_flags);
    
}

//
//  py_parser_get_encoded_ast
//  -------------------------
//  
//  Return the Ast encoded as a string. We use this method to pass entire trees
//  back to the caller to facilitate interlanguage calls.
//

extern "C" EXTERN
void py_parser_get_encoded_ast(ptrdiff_t this_handle, 
                               void** exception_ptr, 
                               void** result_ptr)
{
    ParserStatic::parser_get_encoded_ast(this_handle, 
                                         exception_handler_out(exception_ptr), 
                                         string_result_out(result_ptr));
}

//
//  py_parser_get_encoded_kind_map
//  ------------------------------
//  
//  Return the kind map encoded as a string. We use this method to pass the kind
//  map to the caller to facilitate interlanguage calls.
//

extern "C" EXTERN
void py_parser_get_encoded_kind_map(ptrdiff_t this_handle, 
                                    void** exception_ptr, 
                                    void** result_ptr)
{
    ParserStatic::parser_get_encoded_kind_map(this_handle, 
                                              exception_handler_out(exception_ptr), 
                                              string_result_out(result_ptr));
}

//
//  py_parser_add_error
//  -------------------
//  
//  Add another error to the message list. This is provided so that clients can
//  use the parser message handler for all errors, not just parsing errors.
//

extern "C" EXTERN
void py_parser_add_error(ptrdiff_t this_handle, 
                         void** exception_ptr, 
                         ErrorType error_type, 
                         int64_t location, 
                         char* short_message, 
                         char* long_message)
{
    
    ParserStatic::parser_add_error(this_handle, 
                                   exception_handler_out(exception_ptr), 
                                   error_type, 
                                   location, 
                                   short_message, 
                                   long_message);
    
}

//
//  py_parser_get_error_count
//  -------------------------
//  
//  Return the number of error messages over the error threshhold.
//

extern "C" EXTERN
int py_parser_get_error_count(ptrdiff_t this_handle, void** exception_ptr)
{
    return ParserStatic::parser_get_error_count(this_handle, 
                                                exception_handler_out(exception_ptr));
}

//
//  py_parser_get_warning_count
//  ---------------------------
//  
//  Return the number of error messages under the error threshhold.
//

extern "C" EXTERN
int py_parser_get_warning_count(ptrdiff_t this_handle, void** exception_ptr)
{
    return ParserStatic::parser_get_warning_count(this_handle, 
                                                  exception_handler_out(exception_ptr));
}

//
//  py_parser_get_encoded_error_messages
//  ------------------------------------
//  
//  Return the error messages encoded as a string. We use this method to pass
//  entire trees back to the caller to facilitate interlanguage calls.
//

extern "C" EXTERN
void py_parser_get_encoded_error_messages(ptrdiff_t this_handle, 
                                          void** exception_ptr, 
                                          void** result_ptr)
{
    ParserStatic::parser_get_encoded_error_messages(this_handle, 
                                                    exception_handler_out(exception_ptr), 
                                                    string_result_out(result_ptr));
}

//
//  py_parser_get_source_list
//  -------------------------
//  
//  Return a source list with embedded messages.
//

extern "C" EXTERN
void py_parser_get_source_list(ptrdiff_t this_handle, 
                               void** exception_ptr, 
                               void** result_ptr, 
                               char* source, 
                               int indent)
{
    
    ParserStatic::parser_get_source_list(this_handle, 
                                         exception_handler_out(exception_ptr), 
                                         string_result_out(result_ptr), 
                                         source, 
                                         indent);
    
}

//
//  py_parser_encode
//  ----------------
//  
//  Create a string encoding of a Parser.
//

extern "C" EXTERN
void py_parser_encode(ptrdiff_t this_handle, 
                      void** exception_ptr, 
                      void** result_ptr)
{
    ParserStatic::parser_encode(this_handle, 
                                exception_handler_out(exception_ptr), 
                                string_result_out(result_ptr));
}

//
//  py_parser_decode_1
//  ------------------
//  
//  Decode a previously created string into a parser
//

extern "C" EXTERN
void py_parser_decode_1(ptrdiff_t this_handle, 
                        void** exception_ptr, 
                        char* str)
{
    ParserStatic::parser_decode_1(this_handle, 
                                  exception_handler_out(exception_ptr), 
                                  str);
}
