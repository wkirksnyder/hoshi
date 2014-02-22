#line 244 "u:\\hoshi\\raw\\ParserEngine.cpp"
//
//  ParserEngine                                                         
//  ------------                                                         
//                                                                       
//  The virtual machine used to parse source. Here we have the LALR(k)   
//  parser and a supporting virtual machine. The virtual machine is used 
//  to scan tokens, build Asts and evaluate guards.                      
//

#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "Parser.H"
#include "ParseAction.H"
#include "ParserImpl.H"
#include "ParserData.H"
#include "ParserEngine.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;

//
//  A macro to find the size of an array.
//

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

//
//  wiring tables                                
//  -------------                                
//                                               
//  Tables that help us route nodes to handlers. 
//

VCodeHandler ParserEngine::vcode_handler[] =
{
    handle_null,                    // Null
    handle_halt,                    // Halt
    handle_label,                   // Label
    handle_call,                    // Call
    handle_scan_start,              // ScanStart
    handle_scan_char,               // ScanChar
    handle_scan_accept,             // ScanAccept
    handle_scan_token,              // ScanToken
    handle_scan_error,              // ScanError
    handle_ast_start,               // AstStart
    handle_ast_finish,              // AstFinish
    handle_ast_new,                 // AstNew
    handle_ast_form,                // AstForm
    handle_ast_load,                // AstLoad
    handle_ast_index,               // AstIndex
    handle_ast_child,               // AstChild
    handle_ast_child_slice,         // AstChildSlice
    handle_ast_kind,                // AstKind
    handle_ast_kind_num,            // AstKindNum
    handle_ast_location,            // AstLocation
    handle_ast_location_num,        // AstLocationNum
    handle_ast_lexeme,              // AstLexeme
    handle_ast_lexeme_string,       // AstLexemeString
    handle_assign,                  // Assign
    handle_dump_stack,              // DumpStack
    handle_add,                     // Add
    handle_subtract,                // Subtract
    handle_multiply,                // Multiply
    handle_divide,                  // Divide
    handle_unary_minus,             // UnaryMinus
    handle_return,                  // Return
    handle_branch,                  // Branch
    handle_branch_equal,            // BranchEqual
    handle_branch_not_equal,        // BranchNotEqual
    handle_branch_less_than,        // BranchLessThan
    handle_branch_less_equal,       // BranchLessEqual
    handle_branch_greater_than,     // BranchGreaterThan
    handle_branch_greater_equal     // BranchGreaterEqual
#line 314 "u:\\hoshi\\raw\\ParserEngine.cpp"
};

struct ParserEngine::VCodeHandlerInfo ParserEngine::vcode_handler_info[] = 
{
    {  handle_null,                    OpcodeType::OpcodeNull,              
       "handle_null",                  "Null"                                    },
    {  handle_halt,                    OpcodeType::OpcodeHalt,              
       "handle_halt",                  "Halt"                                    },
    {  handle_label,                   OpcodeType::OpcodeLabel,             
       "handle_label",                 "Label"                                   },
    {  handle_call,                    OpcodeType::OpcodeCall,              
       "handle_call",                  "Call"                                    },
    {  handle_scan_start,              OpcodeType::OpcodeScanStart,         
       "handle_scan_start",            "ScanStart"                               },
    {  handle_scan_char,               OpcodeType::OpcodeScanChar,          
       "handle_scan_char",             "ScanChar"                                },
    {  handle_scan_accept,             OpcodeType::OpcodeScanAccept,        
       "handle_scan_accept",           "ScanAccept"                              },
    {  handle_scan_token,              OpcodeType::OpcodeScanToken,         
       "handle_scan_token",            "ScanToken"                               },
    {  handle_scan_error,              OpcodeType::OpcodeScanError,         
       "handle_scan_error",            "ScanError"                               },
    {  handle_ast_start,               OpcodeType::OpcodeAstStart,          
       "handle_ast_start",             "AstStart"                                },
    {  handle_ast_finish,              OpcodeType::OpcodeAstFinish,         
       "handle_ast_finish",            "AstFinish"                               },
    {  handle_ast_new,                 OpcodeType::OpcodeAstNew,            
       "handle_ast_new",               "AstNew"                                  },
    {  handle_ast_form,                OpcodeType::OpcodeAstForm,           
       "handle_ast_form",              "AstForm"                                 },
    {  handle_ast_load,                OpcodeType::OpcodeAstLoad,           
       "handle_ast_load",              "AstLoad"                                 },
    {  handle_ast_index,               OpcodeType::OpcodeAstIndex,          
       "handle_ast_index",             "AstIndex"                                },
    {  handle_ast_child,               OpcodeType::OpcodeAstChild,          
       "handle_ast_child",             "AstChild"                                },
    {  handle_ast_child_slice,         OpcodeType::OpcodeAstChildSlice,     
       "handle_ast_child_slice",       "AstChildSlice"                           },
    {  handle_ast_kind,                OpcodeType::OpcodeAstKind,           
       "handle_ast_kind",              "AstKind"                                 },
    {  handle_ast_kind_num,            OpcodeType::OpcodeAstKindNum,        
       "handle_ast_kind_num",          "AstKindNum"                              },
    {  handle_ast_location,            OpcodeType::OpcodeAstLocation,       
       "handle_ast_location",          "AstLocation"                             },
    {  handle_ast_location_num,        OpcodeType::OpcodeAstLocationNum,    
       "handle_ast_location_num",      "AstLocationNum"                          },
    {  handle_ast_lexeme,              OpcodeType::OpcodeAstLexeme,         
       "handle_ast_lexeme",            "AstLexeme"                               },
    {  handle_ast_lexeme_string,       OpcodeType::OpcodeAstLexemeString,   
       "handle_ast_lexeme_string",     "AstLexemeString"                         },
    {  handle_assign,                  OpcodeType::OpcodeAssign,            
       "handle_assign",                "Assign"                                  },
    {  handle_dump_stack,              OpcodeType::OpcodeDumpStack,         
       "handle_dump_stack",            "DumpStack"                               },
    {  handle_add,                     OpcodeType::OpcodeAdd,               
       "handle_add",                   "Add"                                     },
    {  handle_subtract,                OpcodeType::OpcodeSubtract,          
       "handle_subtract",              "Subtract"                                },
    {  handle_multiply,                OpcodeType::OpcodeMultiply,          
       "handle_multiply",              "Multiply"                                },
    {  handle_divide,                  OpcodeType::OpcodeDivide,            
       "handle_divide",                "Divide"                                  },
    {  handle_unary_minus,             OpcodeType::OpcodeUnaryMinus,        
       "handle_unary_minus",           "UnaryMinus"                              },
    {  handle_return,                  OpcodeType::OpcodeReturn,            
       "handle_return",                "Return"                                  },
    {  handle_branch,                  OpcodeType::OpcodeBranch,            
       "handle_branch",                "Branch"                                  },
    {  handle_branch_equal,            OpcodeType::OpcodeBranchEqual,       
       "handle_branch_equal",          "BranchEqual"                             },
    {  handle_branch_not_equal,        OpcodeType::OpcodeBranchNotEqual,    
       "handle_branch_not_equal",      "BranchNotEqual"                          },
    {  handle_branch_less_than,        OpcodeType::OpcodeBranchLessThan,    
       "handle_branch_less_than",      "BranchLessThan"                          },
    {  handle_branch_less_equal,       OpcodeType::OpcodeBranchLessEqual,   
       "handle_branch_less_equal",     "BranchLessEqual"                         },
    {  handle_branch_greater_than,     OpcodeType::OpcodeBranchGreaterThan, 
       "handle_branch_greater_than",   "BranchGreaterThan"                       },
    {  handle_branch_greater_equal,    OpcodeType::OpcodeBranchGreaterEqual, 
       "handle_branch_greater_equal",  "BranchGreaterEqual"                      }
#line 348 "u:\\hoshi\\raw\\ParserEngine.cpp"
};

