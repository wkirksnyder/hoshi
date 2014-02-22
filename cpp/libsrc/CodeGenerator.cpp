#line 425 "u:\\hoshi\\raw\\CodeGenerator.cpp"
//
//  CodeGenerator                                                       
//  -------------                                                       
//                                                                      
//  We make heavy use of a virtual machine in our created parsers. This 
//  file contains a number of utilities to manage intermediate code and 
//  finally a translator into virtual machine code.                     
//

#include <cstdint>
#include <cstring>
#include <functional>
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
#include "ErrorHandler.H"
#include "ParserImpl.H"
#include "Grammar.H"
#include "CodeGenerator.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;

//
//  flag that an operand argument is missing. 
//

ICodeOperand CodeGenerator::operand_missing;

//
//  Static information about opcode types. 
//

CodeGenerator::OpcodeInfo CodeGenerator::opcode_table[] = {
    {  "Null",                false,  false,  OpcodeType::OpcodeNull                },
    {  "Halt",                false,  true,   OpcodeType::OpcodeNull                },
    {  "Label",               false,  false,  OpcodeType::OpcodeNull                },
    {  "Call",                false,  false,  OpcodeType::OpcodeNull                },
    {  "ScanStart",           false,  false,  OpcodeType::OpcodeNull                },
    {  "ScanChar",            false,  true,   OpcodeType::OpcodeNull                },
    {  "ScanAccept",          false,  false,  OpcodeType::OpcodeNull                },
    {  "ScanToken",           false,  false,  OpcodeType::OpcodeNull                },
    {  "ScanError",           false,  false,  OpcodeType::OpcodeNull                },
    {  "AstStart",            false,  false,  OpcodeType::OpcodeNull                },
    {  "AstFinish",           false,  false,  OpcodeType::OpcodeNull                },
    {  "AstNew",              false,  false,  OpcodeType::OpcodeNull                },
    {  "AstForm",             false,  false,  OpcodeType::OpcodeNull                },
    {  "AstLoad",             false,  false,  OpcodeType::OpcodeNull                },
    {  "AstIndex",            false,  false,  OpcodeType::OpcodeNull                },
    {  "AstChild",            false,  false,  OpcodeType::OpcodeNull                },
    {  "AstChildSlice",       false,  false,  OpcodeType::OpcodeNull                },
    {  "AstKind",             false,  false,  OpcodeType::OpcodeNull                },
    {  "AstKindNum",          false,  false,  OpcodeType::OpcodeNull                },
    {  "AstLocation",         false,  false,  OpcodeType::OpcodeNull                },
    {  "AstLocationNum",      false,  false,  OpcodeType::OpcodeNull                },
    {  "AstLexeme",           false,  false,  OpcodeType::OpcodeNull                },
    {  "AstLexemeString",     false,  false,  OpcodeType::OpcodeNull                },
    {  "Assign",              false,  false,  OpcodeType::OpcodeNull                },
    {  "DumpStack",           false,  false,  OpcodeType::OpcodeNull                },
    {  "Add",                 false,  false,  OpcodeType::OpcodeNull                },
    {  "Subtract",            false,  false,  OpcodeType::OpcodeNull                },
    {  "Multiply",            false,  false,  OpcodeType::OpcodeNull                },
    {  "Divide",              false,  false,  OpcodeType::OpcodeNull                },
    {  "UnaryMinus",          false,  false,  OpcodeType::OpcodeNull                },
    {  "Return",              false,  true,   OpcodeType::OpcodeNull                },
    {  "Branch",              true,   true,   OpcodeType::OpcodeNull                },
    {  "BranchEqual",         true,   false,  OpcodeType::OpcodeBranchNotEqual      },
    {  "BranchNotEqual",      true,   false,  OpcodeType::OpcodeBranchEqual         },
    {  "BranchLessThan",      true,   false,  OpcodeType::OpcodeBranchGreaterEqual  },
    {  "BranchLessEqual",     true,   false,  OpcodeType::OpcodeBranchGreaterThan   },
    {  "BranchGreaterThan",   true,   false,  OpcodeType::OpcodeBranchLessEqual     },
    {  "BranchGreaterEqual",  true,   false,  OpcodeType::OpcodeBranchLessThan      }
#line 508 "u:\\hoshi\\raw\\CodeGenerator.cpp"
};

//
//  constructor       
//  -----------       
//                    
//  Emit prolog code. 
//

