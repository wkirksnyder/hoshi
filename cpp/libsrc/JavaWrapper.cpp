//
//  JavaWrapper                                                           
//  -----------                                                           
//                                                                        
//  This is glue code to use Hoshi from Java. Java must call C and        
//  each function we wish to access in Java must be here. From this file  
//  we call a language-independent static C++ class (ParserStatic) with   
//  the method we want, and that will in turn make the call into C++.     
//                                                                        
//  These levels of forwarding simplify the coding. With inline functions 
//  the cost should be negligible.                                        
//

#include <codecvt>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <jni.h>
#include "Parser.H"
#include "ParserStatic.H"

using namespace std;
using namespace hoshi;

//
//  Java Symbols                                                     
//  ------------                                                     
//                                                                   
//  These are java symbols we need to access here. We locate them on 
//  initialization.                                                  
//

static jclass global_GrammarError = nullptr;
static jclass global_SourceError = nullptr;
static jclass global_UnknownError = nullptr;

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
//  output here and allow the java client to query them.                
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
//  a thing from a location provided in java.                           
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
//  thing from a location provided in java.                             
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
//  return it to java.                                                  
//

static jstring string_result_in(JNIEnv* env, void* result_handle)
{

    static wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> myconv;

    StringResultStruct* result_ptr = 
        reinterpret_cast<StringResultStruct*>(result_handle);

    u16string str_in = myconv.from_bytes(result_ptr->result_string);

    jstring java_string = env->NewString(static_cast<const jchar*>(str_in.data()),
                                         str_in.length());

    if (java_string == nullptr)
    {
        cerr << "Fatal error: JVM cannot create new string" << endl;
        exit(1);
    }

    return java_string;

}

//
//  check_exceptions                                                     
//  ----------------                                                     
//                                                                       
//  See if the called function found an exception. If so plant it in the 
//  jvm.                                                                 
//

static void check_exceptions(JNIEnv* env, void* exception_handle)
{

    if (exception_handle == nullptr)
    {
        return;
    }

    ExceptionStruct* exception_ptr = 
        reinterpret_cast<ExceptionStruct*>(exception_handle);

    switch (exception_ptr->exception_type)
    {

        case ExceptionType::ExceptionNull:
            return;

        case ExceptionType::ExceptionGrammar:
            env->ThrowNew(global_GrammarError, exception_ptr->exception_string.c_str());
            return;

        case ExceptionType::ExceptionSource:
            env->ThrowNew(global_SourceError, exception_ptr->exception_string.c_str());
            return;

        case ExceptionType::ExceptionUnknown:
            env->ThrowNew(global_UnknownError, exception_ptr->exception_string.c_str());
            return;

    }

}

//
//  string_out                                                            
//  ----------                                                            
//                                                                        
//  Convert a jstring into a C++ string to send to ParserStatic. Java's   
//  String representation is UTF-16. Hoshi uses UTF-8. This function does 
//  the conversion.                                                       
//

static string string_out(JNIEnv* env, jstring str_in)
{

    static wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> myconv;

    jboolean is_copy;
    const jchar *sptr = env->GetStringCritical(str_in, &is_copy);
    
    int length = env->GetStringLength(str_in);
    u16string str_out(length, ' ');
    
    if (sptr == nullptr)
    {
        cerr << "Fatal error: JVM memory exhausted!" << endl;
        exit(1);
    }
    
    for (int i = 0;
         i < length;
         str_out[i] = static_cast<char16_t>(*(sptr + i)), i++);
    
    env->ReleaseStringCritical(str_in, sptr);

    try
    {
        return myconv.to_bytes(str_out);
    }
    catch (...)
    {
        cerr << "Fatal error: Java string is not UTF-16!" << endl;
        exit(1);
    }

}

//
//  kind_map_out                                 
//  ------------                                 
//                                               
//  Convert a marshalled kind_map into C++ form. 
//

