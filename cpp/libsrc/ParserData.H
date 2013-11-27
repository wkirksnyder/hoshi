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

#ifndef PARSER_DATA_H
#define PARSER_DATA_H

#include <cstdint>
#include <mutex>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "OpcodeType.H"
#include "Parser.H"
#include "ParserEngine.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

//
//  BlockType                                                              
//  ---------                                                              
//                                                                         
//  The encoded string representation of a parser consists of a number of  
//  blocks of different kinds of data. This is an enum of the block types. 
//

enum BlockType : int
{
    BlockMinimum           =   0,
    BlockMaximum           =  49,
    BlockVersion           =   0,
    BlockKindMap           =   1,
    BlockSource            =   2,
    BlockLookaheads        =   3,
    BlockErrorRecovery     =   4,
    BlockErrorSymbolNum    =   5,
    BlockEofSymbolNum      =   6,
    BlockTokenCount        =   7,
    BlockTokenNameList     =   8,
    BlockTokenIsTerminal   =   9,
    BlockTokenKind         =  10,
    BlockTokenLexemeNeeded =  11,
    BlockRuleCount         =  12,
    BlockRuleSize          =  13,
    BlockRuleLhs           =  14,
    BlockRuleText          =  15,
    BlockRulePc            =  16,
    BlockScannerPc         =  17,
    BlockStartState        =  18,
    BlockRestartState      =  19,
    BlockCheckedIndexCount =  20,
    BlockCheckedIndex      =  21,
    BlockCheckedDataCount  =  22,
    BlockCheckedData       =  23,
    BlockNumOffsets        =  24,
    BlockSymbolNumOffset   =  25,
    BlockSymbolNumShift    =  26,
    BlockSymbolNumMask     =  27,
    BlockActionTypeOffset  =  28,
    BlockActionTypeShift   =  29,
    BlockActionTypeMask    =  30,
    BlockRuleNumOffset     =  31,
    BlockRuleNumShift      =  32,
    BlockRuleNumMask       =  33,
    BlockStateNumOffset    =  34,
    BlockStateNumShift     =  35,
    BlockStateNumMask      =  36,
    BlockFallbackNumOffset =  37,
    BlockFallbackNumShift  =  38,
    BlockFallbackNumMask   =  39,
    BlockOpcodeMap         =  40,
    BlockInstructionCount  =  41,
    BlockOperandCount      =  42,
    BlockInstructionList   =  43,
    BlockRegisterCount     =  44,
    BlockRegisterList      =  45,
    BlockAstCount          =  46,
    BlockStringCount       =  47,
    BlockStringList        =  48,
    BlockEof               =  49
};

//
//  ParserTemp                                                             
//  ----------                                                             
//                                                                         
//  Some of the stuff we have to accumulate during decoding is only used   
//  for decoding other stuff and can be discarded after the decoding is    
//  complete. We'll create one of these to use during decoding and discard 
//  it after completion.                                                     
//

struct ParserTemp
{
    int64_t version;
    std::map<int, int> kind_map;
    std::map<int, int> opcode_map;
};

//
//  ParserData         
//  ----------         
//                     
//  ParserData proper. 
//

class ParserData final
{
public:

    //
    //  Copy control. 
    //

    ParserData() = default;
    ~ParserData();

    ParserData(const ParserData&) = delete;
    ParserData(ParserData&&) = delete;
    ParserData& operator=(const ParserData&) = delete;
    ParserData& operator=(ParserData&&) = delete;

    static void attach(ParserData*& prsd);
    static void detach(ParserData*& prsd);

    //
    //  Control information.
    //

    static const int64_t min_supported_version = 0;
    static const int64_t current_version = 1;

    std::map<std::string, int> kind_map;
    std::map<int, std::string> kind_imap;
    
    Source src;

    //
    //  Grammar information. 
    //

    int lookaheads = 1;
    bool error_recovery = true;
    int error_symbol_num = -1;
    int eof_symbol_num = -1;

    int token_count = 0;
    std::string* token_name_list = nullptr;
    bool* token_is_terminal = nullptr;
    int* token_kind = nullptr;
    bool* token_lexeme_needed = nullptr;
    
    int rule_count = 0;
    int* rule_size = nullptr;
    int* rule_lhs = nullptr;
    std::string* rule_text = nullptr;
    int64_t* rule_pc = nullptr;

    int64_t scanner_pc = 0;

