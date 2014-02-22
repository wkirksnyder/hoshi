#line 2 "u:\\hoshi\\raw\\ParserImpl.cpp"
//
//  ParserImpl                                                             
//  ----------                                                             
//                                                                         
//  This is the counterpart to Parser. It contains all the implementation  
//  details that we would like to hide from clients.                       
//                                                                         
//  Note that the Parser class is very state-dependent. It is initialized  
//  in an invalid state. We generate the parser via a source file, which   
//  either yields a parser or an error list. If there are no errors we can 
//  parse a source file in the described language.                         
//                                                                         
//  This class is basically a facade. If we have finished generating a     
//  parser and want to parse a source stream we call ParserEngine. If we   
//  want to generate a parser we call a sequence of helper classes to do   
//  the real work.                                                         
//

#ifndef PARSER_IMPL_H
#define PARSER_IMPL_H

#include <cstdint>
#include <memory>
#include <exception>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "OpcodeType.H"
#include "LibraryToken.H"
#include "ErrorHandler.H"
#include "ParserData.H"
#include "ParserEngine.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

//
//  ParserState                                                            
//  -----------                                                            
//                                                                         
//  The parser has several external entry points which modify the internal 
//  state and prepare it for different operations which follow. This is    
//  how we keep track of that state.                                       
//

enum ParserState
{
    Invalid,
    KindMapGood,
    GrammarGood,
    GrammarBad,
    SourceGood,
    SourceBad
};

//
//  ParserImpl             
//  ----------             
//                         
//  The ParserImpl proper. 
//

class ParserImpl final
{
public:

    //
    //  Copy control. 
    //

    ParserImpl() = default;
    ~ParserImpl();

    ParserImpl(const ParserImpl&);
    ParserImpl(ParserImpl&&) noexcept;
    ParserImpl& operator=(const ParserImpl&);
    ParserImpl& operator=(ParserImpl&&) noexcept;

    //
    //  Decode the prior generation parsers so we can parse incoming 
    //  grammars.                                                    
    //

    static void initialize();

    //
    //  State queries. 
    //

    bool is_grammar_loaded();
    bool is_grammar_failed();
    bool is_source_loaded();
    bool is_source_failed();

    //
    //  Result generators.
    //

    void set_kind_map(const std::map<std::string, int>& kind_map);

    void generate(Ast* ast,
                  const Source& src,
                  const std::map<std::string, int>& kind_map = kind_map_missing,
                  const int64_t debug_flags = 0);  

    Ast* generate_ast(const Source& src,
                      const int64_t debug_flags = 0);

    void generate(const Source& src,
                  const std::map<std::string, int>& kind_map = kind_map_missing,
                  const int64_t debug_flags = 0);  

    void parse(const Source& src, const int64_t debug_flags = 0);

    //
    //  Result accessors and error message utilities. 
    //

    std::map<std::string, int> get_kind_map() const;
    std::string get_encoded_kind_map() const;

    Ast* get_ast() const;
    std::string get_encoded_ast() const;

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
    std::string get_encoded_error_messages();

    void dump_source(const Source& src,
                     std::ostream& os = std::cout,
                     int indent = 0) const;

    std::string get_source_list(const Source& src, int indent = 0) const;

    //
    //  Encode and decode. 
    //

    void export_cpp(std::string file_name, std::string identifier) const;

    std::string encode() const;

    void decode(const std::string& str,
                const std::map<std::string, int>& kind_map = kind_map_missing);

    //
    //  Log utilities. 
    //

    void log_heading(const std::string& heading,
                     std::ostream& os = std::cout,
                     int indent = 0) const;

    void start_timer();
    std::string elapsed_time_string();

    std::string get_grammar_kind_string(int kind) const;
    void dump_grammar_ast(Ast* ast,
                          std::ostream& os = std::cout,
                          int indent = 0) const;

    Ast* parse_library_regex(const Source& src, int64_t debug_flags = 0);

private:

    static std::map<std::string, int> kind_map_missing;
    static std::string string_missing;

    ParserState state = ParserState::Invalid;
    ParserData* prsd = nullptr;
    ErrorHandler* errh = nullptr;
    Ast* ast = nullptr;

    static ParserData* grammar_parser_data;
    static ParserData* regex_parser_data;
    static ParserData* charset_parser_data;

    std::chrono::high_resolution_clock::time_point timer;

    void adjust_location(Ast* root, int64_t adjustment);
    void get_source_string(Ast* root, std::string& source, int64_t& adjustment);
    void expand_subtrees(Ast* root, bool& any_changes, int64_t debug_flags);

    static void encode_long(std::ostream& os, int64_t value);
    static int64_t decode_long(std::istream& is);
    static void encode_string(std::ostream& os, const std::string& value);
    static std::string decode_string(std::istream& is);
    static void encode_ast(std::ostream& os, const Ast* ast);

};

} // namespace hoshi

#endif // PARSER_IMPL_H
