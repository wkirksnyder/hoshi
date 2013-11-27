//
//  ParserEngine                                                         
//  ------------                                                         
//                                                                       
//  The virtual machine used to parse source. Here we have the LALR(k)   
//  parser and a supporting virtual machine. The virtual machine is used 
//  to scan tokens, build Asts and evaluate guards.                      
//

#ifndef PARSER_ENGINE_H
#define PARSER_ENGINE_H

#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include "OpcodeType.H"
#include "Parser.H"
#include "ParseAction.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

class ParserData;
class ParserEngine;

//
//  Virtual Machine Types                                              
//  ---------------------                                              
//                                                                     
//  A virtual machine is defined primarily by a list of operands and a 
//  list of instructions. Each instruction contains a pointer into the 
//  list of operands and its operands are a slice beginning there.     
//

union VCodeOperand
{
    int64_t integer;
    char32_t character;
    int64_t register_num;
    int64_t string_num;
    int64_t ast_num;
    int64_t branch_target;
};

typedef void (*VCodeHandler)(ParserEngine& prse,
                             const VCodeOperand* operands,
                             int64_t& pc,
                             int64_t location);

struct VCodeInstruction
{
    VCodeHandler handler;
    int64_t location;
    int64_t operand_offset;
};

struct VCodeRegister
{
    int64_t initial_value;
    std::string name;
};

//
//  ParserEngine             
//  ------------             
//                           
//  The ParserEngine proper. 
//

class ParserEngine final
{
public:

    ParserEngine(ParserImpl& prsi,
                 ErrorHandler& errh,
                 ParserData& prsd,
                 const Source& src,
                 Ast*& ast,
                 int64_t debug_flags)
        : prsi(prsi), errh(errh), prsd(prsd), src(src), ast(ast), debug_flags(debug_flags) {}

    ~ParserEngine();

    void parse();

    static void initialize();
    static VCodeHandler get_vcode_handler(OpcodeType opcode);
    static std::string get_vcode_name(VCodeHandler handler);
    static int get_vcode_opcode(VCodeHandler handler);

private:

    ParserImpl& prsi;
    ErrorHandler& errh;
    ParserData& prsd;
    const Source& src;
    Ast*& ast;
    int64_t debug_flags;

    //
    //  Scanner. 
    //

    struct Token
    {
        int symbol_num = 0;
        std::string lexeme = "";
        int64_t location = -1;
    };

    Token* token_buffer = nullptr;
    int token_front = 0;
    int token_rear = 0;
    int token_current = 0;
    
    int64_t scan_start_loc = 0;
    int64_t scan_next_loc = 0;
    int64_t scan_accept_loc = 0;
    int64_t scan_accept_pc = 0;
    int scan_accept_symbol_num = 0;

    //
    //  Parse stack. 
    //

    std::vector<Ast*> ast_stack;
    int ast_trail_base = 0;
    std::vector<Ast**> ast_trail;
    std::set<int> ast_dirty_base_set;
    std::set<Ast**> ast_dirty_set;

    //
    //  VM registers and memory. 
    //

    std::vector<int64_t> call_stack;
    int64_t* register_list = nullptr;
    Ast** ast_list = nullptr;

    //
    //  Static tables. 
    //

    static VCodeHandler vcode_handler[];

    static struct VCodeHandlerInfo
    {
        VCodeHandler handler;
        OpcodeType opcode;
        char* handler_name;
        char* opcode_name;
    } vcode_handler_info[];

    static void handle_error(ParserEngine& prse,
                             const VCodeOperand* operands,
                             int64_t& pc,
                             int64_t location);
    
    static void handle_null(ParserEngine& prse,
                            const VCodeOperand* operands,
                            int64_t& pc,
                            int64_t location);
    
    static void handle_halt(ParserEngine& prse,
                            const VCodeOperand* operands,
                            int64_t& pc,
                            int64_t location);
    
    static void handle_label(ParserEngine& prse,
                             const VCodeOperand* operands,
                             int64_t& pc,
                             int64_t location);
    
    static void handle_call(ParserEngine& prse,
                            const VCodeOperand* operands,
                            int64_t& pc,
                            int64_t location);
    
    static void handle_scan_start(ParserEngine& prse,
                                  const VCodeOperand* operands,
                                  int64_t& pc,
                                  int64_t location);
    
    static void handle_scan_char(ParserEngine& prse,
                                 const VCodeOperand* operands,
                                 int64_t& pc,
                                 int64_t location);
    
    static void handle_scan_accept(ParserEngine& prse,
                                   const VCodeOperand* operands,
                                   int64_t& pc,
                                   int64_t location);
    
    static void handle_scan_token(ParserEngine& prse,
                                  const VCodeOperand* operands,
                                  int64_t& pc,
                                  int64_t location);
    
    static void handle_scan_error(ParserEngine& prse,
                                  const VCodeOperand* operands,
                                  int64_t& pc,
                                  int64_t location);
    