CodeGenerator::CodeGenerator(ParserImpl& prsi,
                             ErrorHandler& errh,
                             Grammar& gram,
                             ParserData& prsd,
                             int64_t debug_flags)
    : prsi(prsi), errh(errh), gram(gram), prsd(prsd), debug_flags(debug_flags)
{
    
    ICodeLabel* prolog_label = get_label("Prolog");
    prolog_label->is_extern = true;

    emit(OpcodeType::OpcodeLabel, -1, ICodeOperand(prolog_label));

    emit(OpcodeType::OpcodeAssign, -1,
         ICodeOperand(get_register("token_count", 0)),
         ICodeOperand(get_register("0", 0)));

    emit(OpcodeType::OpcodeReturn, -1);

}

//
//  destructor                                                        
//  ----------                                                        
//                                                                    
//  In the destructor we delete all the small objects allocated here. 
//

CodeGenerator::~CodeGenerator()
{

    for (ICodeLabel* label: label_list)
    {
        delete label;
    }

    for (auto mp: register_map)
    {
        delete mp.second;
    }

    for (auto mp: string_map)
    {
        delete mp.second;
    }

}

//
//  get_label                                                         
//  ---------
//                                                                    
//  Labels are just markers to be located by the caller explicitly by 
//  emitting a label instruction.                                     
//

ICodeLabel* CodeGenerator::get_label()
{

    ICodeLabel* label = new ICodeLabel();
    label_list.push_back(label);
    label->label_num = label_list.size();
    label->label_name = "";
    label->is_extern = false;

    return label;

}

ICodeLabel* CodeGenerator::get_label(const string& label_name)
{

    ICodeLabel* label = new ICodeLabel();
    label_list.push_back(label);
    label->label_num = label_list.size();
    label->label_name = label_name;
    label->is_extern = false;

    return label;

}

//
//  get_register
//  ------------
//                                    
//  A register is an integer counter. 
//

ICodeRegister* CodeGenerator::get_register(const string& name)
{

    if (register_map.find(name) == register_map.end())
    {

        ICodeRegister* register_ptr = new ICodeRegister();
        register_ptr->register_name = name;
        register_ptr->initial_value = 0;
        register_map[name] = register_ptr;

        return register_ptr;

    }

    return register_map[name];

}

ICodeRegister* CodeGenerator::get_register(const string& name, int64_t initial_value)
{

    ICodeRegister* register_ptr = get_register(name);
    register_ptr->initial_value = initial_value;

    return register_ptr;

}

//
//  get_ast_operand                                                        
//  ---------------                                                        
//                                                                         
//  Get an available Ast operand. Using an already allocated one if we can 
//  and allocating one if we can't.                                        
//

ICodeAst* CodeGenerator::get_ast_operand()
{

    if (ast_queue.empty())
    {

        ICodeAst* ast_ptr = new ICodeAst();
        ast_ptr->ast_num = ast_list.size();
        ast_list.push_back(ast_ptr);

        return ast_ptr;

    }

    ICodeAst* ast_ptr = ast_queue.front();
    ast_queue.pop();

    return ast_ptr;

}

//
//  free_ast_operand                          
//  ----------------                          
//                                            
//  Free the ast operand when we are through. 
//

void CodeGenerator::free_ast_operand(ICodeAst* ast_ptr)
{
    ast_queue.push(ast_ptr);
}

//
//  free_all_asts                                                       
//  -------------                                                       
//                                                                      
//  Free all our allocated Asts. We should do this before starting on a 
//  reduce action.                                                      
//

void CodeGenerator::free_all_asts()
{

    while (ast_queue.size() > 0)
    {
        ast_queue.pop();
    }

    for (ICodeAst* ast_ptr: ast_list)
    {
        ast_queue.push(ast_ptr);
    }

}

//
//  get_string
//  ----------
//                                    
//  We keep a table of string literals in the data module.
//

string* CodeGenerator::get_string(const string& value)
{

    if (string_map.find(value) == string_map.end())
    {

        string* string_ptr = new string(value);
        string_map[value] = string_ptr;

        return string_ptr;

    }

    return string_map[value];

}

//
//  Temporaries                                                
//  -----------                                                
//                                                             
//  We allocate temporary registers for intermediate values in 
//  expressions.                                               
//

ICodeRegister* CodeGenerator::get_temporary()
{

    if (temporary_queue.empty())
    {

        ostringstream ost;

        ost << "Temp$" << temporary_set.size();
        ICodeRegister* register_ptr = get_register(ost.str());
        temporary_set.insert(register_ptr);

        return register_ptr;

    }

    ICodeRegister* register_ptr = temporary_queue.front();
    temporary_queue.pop();

    return register_ptr;

}

bool CodeGenerator::is_temporary(ICodeRegister* register_ptr) const
{
    return temporary_set.find(register_ptr) != temporary_set.end();
}

