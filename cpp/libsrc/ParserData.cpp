//
//  ParserData                                                             
//  ----------                                                             
//                                                                         
//  Final generated parsing data. Generating a parser is a slow operation  
//  that we don't want to repeat unnecessarily. But once the parser is     
//  generated we can share the data or store it in other forms. That's     
//  what this class is all about.                                          
//                                                                         
//  We provide all the parse tables and copy control where objects of this 
//  class can be shared by many parsers that recognize the same source     
//  language.                                                              
//                                                                         
//  Most of the code here is used to marshall or unmarshall the object as  
//  a string. I'm not sure this capability will be used by clients but I   
//  need it for bootstrapping.                                             
//

#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "OpcodeType.H"
#include "Parser.H"
#include "ParserImpl.H"
#include "ParserData.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;

//
//  Missing argument flag. 
//

map<string, int> ParserData::kind_map_missing;

//
//  wiring tables                                
//  -------------                                
//                                               
//  Tables that help us route block types to handlers. 
//

ParserData::EncodeHandler ParserData::encode_handler[] =
{
    handle_encode_version,                // Version
    handle_encode_kind_map,               // KindMap
    handle_encode_source,                 // Source
    handle_encode_lookaheads,             // Lookaheads
    handle_encode_error_recovery,         // ErrorRecovery
    handle_encode_error_symbol_num,       // ErrorSymbolNum
    handle_encode_eof_symbol_num,         // EofSymbolNum
    handle_encode_token_count,            // TokenCount
    handle_encode_token_name_list,        // TokenNameList
    handle_encode_token_is_terminal,      // TokenIsTerminal
    handle_encode_token_kind,             // TokenKind
    handle_encode_token_lexeme_needed,    // TokenLexemeNeeded
    handle_encode_rule_count,             // RuleCount
    handle_encode_rule_size,              // RuleSize
    handle_encode_rule_lhs,               // RuleLhs
    handle_encode_rule_text,              // RuleText
    handle_encode_rule_pc,                // RulePc
    handle_encode_scanner_pc,             // ScannerPc
    handle_encode_start_state,            // StartState
    handle_encode_restart_state,          // RestartState
    handle_encode_checked_index_count,    // CheckedIndexCount
    handle_encode_checked_index,          // CheckedIndex
    handle_encode_checked_data_count,     // CheckedDataCount
    handle_encode_checked_data,           // CheckedData
    handle_encode_num_offsets,            // NumOffsets
    handle_encode_symbol_num_offset,      // SymbolNumOffset
    handle_encode_symbol_num_shift,       // SymbolNumShift
    handle_encode_symbol_num_mask,        // SymbolNumMask
    handle_encode_action_type_offset,     // ActionTypeOffset
    handle_encode_action_type_shift,      // ActionTypeShift
    handle_encode_action_type_mask,       // ActionTypeMask
    handle_encode_rule_num_offset,        // RuleNumOffset
    handle_encode_rule_num_shift,         // RuleNumShift
    handle_encode_rule_num_mask,          // RuleNumMask
    handle_encode_state_num_offset,       // StateNumOffset
    handle_encode_state_num_shift,        // StateNumShift
    handle_encode_state_num_mask,         // StateNumMask
    handle_encode_fallback_num_offset,    // FallbackNumOffset
    handle_encode_fallback_num_shift,     // FallbackNumShift
    handle_encode_fallback_num_mask,      // FallbackNumMask
    handle_encode_opcode_map,             // OpcodeMap
    handle_encode_instruction_count,      // InstructionCount
    handle_encode_operand_count,          // OperandCount
    handle_encode_instruction_list,       // InstructionList
    handle_encode_register_count,         // RegisterCount
    handle_encode_register_list,          // RegisterList
    handle_encode_ast_count,              // AstCount
    handle_encode_string_count,           // StringCount
    handle_encode_string_list,            // StringList
    handle_encode_eof                     // Eof
};

ParserData::DecodeHandler ParserData::decode_handler[] =
{
    handle_decode_version,                // Version
    handle_decode_kind_map,               // KindMap
    handle_decode_source,                 // Source
    handle_decode_lookaheads,             // Lookaheads
    handle_decode_error_recovery,         // ErrorRecovery
    handle_decode_error_symbol_num,       // ErrorSymbolNum
    handle_decode_eof_symbol_num,         // EofSymbolNum
    handle_decode_token_count,            // TokenCount
    handle_decode_token_name_list,        // TokenNameList
    handle_decode_token_is_terminal,      // TokenIsTerminal
    handle_decode_token_kind,             // TokenKind
    handle_decode_token_lexeme_needed,    // TokenLexemeNeeded
    handle_decode_rule_count,             // RuleCount
    handle_decode_rule_size,              // RuleSize
    handle_decode_rule_lhs,               // RuleLhs
    handle_decode_rule_text,              // RuleText
    handle_decode_rule_pc,                // RulePc
    handle_decode_scanner_pc,             // ScannerPc
    handle_decode_start_state,            // StartState
    handle_decode_restart_state,          // RestartState
    handle_decode_checked_index_count,    // CheckedIndexCount
    handle_decode_checked_index,          // CheckedIndex
    handle_decode_checked_data_count,     // CheckedDataCount
    handle_decode_checked_data,           // CheckedData
    handle_decode_num_offsets,            // NumOffsets
    handle_decode_symbol_num_offset,      // SymbolNumOffset
    handle_decode_symbol_num_shift,       // SymbolNumShift
    handle_decode_symbol_num_mask,        // SymbolNumMask
    handle_decode_action_type_offset,     // ActionTypeOffset
    handle_decode_action_type_shift,      // ActionTypeShift
    handle_decode_action_type_mask,       // ActionTypeMask
    handle_decode_rule_num_offset,        // RuleNumOffset
    handle_decode_rule_num_shift,         // RuleNumShift
    handle_decode_rule_num_mask,          // RuleNumMask
    handle_decode_state_num_offset,       // StateNumOffset
    handle_decode_state_num_shift,        // StateNumShift
    handle_decode_state_num_mask,         // StateNumMask
    handle_decode_fallback_num_offset,    // FallbackNumOffset
    handle_decode_fallback_num_shift,     // FallbackNumShift
    handle_decode_fallback_num_mask,      // FallbackNumMask
    handle_decode_opcode_map,             // OpcodeMap
    handle_decode_instruction_count,      // InstructionCount
    handle_decode_operand_count,          // OperandCount
    handle_decode_instruction_list,       // InstructionList
    handle_decode_register_count,         // RegisterCount
    handle_decode_register_list,          // RegisterList
    handle_decode_ast_count,              // AstCount
    handle_decode_string_count,           // StringCount
    handle_decode_string_list,            // StringList
    handle_decode_eof                     // Eof
};

//
//  Block descriptive names.
//

