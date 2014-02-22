#line 2 "u:\\hoshi\\raw\\CscWrapper.cpp"
//
//  CscWrapper                                                           
//  ----------                                                           
//                                                                        
//  This is glue code to use Hoshi from C#. C# must call C and        
//  each function we wish to access in C# must be here. From this file  
//  we call a language-independent static C++ class (ParserStatic) with   
//  the method we want, and that will in turn make the call into C++.     
//                                                                        
//  These levels of forwarding simplify the coding. With inline functions 
//  the cost should be negligible.                                        
//

#ifndef _WIN64
int dummy_public_symbol_so_linkers_accept_this_file = 0;
#else

#include <codecvt>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <Windows.h>
#include "Parser.H"
#include "ParserStatic.H"

using namespace std;
using namespace hoshi;

//
//  .NET service handlers                                                
//  ---------------------                                                
//                                                                       
//  We need some services, primarily creating exceptions, which must be  
//  handled by asynchronous callbacks. I'm not sure this is the best way 
//  to program this, but what we're going to do is register callback     
//  handlers for each of these services here and call them as needed.    
//                                                                       
//  There is a similar concept in JNI, but there we initiate the process 
//  from the C++ side. In PInvoke we have to initiate it in .NET.        
//

//
//  GrammarError.
//

typedef void (*GrammarErrorCreator)(LPVOID, LPWSTR, INT32);
static GrammarErrorCreator grammar_error_creator = nullptr;

extern "C" _declspec(dllexport) 
void csc_Services_register_grammar_error_creator(GrammarErrorCreator creator)
{
    grammar_error_creator = creator;
}

//
//  SourceError.
//

typedef void (*SourceErrorCreator)(LPVOID, LPWSTR, INT32);
static SourceErrorCreator source_error_creator = nullptr;

extern "C" _declspec(dllexport) 
void csc_Services_register_source_error_creator(SourceErrorCreator creator)
{
    source_error_creator = creator;
}

//
//  UnknownError.
//

typedef void (*UnknownErrorCreator)(LPVOID, LPWSTR, INT32);
static UnknownErrorCreator unknown_error_creator = nullptr;

extern "C" _declspec(dllexport) 
void csc_Services_register_unknown_error_creator(UnknownErrorCreator creator)
{
    unknown_error_creator = creator;
}

//
//  String.
//

typedef void (*StringCreator)(LPVOID, LPWSTR, INT32);
static StringCreator string_creator = nullptr;

extern "C" _declspec(dllexport) 
void csc_Services_register_string_creator(StringCreator creator)
{
    string_creator = creator;
}

//
//  Primitive String Encoders and Decoders                            
//  --------------------------------------                            
//                                                                    
//  Encode or decode small types as strings. We use these to marshall 
//  aggregates to pass back and forth to client languages.            
//

static void encode_long(ostream& os, int64_t value)
{
    os << value << "|";    
}

static int64_t decode_long(istream& is)
{

    ostringstream ost;

    for (;;) 
    {

        char c;
        is >> c;
 
        if (c == '`')
        {
            is >> c;
        }
        else if (c == '|')
        {
            break;
        }

        ost << c;

    }
        
    return stoll(ost.str());

}

static void encode_string(ostream& os, const string& value)
{

    for (char c: value)
    {

        if (c == '`' || c == '|')
        {
            os << '`';
        }
     
        os << c;

    }

    os << value << "|";    

}

static string decode_string(istream& is)
{

    ostringstream ost;

    for (;;) 
    {

        char c;
        is >> c;
 
        if (c == '`')
        {
            is >> c;
        }
        else if (c == '|')
        {
            break;
        }

        ost << c;

    }
        
    return ost.str();

}

//
//  StringResultStruct & ExceptionStruct                                
//  ------------------------------------                                
//                                                                      
//  These are places to stash results too big to return as primitives.  
//  When the eventual called function wants to return these we save the 
//  output here and allow the C# client to query them.                
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
//  a thing from a location provided in C#.                           
//

static StringResult string_result_out(void** result_handle)
{

    return [result_handle](const string& str) -> void
    {
        StringResultStruct* result = new StringResultStruct();
        result->result_string = str;
        *reinterpret_cast<StringResultStruct**>(result_handle) = result;
    };

}
        