void CodeGenerator::free_temporary(ICodeRegister* register_ptr)
{
    temporary_queue.push(register_ptr);
}

void CodeGenerator::free_all_temporaries()
{

    while (temporary_queue.size() > 0)
    {
        temporary_queue.pop();
    }

    for (ICodeRegister* register_ptr: temporary_set)
    {
        temporary_queue.push(register_ptr);
    }

}

//
//  emit                                                                
//  ----                                                                
//                                                                      
//  The compiler I'm using doesn't support initializer_list yet and I   
//  don't feel like going back to the old C varargs stuff. This lets me 
//  have 5 operands per instruction. If I need more than that I have to 
//  use a vector.                                                       
//

void CodeGenerator::emit(OpcodeType opcode, int64_t location,
                         const ICodeOperand& operand1,
                         const ICodeOperand& operand2,
                         const ICodeOperand& operand3,
                         const ICodeOperand& operand4,
                         const ICodeOperand& operand5)
{

    ICodeInstruction instruction;
    instruction.opcode = opcode;
    instruction.location = location;

    if (&operand1 == &operand_missing)
    {
        instruction.operand_count = 0;
    }
    else if (&operand2 == &operand_missing)
    {
        instruction.operand_count = 1;
    }
    else if (&operand3 == &operand_missing)
    {
        instruction.operand_count = 2;
    }
    else if (&operand4 == &operand_missing)
    {
        instruction.operand_count = 3;
    }
    else if (&operand5 == &operand_missing)
    {
        instruction.operand_count = 4;
    }
    else
    {
        instruction.operand_count = 5;
    }
    
    if (instruction.operand_count == 0)
    {
        instruction.operand_list = nullptr;
    }
    else
    {
        instruction.operand_list = new ICodeOperand[instruction.operand_count];
    }

    if (instruction.operand_count > 0)    
    {
        instruction.operand_list[0] = operand1;
    }
    if (instruction.operand_count > 1)    
    {
        instruction.operand_list[1] = operand2;
    }
    if (instruction.operand_count > 2)    
    {
        instruction.operand_list[2] = operand3;
    }
    if (instruction.operand_count > 3)    
    {
        instruction.operand_list[3] = operand4;
    }
    if (instruction.operand_count > 4)    
    {
        instruction.operand_list[4] = operand5;
    }

    icode_list.push_back(instruction);

}

void CodeGenerator::emit(OpcodeType opcode, int64_t location,
                         const vector<ICodeOperand>& operand_list)
{

    ICodeInstruction instruction;
    instruction.opcode = opcode;
    instruction.location = location;
    instruction.operand_count = operand_list.size();
    
    if (instruction.operand_count == 0)
    {
        instruction.operand_list = nullptr;
    }
    else
    {
        instruction.operand_list = new ICodeOperand[instruction.operand_count];
    }

    memcpy(static_cast<void*>(instruction.operand_list),
           static_cast<void*>(const_cast<ICodeOperand*>(operand_list.data())),
           static_cast<size_t>(operand_list.size() * sizeof(ICodeOperand)));

    icode_list.push_back(instruction);

}

//
//  generate                                                            
//  --------                                                            
//                                                                      
//  Optimize our intermediate code, convert it to VM code and inject it 
//  into the parser.                                                    
//