    //
    //  Parse table. 
    //

    int start_state = 0;
    int restart_state = 0;

    int checked_index_count = 0;
    int64_t* checked_index = nullptr;

    int checked_data_count = 0;
    int64_t* checked_data = nullptr;

    int num_offsets = 0;
   
    int symbol_num_offset = 0;
    int symbol_num_shift = 0;
    int64_t symbol_num_mask = 0;

    int action_type_offset = 0;
    int action_type_shift = 0;
    int64_t action_type_mask = 0;

    int rule_num_offset = 0;
    int rule_num_shift = 0;
    int64_t rule_num_mask = 0;

    int state_num_offset = 0;
    int state_num_shift = 0;
    int64_t state_num_mask = 0;

    int fallback_num_offset = 0;
    int fallback_num_shift = 0;
    int64_t fallback_num_mask = 0;

    //
    //  Virtual machine artifacts. 
    //

    int64_t instruction_count = 0;
    VCodeInstruction* instruction_list = nullptr;

    int64_t operand_count = 0;
    VCodeOperand* operand_list = nullptr;

    int64_t register_count = 0;
    VCodeRegister* register_list = nullptr;

    int64_t ast_count = 0;

    int64_t string_count = 0;
    std::string* string_list = nullptr;

    //
    //  External accessors. 
    //

    void set_kind_map(const std::map<std::string, int>& kind_map);
    std::map<std::string, int> get_kind_map() const;
    int get_kind(const std::string& kind_str) const;
    int get_kind_force(const std::string& kind_str);
    std::string get_kind_string(int kind) const;

    void export_cpp(std::string file_name, std::string identifier) const;

    std::string encode() const;
    void decode(const std::string& str,
                const std::map<std::string, int>& ast_types);

private:

    //
    //  String encoding constants. 
    //

    static const char first_data = 0x20;
    static const char last_data = 0x7a;
    static const char escape = 0x7b;
    static const char field_separator = 0x7c;
    static const char block_separator = 0x7d;
    static const char field_separator_negative = 0x7e;

    //
    //  Copy control. 
    //

    int reference_count = 0;
    std::mutex reference_mutex;

    //
    //  String encoding. 
    //

    typedef void (*EncodeHandler)(const ParserData& prsd,
                                  const BlockType block,
                                  std::ostream& os);
                                  
    static void handle_encode_error(const ParserData& prsd,
                                    const BlockType block,
                                    std::ostream& os);
    
    static void handle_encode_version(const ParserData& prsd,
                                      const BlockType block,
                                      std::ostream& os);
    
    static void handle_encode_kind_map(const ParserData& prsd,
                                       const BlockType block,
                                       std::ostream& os);
    
    static void handle_encode_source(const ParserData& prsd,
                                     const BlockType block,
                                     std::ostream& os);
    
    static void handle_encode_lookaheads(const ParserData& prsd,
                                         const BlockType block,
                                         std::ostream& os);
    
    static void handle_encode_error_recovery(const ParserData& prsd,
                                             const BlockType block,
                                             std::ostream& os);
    
    static void handle_encode_error_symbol_num(const ParserData& prsd,
                                               const BlockType block,
                                               std::ostream& os);
    
    static void handle_encode_eof_symbol_num(const ParserData& prsd,
                                             const BlockType block,
                                             std::ostream& os);
    
    static void handle_encode_token_count(const ParserData& prsd,
                                          const BlockType block,
                                          std::ostream& os);
    
    static void handle_encode_token_name_list(const ParserData& prsd,
                                              const BlockType block,
                                              std::ostream& os);
    
    static void handle_encode_token_is_terminal(const ParserData& prsd,
                                                const BlockType block,
                                                std::ostream& os);
    
    static void handle_encode_token_kind(const ParserData& prsd,
                                         const BlockType block,
                                         std::ostream& os);
    
    static void handle_encode_token_lexeme_needed(const ParserData& prsd,
                                                  const BlockType block,
                                                  std::ostream& os);
    
    static void handle_encode_rule_count(const ParserData& prsd,
                                         const BlockType block,
                                         std::ostream& os);
    
    static void handle_encode_rule_size(const ParserData& prsd,
                                        const BlockType block,
                                        std::ostream& os);
    
    static void handle_encode_rule_lhs(const ParserData& prsd,
                                       const BlockType block,
                                       std::ostream& os);
    