//
//  initialize                                                             
//  ----------                                                             
//                                                                         
//  We initialize the class by sorting our opcode information table. We    
//  want to store just the function pointers of our handlers. Sorting this 
//  table gives us a way to get other information about the handlers       
//  quickly.                                                               
//

void ParserEngine::initialize()
{

    for (int i = 1; i < LENGTH(vcode_handler_info); i++)
    {

        for (int j = i;
             j > 0 && vcode_handler_info[j].handler < vcode_handler_info[j - 1].handler;
             j--)
        {
            auto tmp = vcode_handler_info[j];
            vcode_handler_info[j] = vcode_handler_info[j - 1];
            vcode_handler_info[j - 1] = tmp;
        }

    }

}

//
//  get_vcode_handler                     
//  -----------------                     
//                                        
//  Return the handler for a give opcode. 
//

VCodeHandler ParserEngine::get_vcode_handler(OpcodeType opcode)
{

    if (opcode < 0 || opcode >= LENGTH(vcode_handler))
    {
        return nullptr;    
    }

    return vcode_handler[opcode];

}

//
//  get_vcode_name                                      
//  --------------                                      
//                                                      
//  Return the opcode name for a given handler. 
//

string ParserEngine::get_vcode_name(VCodeHandler handler)
{

    int min = 0;
    int max = LENGTH(vcode_handler_info) - 1;
    int mid;

    while (min <= max)
    {

        mid = min + (max - min) / 2;

        if (vcode_handler_info[mid].handler < handler)
        {
            min = mid + 1;
        }
        else if (vcode_handler_info[mid].handler > handler)
        {
            max = mid - 1;
        }
        else
        {
            break;
        }

    }

    return vcode_handler_info[mid].opcode_name;

}

//
//  get_vcode_opcode                                      
//  ----------------                                      
//                                                      
//  Return the opcode code (number) for a given handler. 
//

int ParserEngine::get_vcode_opcode(VCodeHandler handler)
{

    int min = 0;
    int max = LENGTH(vcode_handler_info) - 1;
    int mid;

    while (min <= max)
    {

        mid = min + (max - min) / 2;

        if (vcode_handler_info[mid].handler < handler)
        {
            min = mid + 1;
        }
        else if (vcode_handler_info[mid].handler > handler)
        {
            max = mid - 1;
        }
        else
        {
            break;
        }

    }

    return vcode_handler_info[mid].opcode;

}

//
//  destructor                               
//  ----------                               
//                                           
//  Delete anything we might have allocated. 
//

ParserEngine::~ParserEngine()
{

    delete [] token_buffer;
    token_buffer = nullptr;

    delete [] register_list;
    register_list = nullptr;

    delete [] ast_list;
    ast_list = nullptr;

}

//
//  parse                                                                  
//  -----                                                                  
//                                                                         
//  Parse the provided source into an Ast. This is what all the other work 
//  was leading up to.                                                     
//