void CodeGenerator::generate()
{

    map<ICodeRegister*, int64_t> register_num_map;
    map<string*, int64_t> string_num_map;

    int64_t next_operand;
    int64_t next_instruction;

    //
    //  encode_integer_operand.
    //

    function<void(ICodeOperand)> encode_integer_operand = [&](ICodeOperand operand) -> void
    {
        prsd.operand_list[next_operand++].integer = operand.integer;
    };

    //
    //  encode_kind_operand.
    //

    function<void(ICodeOperand)> encode_kind_operand = [&](ICodeOperand operand) -> void
    {
        prsd.operand_list[next_operand++].integer = operand.integer;
    };

    //
    //  encode_character_operand. 
    //

    function<void(ICodeOperand)> encode_character_operand = [&](ICodeOperand operand) -> void
    {
        prsd.operand_list[next_operand++].character = operand.character;
    };

    //
    //  encode_register_operand. 
    //

    function<void(ICodeOperand)> encode_register_operand = [&](ICodeOperand operand) -> void
    {
        prsd.operand_list[next_operand++].register_num = register_num_map[operand.register_ptr];
    };

    //
    //  encode_ast_operand.
    //

    function<void(ICodeOperand)> encode_ast_operand = [&](ICodeOperand operand) -> void
    {
        prsd.operand_list[next_operand++].ast_num = operand.ast_ptr->ast_num;
    };

    //
    //  encode_string_operand. 
    //

    function<void(ICodeOperand)> encode_string_operand = [&](ICodeOperand operand) -> void
    {
        prsd.operand_list[next_operand++].string_num = string_num_map[operand.string_ptr];
    };

    //
    //  encode_label_operand. 
    //

    function<void(ICodeOperand)> encode_label_operand = [&](ICodeOperand operand) -> void
    {
        prsd.operand_list[next_operand++].branch_target = operand.label_ptr->pc;
    };

    //
    //  generate
    //  --------
    //                                 
    //  The function body begins here. Optimize the intermediate code. 
    //

    if ((debug_flags & DebugType::DebugProgress) != 0)
    {
        cout << "Beginning code generation: " << prsi.elapsed_time_string() << endl;
    }

    if ((debug_flags & DebugType::DebugICode) != 0)
    {
        dump_icode();
    }

    optimize();

    if ((debug_flags & DebugType::DebugICode) != 0)
    {
        dump_icode();
    }

    //
    //  Allocate registers. 
    //

    int64_t register_count = 0;
    for (auto mp: register_map)
    {
        register_num_map[mp.second] = register_count++;
    }

    prsd.register_count = register_count;
    prsd.register_list = new VCodeRegister[prsd.register_count];

    for (auto mp: register_map)
    {
        int64_t register_num = register_num_map[mp.second];
        prsd.register_list[register_num].name = mp.second->register_name;
        prsd.register_list[register_num].initial_value = mp.second->initial_value;
    }
        
    //
    //  Allocate Asts.
    //

    prsd.ast_count = ast_list.size();

    //
    //  Allocate strings. 
    //

    int64_t string_count = 0;
    for (auto mp: string_map)
    {
        string_num_map[mp.second] = string_count++;
    }

    prsd.string_count = string_count;
    prsd.string_list = new string[prsd.string_count];

    for (auto mp: string_map)
    {
        int64_t string_num = string_num_map[mp.second];
        prsd.string_list[string_num] = *mp.second;
    }
        
    //
    //  Count instructions and operands and find the locations of all 
    //  labels.                                                       
    //

    int64_t instruction_count = 0;
    int64_t operand_count = 0;
    for (int i = 0; i < icode_list.size(); i++)
    {

        if (icode_list[i].opcode == OpcodeType::OpcodeLabel)
        {
            icode_list[i].operand_list[0].label_ptr->pc = instruction_count;
            continue;
        }

        if (icode_list[i].opcode != OpcodeType::OpcodeNull)
        {
            operand_count += icode_list[i].operand_count;
            instruction_count++;
        }

    }

    //
    //  Encode the VM instructions. 
    //

    prsd.instruction_count = instruction_count;
    prsd.instruction_list = new VCodeInstruction[prsd.instruction_count];

    prsd.operand_count = operand_count;
    prsd.operand_list = new VCodeOperand[prsd.operand_count];

    next_instruction = 0;
    next_operand = 0;

    for (ICodeInstruction instruction: icode_list)
    {

        if (instruction.opcode == OpcodeType::OpcodeLabel ||
            instruction.opcode == OpcodeType::OpcodeNull)   
        {
            continue;
        }

        prsd.instruction_list[next_instruction].handler =
            ParserEngine::get_vcode_handler(instruction.opcode);
        prsd.instruction_list[next_instruction].location = instruction.location;
        prsd.instruction_list[next_instruction].operand_offset = next_operand;
        next_instruction++;

        switch (instruction.opcode)
        {

            case OpcodeCall:
            case OpcodeBranch:
            {
            
                int operand = 0;
                encode_label_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeScanChar:
            {
            
                int operand = 0;
            
                encode_integer_operand(instruction.operand_list[operand]);
                for (int i = 0; i < instruction.operand_list[operand].integer; i++)    {
            
                    encode_character_operand(instruction.operand_list[operand + 3 * i + 1]);
                    encode_character_operand(instruction.operand_list[operand + 3 * i + 2]);
                    encode_label_operand(instruction.operand_list[operand + 3 * i + 3]);
            
                }
            
                operand += instruction.operand_list[operand].integer * 3 + 1;
                break;
            
            }
            
            case OpcodeScanAccept:
            {
            
                int operand = 0;
                encode_integer_operand(instruction.operand_list[operand++]);
                encode_label_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeScanError:
            case OpcodeAstLexemeString:
            {
            
                int operand = 0;
                encode_string_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeAstStart:
            case OpcodeAstNew:
            {
            
                int operand = 0;
                encode_register_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeAstFinish:
            case OpcodeAstLocationNum:
            {
            
                int operand = 0;
                encode_integer_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeAstForm:
            {
            
                int operand = 0;
                encode_register_operand(instruction.operand_list[operand++]);
                encode_register_operand(instruction.operand_list[operand++]);
                encode_integer_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeAstLoad:
            {
            
                int operand = 0;
                encode_ast_operand(instruction.operand_list[operand++]);
                encode_register_operand(instruction.operand_list[operand++]);
                encode_integer_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeAstIndex:
            {
            
                int operand = 0;
                encode_ast_operand(instruction.operand_list[operand++]);
                encode_integer_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeAstChild:
            case OpcodeAstKind:
            case OpcodeAstLocation:
            case OpcodeAstLexeme:
            {
            
                int operand = 0;
                encode_ast_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeAstChildSlice:
            {
            
                int operand = 0;
                encode_ast_operand(instruction.operand_list[operand++]);
                encode_integer_operand(instruction.operand_list[operand++]);
                encode_integer_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeAstKindNum:
            {
            
                int operand = 0;
                encode_kind_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeAssign:
            case OpcodeUnaryMinus:
            {
            
                int operand = 0;
                encode_register_operand(instruction.operand_list[operand++]);
                encode_register_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
            case OpcodeAdd:
            case OpcodeSubtract:
            case OpcodeMultiply:
            case OpcodeDivide:
            {
            
                int operand = 0;
                encode_register_operand(instruction.operand_list[operand++]);
                encode_register_operand(instruction.operand_list[operand++]);
                encode_register_operand(instruction.operand_list[operand++]);
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
                encode_label_operand(instruction.operand_list[operand++]);
                encode_register_operand(instruction.operand_list[operand++]);
                encode_register_operand(instruction.operand_list[operand++]);
                break;
            
            }
            
#line 1166 "u:\\hoshi\\raw\\CodeGenerator.cpp"
        }

    }

    if ((debug_flags & DebugType::DebugProgress) != 0)
    {
        cout << "Finished code generation: " << prsi.elapsed_time_string() << endl;
    }

}

//
//  optimize                                                           
//  --------                                                           
//                                                                     
//  This function does a light optimization of the intermediate code,  
//  removing only the most emabarassing stupidity. It basically does a 
//  peephole optimization on branches.                                 
//

void CodeGenerator::optimize()
{

    bool any_changes = true;

    map<ICodeLabel*, int64_t> branch_target_map;
    set<ICodeLabel*> used_labels;

    ICodeInstruction null_instruction;
    null_instruction.opcode = OpcodeType::OpcodeNull;
    null_instruction.location = -1;
    null_instruction.operand_count = 0;
    null_instruction.operand_list = nullptr;

    //
    //  next_active_instruction                                          
    //  -----------------------                                          
    //                                                                   
    //  Starting from a given instruction find the next instruction that 
    //  will execute.                                                    
    //

    function<int64_t(int64_t)> next_active_instruction = [&](int64_t start) -> int64_t
    {

        for (int64_t result = start;; result++)
        {

            if (result >= icode_list.size())
            {
                return start;
            }
            
            if (icode_list[result].opcode == OpcodeType::OpcodeLabel ||
                icode_list[result].opcode == OpcodeType::OpcodeNull)   
            {
                continue;
            }

            return result;

        }
  
    };

    //
    //  short_circuit                                             
    //  -------------                                             
    //                                                                   
    //  Handle labels, short circuiting them and recording that they are 
    //  used.                                                            
    //

    function<void(int64_t, int64_t)> short_circuit = [&](int64_t instruction, int64_t operand) -> void
    {

        ICodeLabel* label_ptr = icode_list[instruction].operand_list[operand].label_ptr;

        if (branch_target_map.find(label_ptr) == branch_target_map.end())
        {
            cout << "Branch to non-existent instruction" << endl;
            exit(1);
        }

        int64_t target = next_active_instruction(branch_target_map[label_ptr]);

        if (icode_list[target].opcode == OpcodeType::OpcodeBranch)
        { 
            icode_list[instruction].operand_list[operand] = icode_list[target].operand_list[0];
            any_changes = true;
        }

        used_labels.insert(icode_list[instruction].operand_list[operand].label_ptr);

    };

    //
    //  optimize                     
    //  --------
    //                                 
    //  The function body begins here. 
    //

    any_changes = true;
    while (any_changes)
    {

        any_changes = false;
        branch_target_map.clear();
        used_labels.clear();

        //
        //  Assume all extern labels are used. 
        //

        for (ICodeLabel* label: label_list)
        {
            if (label->is_extern)
            {
                used_labels.insert(label);
            }
        }

        //
        //  Find the locations of all labels. 
        //

        for (int64_t i = 0; i < icode_list.size(); i++)
        {

            if (icode_list[i].opcode == OpcodeType::OpcodeLabel)
            {
                branch_target_map[icode_list[i].operand_list[0].label_ptr] = i;
                continue;
            }

        }
    
        //
        //  Loop over all the branches short-circuiting branches and 
        //  accumulating the labels actually used.                   
        //

        for (int64_t i = 0; i < icode_list.size(); i++)
        {

            ICodeInstruction instruction = icode_list[i];

            if (icode_list[i].opcode == OpcodeType::OpcodeLabel ||
                icode_list[i].opcode == OpcodeType::OpcodeNull)   
            {
                continue;
            }

            switch (instruction.opcode)
            {

                case OpcodeCall:
                case OpcodeBranch:
                {
                    short_circuit(i, 0);
                    break;
                }
                
                case OpcodeScanChar:
                {
                    for (int64_t j = 0; j < instruction.operand_list[0].integer; j++)
                    {
                        short_circuit(i, 3 * j + 3);
                    }
                    break;
                }
                
                case OpcodeScanAccept:
                {
                    short_circuit(i, 1);
                    break;
                }
                
                case OpcodeBranchEqual:
                case OpcodeBranchNotEqual:
                case OpcodeBranchLessThan:
                case OpcodeBranchLessEqual:
                case OpcodeBranchGreaterThan:
                case OpcodeBranchGreaterEqual:
                {
                    short_circuit(i, 0);
                    break;
                }
                
#line 1411 "u:\\hoshi\\raw\\CodeGenerator.cpp"
            }

        }

        //
        //  Remove unused labels. 
        //

        for (int64_t i = 0; i < icode_list.size(); i++)
        {

            if (icode_list[i].opcode == OpcodeType::OpcodeLabel)
            {
                if (used_labels.find(icode_list[i].operand_list[0].label_ptr) == used_labels.end())
                {
                    icode_list[i] = null_instruction;
                    any_changes = true;
                }
            }

        }
    
        //
        //  Remove branches to the next instruction.
        //

        for (int64_t i = 0; i < icode_list.size(); i++)
        {

            if (!opcode_table[icode_list[i].opcode].is_branch)
            {
                continue;
            }

            ICodeLabel* label_ptr = icode_list[i].operand_list[0].label_ptr;

            if (branch_target_map.find(label_ptr) == branch_target_map.end())
            {
                cout << "Branch to non-existent instruction" << endl;
                exit(1);
            }

            if (branch_target_map[label_ptr] > i &&
                branch_target_map[label_ptr] < next_active_instruction(i + 1))
            {
                icode_list[i] = null_instruction;
                any_changes = true;
                continue;
            }

        }

        //
        //  A conditional branch around an unconditional branch can be 
        //  replaced by the opposite unconditional branch.             
        //

        for (int64_t i = 0; i < icode_list.size(); i++)
        {

            if (opcode_table[icode_list[i].opcode].inverse_branch == OpcodeType::OpcodeNull)
            {
                continue;
            }

            int64_t j = i + 1;

            if (icode_list[j].opcode != OpcodeType::OpcodeBranch)
            {
                continue;
            }

            ICodeLabel* label_ptr = icode_list[i].operand_list[0].label_ptr;

            if (branch_target_map.find(label_ptr) == branch_target_map.end())
            {
                cout << "Branch to non-existent instruction" << endl;
                exit(1);
            }

            if (branch_target_map[label_ptr] > j &&
                branch_target_map[label_ptr] < next_active_instruction(j + 1))
            {

                icode_list[i].operand_list[0] = icode_list[j].operand_list[0];
                icode_list[i].opcode = opcode_table[icode_list[i].opcode].inverse_branch;

                icode_list[j] = null_instruction;
                any_changes = true;

                continue;

            }

        }

        //
        //  Anything following a branch up to the next label can be 
        //  deleted.                                                
        //

        for (int64_t i = 0; i < icode_list.size(); i++)
        {

            if (!opcode_table[icode_list[i].opcode].is_no_follow)
            {
                continue;
            }

            for (int64_t j = i + 1;
                 j < icode_list.size() && icode_list[j].opcode != OpcodeType::OpcodeLabel;
                 j++)
            {
                icode_list[j] = null_instruction;
                any_changes = true;
            }

        }

        //
        //  Remove deleted instructions. 
        //

        vector<ICodeInstruction> new_icode_list;
        for (ICodeInstruction instruction : icode_list)
        {

            if (instruction.opcode != OpcodeType::OpcodeNull)
            {
                new_icode_list.push_back(instruction);
            }

        }

        icode_list.swap(new_icode_list);

    }

}

//
//  dump_icode                                                          
//  ----------                                                          
//                                                                      
//  Print the intermediate code on the console. This is quite a bit of  
//  work but essential. Debugging is a nightmare without something like 
//  this.                                                               
//

void CodeGenerator::dump_icode(std::ostream& os, int indent) const
{

    const int max_line_width = 95;
    const int line_num_width = 6;
    const int label_width = 8;
    const int opcode_width = 8;
    const int operand_width = 12;

    ostringstream ost;
    int next_column = 0;

    //
    //  dump_line_num                                              
    //  -------------                                              
    //                                                                
    //  Dump the line number. This is useful after allocating labels. 
    //

    function<void(int)> dump_line_num = [&](int line_num) -> void
    {

        if (ost.str().size() < indent + line_num_width)
        {
            os << ost.str();
            ost.str("");
        }
            
        next_column = indent + line_num_width;
        ost << setw(next_column - ost.str().size()) << line_num << setw(0) << " ";
        next_column++;

    };

    //
    //  dump_label          
    //  ----------          
    //                      
    //  Dump a label value. 
    //

    function<void(string)> dump_label = [&](string value) -> void
    {

        if (ost.str().size() < next_column)
        {
            ost << setw(next_column - ost.str().size()) << "" << setw(0) << " ";
        }
        
        ost << left << value << right;
        next_column = next_column + label_width + 1;

    };

    //
    //  dump_opcode          
    //  -----------          
    //                      
    //  Dump an opcode. 
    //

    function<void(string)> dump_opcode = [&](string value) -> void
    {

        next_column = indent + line_num_width + 1 + label_width + 1;

        if (ost.str().size() < next_column)
        {
            ost << setw(next_column - ost.str().size()) << "" << setw(0) << " ";
        }
        
        ost << left << value << right;
        next_column = next_column + opcode_width + 1;

    };

    //
    //  dump_operand                                                   
    //  ------------                                                   
    //                                                                 
    //  Dump an operand. These are right justified and can wrap lines. 
    //

    function<void(string)> dump_operand = [&](string value) -> void
    {

        if (next_column > max_line_width)
        {
            os << ost.str() << endl;
            next_column = indent + line_num_width + 1 + label_width + 1 + opcode_width + 1;
            ost.str("");
        }

        next_column = next_column + operand_width;
        ost << right << setw(next_column - ost.str().size()) << value << setw(0) << " ";
        next_column++;

    };

    //
    //  integer_string                             
    //  --------------                             
    //                                           
    //  Return a printable string for a integer. 
    //

    function<string(ICodeOperand)> integer_string = [&](ICodeOperand operand) -> string
    {
        return to_string(operand.integer);
    };

    //
    //  kind_string                             
    //  -----------                             
    //                                           
    //  Return a printable string for a integer. 
    //

    function<string(ICodeOperand)> kind_string = [&](ICodeOperand operand) -> string
    {
        return to_string(operand.integer);
    };

    //
    //  character_string                            
    //  ----------------                            
    //                                              
    //  Return a printable string from a character. 
    //

    function<string(ICodeOperand)> character_string = [&](ICodeOperand operand) -> string
    {

        switch (operand.character)
        {

            case '\\': return "'\\\\'";
            case '\n': return "'\\n'";
            case '\r': return "'\\r'";
            case '\t': return "'\\t'";

            default:
            {

                ostringstream ost;

                if (operand.character >= ' ' && operand.character < 128)
                {
                    ost << "'" << static_cast<char>(operand.character) << "'";    
                }
                else
                {
                    ost << setfill('0') << setw(8) << hex << operand.character;
                }

                return ost.str();

            }
               
        }   

        return "****";

    };

    //
    //  register_string                             
    //  ---------------                             
    //                                           
    //  Return a printable string for a register. 
    //

    function<string(ICodeOperand)> register_string = [&](ICodeOperand operand) -> string
    {
        return operand.register_ptr->register_name;
    };

    //
    //  ast_string                             
    //  ----------                             
    //                                           
    //  Return a printable string for an ast operand.
    //

    function<string(ICodeOperand)> ast_string = [&](ICodeOperand operand) -> string
    {
        return "Ast$" + to_string(operand.ast_ptr->ast_num);
    };

    //
    //  string_string                             
    //  -------------                             
    //                                           
    //  Return a printable string for a string. 
    //

    function<string(ICodeOperand)> string_string = [&](ICodeOperand operand) -> string
    {
        ostringstream ost;
        ost << "\"" << *operand.string_ptr << "\"";
        return ost.str();
    };

    //
    //  label_string                            
    //  ------------                            
    //                                          
    //  Return a printable string from a label. 
    //

    function<string(ICodeOperand)> label_string = [&](ICodeOperand operand) -> string
    {

        if (operand.label_ptr->label_name.size() > 0)
        {
            return operand.label_ptr->label_name;
        }

        ostringstream ost;
        ost << "Lab$" << operand.label_ptr->label_num;

        return ost.str();

    };

    //
    //  dump_icode                     
    //  ----------                     
    //                                 
    //  The function body begins here. 
    //

    cout << "Intermediate Code" << endl << endl;

    int line_num = 1;
    for (ICodeInstruction instruction: icode_list)
    {

        ost.str("");
        dump_line_num(line_num++);

        if (instruction.opcode == OpcodeLabel)
        {
            dump_label(label_string(instruction.operand_list[0]));
        }

        dump_opcode(opcode_table[instruction.opcode].name);
        cout << flush;

        switch (instruction.opcode)
        {

            case OpcodeLabel:
            case OpcodeCall:
            case OpcodeBranch:
            {
            
                int operand = 0;
                dump_operand(label_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeScanChar:
            {
            
                int operand = 0;
            
                dump_operand(integer_string(instruction.operand_list[operand]));
                for (int i = 0; i < instruction.operand_list[operand].integer; i++)    {
            
                    dump_operand(character_string(instruction.operand_list[operand + 3 * i + 1]));
                    dump_operand(character_string(instruction.operand_list[operand + 3 * i + 2]));
                    dump_operand(label_string(instruction.operand_list[operand + 3 * i + 3]));
            
                }
            
                operand += instruction.operand_list[operand].integer * 3 + 1;
                break;
            
            }
            
            case OpcodeScanAccept:
            {
            
                int operand = 0;
                dump_operand(integer_string(instruction.operand_list[operand++]));
                dump_operand(label_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeScanError:
            case OpcodeAstLexemeString:
            {
            
                int operand = 0;
                dump_operand(string_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeAstStart:
            case OpcodeAstNew:
            {
            
                int operand = 0;
                dump_operand(register_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeAstFinish:
            case OpcodeAstLocationNum:
            {
            
                int operand = 0;
                dump_operand(integer_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeAstForm:
            {
            
                int operand = 0;
                dump_operand(register_string(instruction.operand_list[operand++]));
                dump_operand(register_string(instruction.operand_list[operand++]));
                dump_operand(integer_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeAstLoad:
            {
            
                int operand = 0;
                dump_operand(ast_string(instruction.operand_list[operand++]));
                dump_operand(register_string(instruction.operand_list[operand++]));
                dump_operand(integer_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeAstIndex:
            {
            
                int operand = 0;
                dump_operand(ast_string(instruction.operand_list[operand++]));
                dump_operand(integer_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeAstChild:
            case OpcodeAstKind:
            case OpcodeAstLocation:
            case OpcodeAstLexeme:
            {
            
                int operand = 0;
                dump_operand(ast_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeAstChildSlice:
            {
            
                int operand = 0;
                dump_operand(ast_string(instruction.operand_list[operand++]));
                dump_operand(integer_string(instruction.operand_list[operand++]));
                dump_operand(integer_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeAstKindNum:
            {
            
                int operand = 0;
                dump_operand(kind_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeAssign:
            case OpcodeUnaryMinus:
            {
            
                int operand = 0;
                dump_operand(register_string(instruction.operand_list[operand++]));
                dump_operand(register_string(instruction.operand_list[operand++]));
                break;
            
            }
            
            case OpcodeAdd:
            case OpcodeSubtract:
            case OpcodeMultiply:
            case OpcodeDivide:
            {
            
                int operand = 0;
                dump_operand(register_string(instruction.operand_list[operand++]));
                dump_operand(register_string(instruction.operand_list[operand++]));
                dump_operand(register_string(instruction.operand_list[operand++]));
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
                dump_operand(label_string(instruction.operand_list[operand++]));
                dump_operand(register_string(instruction.operand_list[operand++]));
                dump_operand(register_string(instruction.operand_list[operand++]));
                break;
            
            }
            
#line 1882 "u:\\hoshi\\raw\\CodeGenerator.cpp"
        }

        os << ost.str() << endl;

    }

}

} // namespace hoshi