//
//  exception_handler_out                                                 
//  ---------------------                                                 
//                                                                        
//  We need to plant callbacks in the ParserStatic class to handle thrown 
//  exceptions. This is essentially a currying function to create such a  
//  thing from a location provided in C#.                             
//

static ExceptionHandler exception_handler_out(void** exception_handle)
{

    return [exception_handle](int exception_type, const string& str) -> void
    {
        ExceptionStruct* exception_ptr = new ExceptionStruct();
        exception_ptr->exception_type = exception_type;
        exception_ptr->exception_string = str;
        *reinterpret_cast<ExceptionStruct**>(exception_handle) = exception_ptr;
    };

}
        
//
//  string_result_in                                                    
//  ----------------                                                    
//                                                                      
//  Take the string result from Hoshi, convert from UTF-8 to UTF-16 and 
//  return it to C#.                                                  
//

static void string_result_in(LPVOID result_handle, void* result_vptr)
{

    static wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> myconv;

    if (result_vptr == nullptr)
    {
        return;
    }
    
    StringResultStruct* result_ptr = 
        reinterpret_cast<StringResultStruct*>(result_vptr);

    u16string str_utf16 = myconv.from_bytes(result_ptr->result_string);
    wstring str_wstr(str_utf16.length(), static_cast<wchar_t>(' '));

    for (int i = 0; i < str_utf16.length(); i++)
    {
        str_wstr[i] = static_cast<wchar_t>(str_utf16[i]);
    }

    (*string_creator)(result_handle, &(str_wstr.front()), str_wstr.size());

}

//
//  check_exceptions                                                     
//  ----------------                                                     
//                                                                       
//  See if the called function found an exception. If so return it to 
//  clr.
//

static void check_exceptions(LPVOID exception_handle, void* exception_vptr)
{

    static wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> myconv;

    if (exception_vptr == nullptr)
    {
        return;
    }

    ExceptionStruct* exception_ptr = 
        reinterpret_cast<ExceptionStruct*>(exception_vptr);

    u16string str_utf16 = myconv.from_bytes(exception_ptr->exception_string);
    wstring str_wstr(str_utf16.length(), static_cast<wchar_t>(' '));

    for (int i = 0; i < str_utf16.length(); i++)
    {
        str_wstr[i] = static_cast<wchar_t>(str_utf16[i]);
    }

    switch (exception_ptr->exception_type)
    {

        case ExceptionType::ExceptionGrammar:
            (*grammar_error_creator)(exception_handle, &(str_wstr.front()), str_wstr.size());
            return;

        case ExceptionType::ExceptionSource:
            (*source_error_creator)(exception_handle, &(str_wstr.front()), str_wstr.size());
            return;

        case ExceptionType::ExceptionUnknown:
            (*unknown_error_creator)(exception_handle, &(str_wstr.front()), str_wstr.size());
            return;

    }

}

//
//  string_out                                                            
//  ----------                                                            
//                                                                        
//  Convert an LPWSTR into a C++ string to send to ParserStatic. C#'s   
//  String representation is UTF-16. Hoshi uses UTF-8. This function does 
//  the conversion.                                                       
//

static string string_out(LPWSTR str_in)
{

    static wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> myconv;

    int length = 0;
    for (auto p = str_in; *p != 0; p++)
    {
        length++;
    }

    u16string str_out(length, ' ');
    
    for (int i = 0;
         i < length;
         str_out[i] = static_cast<char16_t>(str_in[i++]));

    try
    {
        return myconv.to_bytes(str_out);
    }
    catch (...)
    {
        cerr << "Fatal error: CLR string is not UTF-16!" << endl;
        exit(1);
    }

}

//
//  kind_map_out                                 
//  ------------                                 
//                                               
//  Convert a marshalled kind_map into C++ form. 
//

static map<string, int> kind_map_out(LPWSTR str_in)
{
    
    map<string, int> result;
    istringstream is(string_out(str_in));

    int size = decode_long(is);

    while (size--)
    {
        string key = decode_string(is);
        int value = decode_long(is);
        result[key] = value;
    }

    return result;

}