void ParserEngine::parse()
{

    //
    //  Initialize the virtual machine. 
    //

    if (register_list == nullptr)
    {
        register_list = new int64_t[prsd.register_count];
    }

    for (int i = 0; i < prsd.register_count; i++)
    {
        register_list[i] = prsd.register_list[i].initial_value;
    }

    if (ast_list == nullptr)
    {
        ast_list = new Ast*[prsd.ast_count];
    }

    for (int i = 0; i < prsd.ast_count; i++)
    {
        ast_list[i] = nullptr;
    }

    call_vm(0);

    //
    //  Initialize the scanner. 
    //

    if (token_buffer == nullptr)
    {
        token_buffer = new Token[prsd.lookaheads + 1];
    }

    token_front = 0;
    token_rear = 0;
    token_current = 0;

    scan_next_loc = 0;

    //
    //  Initialize the parse stacks. 
    //

    int64_t state = prsd.start_state;
    vector<int64_t> state_stack;
    state_stack.push_back(state);

    //
    //  Get the first token and decode the action. 
    //

    get_token();

    ParseActionType action_type;
    int64_t goto_state;
    int64_t rule_num;
    int64_t fallback_state;

    decode_action(state,
                  token_buffer[token_current].symbol_num,
                  action_type,
                  goto_state,
                  rule_num,
                  fallback_state);

    //
    //  Process parse actions until we see an eof. 
    //

    bool any_errors = false;
    for (;;)
    {

        if ((debug_flags & DebugType::DebugParseAction) != 0)
        {
            cout << "State: " << state << endl;
        }

        switch (action_type)
        {

            //
            //  LaShift Action                                          
            //  --------------                                          
            //                                                          
            //  On a lookahead-shift we consume tokens until we get the 
            //  real action.                                            
            //

            case ParseActionType::ActionLaShift:
            {

                if ((debug_flags & DebugType::DebugParseAction) != 0)
                {
                    cout << "LaShift: " << goto_state << endl;
                }

                state = goto_state;
                token_current = (token_current + 1) % (prsd.lookaheads + 1);
                get_token();
                
                decode_action(state,
                              token_buffer[token_current].symbol_num,
                              action_type,
                              goto_state,
                              rule_num,
                              fallback_state);

                continue;

            }

            //
            //  Shift Action                                              
            //  ------------                                              
            //                                                            
            //  A shift action creates a token ast and shifts it onto the 
            //  stack.                                                    
            //

            case ParseActionType::ActionShift:
            {

                if ((debug_flags & DebugType::DebugParseAction) != 0)
                {
                    cout << "Shift: " << goto_state << endl;
                }

                if (!any_errors)
                {

                    Ast* ast = new Ast(0);
                    ast->set_kind(prsd.token_kind[token_buffer[token_rear].symbol_num]);
                    ast->set_location(token_buffer[token_rear].location);
                    ast->set_lexeme(token_buffer[token_rear].lexeme);
                    ast_stack.push_back(ast);

                }

                state = goto_state;
                state_stack.push_back(state);

                token_rear = (token_rear + 1) % (prsd.lookaheads + 1);
                token_current = token_rear;
                get_token();
                
                decode_action(state,
                              token_buffer[token_current].symbol_num,
                              action_type,
                              goto_state,
                              rule_num,
                              fallback_state);

                continue;

            }

            //
            //  Reduce Action                                              
            //  -------------                                              
            //                                                             
            //  Reduce should remove items from the stack and create a new 
            //  stack item. Note that reduce is affected by our recovery   
            //  scheme. If we are about to underflow the stack (can only   
            //  happen in recovery mode) then we go to the fallback state. 
            //

            case ParseActionType::ActionReduce:
            {

                if ((debug_flags & DebugType::DebugParseAction) != 0)
                {
                    cout << "Reduce: " << prsd.rule_text[rule_num] << endl;
                }

                if (!any_errors)
                {
                    call_vm(prsd.rule_pc[rule_num]);
                }

                if (prsd.rule_size[rule_num] >= state_stack.size())
                {

                    if ((debug_flags & DebugType::DebugParseAction) != 0)
                    {
                        cout << "Restarting at fallback state" << endl;
                    }

                    state_stack.clear();
                    state = fallback_state;
                    state_stack.push_back(state);

                    token_rear = (token_rear + 1) % (prsd.lookaheads + 1);
                    token_current = token_rear;
                    get_token();
                
                    decode_action(state,
                                  token_buffer[token_current].symbol_num,
                                  action_type,
                                  goto_state,
                                  rule_num,
                                  fallback_state);

                }
                else
                {

                    if (prsd.rule_size[rule_num] > 0)
                    {
                        state_stack.erase(state_stack.end() - prsd.rule_size[rule_num],
                                          state_stack.end());
                    }

                    state = state_stack.back();
                    token_current = token_rear;

                    decode_action(state,
                                  prsd.rule_lhs[rule_num],
                                  action_type,
                                  goto_state,
                                  rule_num,
                                  fallback_state);

                }

                continue;

            }

            //
            //  Goto Action                           
            //  -----------                           
            //                                        
            //  We execute a goto following a reduce. 
            //

            case ParseActionType::ActionGoto:
            {

                state = goto_state;
                state_stack.push_back(state);

                decode_action(state,
                              token_buffer[token_current].symbol_num,
                              action_type,
                              goto_state,
                              rule_num,
                              fallback_state);

                continue;

            }

            //
            //  Restart Action                                          
            //  --------------                                          
            //                                                          
            //  Restart actions occur during error recovery. We discard 
            //  the stack and shift to a fallback state.                
            //

            case ParseActionType::ActionRestart:
            {

                if ((debug_flags & DebugType::DebugParseAction) != 0)
                {
                    cout << "Restart: " << goto_state << endl;
                }

                state_stack.clear();
                state = goto_state;
                state_stack.push_back(state);

                token_rear = (token_rear + 1) % (prsd.lookaheads + 1);
                token_current = token_rear;
                get_token();
                
                decode_action(state,
                              token_buffer[token_current].symbol_num,
                              action_type,
                              goto_state,
                              rule_num,
                              fallback_state);

                continue;

            }

            //
            //  Accept Action                                            
            //  -------------                                            
            //                                                           
            //  On an accept if we had no errors we return. If we did we 
            //  throw an exception.                                      
            //

            case ParseActionType::ActionAccept:
            {

                if ((debug_flags & DebugType::DebugParseAction) != 0)
                {
                    cout << "Accept" << endl;
                }

                if (any_errors)
                {

                    ast = nullptr;

                    for (Ast* ast: ast_stack)
                    {
                        delete ast;
                    }

                    ast_stack.clear();

                    throw SourceError("Source errors");

                }

                ast = ast_stack.back();
                ast_stack.pop_back();

                for (Ast* ast: ast_stack)
                {
                    delete ast;
                }

                ast_stack.clear();

                break;

            }

            //
            //  Error                                                    
            //  -----                                                    
            //                                                           
            //  Print an error message and possibly begin parsing again. 
            //

            case ParseActionType::ActionError:
            {

                if ((debug_flags & DebugType::DebugParseAction) != 0)
                {
                    cout << "Error" << endl;
                }

                int symbol_num = token_buffer[token_current].symbol_num;

                if ((symbol_num != prsd.eof_symbol_num | !any_errors) &&
                    symbol_num != prsd.error_symbol_num)
                {

                    //
                    //  Make a list of the tokens we can shift in the current 
                    //  state.                                                
                    //

                    vector<int> valid_symbol_list;

                    for (int i = 0; i < prsd.token_count; i++)
                    {

                        if (!prsd.token_is_terminal[i])
                        {
                            continue;
                        }

                        if (valid_symbol(state_stack, i))
                        {
                            valid_symbol_list.push_back(i);
                        }

                    }

                    //
                    //  Format an error message. 
                    //

                    ostringstream ost;
                    ost << "Syntax error at ";
                    if (prsd.token_lexeme_needed[symbol_num])
                    {
                        ost << token_buffer[token_current].lexeme;
                    }
                    else
                    {
                        ost << prsd.token_name_list[symbol_num];
                    } 
               
                    if (valid_symbol_list.size() < 1 || valid_symbol_list.size() > 10)
                    {
                        ost << ".";
                    }
                    else if (valid_symbol_list.size() == 1)
                    {
                        ost << ". Expected " << prsd.token_name_list[valid_symbol_list[0]] << ".";
                    }
                    else if (valid_symbol_list.size() == 2)
                    {
                        ost << ". Expected " << prsd.token_name_list[valid_symbol_list[0]] << " or "
                                             << prsd.token_name_list[valid_symbol_list[1]] << ".";
                    }
                    else
                    {
                           
                        ost << ". Expected one of ";
                        for (int i = 0; i < valid_symbol_list.size(); i++)
                        {

                            if (i == valid_symbol_list.size() - 1)
                            {
                                ost << " or ";
                            }
                            else if (i != 0)
                            {
                                ost << ", ";
                            }

                            ost << prsd.token_name_list[valid_symbol_list[i]];

                        }

                        ost << ".";
  
                    }

                    errh.add_error(ErrorType::ErrorSyntax,
                                   token_buffer[token_current].location,
                                   ost.str());

                }

                //
                //  Clear the ast stack. 
                //

                ast = nullptr;

                for (Ast* ast: ast_stack)
                {
                    delete ast;
                }

                ast_stack.clear();

                //
                //  If error recovery is turned off we are finished. 
                //

                if (!prsd.error_recovery || symbol_num == prsd.eof_symbol_num)
                {
                    throw SourceError("Source errors");
                }

                //
                //  Restart on the next token. 
                //

                any_errors = true;
                state = prsd.restart_state;
                state_stack.push_back(state);

                token_rear = (token_rear + 1) % (prsd.lookaheads + 1);
                token_current = token_rear;
                get_token();
                
                decode_action(state,
                              token_buffer[token_current].symbol_num,
                              action_type,
                              goto_state,
                              rule_num,
                              fallback_state);

                continue;

            }

        }

        break;

    }

}

//
//  valid_symbol                                                         
//  ------------                                                         
//                                                                       
//  When we see a syntax error we want to produce a list of the symbols  
//  which would have been valid in that context. This function tests one 
//  symbol to see if it is valid.                                        
//