string ParserData::block_name[] =
{
    "Version",
    "KindMap",
    "Source",
    "Lookaheads",
    "ErrorRecovery",
    "ErrorSymbolNum",
    "EofSymbolNum",
    "TokenCount",
    "TokenNameList",
    "TokenIsTerminal",
    "TokenKind",
    "TokenLexemeNeeded",
    "RuleCount",
    "RuleSize",
    "RuleLhs",
    "RuleText",
    "RulePc",
    "ScannerPc",
    "StartState",
    "RestartState",
    "CheckedIndexCount",
    "CheckedIndex",
    "CheckedDataCount",
    "CheckedData",
    "NumOffsets",
    "SymbolNumOffset",
    "SymbolNumShift",
    "SymbolNumMask",
    "ActionTypeOffset",
    "ActionTypeShift",
    "ActionTypeMask",
    "RuleNumOffset",
    "RuleNumShift",
    "RuleNumMask",
    "StateNumOffset",
    "StateNumShift",
    "StateNumMask",
    "FallbackNumOffset",
    "FallbackNumShift",
    "FallbackNumMask",
    "OpcodeMap",
    "InstructionCount",
    "OperandCount",
    "InstructionList",
    "RegisterCount",
    "RegisterList",
    "AstCount",
    "StringCount",
    "StringList",
    "Eof" 
};

//
//  ~ParserData()                              
//  -------------                              
//                                             
//  Called when the reference count reaches 0. 
//

ParserData::~ParserData()
{

    delete [] token_name_list;
    token_name_list = nullptr;

    delete [] token_is_terminal;
    token_is_terminal = nullptr;

    delete [] token_kind;
    token_kind = nullptr;

    delete [] token_lexeme_needed;
    token_lexeme_needed = nullptr;

    delete [] rule_size;
    rule_size = nullptr;

    delete [] rule_lhs;
    rule_lhs = nullptr;

    delete [] rule_text;
    rule_text = nullptr;

    delete [] rule_pc;
    rule_pc = nullptr;

    delete [] checked_index;
    checked_index = nullptr;

    delete [] checked_data;
    checked_data = nullptr;

    delete [] instruction_list;
    instruction_list = nullptr;

    delete [] operand_list;
    operand_list = nullptr;

    delete [] register_list;
    register_list = nullptr;

    delete [] string_list;
    string_list = nullptr;

}

//
//  attach                                             
//  ------                                             
//                                                     
//  Attach a parser implementation to the parser data. 
//

void ParserData::attach(ParserData*& prsd)
{

    if (prsd != nullptr)
    {

        lock_guard<mutex> reference_guard(prsd->reference_mutex);

        ++prsd->reference_count;

    }

}

//
//  detach                                                            
//  ------                                                            
//                                                                    
//  A parser is finished with ParserData. We decrement the use count, 
//  delete if necessary and clear the pointer.                        
//

void ParserData::detach(ParserData*& prsd)
{

    if (prsd != nullptr)
    {

        lock_guard<mutex> reference_guard(prsd->reference_mutex);

        if (--prsd->reference_count == 0)
        {
            delete prsd;
        }

        prsd = nullptr;

    }

}

//
//  set_kind_map                                                    
//  ------------                                                    
//                                                                  
//  Initialize the ast kind maps from a map provided by the client. 
//

void ParserData::set_kind_map(const std::map<std::string, int>& kind_map)
{

    this->kind_map.clear();
    this->kind_imap.clear();

    for (auto mp: kind_map)
    {

        if (mp.second < 0)
        {
            throw out_of_range("Ast codes must be >= 0");
        }

        if (this->kind_map.find(mp.first) != this->kind_map.end())
        {
            throw logic_error("Duplicated Ast kind: " + mp.first);
        }

        if (this->kind_imap.find(mp.second) != this->kind_imap.end())
        {
            throw logic_error("Duplicated Ast kind: " + to_string(mp.second));
        }

        this->kind_map[mp.first] = mp.second;
        this->kind_imap[mp.second] = mp.first;

    }

    get_kind_force("Unknown");

}

//
//  get_kind_map                                                          
//  ------------                                                          
//                                                                        
//  Get the kind_map. The client can use this to find out if he forgot to 
//  define any important kind strings.                                    
//

map<string, int> ParserData::get_kind_map() const
{
    return kind_map;
}

//
//  get_kind                                                          
//  --------                                                          
//                                                                    
//  Get the integer code for a given string.
//

int ParserData::get_kind(const string& kind_str) const
{

    if (kind_map.find(kind_str) == kind_map.end())
    {
        return -1;
    }

    return (*kind_map.find(kind_str)).second;

}

//
//  get_kind_force                                                        
//  --------------                                                        
//                                                                        
//  Get the integer code for a given string if we already know it or add
//  it to our kind map if we don't.                                       
//

int ParserData::get_kind_force(const string& kind_str)
{

    if (kind_map.find(kind_str) != kind_map.end())
    {
        return kind_map[kind_str];
    }

    int max_kind = 0;
    for (auto mp: kind_map)
    {

        if (max_kind < mp.second)
        {
            max_kind = mp.second;
        }

    }

    kind_map[kind_str] = max_kind + 1;
    kind_imap[max_kind + 1] = kind_str;

    return kind_map[kind_str];

}

//
//  get_kind_string                       
//  ---------------                       
//                                        
//  Get the text name for a numeric code. 
//

string ParserData::get_kind_string(int kind) const
{

    if (kind_imap.find(kind) == kind_imap.end())
    {
        return "Unknown";
    }

    return (*kind_imap.find(kind)).second;

}

//
//  encode                                                             
//  ------                                                             
//                                                                     
//  Encode all the parser data as an ASCII string. Used to parse while 
//  bypassing the generator.                                           
//

string ParserData::encode() const
{

    ostringstream ost;

    for (int block = BlockType::BlockMinimum;
         block <= BlockType::BlockMaximum;
         block++)
    {

        encode_int(block, ost);
        (*encode_handler[block])(*this, static_cast<BlockType>(block), ost);
        ost << block_separator;

    }

    return ost.str();

}

//
//  decode                                      
//  ------                                      
//                                              
//  Decode a string containing all parser data. 
//

void ParserData::decode(const string& str,
                        const map<string, int>& ast_types)
{

    set_kind_map(ast_types);

    ParserTemp temp;
    const char *next = str.data();

    while (next < str.data() + str.length())
    {

        BlockType block = static_cast<BlockType>(decode_int(next));
        if (block == BlockType::BlockEof)
        {
            break;
        }

        (*decode_handler[block])(*this, temp, static_cast<BlockType>(block), next);

        next++;

    }

}

//
//  handle_encode_error
//  -------------------                                                     
//                                                                   
//  This should never be called. It means there is a path we haven't 
//  accomodated. It's not a user error, it's a logic error.          
//

void ParserData::handle_encode_error(const ParserData& prsd,
                                     const BlockType block,
                                     ostream& os)
{
    cout << "No ParserData::encode handler for block "
         << block_name[block] << "!" << endl << endl;
    exit(1);
}

//
//  handle_decode_error                                                  
//  -------------------                                                  
//                                                                       
//  This happens when we don't know how to decode something. Most likely 
//  we've added a block type since the string encoding was created. At   
//  this level we just skip past the block.                              
//

void ParserData::handle_decode_error(ParserData& prsd,
                                     ParserTemp& temp,
                                     const BlockType block,
                                     const char*& next)
{

    while (*next != block_separator)
    {
        next++;
    }
    
}