//
//  csc_Parser_new_parser
//  ---------------------
//  
//  Construct a new Parser and return a pointer as a pointer-sized integer.
//  We're pretty open to memory leaks here when called from garbage-collected
//  languages (most higher-level languages). They'll have to explicitly free
//  what they create. Hopefully we'll be able to wrap that in a class so the
//  final client doesn't have to keep track of it.
//

extern "C" _declspec(dllexport)
ptrdiff_t csc_Parser_new_parser()
{
    return ParserStatic::parser_new_parser();
}

//
//  csc_Parser_clone_parser
//  -----------------------
//  
//  Copy new Parser and return a pointer as a pointer-sized integer. This is
//  basically a call to the C++ copy constructor.
//

extern "C" _declspec(dllexport)
ptrdiff_t csc_Parser_clone_parser(ptrdiff_t parser_handle)
{
    return ParserStatic::parser_clone_parser(parser_handle);
}

//
//  csc_Parser_delete_parser
//  ------------------------
//  
//  Delete a parser. This just calls C++ delete. For garbage collected languages
//  this should probably be in the finalizer.
//

extern "C" _declspec(dllexport)
void csc_Parser_delete_parser(ptrdiff_t parser_handle)
{
    ParserStatic::parser_delete_parser(parser_handle);
}

//
//  csc_Parser_is_grammar_loaded
//  ----------------------------
//  
//  Check whether the parser has a grammer loaded.
//

extern "C" _declspec(dllexport)
bool csc_Parser_is_grammar_loaded(ptrdiff_t this_handle, 
                                  LPVOID exception_handle)
{
    
    void* exception_ptr = nullptr;
    
    bool result = ParserStatic::parser_is_grammar_loaded(
            this_handle, 
            exception_handler_out(&exception_ptr));
    
    check_exceptions(exception_handle, exception_ptr);
    
    return result;
    
}

//
//  csc_Parser_is_grammar_failed
//  ----------------------------
//  
//  Check whether the parser has a failed grammar.
//

extern "C" _declspec(dllexport)
bool csc_Parser_is_grammar_failed(ptrdiff_t this_handle, 
                                  LPVOID exception_handle)
{
    
    void* exception_ptr = nullptr;
    
    bool result = ParserStatic::parser_is_grammar_failed(
            this_handle, 
            exception_handler_out(&exception_ptr));
    
    check_exceptions(exception_handle, exception_ptr);
    
    return result;
    
}

//
//  csc_Parser_is_source_loaded
//  ---------------------------
//  
//  Check whether the parser has a source loaded.
//

extern "C" _declspec(dllexport)
bool csc_Parser_is_source_loaded(ptrdiff_t this_handle, 
                                 LPVOID exception_handle)
{
    
    void* exception_ptr = nullptr;
    
    bool result = ParserStatic::parser_is_source_loaded(
            this_handle, 
            exception_handler_out(&exception_ptr));
    
    check_exceptions(exception_handle, exception_ptr);
    
    return result;
    
}

//
//  csc_Parser_is_source_failed
//  ---------------------------
//  
//  Check whether the parser has a grammer failed.
//

extern "C" _declspec(dllexport)
bool csc_Parser_is_source_failed(ptrdiff_t this_handle, 
                                 LPVOID exception_handle)
{
    
    void* exception_ptr = nullptr;
    
    bool result = ParserStatic::parser_is_source_failed(
            this_handle, 
            exception_handler_out(&exception_ptr));
    
    check_exceptions(exception_handle, exception_ptr);
    
    return result;
    
}

//
//  csc_Parser_generate
//  -------------------
//  
//  Generate a parser from a grammar file.
//

extern "C" _declspec(dllexport)
void csc_Parser_generate(ptrdiff_t this_handle, 
                         LPVOID exception_handle, 
                         LPWSTR source, 
                         LPWSTR kind_map, 
                         int64_t debug_flags)
{
    
    void* exception_ptr = nullptr;
    
    ParserStatic::parser_generate(this_handle, 
                                  exception_handler_out(&exception_ptr), 
                                  string_out(source), 
                                  kind_map_out(kind_map), 
                                  debug_flags);
    
    check_exceptions(exception_handle, exception_ptr);
    
}