bool ParserEngine::valid_symbol(vector<int64_t>& base_state_stack, int symbol_num)
{

    vector<int64_t> state_stack = base_state_stack;
    int64_t state = state_stack.back();

    ParseActionType action_type;
    int64_t goto_state;
    int64_t rule_num;
    int64_t fallback_state;

    decode_action(state,
                  symbol_num,
                  action_type,
                  goto_state,
                  rule_num,
                  fallback_state);

    //
    //  Process parse actions until see a shift or error.
    //

    for (;;)
    {

        switch (action_type)
        {

            case ParseActionType::ActionLaShift:
            {
                return true;
            }

            case ParseActionType::ActionShift:
            {
                return true;
            }

            case ParseActionType::ActionReduce:
            {

                if (prsd.rule_size[rule_num] > 0)
                {

                    if (state_stack.size() <= prsd.rule_size[rule_num])
                    {
                        return true;
                    }

                    state_stack.erase(state_stack.end() - prsd.rule_size[rule_num],
                                      state_stack.end());

                }

                state = state_stack.back();

                decode_action(state,
                              prsd.rule_lhs[rule_num],
                              action_type,
                              goto_state,
                              rule_num,
                              fallback_state);

                continue;

            }

            case ParseActionType::ActionGoto:
            {

                state = goto_state;
                state_stack.push_back(state);

                decode_action(state,
                              symbol_num,
                              action_type,
                              goto_state,
                              rule_num,
                              fallback_state);

                continue;

            }

            case ParseActionType::ActionRestart:
            {
                return true;
            }

            case ParseActionType::ActionAccept:
            {
                return true;
            }

            case ParseActionType::ActionError:
            {
                return false;
            }

        }

        break;

    }

    return false;

}

//
//  decode_action                                                   
//  -------------                                                   
//                                                                  
//  For a given state and symbol number find the next parse action. 
//

void ParserEngine::decode_action(const int64_t state,
                                 const int symbol_num,
                                 ParseActionType& action_type,
                                 int64_t& goto_state,
                                 int64_t& rule_num,
                                 int64_t& fallback_state)
{

    int64_t index = prsd.checked_index[state] + symbol_num * prsd.num_offsets;

    if (prsd.checked_data[index] < 0)
    {

        action_type = ParseActionType::ActionError;
        goto_state = 0;
        rule_num = 0;
        fallback_state = 0;

        return;

    }

    int check_symbol_num = (prsd.checked_data[index + prsd.symbol_num_offset] >> prsd.symbol_num_shift) &
                               prsd.symbol_num_mask;

    if (check_symbol_num != symbol_num)
    {

        action_type = ParseActionType::ActionError;
        goto_state = 0;
        rule_num = 0;
        fallback_state = 0;

        return;

    }

    action_type = static_cast<ParseActionType>(
                      (prsd.checked_data[index + prsd.action_type_offset] >> prsd.action_type_shift) &
                      prsd.action_type_mask);
    
    rule_num = (prsd.checked_data[index + prsd.rule_num_offset] >> prsd.rule_num_shift) &
               prsd.rule_num_mask;

    goto_state = (prsd.checked_data[index + prsd.state_num_offset] >> prsd.state_num_shift) &
                 prsd.state_num_mask;
    
    fallback_state = (prsd.checked_data[index + prsd.fallback_num_offset] >> prsd.fallback_num_shift) &
                     prsd.fallback_num_mask;
    
}

//
//  get_token                                                            
//  ---------                                                            
//                                                                       
//  Main scanner facility. Get one token from the input stream and place 
//  it in our token buffer.                                              
//

void ParserEngine::get_token()
{

    if (token_current != token_front)
    {
        return;
    }

    call_vm(prsd.scanner_pc);

    if ((debug_flags & DebugType::DebugScanToken) != 0)
    {

        cout << "Scanned token " 
             << prsd.token_name_list[token_buffer[token_current].symbol_num];

        if (token_buffer[token_current].lexeme.size() > 0)
        {
             cout << ": " << Source::to_ascii_chop(token_buffer[token_current].lexeme);
        }

        cout << endl;

    }

}

//
//  call_vm                                                               
//  -------                                                               
//                                                                        
//  Run the virtual machine starting at a specified program counter until 
//  we see a halt or return.                                              
//