//
//  handle_*_eof
//  ------------
//                                                                       
//  An eof marker.
//

void ParserData::handle_encode_eof(const ParserData& prsd,
                                   const BlockType block,
                                   ostream& os)
{
}

void ParserData::handle_decode_eof(ParserData& prsd,
                                   ParserTemp& temp,
                                   const BlockType block,
                                   const char*& next)
{
}
        
//
//  handle_*_version                                                       
//  ----------------                                                       
//                                                                       
//  The version is used to tell us when something about the encoded form 
//  of a parser has changed and we can't load it.                        
//

void ParserData::handle_encode_version(const ParserData& prsd,
                                       const BlockType block,
                                       ostream& os)
{
    encode_int(ParserData::current_version, os);
}

void ParserData::handle_decode_version(ParserData& prsd,
                                       ParserTemp& temp,
                                       const BlockType block,
                                       const char*& next)
{

    temp.version = decode_int(next);

    if (temp.version < ParserData::min_supported_version)
    {
        throw out_of_range("Version mismatch in Hoshi library");
    }

}

//
//  handle_*_kind_map                                                     
//  -----------------                                                     
//                                                                        
//  Handle the old ast kind maps. We have to translate these into the new 
//  kind maps.                                                            
//

void ParserData::handle_encode_kind_map(const ParserData& prsd,
                                        const BlockType block,
                                        ostream& os)
{

    for (auto mp: prsd.kind_map)
    {
        encode_string(mp.first, os);
        encode_int(mp.second, os);
    }

}

void ParserData::handle_decode_kind_map(ParserData& prsd,
                                        ParserTemp& temp,
                                        const BlockType block,
                                        const char*& next)
{

    map<string, int> old_kind_map;

    while (*next != block_separator)
    {

        string key = decode_string(next);
        int64_t value = decode_int(next);

        old_kind_map[key] = value;

    }
    
    for (auto mp: old_kind_map)
    {

        auto key = mp.first;
        auto value = mp.second;

        if (prsd.kind_map.find(key) != prsd.kind_map.end())
        {
            continue;
        }

        if (prsd.kind_imap.find(value) != prsd.kind_imap.end())
        {
            continue;
        }

        prsd.kind_map[key] = value;
        prsd.kind_imap[value] = key;

    }

    temp.kind_map.clear();

    for (auto mp: old_kind_map)
    {

        auto key = mp.first;
        auto value = mp.second;

        if (prsd.kind_map.find(key) == prsd.kind_map.end())
        {
            prsd.get_kind_force(key);
        }

        temp.kind_map[value] = prsd.kind_map[key];

    }

}

//
//  handle_*_source
//  ---------------
//                                                                       
//  A copy of the grammar source.
//

void ParserData::handle_encode_source(const ParserData& prsd,
                                      const BlockType block,
                                      ostream& os)
{
    encode_string(prsd.src.get_string(0, prsd.src.length()), os);
}

void ParserData::handle_decode_source(ParserData& prsd,
                                      ParserTemp& temp,
                                      const BlockType block,
                                      const char*& next)
{
    prsd.src = Source(decode_string(next));
}

//
//  handle_*_lookaheads
//  -------------------
//
//  Grammar field: lookaheads.
//

void ParserData::handle_encode_lookaheads(const ParserData& prsd,
                                          const BlockType block,
                                          ostream& os)
{
    encode_int(prsd.lookaheads, os);
}

void ParserData::handle_decode_lookaheads(ParserData& prsd,
                                          ParserTemp& temp,
                                          const BlockType block,
                                          const char*& next)
{
    prsd.lookaheads = decode_int(next);
}

//
//  handle_*_error_recovery
//  -----------------------
//
//  Grammar field: error_recovery.
//

void ParserData::handle_encode_error_recovery(const ParserData& prsd,
                                              const BlockType block,
                                              ostream& os)
{
    encode_int(prsd.error_recovery, os);
}

void ParserData::handle_decode_error_recovery(ParserData& prsd,
                                              ParserTemp& temp,
                                              const BlockType block,
                                              const char*& next)
{
    prsd.error_recovery = decode_int(next);
}

//
//  handle_*_error_symbol_num
//  -------------------------
//
//  Grammar field: error_symbol_num.
//

void ParserData::handle_encode_error_symbol_num(const ParserData& prsd,
                                                const BlockType block,
                                                ostream& os)
{
    encode_int(prsd.error_symbol_num, os);
}

void ParserData::handle_decode_error_symbol_num(ParserData& prsd,
                                                ParserTemp& temp,
                                                const BlockType block,
                                                const char*& next)
{
    prsd.error_symbol_num = decode_int(next);
}

//
//  handle_*_eof_symbol_num
//  -----------------------
//
//  Grammar field: eof_symbol_num.
//

void ParserData::handle_encode_eof_symbol_num(const ParserData& prsd,
                                              const BlockType block,
                                              ostream& os)
{
    encode_int(prsd.eof_symbol_num, os);
}

void ParserData::handle_decode_eof_symbol_num(ParserData& prsd,
                                              ParserTemp& temp,
                                              const BlockType block,
                                              const char*& next)
{
    prsd.eof_symbol_num = decode_int(next);
}

//
//  handle_*_token_count
//  --------------------
//
//  Grammar field: token_count.
//

void ParserData::handle_encode_token_count(const ParserData& prsd,
                                           const BlockType block,
                                           ostream& os)
{
    encode_int(prsd.token_count, os);
}

void ParserData::handle_decode_token_count(ParserData& prsd,
                                           ParserTemp& temp,
                                           const BlockType block,
                                           const char*& next)
{
    prsd.token_count = decode_int(next);
}

//
//  handle_*_token_name_list
//  ------------------------
//
//  Grammar field: token_name_list.
//

void ParserData::handle_encode_token_name_list(const ParserData& prsd,
                                               const BlockType block,
                                               ostream& os)
{

    for (int i = 0; i < prsd.token_count; i++)
    {
        encode_string(prsd.token_name_list[i], os);
    }

}

void ParserData::handle_decode_token_name_list(ParserData& prsd,
                                               ParserTemp& temp,
                                               const BlockType block,
                                               const char*& next)
{

    prsd.token_name_list = new string[prsd.token_count];
    
    for (int i = 0; i < prsd.token_count; i++)
    {
        prsd.token_name_list[i] = decode_string(next);
    }

}

//
//  handle_*_token_is_terminal
//  ----------------------------
//
//  Grammar field: token_is_terminal.
//

void ParserData::handle_encode_token_is_terminal(const ParserData& prsd,
                                                 const BlockType block,
                                                 ostream& os)
{

    for (int i = 0; i < prsd.token_count; i++)
    {
        encode_int(prsd.token_is_terminal[i], os);
    }

}

void ParserData::handle_decode_token_is_terminal(ParserData& prsd,
                                                 ParserTemp& temp,
                                                 const BlockType block,
                                                 const char*& next)
{

    prsd.token_is_terminal = new bool[prsd.token_count];
    
    for (int i = 0; i < prsd.token_count; i++)
    {
        prsd.token_is_terminal[i] = decode_int(next);
    }

}