//
//  csc_Parser_parse
//  ----------------
//  
//  Parse a source string saving the Ast and error messages.
//

extern "C" _declspec(dllexport)
void csc_Parser_parse(ptrdiff_t this_handle, 
                      LPVOID exception_handle, 
                      LPWSTR source, 
                      int64_t debug_flags)
{
    
    void* exception_ptr = nullptr;
    
    ParserStatic::parser_parse(this_handle, 
                               exception_handler_out(&exception_ptr), 
                               string_out(source), 
                               debug_flags);
    
    check_exceptions(exception_handle, exception_ptr);
    
}

//
//  csc_Parser_get_encoded_ast
//  --------------------------
//  
//  Return the Ast encoded as a string. We use this method to pass entire trees
//  back to the caller to facilitate interlanguage calls.
//

extern "C" _declspec(dllexport)
void csc_Parser_get_encoded_ast(ptrdiff_t this_handle, 
                                LPVOID exception_handle, 
                                LPVOID result_handle)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_get_encoded_ast(this_handle, 
                                         exception_handler_out(&exception_ptr), 
                                         string_result_out(&result_ptr));
    
    check_exceptions(exception_handle, exception_ptr);
    
    string_result_in(result_handle, result_ptr);
    
}

//
//  csc_Parser_get_encoded_kind_map
//  -------------------------------
//  
//  Return the kind map encoded as a string. We use this method to pass the kind
//  map to the caller to facilitate interlanguage calls.
//

extern "C" _declspec(dllexport)
void csc_Parser_get_encoded_kind_map(ptrdiff_t this_handle, 
                                     LPVOID exception_handle, 
                                     LPVOID result_handle)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_get_encoded_kind_map(this_handle, 
                                              exception_handler_out(&exception_ptr), 
                                              string_result_out(&result_ptr));
    
    check_exceptions(exception_handle, exception_ptr);
    
    string_result_in(result_handle, result_ptr);
    
}

//
//  csc_Parser_get_kind
//  -------------------
//  
//  Get the integer code for a given string.
//

extern "C" _declspec(dllexport)
int csc_Parser_get_kind(ptrdiff_t this_handle, 
                        LPVOID exception_handle, 
                        LPWSTR kind_string)
{
    
    void* exception_ptr = nullptr;
    
    int result = ParserStatic::parser_get_kind(this_handle, 
                                               exception_handler_out(&exception_ptr), 
                                               string_out(kind_string));
    
    check_exceptions(exception_handle, exception_ptr);
    
    return result;
    
}

//
//  csc_Parser_get_kind_force
//  -------------------------
//  
//  Get the integer code for a given string. If it doesn't exist then install
//  it.
//

extern "C" _declspec(dllexport)
int csc_Parser_get_kind_force(ptrdiff_t this_handle, 
                              LPVOID exception_handle, 
                              LPWSTR kind_string)
{
    
    void* exception_ptr = nullptr;
    
    int result = ParserStatic::parser_get_kind_force(this_handle, 
                                                     exception_handler_out(&exception_ptr), 
                                                     string_out(kind_string));
    
    check_exceptions(exception_handle, exception_ptr);
    
    return result;
    
}

//
//  csc_Parser_get_kind_string
//  --------------------------
//  
//  Get the text name for a numeric kind code.
//

extern "C" _declspec(dllexport)
void csc_Parser_get_kind_string(ptrdiff_t this_handle, 
                                LPVOID exception_handle, 
                                LPVOID result_handle, 
                                int kind)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_get_kind_string(this_handle, 
                                         exception_handler_out(&exception_ptr), 
                                         string_result_out(&result_ptr), 
                                         kind);
    
    check_exceptions(exception_handle, exception_ptr);
    
    string_result_in(result_handle, result_ptr);
    
}

//
//  csc_Parser_add_error
//  --------------------
//  
//  Add another error to the message list. This is provided so that clients can
//  use the parser message handler for all errors, not just parsing errors.
//