void ParserEngine::call_vm(int64_t pc)
{

    const int max_line_width = 95;
    const int line_num_width = 6;
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

        if (ost.str().size() < line_num_width)
        {
            cout << ost.str();
            ost.str("");
        }
            
        next_column = line_num_width;
        ost << setw(next_column - ost.str().size()) << line_num << setw(0);
        next_column++;

    };

    //
    //  dump_opcode          
    //  -----------          
    //                      
    //  Dump an opcode. 
    //

    function<void(string)> dump_opcode = [&](string value) -> void
    {

        next_column = line_num_width + 1;

        if (ost.str().size() < next_column)
        {
            ost << setw(next_column - ost.str().size()) << "" << setw(0);
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
            cout << ost.str() << endl;
            next_column = line_num_width + 1 + opcode_width + 1;
            ost.str("");
        }

        next_column = next_column + operand_width;
        ost << right << setw(next_column - ost.str().size()) << value << setw(0);
        next_column++;

    };

    //
    //  integer_string                             
    //  --------------                             
    //                                           
    //  Return a printable string for a integer. 
    //

    function<string(VCodeOperand)> integer_string = [&](VCodeOperand operand) -> string
    {
        return to_string(operand.integer);
    };

    //
    //  kind_string                             
    //  -----------                             
    //                                           
    //  Return a printable string for a kind integer. 
    //

    function<string(VCodeOperand)> kind_string = [&](VCodeOperand operand) -> string
    {
        return to_string(operand.integer);
    };

    //
    //  character_string                            
    //  ----------------                            
    //                                              
    //  Return a printable string from a character. 
    //

    function<string(VCodeOperand)> character_string = [&](VCodeOperand operand) -> string
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

    function<string(VCodeOperand)> register_string = [&](VCodeOperand operand) -> string
    {
        ostringstream ost;
        ost << "Reg$" << operand.register_num;
        return ost.str();
    };

    //
    //  ast_string                             
    //  ----------                             
    //                                           
    //  Return a printable string for an ast. 
    //

    function<string(VCodeOperand)> ast_string = [&](VCodeOperand operand) -> string
    {
        ostringstream ost;
        ost << "Ast$" << operand.ast_num;
        return ost.str();
    };

    //
    //  string_string                             
    //  -------------                             
    //                                           
    //  Return a printable string for a string. 
    //

    function<string(VCodeOperand)> string_string = [&](VCodeOperand operand) -> string
    {
        ostringstream ost;
        ost << "Str$" << operand.string_num;
        return ost.str();
    };

    //
    //  label_string                            
    //  ------------                            
    //                                          
    //  Return a printable string from a label. 
    //

    function<string(VCodeOperand)> label_string = [&](VCodeOperand operand) -> string
    {
        ostringstream ost;
        ost << "Lab$" << operand.branch_target;
        return ost.str();
    };

    //
    //  call_vm
    //  -------
    //                                 
    //  The function body begins here. 
    //

    call_stack.push_back(-1);
    while (pc >= 0)
    {

        if ((debug_flags & DebugType::DebugVCodeExec) != 0)
        {

            VCodeInstruction instruction = prsd.instruction_list[pc];

            ost.str("");
            dump_line_num(pc);
            dump_opcode(get_vcode_name(instruction.handler));

            //
            //  Dump all the operands. 
            //

            switch (get_vcode_opcode(instruction.handler))
            {

                case OpcodeCall:
                case OpcodeBranch:
                {
                
                    int operand = 0;
                    dump_operand(label_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeScanChar:
                {
                
                    int operand = 0;
                
                    dump_operand(integer_string(prsd.operand_list[instruction.operand_offset + operand]));
                    for (int i = 0; i < prsd.operand_list[instruction.operand_offset + operand].integer; i++)    {
                
                        dump_operand(character_string(prsd.operand_list[instruction.operand_offset + operand + 3 * i + 1]));
                        dump_operand(character_string(prsd.operand_list[instruction.operand_offset + operand + 3 * i + 2]));
                        dump_operand(label_string(prsd.operand_list[instruction.operand_offset + operand + 3 * i + 3]));
                
                    }
                
                    operand += prsd.operand_list[instruction.operand_offset + operand].integer * 3 + 1;
                    break;
                
                }
                
                case OpcodeScanAccept:
                {
                
                    int operand = 0;
                    dump_operand(integer_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(label_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeScanError:
                case OpcodeAstLexemeString:
                {
                
                    int operand = 0;
                    dump_operand(string_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeAstStart:
                case OpcodeAstNew:
                {
                
                    int operand = 0;
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeAstFinish:
                case OpcodeAstLocationNum:
                {
                
                    int operand = 0;
                    dump_operand(integer_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeAstForm:
                {
                
                    int operand = 0;
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(integer_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeAstLoad:
                {
                
                    int operand = 0;
                    dump_operand(ast_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(integer_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeAstIndex:
                {
                
                    int operand = 0;
                    dump_operand(ast_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(integer_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeAstChild:
                case OpcodeAstKind:
                case OpcodeAstLocation:
                case OpcodeAstLexeme:
                {
                
                    int operand = 0;
                    dump_operand(ast_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeAstChildSlice:
                {
                
                    int operand = 0;
                    dump_operand(ast_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(integer_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(integer_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeAstKindNum:
                {
                
                    int operand = 0;
                    dump_operand(kind_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeAssign:
                case OpcodeUnaryMinus:
                {
                
                    int operand = 0;
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
                case OpcodeAdd:
                case OpcodeSubtract:
                case OpcodeMultiply:
                case OpcodeDivide:
                {
                
                    int operand = 0;
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
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
                    dump_operand(label_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    dump_operand(register_string(prsd.operand_list[instruction.operand_offset + operand++]));
                    break;
                
                }
                
#line 1519 "u:\\hoshi\\raw\\ParserEngine.cpp"
            }

            cout << ost.str() << endl;

        }

        //
        //  All that debugging code kind of obscures the point of this 
        //  function. Here we are ready to interpret the VMCode.       
        //

        int64_t last_pc = pc++;
        (prsd.instruction_list[last_pc].handler)(
            *this,
            prsd.operand_list + prsd.instruction_list[last_pc].operand_offset, 
            pc,
            prsd.instruction_list[last_pc].location);

    }
    
}

//
//  handle_error                                                     
//  ------------                                                     
//                                                                   
//  This should never be called. It means there is a path we haven't 
//  accomodated. It's not a user error, it's a logic error.          
//

#line 1554 "u:\\hoshi\\raw\\ParserEngine.cpp"
void ParserEngine::handle_error(ParserEngine& prse,
                                const VCodeOperand* operands,
                                int64_t& pc,
                                int64_t location)
#line 1555 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    cout << "No ParserEngine::vcode handler for Opcode!" << endl << endl;
    exit(1);
}

//
//  handle_halt                     
//  -----------                     
//                                  
//  Halt stops the virtual machine. 
//

void ParserEngine::handle_halt(ParserEngine& prse,
                               const VCodeOperand* operands,
                               int64_t& pc,
                               int64_t location)
#line 1569 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    pc = -1;
}

//
//  handle_call                     
//  -----------                     
//                                  
//  Call saves the return location and branches.
//

void ParserEngine::handle_call(ParserEngine& prse,
                               const VCodeOperand* operands,
                               int64_t& pc,
                               int64_t location)
#line 1582 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.call_stack.push_back(pc);
    pc = operands[0].branch_target;
}

//
//  handle_return
//  -------------                     
//                                  
//  Return pops an address off the call stack and branches to it.
//

void ParserEngine::handle_return(ParserEngine& prse,
                                 const VCodeOperand* operands,
                                 int64_t& pc,
                                 int64_t location)
#line 1596 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    pc = prse.call_stack.back();
    prse.call_stack.pop_back();
}

//
//  handle_assign                                
//  -------------                                
//                                               
//  Assign the value of one register to another. 
//

void ParserEngine::handle_assign(ParserEngine& prse,
                                 const VCodeOperand* operands,
                                 int64_t& pc,
                                 int64_t location)
#line 1610 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.register_list[operands[0].integer] =
        prse.register_list[operands[1].integer];
}

//
//  Arithmetic handling functions               
//  -----------------------------               
//                                              
//  Simple functions for arithmetic operations. 
//

void ParserEngine::handle_add(ParserEngine& prse,
                              const VCodeOperand* operands,
                              int64_t& pc,
                              int64_t location)
#line 1624 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.register_list[operands[0].register_num] =
        prse.register_list[operands[1].register_num] +
        prse.register_list[operands[2].register_num];
}

void ParserEngine::handle_subtract(ParserEngine& prse,
                                   const VCodeOperand* operands,
                                   int64_t& pc,
                                   int64_t location)
#line 1632 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.register_list[operands[0].register_num] =
        prse.register_list[operands[1].register_num] -
        prse.register_list[operands[2].register_num];
}

void ParserEngine::handle_multiply(ParserEngine& prse,
                                   const VCodeOperand* operands,
                                   int64_t& pc,
                                   int64_t location)
#line 1640 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.register_list[operands[0].register_num] =
        prse.register_list[operands[1].register_num] *
        prse.register_list[operands[2].register_num];
}

void ParserEngine::handle_divide(ParserEngine& prse,
                                 const VCodeOperand* operands,
                                 int64_t& pc,
                                 int64_t location)
#line 1648 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.register_list[operands[0].register_num] =
        prse.register_list[operands[1].register_num] /
        prse.register_list[operands[2].register_num];
}

void ParserEngine::handle_unary_minus(ParserEngine& prse,
                                      const VCodeOperand* operands,
                                      int64_t& pc,
                                      int64_t location)
#line 1656 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.register_list[operands[0].register_num] =
        -prse.register_list[operands[1].register_num];
}

//
//  Conditional and unconditional branches  
//  --------------------------------------  
//                                          
//  Not much to say. These are all trivial. 
//

void ParserEngine::handle_branch(ParserEngine& prse,
                                 const VCodeOperand* operands,
                                 int64_t& pc,
                                 int64_t location)
#line 1670 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    pc = operands[0].integer;
}

void ParserEngine::handle_branch_equal(ParserEngine& prse,
                                       const VCodeOperand* operands,
                                       int64_t& pc,
                                       int64_t location)
#line 1676 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    if (prse.register_list[operands[1].register_num] ==
        prse.register_list[operands[2].register_num])
    {
        pc = operands[0].integer;
    }
}

void ParserEngine::handle_branch_not_equal(ParserEngine& prse,
                                           const VCodeOperand* operands,
                                           int64_t& pc,
                                           int64_t location)
#line 1686 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    if (prse.register_list[operands[1].register_num] !=
        prse.register_list[operands[2].register_num])
    {
        pc = operands[0].integer;
    }
}

void ParserEngine::handle_branch_less_than(ParserEngine& prse,
                                           const VCodeOperand* operands,
                                           int64_t& pc,
                                           int64_t location)
#line 1696 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    if (prse.register_list[operands[1].register_num] <
        prse.register_list[operands[2].register_num])
    {
        pc = operands[0].integer;
    }
}

void ParserEngine::handle_branch_less_equal(ParserEngine& prse,
                                            const VCodeOperand* operands,
                                            int64_t& pc,
                                            int64_t location)
#line 1706 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    if (prse.register_list[operands[1].register_num] <=
        prse.register_list[operands[2].register_num])
    {
        pc = operands[0].integer;
    }
}

void ParserEngine::handle_branch_greater_than(ParserEngine& prse,
                                              const VCodeOperand* operands,
                                              int64_t& pc,
                                              int64_t location)
#line 1716 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    if (prse.register_list[operands[1].register_num] >
        prse.register_list[operands[2].register_num])
    {
        pc = operands[0].integer;
    }
}

void ParserEngine::handle_branch_greater_equal(ParserEngine& prse,
                                               const VCodeOperand* operands,
                                               int64_t& pc,
                                               int64_t location)
#line 1726 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    if (prse.register_list[operands[1].register_num] >=
        prse.register_list[operands[2].register_num])
    {
        pc = operands[0].integer;
    }
}

//
//  handle_scan_start                                               
//  -----------------                                               
//                                                                  
//  Set up for scanning. If we are already at the end of our source 
//  stream return an eof token.                                     
//

void ParserEngine::handle_scan_start(ParserEngine& prse,
                                     const VCodeOperand* operands,
                                     int64_t& pc,
                                     int64_t location)
#line 1744 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    //
    //  If we hit the end of the source buffer then build an eof token and 
    //  return. This is an early exit from the scanning code.              
    //

    if (prse.scan_next_loc >= prse.src.length())
    {

        if ((prse.token_front + 1) % (prse.prsd.lookaheads + 1) == prse.token_rear)
        {
            cout << "Token buffer overflow!" << endl << endl;
            exit(1);
        }

        prse.token_buffer[prse.token_front].symbol_num = prse.prsd.eof_symbol_num;
        prse.token_buffer[prse.token_front].lexeme.clear();
        prse.token_buffer[prse.token_front].location = -1;

        prse.token_front = (prse.token_front + 1) % (prse.prsd.lookaheads + 1);

        pc = prse.call_stack.back();
        prse.call_stack.pop_back();

        return;

    }

    //
    //  Initialize local storage to scan the next token. 
    //

    prse.scan_start_loc = prse.scan_next_loc;
    prse.scan_accept_loc = -1;
    prse.scan_accept_pc = -1;
    prse.scan_accept_symbol_num = -1;

}

//
//  handle_scan_accept                                                     
//  ------------------                                                     
//                                                                         
//  When we hit the accept condition for a token we save enough            
//  information to build that token. We will then keep scanning and return 
//  this if it is the last accepted token.                                 
//

void ParserEngine::handle_scan_accept(ParserEngine& prse,
                                      const VCodeOperand* operands,
                                      int64_t& pc,
                                      int64_t location)
#line 1795 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.scan_accept_loc = prse.scan_next_loc;
    prse.scan_accept_symbol_num = operands[0].integer;
    prse.scan_accept_pc = operands[1].branch_target;
}

//
//  handle_scan_token                       
//  -----------------                       
//                                               
//  Save the accepted token in the token buffer. 
//

void ParserEngine::handle_scan_token(ParserEngine& prse,
                                     const VCodeOperand* operands,
                                     int64_t& pc,
                                     int64_t location)
#line 1810 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    //
    //  A buffer full situation is a program error. 
    //

    if ((prse.token_front + 1) % (prse.prsd.lookaheads + 1) == prse.token_rear)
    {
        cout << "Token buffer overflow!" << endl << endl;
        exit(1);
    }

    //
    //  Save the token in our token buffer. 
    //

    prse.token_buffer[prse.token_front].symbol_num = prse.scan_accept_symbol_num;
 
    if (prse.prsd.token_lexeme_needed[prse.scan_accept_symbol_num])
    {
        prse.token_buffer[prse.token_front].lexeme =
            prse.src.get_string(prse.scan_start_loc, prse.scan_accept_loc);
    }
    else
    {
        prse.token_buffer[prse.token_front].lexeme.clear();
    }

    prse.token_buffer[prse.token_front].location = prse.scan_start_loc;
    prse.token_front = (prse.token_front + 1) % (prse.prsd.lookaheads + 1);

}

//
//  handle_scan_error                       
//  -----------------                       
//                                               
//  Generate an error for a token.
//

void ParserEngine::handle_scan_error(ParserEngine& prse,
                                     const VCodeOperand* operands,
                                     int64_t& pc,
                                     int64_t location)
#line 1852 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    //
    //  Create an error message. 
    //

    prse.errh.add_error(ErrorType::ErrorLexical,
                        prse.scan_start_loc,
                        prse.prsd.string_list[operands[0].string_num]);

    //
    //  A buffer full situation is a program error. 
    //

    if ((prse.token_front + 1) % (prse.prsd.lookaheads + 1) == prse.token_rear)
    {
        cout << "Token buffer overflow!" << endl << endl;
        exit(1);
    }

    //
    //  Save the token in our token buffer. 
    //

    prse.token_buffer[prse.token_front].symbol_num = prse.prsd.error_symbol_num;
    prse.token_buffer[prse.token_front].lexeme =
        prse.src.get_string(prse.scan_start_loc, prse.scan_accept_loc);
    prse.token_buffer[prse.token_front].location = prse.scan_start_loc;

    prse.token_front = (prse.token_front + 1) % (prse.prsd.lookaheads + 1);

}

