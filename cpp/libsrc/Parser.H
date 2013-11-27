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

#ifndef HOSHI_PARSER_H
#define HOSHI_PARSER_H

#include <cstdint>
#include <memory>
#include <exception>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

//
//  Forward declarations. 
//

class ParserImpl;
class Ast;

//
//  Exceptions                                                             
//  ----------                                                             
//                                                                         
//  We will indicate the presence of errors through exceptions but the     
//  client will have to examine an error message to get more details about 
//  the errors found.                                                      
//

class GrammarError : public std::exception
{
public:

    GrammarError(const std::string& message) : message(message) {}
    const char* what() { return message.data(); }

private:

    std::string message;

};

class SourceError : public std::exception
{
public:

    SourceError(const std::string& message) : message(message) {}
    const char* what() { return message.data(); }

private:

    std::string message;

};

//
//  DebugFlags                                                       
//  ----------                                                       
//                                                                   
//  We allow quite a few debugging options. It's the only way to get 
//  through such a large library.                                    
//

enum DebugFlags : int64_t
{

    DebugProgress       = 1 <<   0,
    DebugAstHandlers    = 1 <<   1,
    DebugGrammar        = 1 <<   2,
    DebugGrammarAst     = 1 <<   3,
    DebugLalr           = 1 <<   4,
    DebugScanner        = 1 <<   5,
    DebugActions        = 1 <<   6,
    DebugICode          = 1 <<   7,
    DebugVCodeExec      = 1 <<   8,
    DebugScanToken      = 1 <<   9,
    DebugParseAction    = 1 <<  10

};

//
//  ErrorType                                                              
//  ---------                                                              
//                                                                         
//  We'll encode each error message with an enumerated type. For now we're 
//  not going to do anything with this but at some point we may want to    
//  add options like classifying them as warnings or errors, disabling and 
//  so forth.                                                              
//

enum ErrorType : int
{
    ErrorMinimum           =   0,
    ErrorMaximum           =  20,
    ErrorError             =   0,
    ErrorWarning           =   1,
    ErrorUnknownMacro      =   2,
    ErrorDupGrammarOption  =   3,
    ErrorDupToken          =   4,
    ErrorDupTokenOption    =   5,
    ErrorUnusedTerm        =   6,
    ErrorUndefinedNonterm  =   7,
    ErrorUnusedNonterm     =   8,
    ErrorUselessNonterm    =   9,
    ErrorUselessRule       =  10,
    ErrorReadsCycle        =  11,
    ErrorSymbolSelfProduce =  12,
    ErrorLalrConflict      =  13,
    ErrorWordOverflow      =  14,
    ErrorCharacterRange    =  15,
    ErrorRegexConflict     =  16,
    ErrorDupAstItem        =  17,
    ErrorSyntax            =  18,
    ErrorLexical           =  19,
    ErrorAstIndex          =  20
};

//
//  Source                                                                 
//  ------                                                                 
//                                                                         
//  This class is an abstraction that provides source text to the rest of  
//  the program. The remainder of the program should generally assume that 
//  when it sees an individual character it is in UCS-4/UTF-32, but        
//  strings are always returned as UTF-8. We're also including a few       
//  utilities here for handling those characters and strings. The          
//  implementation of this class needs work, but if we can maintain the    
//  abstraction we should be able to polish it eventually without changing 
//  the remainder of the program.                                          
//

class Source
{
public:

    Source(const std::string& str = "");

    //
    //  Text & character utilities. 
    //

    static const char32_t eof_char = 1 << 31;

    static int64_t char_length(const std::string& str);
    static std::string to_ascii_chop(const std::string& str);
    static std::string to_ascii_cpp(const std::string& str);
    static std::u32string to_utf32(const std::string& str);

    //
    //  Accessors. 
    //

    int64_t length() const;
    char32_t get_char(int64_t location) const;
    std::string get_string(int64_t first, int64_t last) const;
    void get_source_position(int64_t location, 
                             int64_t& line_num,
                             int64_t& column_num,
                             std::string& line) const;

    //
    //  Copying is going to be expensive but we can allow it.
    //

    ~Source() = default;
    Source(const Source&) = default;
    Source(Source&&) = default;
    Source& operator=(const Source&) = default;
    Source& operator=(Source&&) = default;

protected:

    std::u32string source;

};

//
//  SourceFile                                                           
//  ----------                                                           
//                                                                       
//  This subclass must be passed a file name to read. We will assume the 
//  file is in UTF-8.                                                    
//

class SourceFile : public Source
{
public:

    explicit SourceFile(const std::string& file_name);

    ~SourceFile() = default;
    SourceFile(const SourceFile&) = default;
    SourceFile(SourceFile&&) = default;
    SourceFile& operator=(const SourceFile&) = default;
    SourceFile& operator=(SourceFile&&) = default;

};