    static void handle_encode_rule_text(const ParserData& prsd,
                                        const BlockType block,
                                        std::ostream& os);
    
    static void handle_encode_rule_pc(const ParserData& prsd,
                                      const BlockType block,
                                      std::ostream& os);
    
    static void handle_encode_scanner_pc(const ParserData& prsd,
                                         const BlockType block,
                                         std::ostream& os);
    
    static void handle_encode_start_state(const ParserData& prsd,
                                          const BlockType block,
                                          std::ostream& os);
    
    static void handle_encode_restart_state(const ParserData& prsd,
                                            const BlockType block,
                                            std::ostream& os);
    
    static void handle_encode_checked_index_count(const ParserData& prsd,
                                                  const BlockType block,
                                                  std::ostream& os);
    
    static void handle_encode_checked_index(const ParserData& prsd,
                                            const BlockType block,
                                            std::ostream& os);
    
    static void handle_encode_checked_data_count(const ParserData& prsd,
                                                 const BlockType block,
                                                 std::ostream& os);
    
    static void handle_encode_checked_data(const ParserData& prsd,
                                           const BlockType block,
                                           std::ostream& os);
    
    static void handle_encode_num_offsets(const ParserData& prsd,
                                          const BlockType block,
                                          std::ostream& os);
    
    static void handle_encode_symbol_num_offset(const ParserData& prsd,
                                                const BlockType block,
                                                std::ostream& os);
    
    static void handle_encode_symbol_num_shift(const ParserData& prsd,
                                               const BlockType block,
                                               std::ostream& os);
    
    static void handle_encode_symbol_num_mask(const ParserData& prsd,
                                              const BlockType block,
                                              std::ostream& os);
    
    static void handle_encode_action_type_offset(const ParserData& prsd,
                                                 const BlockType block,
                                                 std::ostream& os);
    
    static void handle_encode_action_type_shift(const ParserData& prsd,
                                                const BlockType block,
                                                std::ostream& os);
    
    static void handle_encode_action_type_mask(const ParserData& prsd,
                                               const BlockType block,
                                               std::ostream& os);
    
    static void handle_encode_rule_num_offset(const ParserData& prsd,
                                              const BlockType block,
                                              std::ostream& os);
    
    static void handle_encode_rule_num_shift(const ParserData& prsd,
                                             const BlockType block,
                                             std::ostream& os);
    
    static void handle_encode_rule_num_mask(const ParserData& prsd,
                                            const BlockType block,
                                            std::ostream& os);
    
    static void handle_encode_state_num_offset(const ParserData& prsd,
                                               const BlockType block,
                                               std::ostream& os);
    
    static void handle_encode_state_num_shift(const ParserData& prsd,
                                              const BlockType block,
                                              std::ostream& os);
    
    static void handle_encode_state_num_mask(const ParserData& prsd,
                                             const BlockType block,
                                             std::ostream& os);
    
    static void handle_encode_fallback_num_offset(const ParserData& prsd,
                                                  const BlockType block,
                                                  std::ostream& os);
    
    static void handle_encode_fallback_num_shift(const ParserData& prsd,
                                                 const BlockType block,
                                                 std::ostream& os);
    
    static void handle_encode_fallback_num_mask(const ParserData& prsd,
                                                const BlockType block,
                                                std::ostream& os);
    
    static void handle_encode_opcode_map(const ParserData& prsd,
                                         const BlockType block,
                                         std::ostream& os);
    
    static void handle_encode_instruction_count(const ParserData& prsd,
                                                const BlockType block,
                                                std::ostream& os);
    
    static void handle_encode_operand_count(const ParserData& prsd,
                                            const BlockType block,
                                            std::ostream& os);
    
    static void handle_encode_instruction_list(const ParserData& prsd,
                                               const BlockType block,
                                               std::ostream& os);
    
    static void handle_encode_register_count(const ParserData& prsd,
                                             const BlockType block,
                                             std::ostream& os);
    
    static void handle_encode_register_list(const ParserData& prsd,
                                            const BlockType block,
                                            std::ostream& os);
    
    static void handle_encode_ast_count(const ParserData& prsd,
                                        const BlockType block,
                                        std::ostream& os);
    
    static void handle_encode_string_count(const ParserData& prsd,
                                           const BlockType block,
                                           std::ostream& os);
    
    static void handle_encode_string_list(const ParserData& prsd,
                                          const BlockType block,
                                          std::ostream& os);
    
