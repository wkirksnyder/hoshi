//
//  CodeGenerator                                                       
//  -------------                                                       
//                                                                      
//  We make heavy use of a virtual machine in our created parsers. This 
//  file contains a number of utilities to manage intermediate code and 
//  finally a translator into virtual machine code.                     
//

#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "OpcodeType.H"
#include "Parser.H"
#include "ParserImpl.H"
#include "ParserEngine.H"
#include "ErrorHandler.H"
#include "Grammar.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

//
//  ICodeRegister                                              
//  -------------                                              
//                                                             
//  We provide an unlimited number of named integer registers. 
//

struct ICodeRegister final
{

    friend class CodeGenerator;

public:

    std::string register_name = "";
    int64_t initial_value = 0;

    //
    //  Force these to be created in the CodeGenerator class. 
    //

    ICodeRegister(const ICodeRegister&) = delete;
    ICodeRegister(ICodeRegister&&) = delete;
    ICodeRegister& operator=(const ICodeRegister&) = delete;
    ICodeRegister& operator=(ICodeRegister&&) = delete;

private:

    ICodeRegister() = default;

};

//
//  ICodeAst                                                  
//  --------                                                  
//                                                            
//  We need to keep Ast handle (pointer to pointer) operands. 
//

struct ICodeAst final
{

    friend class CodeGenerator;

public:

    int ast_num = 0;

    //
    //  Force these to be created in the CodeGenerator class. 
    //

    ICodeAst(const ICodeAst&) = delete;
    ICodeAst(ICodeAst&&) = delete;
    ICodeAst& operator=(const ICodeAst&) = delete;
    ICodeAst& operator=(ICodeAst&&) = delete;

private:

    ICodeAst() = default;

};

//
//  ICodeLabel                                                         
//  ----------                                                         
//                                                                     
//  Labels are generated on demand and placed in a list so we can free 
//  them in the destructor.                                                              
//

struct ICodeLabel final
{

    friend class CodeGenerator;

public:

    int label_num = 0;
    std::string label_name = "";
    bool is_extern = false;
    int64_t pc = -1;

    //
    //  Force these to be created in the CodeGenerator class. 
    //

    ICodeLabel(const ICodeLabel&) = delete;
    ICodeLabel(ICodeLabel&&) = delete;
    ICodeLabel& operator=(const ICodeLabel&) = delete;
    ICodeLabel& operator=(ICodeLabel&&) = delete;

private:

    ICodeLabel() = default;

};

//
//  ICodeOperand                                                         
//  ------------                                                         
//                                                                       
//  An operand in intermediate code. In the virtual machine operands     
//  should be just integers although we will use those integers in       
//  different ways. At this level we model that as a union of            
//  integer-sized things. That means for anything larger than an integer 
//  we have to allocate the thing elsewhere and store a pointer to it in 
//  the operand.                                                         
//

union ICodeOperand final
{

    int64_t integer;
    char32_t character;
    std::string* string_ptr;
    ICodeRegister* register_ptr;
    ICodeAst* ast_ptr;
    ICodeLabel* label_ptr;

    ICodeOperand() = default;
    explicit ICodeOperand(int64_t integer) : integer(integer) {}
    explicit ICodeOperand(size_t integer) : integer(integer) {}
    explicit ICodeOperand(char32_t character) : character(character) {}
    explicit ICodeOperand(std::string* string_ptr) : string_ptr(string_ptr) {}
    explicit ICodeOperand(ICodeRegister* register_ptr) : register_ptr(register_ptr) {}
    explicit ICodeOperand(ICodeAst* ast_ptr) : ast_ptr(ast_ptr) {}
    explicit ICodeOperand(ICodeLabel* label_ptr) : label_ptr(label_ptr) {}

};

//
//  ICodeInstruction                                                  
//  ----------------                                                  
//                                                                    
//  An intermediate code instruction. We don't need much more than an 
//  opcode and a list of operands.                                    
//

struct ICodeInstruction final
{

    OpcodeType opcode = OpcodeType::OpcodeNull;
    int64_t location = 0;
    int operand_count = 0;
    ICodeOperand* operand_list = nullptr;

    ICodeInstruction() = default;

    ~ICodeInstruction() { delete [] operand_list; }