//
//  handle_*_token_kind
//  -------------------
//
//  Grammar field: token_kind.
//

void ParserData::handle_encode_token_kind(const ParserData& prsd,
                                          const BlockType block,
                                          ostream& os)
{

    for (int i = 0; i < prsd.token_count; i++)
    {
        encode_int(prsd.token_kind[i], os);
    }

}

void ParserData::handle_decode_token_kind(ParserData& prsd,
                                          ParserTemp& temp,
                                          const BlockType block,
                                          const char*& next)
{

    prsd.token_kind = new int[prsd.token_count];
    
    for (int i = 0; i < prsd.token_count; i++)
    {
        prsd.token_kind[i] = decode_int(next);
    }

}

//
//  handle_*_token_lexeme_needed
//  ----------------------------
//
//  Grammar field: token_lexeme_needed.
//

void ParserData::handle_encode_token_lexeme_needed(const ParserData& prsd,
                                                   const BlockType block,
                                                   ostream& os)
{

    for (int i = 0; i < prsd.token_count; i++)
    {
        encode_int(prsd.token_lexeme_needed[i], os);
    }

}

void ParserData::handle_decode_token_lexeme_needed(ParserData& prsd,
                                                   ParserTemp& temp,
                                                   const BlockType block,
                                                   const char*& next)
{

    prsd.token_lexeme_needed = new bool[prsd.token_count];
    
    for (int i = 0; i < prsd.token_count; i++)
    {
        prsd.token_lexeme_needed[i] = decode_int(next);
    }

}

//
//  handle_*_rule_count
//  -------------------
//
//  Grammar field: rule_count.
//

void ParserData::handle_encode_rule_count(const ParserData& prsd,
                                          const BlockType block,
                                          ostream& os)
{
    encode_int(prsd.rule_count, os);
}

void ParserData::handle_decode_rule_count(ParserData& prsd,
                                          ParserTemp& temp,
                                          const BlockType block,
                                          const char*& next)
{
    prsd.rule_count = decode_int(next);
}

//
//  handle_*_rule_size
//  ------------------
//
//  Grammar field: rule_size.
//

void ParserData::handle_encode_rule_size(const ParserData& prsd,
                                         const BlockType block,
                                         ostream& os)
{

    for (int i = 0; i < prsd.rule_count; i++)
    {
        encode_int(prsd.rule_size[i], os);
    }

}

void ParserData::handle_decode_rule_size(ParserData& prsd,
                                         ParserTemp& temp,
                                         const BlockType block,
                                         const char*& next)
{

    prsd.rule_size = new int[prsd.rule_count];
    
    for (int i = 0; i < prsd.rule_count; i++)
    {
        prsd.rule_size[i] = decode_int(next);
    }

}

//
//  handle_*_rule_lhs
//  -----------------
//
//  Grammar field: rule_lhs.
//

void ParserData::handle_encode_rule_lhs(const ParserData& prsd,
                                        const BlockType block,
                                        ostream& os)
{

    for (int i = 0; i < prsd.rule_count; i++)
    {
        encode_int(prsd.rule_lhs[i], os);
    }

}

void ParserData::handle_decode_rule_lhs(ParserData& prsd,
                                        ParserTemp& temp,
                                        const BlockType block,
                                        const char*& next)
{

    prsd.rule_lhs = new int[prsd.rule_count];
    
    for (int i = 0; i < prsd.rule_count; i++)
    {
        prsd.rule_lhs[i] = decode_int(next);
    }

}

//
//  handle_*_rule_text
//  ------------------
//
//  Grammar field: rule_text.
//

void ParserData::handle_encode_rule_text(const ParserData& prsd,
                                         const BlockType block,
                                         ostream& os)
{

    for (int i = 0; i < prsd.rule_count; i++)
    {
        encode_string(prsd.rule_text[i], os);
    }

}

void ParserData::handle_decode_rule_text(ParserData& prsd,
                                         ParserTemp& temp,
                                         const BlockType block,
                                         const char*& next)
{

    prsd.rule_text = new string[prsd.rule_count];
    
    for (int i = 0; i < prsd.rule_count; i++)
    {
        prsd.rule_text[i] = decode_string(next);
    }

}

//
//  handle_*_rule_pc
//  ----------------
//
//  Grammar field: rule_pc.
//

void ParserData::handle_encode_rule_pc(const ParserData& prsd,
                                       const BlockType block,
                                       ostream& os)
{

    for (int i = 0; i < prsd.rule_count; i++)
    {
        encode_int(prsd.rule_pc[i], os);
    }

}

void ParserData::handle_decode_rule_pc(ParserData& prsd,
                                       ParserTemp& temp,
                                       const BlockType block,
                                       const char*& next)
{

    prsd.rule_pc = new int64_t[prsd.rule_count];
    
    for (int i = 0; i < prsd.rule_count; i++)
    {
        prsd.rule_pc[i] = decode_int(next);
    }

}

//
//  handle_*_scanner_pc
//  -------------------
//
//  Grammar field: scanner_pc.
//

void ParserData::handle_encode_scanner_pc(const ParserData& prsd,
                                          const BlockType block,
                                          ostream& os)
{
    encode_int(prsd.scanner_pc, os);
}

void ParserData::handle_decode_scanner_pc(ParserData& prsd,
                                          ParserTemp& temp,
                                          const BlockType block,
                                          const char*& next)
{
    prsd.scanner_pc = decode_int(next);
}

//
//  handle_*_start_state
//  --------------------
//
//  Parse table field: start_state.
//

void ParserData::handle_encode_start_state(const ParserData& prsd,
                                           const BlockType block,
                                           ostream& os)
{
    encode_int(prsd.start_state, os);
}

void ParserData::handle_decode_start_state(ParserData& prsd,
                                           ParserTemp& temp,
                                           const BlockType block,
                                           const char*& next)
{
    prsd.start_state = decode_int(next);
}

//
//  handle_*_restart_state
//  ----------------------
//
//  Parse table field: restart_state.
//

void ParserData::handle_encode_restart_state(const ParserData& prsd,
                                             const BlockType block,
                                             ostream& os)
{
    encode_int(prsd.restart_state, os);
}

void ParserData::handle_decode_restart_state(ParserData& prsd,
                                             ParserTemp& temp,
                                             const BlockType block,
                                             const char*& next)
{
    prsd.restart_state = decode_int(next);
}

//
//  handle_*_checked_index_count
//  ----------------------------
//
//  Parse table field: checked_index_count.
//

void ParserData::handle_encode_checked_index_count(const ParserData& prsd,
                                                   const BlockType block,
                                                   ostream& os)
{
    encode_int(prsd.checked_index_count, os);
}

void ParserData::handle_decode_checked_index_count(ParserData& prsd,
                                                   ParserTemp& temp,
                                                   const BlockType block,
                                                   const char*& next)
{
    prsd.checked_index_count = decode_int(next);
}

//
//  handle_*_checked_index
//  ----------------------
//
//  Parse table field: checked_index.
//