    static void handle_encode_eof(const ParserData& prsd,
                                  const BlockType block,
                                  std::ostream& os);
    
    static EncodeHandler encode_handler[];

    //
    //  String decoding.
    //

    typedef void (*DecodeHandler)(ParserData& prsd,
                                  ParserTemp& temp,
                                  const BlockType block,
                                  const char*& next);

    static void handle_decode_error(ParserData& prsd,
                                    ParserTemp& temp,
                                    const BlockType block,
                                    const char*& next);
    
    static void handle_decode_version(ParserData& prsd,
                                      ParserTemp& temp,
                                      const BlockType block,
                                      const char*& next);
    
    static void handle_decode_kind_map(ParserData& prsd,
                                       ParserTemp& temp,
                                       const BlockType block,
                                       const char*& next);
    
    static void handle_decode_source(ParserData& prsd,
                                     ParserTemp& temp,
                                     const BlockType block,
                                     const char*& next);
    
    static void handle_decode_lookaheads(ParserData& prsd,
                                         ParserTemp& temp,
                                         const BlockType block,
                                         const char*& next);
    
    static void handle_decode_error_recovery(ParserData& prsd,
                                             ParserTemp& temp,
                                             const BlockType block,
                                             const char*& next);
    
    static void handle_decode_error_symbol_num(ParserData& prsd,
                                               ParserTemp& temp,
                                               const BlockType block,
                                               const char*& next);
    
    static void handle_decode_eof_symbol_num(ParserData& prsd,
                                             ParserTemp& temp,
                                             const BlockType block,
                                             const char*& next);
    
    static void handle_decode_token_count(ParserData& prsd,
                                          ParserTemp& temp,
                                          const BlockType block,
                                          const char*& next);
    
    static void handle_decode_token_name_list(ParserData& prsd,
                                              ParserTemp& temp,
                                              const BlockType block,
                                              const char*& next);
    
    static void handle_decode_token_is_terminal(ParserData& prsd,
                                                ParserTemp& temp,
                                                const BlockType block,
                                                const char*& next);
    
    static void handle_decode_token_kind(ParserData& prsd,
                                         ParserTemp& temp,
                                         const BlockType block,
                                         const char*& next);
    
    static void handle_decode_token_lexeme_needed(ParserData& prsd,
                                                  ParserTemp& temp,
                                                  const BlockType block,
                                                  const char*& next);
    
    static void handle_decode_rule_count(ParserData& prsd,
                                         ParserTemp& temp,
                                         const BlockType block,
                                         const char*& next);
    
    static void handle_decode_rule_size(ParserData& prsd,
                                        ParserTemp& temp,
                                        const BlockType block,
                                        const char*& next);
    
    static void handle_decode_rule_lhs(ParserData& prsd,
                                       ParserTemp& temp,
                                       const BlockType block,
                                       const char*& next);
    
    static void handle_decode_rule_text(ParserData& prsd,
                                        ParserTemp& temp,
                                        const BlockType block,
                                        const char*& next);
    
    static void handle_decode_rule_pc(ParserData& prsd,
                                      ParserTemp& temp,
                                      const BlockType block,
                                      const char*& next);
    
    static void handle_decode_scanner_pc(ParserData& prsd,
                                         ParserTemp& temp,
                                         const BlockType block,
                                         const char*& next);
    
    static void handle_decode_start_state(ParserData& prsd,
                                          ParserTemp& temp,
                                          const BlockType block,
                                          const char*& next);
    
    static void handle_decode_restart_state(ParserData& prsd,
                                            ParserTemp& temp,
                                            const BlockType block,
                                            const char*& next);
    
    static void handle_decode_checked_index_count(ParserData& prsd,
                                                  ParserTemp& temp,
                                                  const BlockType block,
                                                  const char*& next);
    
    static void handle_decode_checked_index(ParserData& prsd,
                                            ParserTemp& temp,
                                            const BlockType block,
                                            const char*& next);
    
    static void handle_decode_checked_data_count(ParserData& prsd,
                                                 ParserTemp& temp,
                                                 const BlockType block,
                                                 const char*& next);
    
    static void handle_decode_checked_data(ParserData& prsd,
                                           ParserTemp& temp,
                                           const BlockType block,
                                           const char*& next);
    
    static void handle_decode_num_offsets(ParserData& prsd,
                                          ParserTemp& temp,
                                          const BlockType block,
                                          const char*& next);
    
