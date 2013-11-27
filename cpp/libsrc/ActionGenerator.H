//
//  ActionGenerator                                                        
//  ---------------                                                        
//                                                                         
//  Generate intermediate code for token actions, reduce actions and guard 
//  conditions.                                                            
//

#ifndef ACTION_GENERATOR_H
#define ACTION_GENERATOR_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "AstType.H"
#include "Parser.H"
#include "Grammar.H"
#include "CodeGenerator.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

class ActionGenerator final
{
public:

    ActionGenerator(ParserImpl& prsi,
                    ErrorHandler& errh,
                    CodeGenerator& code,
                    int64_t debug_flags)
        : prsi(prsi), errh(errh), code(code), debug_flags(debug_flags) {}

    void generate_action(Ast* root);
    void generate_condition(Ast* root,
                            ICodeLabel* true_label,
                            ICodeLabel* false_label);

private:

    ParserImpl& prsi;
    ErrorHandler& errh;
    CodeGenerator& code;
    int64_t debug_flags;

    //
    //  Context                                                         
    //  -------                                                         
    //                                                                  
    //  We will perform something like a depth first search on the Ast. 
    //  This structure is passed along to communicate among the nodes.  
    //

    struct Context
    {
        ICodeRegister* target_register = nullptr;
        ICodeLabel* true_label = nullptr;
        ICodeLabel* false_label = nullptr;
        OpcodeType opcode = OpcodeType::OpcodeNull;
    };

    //
    //  Statement coding functions                                       
    //  --------------------------                                       
    //                                                                   
    //  We have three separate contexts for which we must generate code: 
    //  statement, expressions and conditions. These functions handle    
    //  statements.                                                      
    //

    static void (*statement_handler[])(ActionGenerator& actg,
                                       Ast* root,
                                       Context& ctx);

    static char *statement_handler_name[];

    static void handle_statement(ActionGenerator& actg,
                                 Ast* root,
                                 Context& ctx);

    static void handle_statement_error(ActionGenerator& actg,
                                       Ast* root,
                                       Context& ctx);
    
    static void handle_statement_statement_list(ActionGenerator& actg,
                                                Ast* root,
                                                Context& ctx);
    
    static void handle_statement_assign(ActionGenerator& actg,
                                        Ast* root,
                                        Context& ctx);
    
    static void handle_statement_dump_stack(ActionGenerator& actg,
                                            Ast* root,
                                            Context& ctx);
    

    //
    //  Expression coding functions 
    //  --------------------------- 
    //                                                                   
    //  We have three separate contexts for which we must generate code: 
    //  statement, expressions and conditions. These functions handle    
    //  expressions.                                                      
    //

    static void (*expression_handler[])(ActionGenerator& actg,
                                        Ast* root,
                                        Context& ctx);

    static char *expression_handler_name[];

    static void handle_expression(ActionGenerator& actg,
                                  Ast* root,
                                  Context& ctx);

    static void handle_expression_error(ActionGenerator& actg,
                                        Ast* root,
                                        Context& ctx);
    
    static void handle_expression_math_binop(ActionGenerator& actg,
                                             Ast* root,
                                             Context& ctx);
    
    static void handle_expression_math_unop(ActionGenerator& actg,
                                            Ast* root,
                                            Context& ctx);
    
    static void handle_expression_identifier(ActionGenerator& actg,
                                             Ast* root,
                                             Context& ctx);
    
    static void handle_expression_integer(ActionGenerator& actg,
                                          Ast* root,
                                          Context& ctx);
    
    static void handle_expression_relation(ActionGenerator& actg,
                                           Ast* root,
                                           Context& ctx);
    
    static void handle_expression_add(ActionGenerator& actg,
                                      Ast* root,
                                      Context& ctx);
    
    static void handle_expression_subtract(ActionGenerator& actg,
                                           Ast* root,
                                           Context& ctx);
    
    static void handle_expression_multiply(ActionGenerator& actg,
                                           Ast* root,
                                           Context& ctx);
    
    static void handle_expression_divide(ActionGenerator& actg,
                                         Ast* root,
                                         Context& ctx);
    
    static void handle_expression_unary_minus(ActionGenerator& actg,
                                              Ast* root,
                                              Context& ctx);
    
    static void handle_expression_token_count(ActionGenerator& actg,
                                              Ast* root,
                                              Context& ctx);
    

    //
    //  Condition coding functions 
    //  -------------------------- 
    //                                                                   
    //  We have three separate contexts for which we must generate code: 
    //  statement, expressions and conditions. These functions handle    
    //  conditions.                                                      
    //

    static void (*condition_handler[])(ActionGenerator& actg,
                                       Ast* root,
                                       Context& ctx);

    static char *condition_handler_name[];

    static void handle_condition(ActionGenerator& actg,
                                 Ast* root,
                                 Context& ctx);

    static void handle_condition_error(ActionGenerator& actg,
                                       Ast* root,
                                       Context& ctx);
    
    static void handle_condition_relation(ActionGenerator& actg,
                                          Ast* root,
                                          Context& ctx);
    
    static void handle_condition_math(ActionGenerator& actg,
                                      Ast* root,
                                      Context& ctx);
    
    static void handle_condition_equal(ActionGenerator& actg,
                                       Ast* root,
                                       Context& ctx);
    
    static void handle_condition_not_equal(ActionGenerator& actg,
                                           Ast* root,
                                           Context& ctx);
    
    static void handle_condition_less_than(ActionGenerator& actg,
                                           Ast* root,
                                           Context& ctx);
    
    static void handle_condition_less_equal(ActionGenerator& actg,
                                            Ast* root,
                                            Context& ctx);
    
    static void handle_condition_greater_than(ActionGenerator& actg,
                                              Ast* root,
                                              Context& ctx);
    
    static void handle_condition_greater_equal(ActionGenerator& actg,
                                               Ast* root,
                                               Context& ctx);
    
    static void handle_condition_and(ActionGenerator& actg,
                                     Ast* root,
                                     Context& ctx);
    
    static void handle_condition_or(ActionGenerator& actg,
                                    Ast* root,
                                    Context& ctx);
    
    static void handle_condition_not(ActionGenerator& actg,
                                     Ast* root,
                                     Context& ctx);
    

};

} // namespace hoshi

#endif // ACTION_GENERATOR_H