void ParserData::handle_encode_checked_index(const ParserData& prsd,
                                             const BlockType block,
                                             ostream& os)
{
    
    for (int i = 0; i < prsd.checked_index_count; i++)
    {
        encode_int(prsd.checked_index[i], os);
    }

}

void ParserData::handle_decode_checked_index(ParserData& prsd,
                                             ParserTemp& temp,
                                             const BlockType block,
                                             const char*& next)
{

    prsd.checked_index = new int64_t[prsd.checked_index_count];

    for (int i = 0; i < prsd.checked_index_count; i++)
    {
        prsd.checked_index[i] = decode_int(next);
    }

}

//
//  handle_*_checked_data_count
//  ---------------------------
//
//  Parse table field: checked_data_count.
//

void ParserData::handle_encode_checked_data_count(const ParserData& prsd,
                                                  const BlockType block,
                                                  ostream& os)
{
    encode_int(prsd.checked_data_count, os);
}

void ParserData::handle_decode_checked_data_count(ParserData& prsd,
                                                  ParserTemp& temp,
                                                  const BlockType block,
                                                  const char*& next)
{
    prsd.checked_data_count = decode_int(next);
}

//
//  handle_*_checked_data
//  ---------------------
//
//  Parse table field: checked_data.
//

void ParserData::handle_encode_checked_data(const ParserData& prsd,
                                            const BlockType block,
                                            ostream& os)
{
    
    for (int i = 0; i < prsd.checked_data_count; i++)
    {
        encode_int(prsd.checked_data[i], os);
    }

}

void ParserData::handle_decode_checked_data(ParserData& prsd,
                                            ParserTemp& temp,
                                            const BlockType block,
                                            const char*& next)
{

    prsd.checked_data = new int64_t[prsd.checked_data_count];

    for (int i = 0; i < prsd.checked_data_count; i++)
    {
        prsd.checked_data[i] = decode_int(next);
    }

}

//
//  handle_*_num_offsets
//  --------------------
//
//  Parse table field: num_offsets.
//

void ParserData::handle_encode_num_offsets(const ParserData& prsd,
                                           const BlockType block,
                                           ostream& os)
{
    encode_int(prsd.num_offsets, os);
}

void ParserData::handle_decode_num_offsets(ParserData& prsd,
                                           ParserTemp& temp,
                                           const BlockType block,
                                           const char*& next)
{
    prsd.num_offsets = decode_int(next);
}

//
//  handle_*_symbol_num_offset
//  --------------------------
//
//  Parse table field: symbol_num_offset.
//

void ParserData::handle_encode_symbol_num_offset(const ParserData& prsd,
                                                 const BlockType block,
                                                 ostream& os)
{
    encode_int(prsd.symbol_num_offset, os);
}

void ParserData::handle_decode_symbol_num_offset(ParserData& prsd,
                                                 ParserTemp& temp,
                                                 const BlockType block,
                                                 const char*& next)
{
    prsd.symbol_num_offset = decode_int(next);
}

//
//  handle_*_symbol_num_shift
//  -------------------------
//
//  Parse table field: symbol_num_shift.
//

void ParserData::handle_encode_symbol_num_shift(const ParserData& prsd,
                                                const BlockType block,
                                                ostream& os)
{
    encode_int(prsd.symbol_num_shift, os);
}

void ParserData::handle_decode_symbol_num_shift(ParserData& prsd,
                                                ParserTemp& temp,
                                                const BlockType block,
                                                const char*& next)
{
    prsd.symbol_num_shift = decode_int(next);
}

//
//  handle_*_symbol_num_mask
//  ------------------------
//
//  Parse table field: symbol_num_mask.
//

void ParserData::handle_encode_symbol_num_mask(const ParserData& prsd,
                                               const BlockType block,
                                               ostream& os)
{
    encode_int(prsd.symbol_num_mask, os);
}

void ParserData::handle_decode_symbol_num_mask(ParserData& prsd,
                                               ParserTemp& temp,
                                               const BlockType block,
                                               const char*& next)
{
    prsd.symbol_num_mask = decode_int(next);
}

//
//  handle_*_action_type_offset
//  ---------------------------
//
//  Parse table field: action_type_offset.
//

void ParserData::handle_encode_action_type_offset(const ParserData& prsd,
                                                  const BlockType block,
                                                  ostream& os)
{
    encode_int(prsd.action_type_offset, os);
}

void ParserData::handle_decode_action_type_offset(ParserData& prsd,
                                                  ParserTemp& temp,
                                                  const BlockType block,
                                                  const char*& next)
{
    prsd.action_type_offset = decode_int(next);
}

//
//  handle_*_action_type_shift
//  --------------------------
//
//  Parse table field: action_type_shift.
//

void ParserData::handle_encode_action_type_shift(const ParserData& prsd,
                                                 const BlockType block,
                                                 ostream& os)
{
    encode_int(prsd.action_type_shift, os);
}

void ParserData::handle_decode_action_type_shift(ParserData& prsd,
                                                 ParserTemp& temp,
                                                 const BlockType block,
                                                 const char*& next)
{
    prsd.action_type_shift = decode_int(next);
}

//
//  handle_*_action_type_mask
//  -------------------------
//
//  Parse table field: action_type_mask.
//

void ParserData::handle_encode_action_type_mask(const ParserData& prsd,
                                                const BlockType block,
                                                ostream& os)
{
    encode_int(prsd.action_type_mask, os);
}

void ParserData::handle_decode_action_type_mask(ParserData& prsd,
                                                ParserTemp& temp,
                                                const BlockType block,
                                                const char*& next)
{
    prsd.action_type_mask = decode_int(next);
}

//
//  handle_*_rule_num_offset
//  ------------------------
//
//  Parse table field: rule_num_offset.
//

void ParserData::handle_encode_rule_num_offset(const ParserData& prsd,
                                               const BlockType block,
                                               ostream& os)
{
    encode_int(prsd.rule_num_offset, os);
}

void ParserData::handle_decode_rule_num_offset(ParserData& prsd,
                                               ParserTemp& temp,
                                               const BlockType block,
                                               const char*& next)
{
    prsd.rule_num_offset = decode_int(next);
}

//
//  handle_*_rule_num_shift
//  -----------------------
//
//  Parse table field: rule_num_shift.
//

void ParserData::handle_encode_rule_num_shift(const ParserData& prsd,
                                              const BlockType block,
                                              ostream& os)
{
    encode_int(prsd.rule_num_shift, os);
}

void ParserData::handle_decode_rule_num_shift(ParserData& prsd,
                                              ParserTemp& temp,
                                              const BlockType block,
                                              const char*& next)
{
    prsd.rule_num_shift = decode_int(next);
}

//
//  handle_*_rule_num_mask
//  ----------------------
//
//  Parse table field: rule_num_mask.
//

void ParserData::handle_encode_rule_num_mask(const ParserData& prsd,
                                             const BlockType block,
                                             ostream& os)
{
    encode_int(prsd.rule_num_mask, os);
}