    static void handle_decode_symbol_num_offset(ParserData& prsd,
                                                ParserTemp& temp,
                                                const BlockType block,
                                                const char*& next);
    
    static void handle_decode_symbol_num_shift(ParserData& prsd,
                                               ParserTemp& temp,
                                               const BlockType block,
                                               const char*& next);
    
    static void handle_decode_symbol_num_mask(ParserData& prsd,
                                              ParserTemp& temp,
                                              const BlockType block,
                                              const char*& next);
    
    static void handle_decode_action_type_offset(ParserData& prsd,
                                                 ParserTemp& temp,
                                                 const BlockType block,
                                                 const char*& next);
    
    static void handle_decode_action_type_shift(ParserData& prsd,
                                                ParserTemp& temp,
                                                const BlockType block,
                                                const char*& next);
    
    static void handle_decode_action_type_mask(ParserData& prsd,
                                               ParserTemp& temp,
                                               const BlockType block,
                                               const char*& next);
    
    static void handle_decode_rule_num_offset(ParserData& prsd,
                                              ParserTemp& temp,
                                              const BlockType block,
                                              const char*& next);
    
    static void handle_decode_rule_num_shift(ParserData& prsd,
                                             ParserTemp& temp,
                                             const BlockType block,
                                             const char*& next);
    
    static void handle_decode_rule_num_mask(ParserData& prsd,
                                            ParserTemp& temp,
                                            const BlockType block,
                                            const char*& next);
    
    static void handle_decode_state_num_offset(ParserData& prsd,
                                               ParserTemp& temp,
                                               const BlockType block,
                                               const char*& next);
    
    static void handle_decode_state_num_shift(ParserData& prsd,
                                              ParserTemp& temp,
                                              const BlockType block,
                                              const char*& next);
    
    static void handle_decode_state_num_mask(ParserData& prsd,
                                             ParserTemp& temp,
                                             const BlockType block,
                                             const char*& next);
    
    static void handle_decode_fallback_num_offset(ParserData& prsd,
                                                  ParserTemp& temp,
                                                  const BlockType block,
                                                  const char*& next);
    
    static void handle_decode_fallback_num_shift(ParserData& prsd,
                                                 ParserTemp& temp,
                                                 const BlockType block,
                                                 const char*& next);
    
    static void handle_decode_fallback_num_mask(ParserData& prsd,
                                                ParserTemp& temp,
                                                const BlockType block,
                                                const char*& next);
    
    static void handle_decode_opcode_map(ParserData& prsd,
                                         ParserTemp& temp,
                                         const BlockType block,
                                         const char*& next);
    
    static void handle_decode_instruction_count(ParserData& prsd,
                                                ParserTemp& temp,
                                                const BlockType block,
                                                const char*& next);
    
    static void handle_decode_operand_count(ParserData& prsd,
                                            ParserTemp& temp,
                                            const BlockType block,
                                            const char*& next);
    
    static void handle_decode_instruction_list(ParserData& prsd,
                                               ParserTemp& temp,
                                               const BlockType block,
                                               const char*& next);
    
    static void handle_decode_register_count(ParserData& prsd,
                                             ParserTemp& temp,
                                             const BlockType block,
                                             const char*& next);
    
    static void handle_decode_register_list(ParserData& prsd,
                                            ParserTemp& temp,
                                            const BlockType block,
                                            const char*& next);
    
    static void handle_decode_ast_count(ParserData& prsd,
                                        ParserTemp& temp,
                                        const BlockType block,
                                        const char*& next);
    
    static void handle_decode_string_count(ParserData& prsd,
                                           ParserTemp& temp,
                                           const BlockType block,
                                           const char*& next);
    
    static void handle_decode_string_list(ParserData& prsd,
                                          ParserTemp& temp,
                                          const BlockType block,
                                          const char*& next);
    
    static void handle_decode_eof(ParserData& prsd,
                                  ParserTemp& temp,
                                  const BlockType block,
                                  const char*& next);
    
    static DecodeHandler decode_handler[];

    //
    //  Encode/decode utilities. 
    //

    static void encode_int(int64_t i, std::ostream& os);
    static int64_t decode_int(const char*& next);

    static void encode_string(std::string s, std::ostream& os);
    static std::string decode_string(const char*& next);

    static std::string block_name[];

};

} // namespace hoshi

#endif // PARSER_DATA_H