    static void handle_ast_start(ParserEngine& prse,
                                 const VCodeOperand* operands,
                                 int64_t& pc,
                                 int64_t location);
    
    static void handle_ast_finish(ParserEngine& prse,
                                  const VCodeOperand* operands,
                                  int64_t& pc,
                                  int64_t location);
    
    static void handle_ast_new(ParserEngine& prse,
                               const VCodeOperand* operands,
                               int64_t& pc,
                               int64_t location);
    
    static void handle_ast_form(ParserEngine& prse,
                                const VCodeOperand* operands,
                                int64_t& pc,
                                int64_t location);
    
    static void handle_ast_load(ParserEngine& prse,
                                const VCodeOperand* operands,
                                int64_t& pc,
                                int64_t location);
    
    static void handle_ast_index(ParserEngine& prse,
                                 const VCodeOperand* operands,
                                 int64_t& pc,
                                 int64_t location);
    
    static void handle_ast_child(ParserEngine& prse,
                                 const VCodeOperand* operands,
                                 int64_t& pc,
                                 int64_t location);
    
    static void handle_ast_child_slice(ParserEngine& prse,
                                       const VCodeOperand* operands,
                                       int64_t& pc,
                                       int64_t location);
    
    static void handle_ast_kind(ParserEngine& prse,
                                const VCodeOperand* operands,
                                int64_t& pc,
                                int64_t location);
    
    static void handle_ast_kind_num(ParserEngine& prse,
                                    const VCodeOperand* operands,
                                    int64_t& pc,
                                    int64_t location);
    
    static void handle_ast_location(ParserEngine& prse,
                                    const VCodeOperand* operands,
                                    int64_t& pc,
                                    int64_t location);
    
    static void handle_ast_location_num(ParserEngine& prse,
                                        const VCodeOperand* operands,
                                        int64_t& pc,
                                        int64_t location);
    
    static void handle_ast_lexeme(ParserEngine& prse,
                                  const VCodeOperand* operands,
                                  int64_t& pc,
                                  int64_t location);
    
    static void handle_ast_lexeme_string(ParserEngine& prse,
                                         const VCodeOperand* operands,
                                         int64_t& pc,
                                         int64_t location);
    
    static void handle_assign(ParserEngine& prse,
                              const VCodeOperand* operands,
                              int64_t& pc,
                              int64_t location);
    
    static void handle_dump_stack(ParserEngine& prse,
                                  const VCodeOperand* operands,
                                  int64_t& pc,
                                  int64_t location);
    
    static void handle_add(ParserEngine& prse,
                           const VCodeOperand* operands,
                           int64_t& pc,
                           int64_t location);
    
    static void handle_subtract(ParserEngine& prse,
                                const VCodeOperand* operands,
                                int64_t& pc,
                                int64_t location);
    
    static void handle_multiply(ParserEngine& prse,
                                const VCodeOperand* operands,
                                int64_t& pc,
                                int64_t location);
    
    static void handle_divide(ParserEngine& prse,
                              const VCodeOperand* operands,
                              int64_t& pc,
                              int64_t location);
    
    static void handle_unary_minus(ParserEngine& prse,
                                   const VCodeOperand* operands,
                                   int64_t& pc,
                                   int64_t location);
    
    static void handle_return(ParserEngine& prse,
                              const VCodeOperand* operands,
                              int64_t& pc,
                              int64_t location);
    
    static void handle_branch(ParserEngine& prse,
                              const VCodeOperand* operands,
                              int64_t& pc,
                              int64_t location);
    
    static void handle_branch_equal(ParserEngine& prse,
                                    const VCodeOperand* operands,
                                    int64_t& pc,
                                    int64_t location);
    
    static void handle_branch_not_equal(ParserEngine& prse,
                                        const VCodeOperand* operands,
                                        int64_t& pc,
                                        int64_t location);
    
    static void handle_branch_less_than(ParserEngine& prse,
                                        const VCodeOperand* operands,
                                        int64_t& pc,
                                        int64_t location);
    
    static void handle_branch_less_equal(ParserEngine& prse,
                                         const VCodeOperand* operands,
                                         int64_t& pc,
                                         int64_t location);
    
    static void handle_branch_greater_than(ParserEngine& prse,
                                           const VCodeOperand* operands,
                                           int64_t& pc,
                                           int64_t location);
    
    static void handle_branch_greater_equal(ParserEngine& prse,
                                            const VCodeOperand* operands,
                                            int64_t& pc,
                                            int64_t location);
    

    //
    //  Utilities. 
    //

    bool valid_symbol(std::vector<int64_t>& base_state_stack, int symbol_num);

    void decode_action(const int64_t state,
                       const int symbol_num,
                       ParseActionType& action_type,
                       int64_t& goto_state,
                       int64_t& rule_num,
                       int64_t& fallback_state);

    void get_token();
    void call_vm(int64_t pc);

};

} // namespace hoshi

#endif // PARSER_ENGINE_H