//
//  handle_scan_char                                                      
//  ----------------                                                      
//                                                                        
//  Handle the state transitions in the DFA. The state is represented by  
//  our position in the VM instructions, here we look for a branch target 
//  for the incoming character. If we find it we make that transition. If 
//  not and we passed an accepting state we go back to it. Otherwise we   
//  have a scanning error.                                                
//

void ParserEngine::handle_scan_char(ParserEngine& prse,
                                    const VCodeOperand* operands,
                                    int64_t& pc,
                                    int64_t location)
#line 1898 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    //
    //  Try to consume the next character and advance to the next state. 
    //

    if (prse.scan_next_loc < prse.src.length())
    {

        int64_t min = 0;
        int64_t max = operands[0].integer - 1;

        while (min <= max)
        {

            int64_t mid = min + (max - min) / 2;
            if (prse.src.get_char(prse.scan_next_loc) < operands[mid * 3 + 1].character)
            {
                max = mid - 1;
            }
            else if (prse.src.get_char(prse.scan_next_loc) > operands[mid * 3 + 2].character)
            {
                min = mid + 1;
            }
            else
            {
                pc = operands[mid * 3 + 3].branch_target;
                prse.scan_next_loc++;
                return;
            }

        }

    }

    //
    //  We failed to advance. If we've already accepted a token then 
    //  return it.                                                   
    //

    if (prse.scan_accept_pc >= 0)
    {
        pc = prse.scan_accept_pc;
        prse.scan_next_loc = prse.scan_accept_loc;
        return;
    }

    //
    //  Create an error message. 
    //

    ostringstream ost;

    ost << "Invalid token at ";

    switch (prse.src.get_char(prse.scan_start_loc))
    {

        case '\\': 
            ost <<  "'\\\\'";
            break;

        case '\n': 
            ost <<  "'\\n'";
            break;

        case '\r': 
            ost <<  "'\\r'";
            break;

        case '\t': 
            ost <<  "'\\t'";
            break;

        default:
        {

            if (prse.src.get_char(prse.scan_start_loc) >= ' ' &&
                prse.src.get_char(prse.scan_start_loc) < 128)
            {
                ost << "'" << static_cast<char>(prse.src.get_char(prse.scan_start_loc)) << "'";    
            }
            else
            {
                ost << setfill('0') << setw(8) << hex << prse.src.get_char(prse.scan_start_loc);
            }

        }
           
    }   

    ost << ".";
    prse.errh.add_error(ErrorType::ErrorLexical,
                        prse.scan_start_loc,
                        ost.str());

    //
    //  Construct an error token and return. 
    //

    if ((prse.token_front + 1) % (prse.prsd.lookaheads + 1) == prse.token_rear)
    {
        cout << "Token buffer overflow!" << endl << endl;
        exit(1);
    }

    prse.token_buffer[prse.token_front].symbol_num = prse.prsd.error_symbol_num;
    prse.token_buffer[prse.token_front].lexeme = prse.src.get_string(prse.scan_start_loc, prse.scan_start_loc + 1);
    prse.token_buffer[prse.token_front].location = -1;

    prse.token_front = (prse.token_front + 1) % (prse.prsd.lookaheads + 1);

    prse.scan_next_loc = prse.scan_start_loc + 1;

    pc = prse.call_stack.back();
    prse.call_stack.pop_back();

    return;

}