void ParserData::handle_decode_rule_num_mask(ParserData& prsd,
                                             ParserTemp& temp,
                                             const BlockType block,
                                             const char*& next)
{
    prsd.rule_num_mask = decode_int(next);
}

//
//  handle_*_state_num_offset
//  -------------------------
//
//  Parse table field: state_num_offset.
//

void ParserData::handle_encode_state_num_offset(const ParserData& prsd,
                                                const BlockType block,
                                                ostream& os)
{
    encode_int(prsd.state_num_offset, os);
}

void ParserData::handle_decode_state_num_offset(ParserData& prsd,
                                                ParserTemp& temp,
                                                const BlockType block,
                                                const char*& next)
{
    prsd.state_num_offset = decode_int(next);
}

//
//  handle_*_state_num_shift
//  ------------------------
//
//  Parse table field: state_num_shift.
//

void ParserData::handle_encode_state_num_shift(const ParserData& prsd,
                                               const BlockType block,
                                               ostream& os)
{
    encode_int(prsd.state_num_shift, os);
}

void ParserData::handle_decode_state_num_shift(ParserData& prsd,
                                               ParserTemp& temp,
                                               const BlockType block,
                                               const char*& next)
{
    prsd.state_num_shift = decode_int(next);
}

//
//  handle_*_state_num_mask
//  -----------------------
//
//  Parse table field: state_num_mask.
//

void ParserData::handle_encode_state_num_mask(const ParserData& prsd,
                                              const BlockType block,
                                              ostream& os)
{
    encode_int(prsd.state_num_mask, os);
}

void ParserData::handle_decode_state_num_mask(ParserData& prsd,
                                              ParserTemp& temp,
                                              const BlockType block,
                                              const char*& next)
{
    prsd.state_num_mask = decode_int(next);
}

//
//  handle_*_fallback_num_offset
//  ----------------------------
//
//  Parse table field: fallback_num_offset.
//

void ParserData::handle_encode_fallback_num_offset(const ParserData& prsd,
                                                   const BlockType block,
                                                   ostream& os)
{
    encode_int(prsd.fallback_num_offset, os);
}

void ParserData::handle_decode_fallback_num_offset(ParserData& prsd,
                                                   ParserTemp& temp,
                                                   const BlockType block,
                                                   const char*& next)
{
    prsd.fallback_num_offset = decode_int(next);
}

//
//  handle_*_fallback_num_shift
//  ---------------------------
//
//  Parse table field: fallback_num_shift.
//

void ParserData::handle_encode_fallback_num_shift(const ParserData& prsd,
                                                  const BlockType block,
                                                  ostream& os)
{
    encode_int(prsd.fallback_num_shift, os);
}

void ParserData::handle_decode_fallback_num_shift(ParserData& prsd,
                                                  ParserTemp& temp,
                                                  const BlockType block,
                                                  const char*& next)
{
    prsd.fallback_num_shift = decode_int(next);
}

//
//  handle_*_fallback_num_mask
//  --------------------------
//
//  Parse table field: fallback_num_mask.
//

void ParserData::handle_encode_fallback_num_mask(const ParserData& prsd,
                                                 const BlockType block,
                                                 ostream& os)
{
    encode_int(prsd.fallback_num_mask, os);
}

void ParserData::handle_decode_fallback_num_mask(ParserData& prsd,
                                                 ParserTemp& temp,
                                                 const BlockType block,
                                                 const char*& next)
{
    prsd.fallback_num_mask = decode_int(next);
}

//
//  handle_*_opcode_map
//  -------------------
//
//  VM field: opcode_map.
//

void ParserData::handle_encode_opcode_map(const ParserData& prsd,
                                          const BlockType block,
                                          ostream& os)
{

    for (int i = OpcodeType::OpcodeMinimum;
         i <= OpcodeType::OpcodeMaximum;
         i++)
    {

        string name = ParserEngine::get_vcode_name(
                          ParserEngine::get_vcode_handler(
                              static_cast<OpcodeType>(i)));

        encode_string(name, os);
        encode_int(i, os);

    }

}

void ParserData::handle_decode_opcode_map(ParserData& prsd,
                                          ParserTemp& temp,
                                          const BlockType block,
                                          const char*& next)
{

    map<string, int> curr_opcode_map;

    for (int i = OpcodeType::OpcodeMinimum;
         i <= OpcodeType::OpcodeMaximum;
         i++)
    {

        string name = ParserEngine::get_vcode_name(
                          ParserEngine::get_vcode_handler(
                              static_cast<OpcodeType>(i)));

        curr_opcode_map[name] = i;

    }

    temp.opcode_map.clear();

    while (*next != block_separator)
    {

        string name = decode_string(next);
        int64_t code = decode_int(next);

        if (curr_opcode_map.find(name) == curr_opcode_map.end())
        {
            throw out_of_range("Version mismatch in Hoshi library");
        }
           
        temp.opcode_map[code] = curr_opcode_map[name];

    }
    
}

//
//  handle_*_instruction_count
//  --------------------------
//
//  VM  field: instruction_count.
//

void ParserData::handle_encode_instruction_count(const ParserData& prsd,
                                                 const BlockType block,
                                                 ostream& os)
{
    encode_int(prsd.instruction_count, os);
}

void ParserData::handle_decode_instruction_count(ParserData& prsd,
                                                 ParserTemp& temp,
                                                 const BlockType block,
                                                 const char*& next)
{
    prsd.instruction_count = decode_int(next);
}

//
//  handle_*_operand_count
//  ----------------------
//
//  VM  field: operand_count.
//

void ParserData::handle_encode_operand_count(const ParserData& prsd,
                                             const BlockType block,
                                             ostream& os)
{
    encode_int(prsd.operand_count, os);
}

void ParserData::handle_decode_operand_count(ParserData& prsd,
                                             ParserTemp& temp,
                                             const BlockType block,
                                             const char*& next)
{
    prsd.operand_count = decode_int(next);
}

//
//  handle_encode_instruction_list
//  ------------------------------
//
//  VM field: instruction_list.
//