static map<string, int> kind_map_out(JNIEnv* env, jstring str_in)
{
    
    map<string, int> result;
    istringstream is(string_out(env, str_in));

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
//  Java_hoshi_Initializer_initialize_1jni
//  --------------------------------------
//  
//  Initialize anything required by JNI. This is called immediately after
//  loading the dynamic library.
//

extern "C" JNIEXPORT void JNICALL
Java_hoshi_Initializer_initialize_1jni(JNIEnv* env, jclass clazz)
{

    jclass cls = env->FindClass("hoshi/GrammarError");
    if (cls == nullptr)
    {
        cerr << "Fatal error: Cannot find GrammarError in JVM!" << endl;
        exit(1);
    }
    
    global_GrammarError = static_cast<jclass>(env->NewGlobalRef(cls));
    if (global_GrammarError == nullptr)
    {
        cerr << "Fatal error: JVM memory exhausted!" << endl;
        exit(1);
    }
    
    env->DeleteLocalRef(cls);
    cls = nullptr;
    
    cls = env->FindClass("hoshi/SourceError");
    if (cls == nullptr)
    {
        cerr << "Fatal error: Cannot find SourceError in JVM!" << endl;
        exit(1);
    }
    
    global_SourceError = static_cast<jclass>(env->NewGlobalRef(cls));
    if (global_SourceError == nullptr)
    {
        cerr << "Fatal error: JVM memory exhausted!" << endl;
        exit(1);
    }
    
    env->DeleteLocalRef(cls);
    cls = nullptr;
    
    cls = env->FindClass("hoshi/UnknownError");
    if (cls == nullptr)
    {
        cerr << "Fatal error: Cannot find UnknownError in JVM!" << endl;
        exit(1);
    }
    
    global_UnknownError = static_cast<jclass>(env->NewGlobalRef(cls));
    if (global_UnknownError == nullptr)
    {
        cerr << "Fatal error: JVM memory exhausted!" << endl;
        exit(1);
    }
    
    env->DeleteLocalRef(cls);
    cls = nullptr;
    
}

//
//  Java_hoshi_Parser_new_1parser
//  -----------------------------
//  
//  Construct a new Parser and return a pointer as a pointer-sized integer.
//  We're pretty open to memory leaks here when called from garbage-collected
//  languages (most higher-level languages). They'll have to explicitly free
//  what they create. Hopefully we'll be able to wrap that in a class so the
//  final client doesn't have to keep track of it.
//

extern "C" JNIEXPORT ptrdiff_t JNICALL
Java_hoshi_Parser_new_1parser(JNIEnv* env, jclass clazz)
{
    return ParserStatic::parser_new_parser();
}

//
//  Java_hoshi_Parser_clone_1parser
//  -------------------------------
//  
//  Copy new Parser and return a pointer as a pointer-sized integer. This is
//  basically a call to the C++ copy constructor.
//

extern "C" JNIEXPORT ptrdiff_t JNICALL
Java_hoshi_Parser_clone_1parser(JNIEnv* env, 
                                jclass clazz, 
                                ptrdiff_t parser_handle)
{
    return ParserStatic::parser_clone_parser(parser_handle);
}

//
//  Java_hoshi_Parser_delete_1parser
//  --------------------------------
//  
//  Delete a parser. This just calls C++ delete. For garbage collected languages
//  this should probably be in the finalizer.
//

extern "C" JNIEXPORT void JNICALL
Java_hoshi_Parser_delete_1parser(JNIEnv* env, 
                                 jclass clazz, 
                                 ptrdiff_t parser_handle)
{
    ParserStatic::parser_delete_parser(parser_handle);
}

//
//  Java_hoshi_Parser_is_1grammar_1loaded
//  -------------------------------------
//  
//  Check whether the parser has a grammer loaded.
//

extern "C" JNIEXPORT bool JNICALL
Java_hoshi_Parser_is_1grammar_1loaded(JNIEnv* env, 
                                      jclass clazz, 
                                      ptrdiff_t this_handle)
{
    
    void* exception_ptr = nullptr;
    
    return ParserStatic::parser_is_grammar_loaded(this_handle, 
                                                  exception_handler_out(&exception_ptr));
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_is_1grammar_1failed
//  -------------------------------------
//  
//  Check whether the parser has a failed grammar.
//

extern "C" JNIEXPORT bool JNICALL
Java_hoshi_Parser_is_1grammar_1failed(JNIEnv* env, 
                                      jclass clazz, 
                                      ptrdiff_t this_handle)
{
    
    void* exception_ptr = nullptr;
    
    return ParserStatic::parser_is_grammar_failed(this_handle, 
                                                  exception_handler_out(&exception_ptr));
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_is_1source_1loaded
//  ------------------------------------
//  
//  Check whether the parser has a source loaded.
//

extern "C" JNIEXPORT bool JNICALL
Java_hoshi_Parser_is_1source_1loaded(JNIEnv* env, 
                                     jclass clazz, 
                                     ptrdiff_t this_handle)
{
    
    void* exception_ptr = nullptr;
    
    return ParserStatic::parser_is_source_loaded(this_handle, 
                                                 exception_handler_out(&exception_ptr));
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_is_1source_1failed
//  ------------------------------------
//  
//  Check whether the parser has a grammer failed.
//

extern "C" JNIEXPORT bool JNICALL
Java_hoshi_Parser_is_1source_1failed(JNIEnv* env, 
                                     jclass clazz, 
                                     ptrdiff_t this_handle)
{
    
    void* exception_ptr = nullptr;
    
    return ParserStatic::parser_is_source_failed(this_handle, 
                                                 exception_handler_out(&exception_ptr));
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_generate
//  --------------------------
//  
//  Generate a parser from a grammar file.
//

extern "C" JNIEXPORT void JNICALL
Java_hoshi_Parser_generate(JNIEnv* env, 
                           jclass clazz, 
                           ptrdiff_t this_handle, 
                           jstring source, 
                           jstring kind_map, 
                           int64_t debug_flags)
{
    
    void* exception_ptr = nullptr;
    
    ParserStatic::parser_generate(this_handle, 
                                  exception_handler_out(&exception_ptr), 
                                  string_out(env, source), 
                                  kind_map_out(env, kind_map), 
                                  debug_flags);
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_parse
//  -----------------------
//  
//  Parse a source string saving the Ast and error messages.
//

extern "C" JNIEXPORT void JNICALL
Java_hoshi_Parser_parse(JNIEnv* env, 
                        jclass clazz, 
                        ptrdiff_t this_handle, 
                        jstring source, 
                        int64_t debug_flags)
{
    
    void* exception_ptr = nullptr;
    
    ParserStatic::parser_parse(this_handle, 
                               exception_handler_out(&exception_ptr), 
                               string_out(env, source), 
                               debug_flags);
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_get_1encoded_1ast
//  -----------------------------------
//  
//  Return the Ast encoded as a string. We use this method to pass entire trees
//  back to the caller to facilitate interlanguage calls.
//

extern "C" JNIEXPORT jstring JNICALL
Java_hoshi_Parser_get_1encoded_1ast(JNIEnv* env, 
                                    jclass clazz, 
                                    ptrdiff_t this_handle)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_get_encoded_ast(this_handle, 
                                         exception_handler_out(&exception_ptr), 
                                         string_result_out(&result_ptr));
    
    check_exceptions(env, exception_ptr);
    
    return string_result_in(env, result_ptr);
    
}

//
//  Java_hoshi_Parser_get_1encoded_1kind_1map
//  -----------------------------------------
//  
//  Return the kind map encoded as a string. We use this method to pass the kind
//  map to the caller to facilitate interlanguage calls.
//

extern "C" JNIEXPORT jstring JNICALL
Java_hoshi_Parser_get_1encoded_1kind_1map(JNIEnv* env, 
                                          jclass clazz, 
                                          ptrdiff_t this_handle)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_get_encoded_kind_map(this_handle, 
                                              exception_handler_out(&exception_ptr), 
                                              string_result_out(&result_ptr));
    
    check_exceptions(env, exception_ptr);
    
    return string_result_in(env, result_ptr);
    
}

//
//  Java_hoshi_Parser_get_1kind
//  ---------------------------
//  
//  Get the integer code for a given string.
//

extern "C" JNIEXPORT int JNICALL
Java_hoshi_Parser_get_1kind(JNIEnv* env, 
                            jclass clazz, 
                            ptrdiff_t this_handle, 
                            jstring kind_string)
{
    
    void* exception_ptr = nullptr;
    
    return ParserStatic::parser_get_kind(this_handle, 
                                         exception_handler_out(&exception_ptr), 
                                         string_out(env, kind_string));
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_get_1kind_1force
//  ----------------------------------
//  
//  Get the integer code for a given string. If it doesn't exist then install
//  it.
//

extern "C" JNIEXPORT int JNICALL
Java_hoshi_Parser_get_1kind_1force(JNIEnv* env, 
                                   jclass clazz, 
                                   ptrdiff_t this_handle, 
                                   jstring kind_string)
{
    
    void* exception_ptr = nullptr;
    
    return ParserStatic::parser_get_kind_force(this_handle, 
                                               exception_handler_out(&exception_ptr), 
                                               string_out(env, kind_string));
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_get_1kind_1string
//  -----------------------------------
//  
//  Get the text name for a numeric kind code.
//

extern "C" JNIEXPORT jstring JNICALL
Java_hoshi_Parser_get_1kind_1string(JNIEnv* env, 
                                    jclass clazz, 
                                    ptrdiff_t this_handle, 
                                    int kind)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_get_kind_string(this_handle, 
                                         exception_handler_out(&exception_ptr), 
                                         string_result_out(&result_ptr), 
                                         kind);
    
    check_exceptions(env, exception_ptr);
    
    return string_result_in(env, result_ptr);
    
}

//
//  Java_hoshi_Parser_add_1error
//  ----------------------------
//  
//  Add another error to the message list. This is provided so that clients can
//  use the parser message handler for all errors, not just parsing errors.
//

extern "C" JNIEXPORT void JNICALL
Java_hoshi_Parser_add_1error(JNIEnv* env, 
                             jclass clazz, 
                             ptrdiff_t this_handle, 
                             ErrorType error_type, 
                             int64_t location, 
                             jstring short_message, 
                             jstring long_message)
{
    
    void* exception_ptr = nullptr;
    
    ParserStatic::parser_add_error(this_handle, 
                                   exception_handler_out(&exception_ptr), 
                                   error_type, 
                                   location, 
                                   string_out(env, short_message), 
                                   string_out(env, long_message));
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_get_1error_1count
//  -----------------------------------
//  
//  Return the number of error messages over the error threshhold.
//

extern "C" JNIEXPORT int JNICALL
Java_hoshi_Parser_get_1error_1count(JNIEnv* env, 
                                    jclass clazz, 
                                    ptrdiff_t this_handle)
{
    
    void* exception_ptr = nullptr;
    
    return ParserStatic::parser_get_error_count(this_handle, 
                                                exception_handler_out(&exception_ptr));
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_get_1warning_1count
//  -------------------------------------
//  
//  Return the number of error messages under the error threshhold.
//

extern "C" JNIEXPORT int JNICALL
Java_hoshi_Parser_get_1warning_1count(JNIEnv* env, 
                                      jclass clazz, 
                                      ptrdiff_t this_handle)
{
    
    void* exception_ptr = nullptr;
    
    return ParserStatic::parser_get_warning_count(this_handle, 
                                                  exception_handler_out(&exception_ptr));
    
    check_exceptions(env, exception_ptr);
    
}

//
//  Java_hoshi_Parser_get_1encoded_1error_1messages
//  -----------------------------------------------
//  
//  Return the error messages encoded as a string. We use this method to pass
//  entire trees back to the caller to facilitate interlanguage calls.
//

extern "C" JNIEXPORT jstring JNICALL
Java_hoshi_Parser_get_1encoded_1error_1messages(JNIEnv* env, 
                                                jclass clazz, 
                                                ptrdiff_t this_handle)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_get_encoded_error_messages(this_handle, 
                                                    exception_handler_out(&exception_ptr), 
                                                    string_result_out(&result_ptr));
    
    check_exceptions(env, exception_ptr);
    
    return string_result_in(env, result_ptr);
    
}

//
//  Java_hoshi_Parser_get_1source_1list
//  -----------------------------------
//  
//  Return a source list with embedded messages.
//

extern "C" JNIEXPORT jstring JNICALL
Java_hoshi_Parser_get_1source_1list(JNIEnv* env, 
                                    jclass clazz, 
                                    ptrdiff_t this_handle, 
                                    jstring source, 
                                    int indent)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_get_source_list(this_handle, 
                                         exception_handler_out(&exception_ptr), 
                                         string_result_out(&result_ptr), 
                                         string_out(env, source), 
                                         indent);
    
    check_exceptions(env, exception_ptr);
    
    return string_result_in(env, result_ptr);
    
}

//
//  Java_hoshi_Parser_encode
//  ------------------------
//  
//  Create a string encoding of a Parser.
//

extern "C" JNIEXPORT jstring JNICALL
Java_hoshi_Parser_encode(JNIEnv* env, jclass clazz, ptrdiff_t this_handle)
{
    
    void* exception_ptr = nullptr;
    void* result_ptr = nullptr;
    
    ParserStatic::parser_encode(this_handle, 
                                exception_handler_out(&exception_ptr), 
                                string_result_out(&result_ptr));
    
    check_exceptions(env, exception_ptr);
    
    return string_result_in(env, result_ptr);
    
}

//
//  Java_hoshi_Parser_decode
//  ------------------------
//  
//  Decode a previously created string into a parser
//

extern "C" JNIEXPORT void JNICALL
Java_hoshi_Parser_decode(JNIEnv* env, 
                         jclass clazz, 
                         ptrdiff_t this_handle, 
                         jstring str, 
                         jstring kind_map)
{
    
    void* exception_ptr = nullptr;
    
    ParserStatic::parser_decode(this_handle, 
                                exception_handler_out(&exception_ptr), 
                                string_out(env, str), 
                                kind_map_out(env, kind_map));
    
    check_exceptions(env, exception_ptr);
    
}


