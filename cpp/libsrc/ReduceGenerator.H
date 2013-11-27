//
//  ReduceGenerator                                                       
//  ---------------                                                       
//                                                                        
//  When we reduce by a rule we generally create an Ast from those on top 
//  of the stack and we can perform some to affect guard conditions. This 
//  module contains the Ast forming code and calls the ActionGenerator to 
//  do the guard actions.                                                 
//

#ifndef REDUCE_GENERATOR_H
#define REDUCE_GENERATOR_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "AstType.H"
#include "ErrorHandler.H"
#include "Parser.H"
#include "ParserImpl.H"
#include "ParserData.H"
#include "Grammar.H"
#include "CodeGenerator.H"
#include "ActionGenerator.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

class ReduceGenerator final
{
public:

    ReduceGenerator(ParserImpl& prsi, 
                    ErrorHandler& errh,
                    Grammar& gram,
                    CodeGenerator& code,
                    ActionGenerator& actg,
                    ParserData& prsd,
                    int64_t debug_flags)
        : prsi(prsi), errh(errh), gram(gram), code(code), actg(actg), prsd(prsd),
          debug_flags(debug_flags) {}

    void generate();
    void save_parser_data();

private:

    ParserImpl& prsi;
    ErrorHandler& errh;
    Grammar& gram;
    CodeGenerator& code;
    ActionGenerator& actg;
    ParserData& prsd;
    int64_t debug_flags;

    //
    //  Phase                                                           
    //  -----                                                           
    //                                                                  
    //  We have to go over the Ast multiple times. This indicates which 
    //  phase we are performing.                                        
    //

    enum PhaseType
    {
        PhaseTop = 0,
        PhaseChildren = 1,
        PhaseData = 2
    };
        
    //
    //  Context                                                         
    //  -------                                                         
    //                                                                  
    //  We will perform something like a depth first search on the Ast. 
    //  This structure is passed along to communicate among the nodes.  
    //

    struct Context
    {

        std::set<int> processed_set;    
        PhaseType phase = PhaseType::PhaseTop;
        Rule *rule = nullptr;
        ICodeRegister* base_ptr = nullptr;
        ICodeAst* ast_ptr = nullptr;
        int64_t integer_value = 0;

    };

    //
    //  Odd to keep a list rather than an instance variable, but it helps 
    //  keep the visibility of code generator stuff narrower.
    //

    std::vector<ICodeLabel*> rule_label;

    //
    //  The generic node handler and specific handlers for various node 
    //  types.                                                          
    //

    static void (*former_handler[])(ReduceGenerator& redg,
                                    Ast* root,
                                    Context& ctx);

    static char *former_handler_name[];

    static void handle_former(ReduceGenerator& redg,
                              Ast* root,
                              Context& ctx);

    static void handle_error(ReduceGenerator& redg,
                             Ast* root,
                             Context& ctx);
    
    static void handle_ast_former(ReduceGenerator& redg,
                                  Ast* root,
                                  Context& ctx);
    
    static void handle_ast_child(ReduceGenerator& redg,
                                 Ast* root,
                                 Context& ctx);
    
    static void handle_ast_kind(ReduceGenerator& redg,
                                Ast* root,
                                Context& ctx);
    
    static void handle_ast_location(ReduceGenerator& redg,
                                    Ast* root,
                                    Context& ctx);
    
    static void handle_ast_location_string(ReduceGenerator& redg,
                                           Ast* root,
                                           Context& ctx);
    
    static void handle_ast_lexeme(ReduceGenerator& redg,
                                  Ast* root,
                                  Context& ctx);
    
    static void handle_ast_lexeme_string(ReduceGenerator& redg,
                                         Ast* root,
                                         Context& ctx);
    
    static void handle_ast_dot(ReduceGenerator& redg,
                               Ast* root,
                               Context& ctx);
    
    static void handle_ast_slice(ReduceGenerator& redg,
                                 Ast* root,
                                 Context& ctx);
    
    static void handle_identifier(ReduceGenerator& redg,
                                  Ast* root,
                                  Context& ctx);
    
    static void handle_integer(ReduceGenerator& redg,
                               Ast* root,
                               Context& ctx);
    
    static void handle_negative_integer(ReduceGenerator& redg,
                                        Ast* root,
                                        Context& ctx);
    

};

} // namespace hoshi

#endif // REDUCE_GENERATOR_H