extern "C" _declspec(dllexport)
void csc_Parser_add_error(ptrdiff_t this_handle, 
                          LPVOID exception_handle, 
                          ErrorType error_type, 
                          int64_t location, 
                          LPWSTR short_message, 
                          LPWSTR long_message)
{
    
    void* exception_ptr = nullptr;
    
    ParserStatic::parser_add_error(this_handle, 
                                   exception_handler_out(&exception_ptr), 
                                   error_type, 
                                   location, 
                                   string_out(short_message), 
                                   string_out(long_message));
    
    check_exceptions(exception_handle, exception_ptr);
    
}

//
//  csc_Parser_get_error_count
//  --------------------------
//  
//  Return the number of error messages over the error threshhold.
//

extern "C" _declspec(dllexport)
int csc_Parser_get_error_count(ptrdiff_t this_handle, LPVOID exception_handle)
{
    
    void* exception_ptr = nullptr;
    
    int result = ParserStatic::parser_get_error_count(this_handle, 
                                                      exception_handler_out(&exception_ptr));
    
    check_exceptions(exception_handle, exception_ptr);
    
    return result;
    
}

//
//  csc_Parser_get_warning_count
//  ----------------------------
//  
//  Return the number of error messages under the error threshhold.
//

extern "C" _declspec(dllexport)
int csc_Parser_get_warning_count(ptrdiff_t this_handle, 
                                 LPVOID exception_handle)
{
    
    void* exception_ptr = nullptr;
    
    int result = ParserStatic::parser_get_warning_count(
            this_handle, 
            exception_handler_out(&exception_ptr));
    
    check_exceptions(exception_handle, exception_ptr);
    
    return result;
    
}

//
//  csc_Parser_get_encoded_error_messages
//  -------------------------------------
//  
//  Return the error messages encoded as a string. We use this method to pass
//  entire lists back to the caller to facilitate interlanguage calls.
//

extern "C" _declspec(dllexport)
void csc_Parser_get_encoded_error_messages(ptrdiff_t this_handle, 
                                           LPVOID exception_handle, 
                                           LPVOID result_handle)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_get_encoded_error_messages(this_handle, 
                                                    exception_handler_out(&exception_ptr), 
                                                    string_result_out(&result_ptr));
    
    check_exceptions(exception_handle, exception_ptr);
    
    string_result_in(result_handle, result_ptr);
    
}

//
//  csc_Parser_get_source_list
//  --------------------------
//  
//  Return a source list with embedded messages.
//

extern "C" _declspec(dllexport)
void csc_Parser_get_source_list(ptrdiff_t this_handle, 
                                LPVOID exception_handle, 
                                LPVOID result_handle, 
                                LPWSTR source, 
                                int indent)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_get_source_list(this_handle, 
                                         exception_handler_out(&exception_ptr), 
                                         string_result_out(&result_ptr), 
                                         string_out(source), 
                                         indent);
    
    check_exceptions(exception_handle, exception_ptr);
    
    string_result_in(result_handle, result_ptr);
    
}

//
//  csc_Parser_encode
//  -----------------
//  
//  Create a string encoding of a Parser.
//

extern "C" _declspec(dllexport)
void csc_Parser_encode(ptrdiff_t this_handle, 
                       LPVOID exception_handle, 
                       LPVOID result_handle)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_encode(this_handle, 
                                exception_handler_out(&exception_ptr), 
                                string_result_out(&result_ptr));
    
    check_exceptions(exception_handle, exception_ptr);
    
    string_result_in(result_handle, result_ptr);
    
}

//
//  csc_Parser_decode
//  -----------------
//  
//  Decode a previously created string into a parser
//

extern "C" _declspec(dllexport)
void csc_Parser_decode(ptrdiff_t this_handle, 
                       LPVOID exception_handle, 
                       LPWSTR str, 
                       LPWSTR kind_map)
{
    
    void* exception_ptr = nullptr;
    
    ParserStatic::parser_decode(this_handle, 
                                exception_handler_out(&exception_ptr), 
                                string_out(str), 
                                kind_map_out(kind_map));
    
    check_exceptions(exception_handle, exception_ptr);
    
}

#line 420 "u:\\hoshi\\raw\\CscWrapper.cpp"
#endif // _WIN64