    ICodeInstruction(const ICodeInstruction& rhs)
        : opcode(rhs.opcode), location(rhs.location), operand_count(rhs.operand_count)
    {

        if (operand_count == 0)
        {
            operand_list = nullptr;
        }
        else
        {
            operand_list = new ICodeOperand[operand_count];
            memcpy(static_cast<void*>(operand_list),
                   static_cast<void*>(rhs.operand_list),
                   static_cast<size_t>(operand_count * sizeof(ICodeOperand)));
        }

    }

    ICodeInstruction(ICodeInstruction&& rhs) noexcept
        : opcode(rhs.opcode), location(rhs.location), operand_count(rhs.operand_count),
          operand_list(rhs.operand_list)
    {
        rhs.operand_list = nullptr;
        rhs.operand_count = 0;
    }

    ICodeInstruction& operator=(const ICodeInstruction& rhs)
    {
        
        if (&rhs != this)
        {

            opcode = rhs.opcode;
            location = rhs.location;
            operand_count = rhs.operand_count;

            if (operand_count == 0)
            {
                operand_list = nullptr;
            }
            else
            {
                operand_list = new ICodeOperand[operand_count];
                memcpy(static_cast<void*>(operand_list),
                       static_cast<void*>(rhs.operand_list),
                       static_cast<size_t>(operand_count * sizeof(ICodeOperand)));
            }

        }

        return *this;

    }

    ICodeInstruction& operator=(ICodeInstruction&& rhs) noexcept
    {

        opcode = rhs.opcode;
        location = rhs.location;
        operand_count = rhs.operand_count;
        std::swap(operand_list, rhs.operand_list);

        return *this;

    }

};

//
//  CodeGenerator         
//  -------------         
//                        
//  CodeGenerator proper. 
//

class CodeGenerator final
{
public:

    CodeGenerator(ParserImpl& prsi,
                  ErrorHandler& errh,
                  Grammar& gram,
                  ParserData& prsd,
                  int64_t debug_flags);

    ~CodeGenerator();

    ICodeLabel* get_label();
    ICodeLabel* get_label(const std::string& label_name);

    ICodeRegister* get_register(const std::string& name);
    ICodeRegister* get_register(const std::string& name, int64_t initial_value);

    ICodeAst* get_ast_operand();
    void free_ast_operand(ICodeAst* ast_ptr);
    void free_all_asts();

    std::string* get_string(const std::string& value);

    void emit(OpcodeType opcode, int64_t location,
              const ICodeOperand& operand1 = operand_missing,
              const ICodeOperand& operand2 = operand_missing,
              const ICodeOperand& operand3 = operand_missing,
              const ICodeOperand& operand4 = operand_missing,
              const ICodeOperand& operand5 = operand_missing);

    void emit(OpcodeType opcode, int64_t location,
              const std::vector<ICodeOperand>& operand_list);

    void dump_icode(std::ostream& os = std::cout, int indent = 0) const;
    
    ICodeRegister* get_temporary();
    bool is_temporary(ICodeRegister* register_ptr) const;
    void free_temporary(ICodeRegister* register_ptr);
    void free_all_temporaries();

    void generate();

    //
    //  Disable copying. 
    //

    CodeGenerator(const CodeGenerator&) = delete;
    CodeGenerator(CodeGenerator&&) = delete;
    CodeGenerator& operator=(const CodeGenerator&) = delete;
    CodeGenerator& operator=(CodeGenerator&&) = delete;

private:

    ParserImpl& prsi;
    ErrorHandler& errh;
    Grammar& gram;
    ParserData& prsd;
    int64_t debug_flags;

    static ICodeOperand operand_missing;

    struct OpcodeInfo
    {
        char* name;
        bool is_branch;
        bool is_no_follow;
        OpcodeType inverse_branch;
    };

    static OpcodeInfo opcode_table[];

    std::set<ICodeRegister*> temporary_set;
    std::queue<ICodeRegister*> temporary_queue;

    std::vector<ICodeLabel*> label_list;
    std::map<std::string, ICodeRegister*> register_map;
    std::vector<ICodeAst*> ast_list;
    std::queue<ICodeAst*> ast_queue;
    std::map<std::string, std::string*> string_map;

    std::vector<ICodeInstruction> icode_list;

    void optimize();

};

} // namespace hoshi

#endif // CODE_GENERATOR_H