void ParserData::handle_encode_instruction_list(const ParserData& prsd,
                                                const BlockType block,
                                                ostream& os)
{

    //
    //  Operand encoders.
    //

    function<void(VCodeOperand)> encode_integer_operand = [&](VCodeOperand operand) -> void
    {
        encode_int(operand.integer, os);
    };

    function<void(VCodeOperand)> encode_kind_operand = [&](VCodeOperand operand) -> void
    {
        encode_int(operand.integer, os);
    };

    function<void(VCodeOperand)> encode_character_operand = [&](VCodeOperand operand) -> void
    {
        encode_int(operand.character, os);
    };

    function<void(VCodeOperand)> encode_register_operand = [&](VCodeOperand operand) -> void
    {
        encode_int(operand.register_num, os);
    };

    function<void(VCodeOperand)> encode_ast_operand = [&](VCodeOperand operand) -> void
    {
        encode_int(operand.ast_num, os);
    };

    function<void(VCodeOperand)> encode_string_operand = [&](VCodeOperand operand) -> void
    {
        encode_int(operand.string_num, os);
    };

    function<void(VCodeOperand)> encode_label_operand = [&](VCodeOperand operand) -> void
    {
        encode_int(operand.branch_target, os);
    };

    //
    //  handle_encode_instruction_list
    //  ------------------------------
    //                                 
    //  The function body begins here. 
    //

    for (int64_t i = 0; i < prsd.instruction_count; i++)
    {

        VCodeInstruction& instruction = prsd.instruction_list[i];

        encode_int(ParserEngine::get_vcode_opcode(instruction.handler), os);
        encode_int(instruction.location, os);
        encode_int(instruction.operand_offset, os);

        //
        //  Encode the operands.
        //

        switch (ParserEngine::get_vcode_opcode(instruction.handler))
        {

            case OpcodeCall:
            case OpcodeBranch:
            {
            
                int operand = 0;
                encode_label_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeScanChar:
            {
            
                int operand = 0;
            
                encode_integer_operand(prsd.operand_list[instruction.operand_offset + operand]);
                for (int i = 0; i < prsd.operand_list[instruction.operand_offset + operand].integer; i++)    {
            
                    encode_character_operand(prsd.operand_list[instruction.operand_offset + operand + 3 * i + 1]);
                    encode_character_operand(prsd.operand_list[instruction.operand_offset + operand + 3 * i + 2]);
                    encode_label_operand(prsd.operand_list[instruction.operand_offset + operand + 3 * i + 3]);
            
                }
            
                operand += prsd.operand_list[instruction.operand_offset + operand].integer * 3 + 1;
                break;
            
            }
            
            case OpcodeScanAccept:
            {
            
                int operand = 0;
                encode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_label_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeScanError:
            case OpcodeAstLexemeString:
            {
            
                int operand = 0;
                encode_string_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstStart:
            case OpcodeAstNew:
            {
            
                int operand = 0;
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstFinish:
            case OpcodeAstLocationNum:
            {
            
                int operand = 0;
                encode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstForm:
            {
            
                int operand = 0;
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstLoad:
            {
            
                int operand = 0;
                encode_ast_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstIndex:
            {
            
                int operand = 0;
                encode_ast_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstChild:
            case OpcodeAstKind:
            case OpcodeAstLocation:
            case OpcodeAstLexeme:
            {
            
                int operand = 0;
                encode_ast_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstChildSlice:
            {
            
                int operand = 0;
                encode_ast_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstKindNum:
            {
            
                int operand = 0;
                encode_kind_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAssign:
            case OpcodeUnaryMinus:
            {
            
                int operand = 0;
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAdd:
            case OpcodeSubtract:
            case OpcodeMultiply:
            case OpcodeDivide:
            {
            
                int operand = 0;
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeBranchEqual:
            case OpcodeBranchNotEqual:
            case OpcodeBranchLessThan:
            case OpcodeBranchLessEqual:
            case OpcodeBranchGreaterThan:
            case OpcodeBranchGreaterEqual:
            {
            
                int operand = 0;
                encode_label_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                encode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
        }

    }

}

//
//  handle_decode_instruction_list
//  ------------------------------
//
//  VM field: instruction_list.
//

void ParserData::handle_decode_instruction_list(ParserData& prsd,
                                                ParserTemp& temp,
                                                const BlockType block,
                                                const char*& next)
{

    //
    //  Operand decoders.
    //

    function<void(VCodeOperand&)> decode_integer_operand = [&](VCodeOperand& operand) -> void
    {
        operand.integer = decode_int(next);
    };

    function<void(VCodeOperand&)> decode_kind_operand = [&](VCodeOperand& operand) -> void
    {

        int kind = decode_int(next);
        if (temp.kind_map.find(kind) == temp.kind_map.end())
        {
            throw out_of_range("Version mismatch in Hoshi library");
        }

        operand.integer = temp.kind_map[kind];

    };

    function<void(VCodeOperand&)> decode_character_operand = [&](VCodeOperand& operand) -> void
    {
        operand.character = decode_int(next);
    };

    function<void(VCodeOperand&)> decode_register_operand = [&](VCodeOperand& operand) -> void
    {
        operand.register_num = decode_int(next);
    };

    function<void(VCodeOperand&)> decode_ast_operand = [&](VCodeOperand& operand) -> void
    {
        operand.ast_num = decode_int(next);
    };

    function<void(VCodeOperand&)> decode_string_operand = [&](VCodeOperand& operand) -> void
    {
        operand.string_num = decode_int(next);
    };

    function<void(VCodeOperand&)> decode_label_operand = [&](VCodeOperand& operand) -> void
    {
        operand.branch_target = decode_int(next);
    };

    //
    //  handle_decode_instruction_list
    //  ------------------------------
    //                                 
    //  The function body begins here. 
    //

    prsd.instruction_list = new VCodeInstruction[prsd.instruction_count];
    prsd.operand_list = new VCodeOperand[prsd.operand_count];

    for (int64_t i = 0; i < prsd.instruction_count; i++)
    {

        VCodeInstruction& instruction = prsd.instruction_list[i];

        int code = decode_int(next);
        if (temp.opcode_map.find(code) == temp.opcode_map.end())
        {
            throw out_of_range("Version mismatch in Hoshi library");
        }

        instruction.handler = ParserEngine::get_vcode_handler(static_cast<OpcodeType>(temp.opcode_map[code]));
        instruction.location = decode_int(next);
        instruction.operand_offset = decode_int(next);

        //
        //  Encode the operands.
        //

        switch (ParserEngine::get_vcode_opcode(instruction.handler))
        {

            case OpcodeCall:
            case OpcodeBranch:
            {
            
                int operand = 0;
                decode_label_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeScanChar:
            {
            
                int operand = 0;
            
                decode_integer_operand(prsd.operand_list[instruction.operand_offset + operand]);
                for (int i = 0; i < prsd.operand_list[instruction.operand_offset + operand].integer; i++)    {
            
                    decode_character_operand(prsd.operand_list[instruction.operand_offset + operand + 3 * i + 1]);
                    decode_character_operand(prsd.operand_list[instruction.operand_offset + operand + 3 * i + 2]);
                    decode_label_operand(prsd.operand_list[instruction.operand_offset + operand + 3 * i + 3]);
            
                }
            
                operand += prsd.operand_list[instruction.operand_offset + operand].integer * 3 + 1;
                break;
            
            }
            
            case OpcodeScanAccept:
            {
            
                int operand = 0;
                decode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_label_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeScanError:
            case OpcodeAstLexemeString:
            {
            
                int operand = 0;
                decode_string_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstStart:
            case OpcodeAstNew:
            {
            
                int operand = 0;
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstFinish:
            case OpcodeAstLocationNum:
            {
            
                int operand = 0;
                decode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstForm:
            {
            
                int operand = 0;
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstLoad:
            {
            
                int operand = 0;
                decode_ast_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstIndex:
            {
            
                int operand = 0;
                decode_ast_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstChild:
            case OpcodeAstKind:
            case OpcodeAstLocation:
            case OpcodeAstLexeme:
            {
            
                int operand = 0;
                decode_ast_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstChildSlice:
            {
            
                int operand = 0;
                decode_ast_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_integer_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAstKindNum:
            {
            
                int operand = 0;
                decode_kind_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAssign:
            case OpcodeUnaryMinus:
            {
            
                int operand = 0;
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeAdd:
            case OpcodeSubtract:
            case OpcodeMultiply:
            case OpcodeDivide:
            {
            
                int operand = 0;
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
            case OpcodeBranchEqual:
            case OpcodeBranchNotEqual:
            case OpcodeBranchLessThan:
            case OpcodeBranchLessEqual:
            case OpcodeBranchGreaterThan:
            case OpcodeBranchGreaterEqual:
            {
            
                int operand = 0;
                decode_label_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                decode_register_operand(prsd.operand_list[instruction.operand_offset + operand++]);
                break;
            
            }
            
        }

    }

}

//
//  handle_*_register_count
//  -----------------------
//
//  VM  field: register_count.
//

void ParserData::handle_encode_register_count(const ParserData& prsd,
                                              const BlockType block,
                                              ostream& os)
{
    encode_int(prsd.register_count, os);
}

void ParserData::handle_decode_register_count(ParserData& prsd,
                                              ParserTemp& temp,
                                              const BlockType block,
                                              const char*& next)
{
    prsd.register_count = decode_int(next);
}

//
//  handle_*_register_list
//  ----------------------
//
//  VM field: register_list.
//

void ParserData::handle_encode_register_list(const ParserData& prsd,
                                             const BlockType block,
                                             ostream& os)
{

    for (int i = 0; i < prsd.register_count; i++)
    {
        encode_int(prsd.register_list[i].initial_value, os);
        encode_string(prsd.register_list[i].name, os);
    }

}

void ParserData::handle_decode_register_list(ParserData& prsd,
                                             ParserTemp& temp,
                                             const BlockType block,
                                             const char*& next)
{

    prsd.register_list = new VCodeRegister[prsd.register_count];
    
    for (int i = 0; i < prsd.register_count; i++)
    {
        prsd.register_list[i].initial_value = decode_int(next);
        prsd.register_list[i].name = decode_string(next);
    }

}

//
//  handle_*_ast_count
//  ------------------
//
//  VM  field: ast_count.
//

void ParserData::handle_encode_ast_count(const ParserData& prsd,
                                         const BlockType block,
                                         ostream& os)
{
    encode_int(prsd.ast_count, os);
}

void ParserData::handle_decode_ast_count(ParserData& prsd,
                                         ParserTemp& temp,
                                         const BlockType block,
                                         const char*& next)
{
    prsd.ast_count = decode_int(next);
}

//
//  handle_*_string_count
//  ---------------------
//
//  VM  field: string_count.
//

void ParserData::handle_encode_string_count(const ParserData& prsd,
                                            const BlockType block,
                                            ostream& os)
{
    encode_int(prsd.string_count, os);
}

void ParserData::handle_decode_string_count(ParserData& prsd,
                                            ParserTemp& temp,
                                            const BlockType block,
                                            const char*& next)
{
    prsd.string_count = decode_int(next);
}

//
//  handle_*_string_list
//  --------------------
//
//  VM field: string_list.
//

void ParserData::handle_encode_string_list(const ParserData& prsd,
                                           const BlockType block,
                                           ostream& os)
{

    for (int i = 0; i < prsd.string_count; i++)
    {
        encode_string(prsd.string_list[i], os);
    }

}

void ParserData::handle_decode_string_list(ParserData& prsd,
                                           ParserTemp& temp,
                                           const BlockType block,
                                           const char*& next)
{

    prsd.string_list = new string[prsd.string_count];
    
    for (int i = 0; i < prsd.string_count; i++)
    {
        prsd.string_list[i] = decode_string(next);
    }

}

//
//  encode_int                                                             
//  ----------                                                             
//                                                                         
//  Encode an integer into a compact ASCII form. We use a long sequence of 
//  ASCII characters as digits so the encoded numbers are quite short. The 
//  number always ends with a field separator.                             
//

void ParserData::encode_int(int64_t i, ostream& os)
{

    bool negative = false;

    if (i < 0)
    {
        negative = true;
        i *= -1;
    }

    while (i > 0)
    {
        os << static_cast<char>(first_data + (i & 0x3f));
        i >>= 6;
    }

    if (negative)
    {
        os << field_separator_negative;
    }
    else
    {
        os << field_separator;
    }

}

//
//  decode_int                               
//  ----------                               
//                                           
//  Decode the ASCII encoding of an integer. 
//

int64_t ParserData::decode_int(const char*& next)
{

    int64_t result = 0;
    int64_t shift = 0;

    while (*next >= first_data && *next <= first_data + 0x3f)
    {
        result |= static_cast<int64_t>(*next - first_data) << shift;
        shift += 6;
        next++;
    }

    if (*next == field_separator_negative)
    {
        result *= -1;
        next++;
    }
    else if (*next == field_separator)
    {
        next++;
    }
    else
    {
        throw out_of_range("Invalid encoded integer");
    }

    return result;

}
        
//
//  encode_string                                                        
//  -------------                                                        
//                                                                       
//  Encode a UTF-8 string into ASCII. This isn't terribly compact but we 
//  don't expect to have many non-ASCII characters.                      
//

void ParserData::encode_string(string s, ostream& os)
{

    for (auto c: s)
    {

        unsigned char ch = *reinterpret_cast<unsigned char *>(&c);
        if (ch >= first_data && c <= last_data)
        {
            os << ch;
            continue;
        }

        os << escape;
        os << static_cast<char>(first_data + (ch >> 4));
        os << static_cast<char>(first_data + (ch & 0x0f));

    }

    os << field_separator;

}

//
//  decode_string                               
//  -------------                               
//                                           
//  Decode the ASCII encoding of an string. 
//

string ParserData::decode_string(const char*& next)
{

    string result;

    for (;;) 
    {

        if (*next == field_separator)
        {
            next++;
            break;
        }
        else if (*next == escape)
        {
            unsigned char ch = *(next + 1) << 4 | *(next + 2);
            result.push_back(*reinterpret_cast<char*>(&ch));
            next += 3;
        }
        else if (*next >= first_data && *next <= last_data)
        {
            result.push_back(*next);
            next++;
        }
        else
        {
            throw out_of_range("Invalid encoded string");
        }

    }

    return result;

}

//
//  export_cpp                                                         
//  ----------                                                         
//                                                                     
//  Export the object as a C++ string. Mostly useful for bootstrapping 
//  where we have to create a parser without having the library yet.   
//

void ParserData::export_cpp(string file_name, string identifier) const
{

    static const int max_width = 75;

    ofstream os(file_name.c_str());

    os << "static const char* " << identifier << " =" << endl
       << "{" << endl;

    os << "    \"";
    int width = 5;

    for (auto c: encode())
    {
        
        if (width > max_width)
        {
            os << "\"" << endl << "    \"";
            width = 5; 
        }

        if (c == '"' || c == '\\')
        {
            os << '\\';
            width++;
        }

        os << c; 
        width++;

    }

    os << "\"" << endl
       << "};" << endl;

    os.close();

}

} // namespace hoshi