//
//  handle_ast_start                                                     
//  ----------------                                                     
//                                                                       
//  This instruction is executed at the beginning of a reduce action. We 
//  clear out the set of Ast references that have been used so cannot be 
//  deleted.                                                             
//

void ParserEngine::handle_ast_start(ParserEngine& prse,
                                    const VCodeOperand* operands,
                                    int64_t& pc,
                                    int64_t location)
#line 2030 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.ast_dirty_set.clear();
    prse.ast_dirty_base_set.clear();
    prse.register_list[operands[0].register_num] = prse.ast_stack.size();
}

//
//  handle_ast_finish                                                      
//  -----------------                                                      
//                                                                         
//  This is the end of a reduce action. We should have our final result on 
//  the stack top and just under that the rhs items of the rule. We        
//  replace by null all the used pointers, delete the items in the rhs     
//  then delete the rhs from the stack.                                    
//

void ParserEngine::handle_ast_finish(ParserEngine& prse,
                                     const VCodeOperand* operands,
                                     int64_t& pc,
                                     int64_t location)
#line 2048 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    for (auto ast_ptr: prse.ast_dirty_set)
    {
        *ast_ptr = nullptr;
    }

    for (auto i: prse.ast_dirty_base_set)
    {
        prse.ast_stack[i] = nullptr;
    }

    auto first = prse.ast_stack.end() - 1 - operands[0].integer;
    auto last = prse.ast_stack.end() - 1;

    if (first < last)
    {

        for (auto it = first; it < last; it++)
        {
            delete *it;
        }

        prse.ast_stack.erase(first, last);

    }
    
}

//
//  handle_ast_new                                                        
//  --------------                                                        
//                                                                        
//  This is the start of a set former. We mark the top of stack in a      
//  register. Later the difference between the then top of stack and this 
//  mark is the number of children in the formed Ast.                     
//

void ParserEngine::handle_ast_new(ParserEngine& prse,
                                  const VCodeOperand* operands,
                                  int64_t& pc,
                                  int64_t location)
#line 2088 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.register_list[operands[0].register_num] = prse.ast_stack.size();
}

//
//  handle_ast_form                                                       
//  ---------------                                                       
//                                                                        
//  This is the end of the first phase of an Ast former. The stack        
//  contains the children of the new Ast node. We create the new Ast with 
//  the desired children, pop the children and push the new Ast.          
//

void ParserEngine::handle_ast_form(ParserEngine& prse,
                                   const VCodeOperand* operands,
                                   int64_t& pc,
                                   int64_t location)
#line 2103 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    int num_children = prse.ast_stack.size() - prse.register_list[operands[1].register_num];
    Ast* ast = new Ast(num_children);

    int64_t ast_location = -1;
    for (int i = operands[2].integer; i > 0 && ast_location < 0; i--)
    {
        ast_location = prse.ast_stack[prse.register_list[operands[0].register_num] - i]->get_location();
    }

    ast->set_location(ast_location);
    
    for (int i = 0; i < num_children; i++)
    {
        ast->set_child(i, prse.ast_stack[prse.register_list[operands[1].register_num] + i]);
    }
    
    if (num_children > 0)
    {
        prse.ast_stack.erase(prse.ast_stack.end() - num_children, prse.ast_stack.end());
    }

    prse.ast_stack.push_back(ast);

}                                                                                          

//
//  handle_ast_load                                                     
//  ---------------                                                     
//                                                                      
//  This first opcode in a child reference. We load an Ast pointer from 
//  the stack into a temporary Ast register.                            
//

void ParserEngine::handle_ast_load(ParserEngine& prse,
                                   const VCodeOperand* operands,
                                   int64_t& pc,
                                   int64_t location)
#line 2140 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    int index = prse.register_list[operands[1].integer] + operands[2].integer;

    if (index < 0 || index >= prse.ast_stack.size())
    {
        cout << "Program error! Invalid Ast index in ParserEngine::handle_ast_load" << endl;
        exit(1);
    }

    prse.ast_list[operands[0].ast_num] = prse.ast_stack[index];
    prse.ast_trail_base = index;
    prse.ast_trail.clear();
    
}