//
//  ErrorMessage                                                      
//  ------------                                                      
//                                                                    
//  Both the parser generator and the resulting parser should return  
//  errors in this form.                                              
//                                                                    

class ErrorMessage
{

    friend class ErrorHandler;

public:

    ErrorType get_type() const;
    std::string get_tag() const;
    int get_severity() const;
    int64_t get_location() const;
    int64_t get_line_num() const;
    int64_t get_column_num() const;
    std::string get_source_line() const;
    std::string get_short_message() const;
    std::string get_long_message() const;
    std::string get_string() const;

private:

    ErrorType error_type;
    int64_t location;
    int64_t line_num;
    int64_t column_num;
    std::string source_line;
    std::string short_message;
    std::string long_message;

};

//
//  Parser                                                                 
//  ------                                                                 
//                                                                         
//  The Parser proper. Note that this can be copied freely and             
//  inexpensively, and should be if you want to use it in multiple         
//  threads. A single parser is not re-entrant, but can share parse tables 
//  with other instances.                                                  
//

class Parser final
{
public:

    //
    //  Static initializer. 
    //

    static void initialize();

    //
    //  Copy control. 
    //

    Parser();
    ~Parser();

    Parser(const Parser&);
    Parser(Parser&&) noexcept;
    Parser& operator=(const Parser&);
    Parser& operator=(Parser&&) noexcept;

    //
    //  State queries. 
    //

    bool is_grammar_loaded();
    bool is_grammar_failed();
    bool is_parse_successful();
    bool is_parse_failed();

    //
    //  Result generators.
    //

    void set_ast_kind_map(const std::map<std::string, int>& ast_kind_map);

    void generate(Ast* ast,
                  const Source& src,
                  const std::map<std::string, int>& ast_kind_map = kind_map_missing,
                  const int64_t debug_flags = 0);  

    Ast* generate_ast(const Source& src,
                      const int64_t debug_flags = 0);

    void generate(const Source& src,
                  const std::map<std::string, int>& ast_kind_map = kind_map_missing,
                  const int64_t debug_flags = 0);  

    void parse(const Source& src, const int64_t debug_flags = 0);

    //
    //  Result accessors and error message utilities. 
    //

    std::map<std::string, int> get_ast_kind_map() const;

    Ast* get_ast();

    void dump_ast(Ast* ast,
                  std::ostream& os = std::cout,
                  int indent = 0) const;

    int get_kind(const std::string& kind_str) const;
    int get_kind_force(const std::string& kind_str);
    std::string get_kind_string(int kind) const;
    std::string get_kind_string(const Ast* root) const;

    void add_error(ErrorType error_type,
                   int64_t location,
                   const std::string& short_message,
                   const std::string& long_message = string_missing);

    int get_error_count() const;
    int get_warning_count() const;
    std::vector<ErrorMessage> get_error_messages();
    void dump_source(const Source& src,
                     std::ostream& os = std::cout,
                     int indent = 0) const;

    //
    //  Encode and decode. 
    //

    void export_cpp(std::string file_name, std::string identifier) const;

    std::string encode() const;

    void decode(const std::string& str,
                const std::map<std::string, int>& ast_kind_map);

private:

    static std::map<std::string, int> kind_map_missing;
    static std::string string_missing;

    ParserImpl *impl = nullptr;

};

//
//  Ast (Abstract Syntax Tree)                                          
//  --------------------------                                          
//                                                                      
//  An abstract syntax tree holds the important syntactic elements from 
//  the source in an easily traversable form.                           
//                                                                      
//  This class has a lot of naked pointers. The convention is that a    
//  parent owns all its children. When we delete a node we delete the   
//  entire subtree. If we need to copy a pointer and claim ownership we 
//  must clone the subtree.                                             
//

class Ast final
{

    friend class ParserEngine;

public:

    Ast(int num_children);
    ~Ast();

    Ast(const Ast&) = delete;
    Ast(Ast&&) = delete;
    Ast& operator=(const Ast&) = delete;
    Ast& operator=(Ast&&) = delete;

    int get_kind() const;
    void set_kind(int kind);

    int64_t get_location() const;
    void set_location(int64_t location);

    std::string get_lexeme() const;
    void set_lexeme(const std::string& lexeme);

    Ast* get_parent() const;

    int get_num_children() const;
    Ast* get_child(int index) const;
    void set_child(int index, Ast* ast);

    Ast* clone() const;

    static void encode_cpp(const Ast* root,
                           const Source* src,
                           const Parser& parser,
                           const std::string& file_name,
                           const std::string& identifier);

    static void decode_cpp(Ast*& root,
                           Source*& src,
                           const Parser& parser,
                           const char* item[]);

private:

    int kind = 0;
    int64_t location = -1;
    std::string lexeme = "";
    Ast* parent = nullptr;
    int num_children = 0;
    Ast** children = nullptr;

};

} // namespace hoshi

#endif // HOSHI_PARSER_H