//
//  handle_ast_index                                        
//  ----------------                                        
//                                                          
//  Move down an Ast subtree as part of a child expression. 
//

void ParserEngine::handle_ast_index(ParserEngine& prse,
                                    const VCodeOperand* operands,
                                    int64_t& pc,
                                    int64_t location)
#line 2165 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    Ast* ast = prse.ast_list[operands[0].ast_num];
    int index = operands[1].integer;

    if (index < 0)
    {
        index = ast->get_num_children() + index;
    }

    if (index < 0 || index >= ast->get_num_children())
    {
        prse.errh.add_error(ErrorType::ErrorAstIndex,
                            location,
                            "Invalid Ast Index");
        pc = -1;
        return;
    }

    prse.ast_list[operands[0].ast_num] = ast->get_child(index);
    prse.ast_trail.push_back(ast->children + index);

}

//
//  handle_ast_child                                   
//  ----------------                                   
//                                                     
//  Move an Ast pointer from a temporary to the stack. 
//

void ParserEngine::handle_ast_child(ParserEngine& prse,
                                    const VCodeOperand* operands,
                                    int64_t& pc,
                                    int64_t location)
#line 2198 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    bool is_dirty = false;

    if (prse.ast_dirty_base_set.find(prse.ast_trail_base) != prse.ast_dirty_base_set.end())
    {
        is_dirty = true;
    }

    for (auto ast_ref: prse.ast_trail)
    {

        if (prse.ast_dirty_set.find(ast_ref) != prse.ast_dirty_set.end())
        {
            is_dirty = true;
        }

    }

    if (is_dirty)
    {
        prse.ast_stack.push_back(prse.ast_list[operands[0].ast_num]->clone());
    }
    else
    {

        prse.ast_stack.push_back(prse.ast_list[operands[0].ast_num]);

        if (prse.ast_trail.size() == 0)
        {
            prse.ast_dirty_base_set.insert(prse.ast_trail_base);
        }
        else
        {
            prse.ast_dirty_set.insert(prse.ast_trail.back());
        }

    }

}

//
//  handle_ast_child_slice                                  
//  ----------------------                                  
//                                                          
//  Copy a slice of children from a temporary to the stack. 
//

void ParserEngine::handle_ast_child_slice(ParserEngine& prse,
                                          const VCodeOperand* operands,
                                          int64_t& pc,
                                          int64_t location)
#line 2248 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    Ast* ast = prse.ast_list[operands[0].ast_num];

    int first = operands[1].integer;
    if (first < 0)
    {
        first = ast->get_num_children() + first;
    }

    int last = operands[2].integer;
    if (last < 0)
    {
        last = ast->get_num_children() + last;
    }

    bool is_dirty = false;

    if (prse.ast_dirty_base_set.find(prse.ast_trail_base) != prse.ast_dirty_base_set.end())
    {
        is_dirty = true;
    }

    for (auto ast_ref: prse.ast_trail)
    {

        if (prse.ast_dirty_set.find(ast_ref) != prse.ast_dirty_set.end())
        {
            is_dirty = true;
        }

    }
      
    for (int i = first; i <= last; i++)
    {

        if (i < 0 || i >= ast->get_num_children())
        {
            prse.errh.add_error(ErrorType::ErrorAstIndex,
                                location,
                                "Invalid Ast Index");
            pc = -1;
            return;
        }

        if (is_dirty || prse.ast_dirty_set.find(ast->children + i) != prse.ast_dirty_set.end())
        {
            prse.ast_stack.push_back(ast->get_child(i)->clone());
        }
        else
        {
            prse.ast_stack.push_back(ast->get_child(i));
            prse.ast_dirty_set.insert(ast->children + i);
        }

    }

}

//
//  handle_[simple data transfer]                
//  -----------------------------                
//                                               
//  These instructions transfer simple Ast data. 
//

void ParserEngine::handle_ast_kind(ParserEngine& prse,
                                   const VCodeOperand* operands,
                                   int64_t& pc,
                                   int64_t location)
#line 2316 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.ast_stack.back()->set_kind(prse.ast_list[operands[0].ast_num]->get_kind());
}

void ParserEngine::handle_ast_kind_num(ParserEngine& prse,
                                       const VCodeOperand* operands,
                                       int64_t& pc,
                                       int64_t location)
#line 2322 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.ast_stack.back()->set_kind(operands[0].integer);
}

void ParserEngine::handle_ast_location(ParserEngine& prse,
                                       const VCodeOperand* operands,
                                       int64_t& pc,
                                       int64_t location)
#line 2328 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.ast_stack.back()->set_location(prse.ast_list[operands[0].ast_num]->get_location());
}

void ParserEngine::handle_ast_location_num(ParserEngine& prse,
                                           const VCodeOperand* operands,
                                           int64_t& pc,
                                           int64_t location)
#line 2334 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.ast_stack.back()->set_location(operands[0].integer);
}

void ParserEngine::handle_ast_lexeme(ParserEngine& prse,
                                     const VCodeOperand* operands,
                                     int64_t& pc,
                                     int64_t location)
#line 2340 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.ast_stack.back()->set_lexeme(prse.ast_list[operands[0].ast_num]->get_lexeme());
}

void ParserEngine::handle_ast_lexeme_string(ParserEngine& prse,
                                            const VCodeOperand* operands,
                                            int64_t& pc,
                                            int64_t location)
#line 2346 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
    prse.ast_stack.back()->set_lexeme(prse.prsd.string_list[operands[0].string_num]);
}

//
//  handle_dump_stack                                                     
//  -----------------                                                     
//                                                                        
//  Dump the Ast stack on the console. This is a big listing so hopefully 
//  will be used sparingly.                                               
//

void ParserEngine::handle_dump_stack(ParserEngine& prse,
                                     const VCodeOperand* operands,
                                     int64_t& pc,
                                     int64_t location)
#line 2360 "u:\\hoshi\\raw\\ParserEngine.cpp"
{

    for (int i = prse.ast_stack.size() - 1; i >= 0; i--)
    {

        cout << "Stack item " << to_string(i) << endl
             << setw(string("Stack item ").length() + to_string(i).length()) << setfill('-') << ""    
             << setw(0) << setfill(' ')
             << endl << endl;

        prse.prsi.dump_ast(prse.ast_stack[i]);

    }

}

//
//  Dummy
//  -----
//                                                         
//  We need to create handlers for dummy opcodes.
//

void ParserEngine::handle_null(ParserEngine& prse,
                               const VCodeOperand* operands,
                               int64_t& pc,
                               int64_t location)
#line 2385 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
}

void ParserEngine::handle_label(ParserEngine& prse,
                                const VCodeOperand* operands,
                                int64_t& pc,
                                int64_t location)
#line 2390 "u:\\hoshi\\raw\\ParserEngine.cpp"
{
}

} // namespace hoshi


