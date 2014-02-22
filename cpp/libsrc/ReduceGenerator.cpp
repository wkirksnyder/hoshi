#line 189 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
//
//  ReduceGenerator                                                       
//  ---------------                                                       
//                                                                        
//  When we reduce by a rule we generally create an Ast from those on top 
//  of the stack and we can perform some to affect guard conditions. This 
//  module contains the Ast forming code and calls the ActionGenerator to 
//  do the guard actions.                                                 
//

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
#include "Grammar.H"
#include "CodeGenerator.H"
#include "ReduceGenerator.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;

//
//  Statement wiring tables                                
//  -----------------------                                
//                                               
//  Tables that help us route nodes to handlers. 
//

void (*ReduceGenerator::former_handler[])(ReduceGenerator& redg,
                                          Ast* root,
                                          Context& ctx)
{
    handle_error,                  // Unknown
    handle_error,                  // Null
    handle_error,                  // Grammar
    handle_error,                  // OptionList
    handle_error,                  // TokenList
    handle_error,                  // RuleList
    handle_error,                  // Lookaheads
    handle_error,                  // ErrorRecovery
    handle_error,                  // Conflicts
    handle_error,                  // KeepWhitespace
    handle_error,                  // CaseSensitive
    handle_error,                  // TokenDeclaration
    handle_error,                  // TokenOptionList
    handle_error,                  // TokenTemplate
    handle_error,                  // TokenDescription
    handle_error,                  // TokenRegexList
    handle_error,                  // TokenRegex
    handle_error,                  // TokenPrecedence
    handle_error,                  // TokenAction
    handle_error,                  // TokenLexeme
    handle_error,                  // TokenIgnore
    handle_error,                  // TokenError
    handle_error,                  // Rule
    handle_error,                  // RuleRhsList
    handle_error,                  // RuleRhs
    handle_error,                  // Optional
    handle_error,                  // ZeroClosure
    handle_error,                  // OneClosure
    handle_error,                  // Group
    handle_error,                  // RulePrecedence
    handle_error,                  // RulePrecedenceList
    handle_error,                  // RulePrecedenceSpec
    handle_error,                  // RuleLeftAssoc
    handle_error,                  // RuleRightAssoc
    handle_error,                  // RuleOperatorList
    handle_error,                  // RuleOperatorSpec
    handle_error,                  // TerminalReference
    handle_error,                  // NonterminalReference
    handle_error,                  // Empty
    handle_ast_former,             // AstFormer
    handle_error,                  // AstItemList
    handle_ast_child,              // AstChild
    handle_ast_kind,               // AstKind
    handle_ast_location,           // AstLocation
    handle_ast_location_string,    // AstLocationString
    handle_ast_lexeme,             // AstLexeme
    handle_ast_lexeme_string,      // AstLexemeString
    handle_error,                  // AstLocator
    handle_ast_dot,                // AstDot
    handle_ast_slice,              // AstSlice
    handle_error,                  // Token
    handle_error,                  // Options
    handle_error,                  // ReduceActions
    handle_error,                  // RegexString
    handle_error,                  // CharsetString
    handle_error,                  // MacroString
    handle_identifier,             // Identifier
    handle_integer,                // Integer
    handle_negative_integer,       // NegativeInteger
    handle_error,                  // String
    handle_error,                  // TripleString
    handle_error,                  // True
    handle_error,                  // False
    handle_error,                  // Regex
    handle_error,                  // RegexOr
    handle_error,                  // RegexList
    handle_error,                  // RegexOptional
    handle_error,                  // RegexZeroClosure
    handle_error,                  // RegexOneClosure
    handle_error,                  // RegexChar
    handle_error,                  // RegexWildcard
    handle_error,                  // RegexWhitespace
    handle_error,                  // RegexNotWhitespace
    handle_error,                  // RegexDigits
    handle_error,                  // RegexNotDigits
    handle_error,                  // RegexEscape
    handle_error,                  // RegexAltNewline
    handle_error,                  // RegexNewline
    handle_error,                  // RegexCr
    handle_error,                  // RegexVBar
    handle_error,                  // RegexStar
    handle_error,                  // RegexPlus
    handle_error,                  // RegexQuestion
    handle_error,                  // RegexPeriod
    handle_error,                  // RegexDollar
    handle_error,                  // RegexSpace
    handle_error,                  // RegexLeftParen
    handle_error,                  // RegexRightParen
    handle_error,                  // RegexLeftBracket
    handle_error,                  // RegexRightBracket
    handle_error,                  // RegexLeftBrace
    handle_error,                  // RegexRightBrace
    handle_error,                  // Charset
    handle_error,                  // CharsetInvert
    handle_error,                  // CharsetRange
    handle_error,                  // CharsetChar
    handle_error,                  // CharsetWhitespace
    handle_error,                  // CharsetNotWhitespace
    handle_error,                  // CharsetDigits
    handle_error,                  // CharsetNotDigits
    handle_error,                  // CharsetEscape
    handle_error,                  // CharsetAltNewline
    handle_error,                  // CharsetNewline
    handle_error,                  // CharsetCr
    handle_error,                  // CharsetCaret
    handle_error,                  // CharsetDash
    handle_error,                  // CharsetDollar
    handle_error,                  // CharsetLeftBracket
    handle_error,                  // CharsetRightBracket
    handle_error,                  // ActionStatementList
    handle_error,                  // ActionAssign
    handle_error,                  // ActionEqual
    handle_error,                  // ActionNotEqual
    handle_error,                  // ActionLessThan
    handle_error,                  // ActionLessEqual
    handle_error,                  // ActionGreaterThan
    handle_error,                  // ActionGreaterEqual
    handle_error,                  // ActionAdd
    handle_error,                  // ActionSubtract
    handle_error,                  // ActionMultiply
    handle_error,                  // ActionDivide
    handle_error,                  // ActionUnaryMinus
    handle_error,                  // ActionAnd
    handle_error,                  // ActionOr
    handle_error,                  // ActionNot
    handle_error,                  // ActionDumpStack
    handle_error                   // ActionTokenCount
#line 258 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
};

char *ReduceGenerator::former_handler_name[]
{
    "handle_error",                // Unknown
    "handle_error",                // Null
    "handle_error",                // Grammar
    "handle_error",                // OptionList
    "handle_error",                // TokenList
    "handle_error",                // RuleList
    "handle_error",                // Lookaheads
    "handle_error",                // ErrorRecovery
    "handle_error",                // Conflicts
    "handle_error",                // KeepWhitespace
    "handle_error",                // CaseSensitive
    "handle_error",                // TokenDeclaration
    "handle_error",                // TokenOptionList
    "handle_error",                // TokenTemplate
    "handle_error",                // TokenDescription
    "handle_error",                // TokenRegexList
    "handle_error",                // TokenRegex
    "handle_error",                // TokenPrecedence
    "handle_error",                // TokenAction
    "handle_error",                // TokenLexeme
    "handle_error",                // TokenIgnore
    "handle_error",                // TokenError
    "handle_error",                // Rule
    "handle_error",                // RuleRhsList
    "handle_error",                // RuleRhs
    "handle_error",                // Optional
    "handle_error",                // ZeroClosure
    "handle_error",                // OneClosure
    "handle_error",                // Group
    "handle_error",                // RulePrecedence
    "handle_error",                // RulePrecedenceList
    "handle_error",                // RulePrecedenceSpec
    "handle_error",                // RuleLeftAssoc
    "handle_error",                // RuleRightAssoc
    "handle_error",                // RuleOperatorList
    "handle_error",                // RuleOperatorSpec
    "handle_error",                // TerminalReference
    "handle_error",                // NonterminalReference
    "handle_error",                // Empty
    "handle_ast_former",           // AstFormer
    "handle_error",                // AstItemList
    "handle_ast_child",            // AstChild
    "handle_ast_kind",             // AstKind
    "handle_ast_location",         // AstLocation
    "handle_ast_location_string",  // AstLocationString
    "handle_ast_lexeme",           // AstLexeme
    "handle_ast_lexeme_string",    // AstLexemeString
    "handle_error",                // AstLocator
    "handle_ast_dot",              // AstDot
    "handle_ast_slice",            // AstSlice
    "handle_error",                // Token
    "handle_error",                // Options
    "handle_error",                // ReduceActions
    "handle_error",                // RegexString
    "handle_error",                // CharsetString
    "handle_error",                // MacroString
    "handle_identifier",           // Identifier
    "handle_integer",              // Integer
    "handle_negative_integer",     // NegativeInteger
    "handle_error",                // String
    "handle_error",                // TripleString
    "handle_error",                // True
    "handle_error",                // False
    "handle_error",                // Regex
    "handle_error",                // RegexOr
    "handle_error",                // RegexList
    "handle_error",                // RegexOptional
    "handle_error",                // RegexZeroClosure
    "handle_error",                // RegexOneClosure
    "handle_error",                // RegexChar
    "handle_error",                // RegexWildcard
    "handle_error",                // RegexWhitespace
    "handle_error",                // RegexNotWhitespace
    "handle_error",                // RegexDigits
    "handle_error",                // RegexNotDigits
    "handle_error",                // RegexEscape
    "handle_error",                // RegexAltNewline
    "handle_error",                // RegexNewline
    "handle_error",                // RegexCr
    "handle_error",                // RegexVBar
    "handle_error",                // RegexStar
    "handle_error",                // RegexPlus
    "handle_error",                // RegexQuestion
    "handle_error",                // RegexPeriod
    "handle_error",                // RegexDollar
    "handle_error",                // RegexSpace
    "handle_error",                // RegexLeftParen
    "handle_error",                // RegexRightParen
    "handle_error",                // RegexLeftBracket
    "handle_error",                // RegexRightBracket
    "handle_error",                // RegexLeftBrace
    "handle_error",                // RegexRightBrace
    "handle_error",                // Charset
    "handle_error",                // CharsetInvert
    "handle_error",                // CharsetRange
    "handle_error",                // CharsetChar
    "handle_error",                // CharsetWhitespace
    "handle_error",                // CharsetNotWhitespace
    "handle_error",                // CharsetDigits
    "handle_error",                // CharsetNotDigits
    "handle_error",                // CharsetEscape
    "handle_error",                // CharsetAltNewline
    "handle_error",                // CharsetNewline
    "handle_error",                // CharsetCr
    "handle_error",                // CharsetCaret
    "handle_error",                // CharsetDash
    "handle_error",                // CharsetDollar
    "handle_error",                // CharsetLeftBracket
    "handle_error",                // CharsetRightBracket
    "handle_error",                // ActionStatementList
    "handle_error",                // ActionAssign
    "handle_error",                // ActionEqual
    "handle_error",                // ActionNotEqual
    "handle_error",                // ActionLessThan
    "handle_error",                // ActionLessEqual
    "handle_error",                // ActionGreaterThan
    "handle_error",                // ActionGreaterEqual
    "handle_error",                // ActionAdd
    "handle_error",                // ActionSubtract
    "handle_error",                // ActionMultiply
    "handle_error",                // ActionDivide
    "handle_error",                // ActionUnaryMinus
    "handle_error",                // ActionAnd
    "handle_error",                // ActionOr
    "handle_error",                // ActionNot
    "handle_error",                // ActionDumpStack
    "handle_error"                 // ActionTokenCount
#line 284 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
};

//
//  generate                                                            
//  --------                                                            
//                                                                      
//  Traverse the Ast attached to the rule generating code to adjust the 
//  Ast stack.                                                          
//

void ReduceGenerator::generate()
{

    rule_label.clear();

    for (Rule* rule: gram.rule_list)
    {

        if ((rule->ast_former_ast == nullptr ||
                rule->ast_former_ast->get_kind() == AstType::AstNull) &&
            (rule->action_ast == nullptr ||
                rule->action_ast->get_kind() == AstType::AstNull))  
        {
            rule_label.push_back(nullptr);
        }
        else
        {

            ICodeLabel* label_ptr = code.get_label();
            rule_label.push_back(label_ptr);
            label_ptr->is_extern = true;

            code.emit(OpcodeType::OpcodeLabel,
                      rule->location,
                      ICodeOperand(label_ptr));

            if (rule->ast_former_ast != nullptr &&
                rule->ast_former_ast->get_kind() != AstType::AstNull)
            {

                Ast* root = rule->ast_former_ast;

                if (root == nullptr || root->get_kind() == AstType::AstNull)
                {
                    return;
                }

                Context ctx;

                ctx.rule = rule;
                ctx.base_ptr = code.get_temporary();

                code.emit(OpcodeType::OpcodeAstStart,
                          root->get_location(),
                          ICodeOperand(ctx.base_ptr));

                if ((debug_flags & DebugType::DebugAstHandlers) != 0)
                {
                    prsi.dump_grammar_ast(root);
                }

                ctx.phase = PhaseType::PhaseTop;
                handle_former(*this, root, ctx);

                code.emit(OpcodeType::OpcodeAstFinish,
                          root->get_location(),
                          ICodeOperand(rule->rhs.size()));

                code.free_temporary(ctx.base_ptr);

            }

            if (rule->action_ast != nullptr &&
                rule->action_ast->get_kind() != AstType::AstNull)
            {
                actg.generate_action(rule->action_ast);
            }

            code.emit(OpcodeType::OpcodeReturn, rule->location);

        }

    }

}

//
//  handle_former                                                         
//  -------------                                                         
//                                                                         
//  Route a call to the appropriate handler. This function is the only one 
//  that should know about our routing table.                              
//

void ReduceGenerator::handle_former(ReduceGenerator& redg,
                                    Ast* root,
                                    Context& ctx)
{
    
    if (root == nullptr)
    {
        cout << "Nullptr in ReduceGenerator::handle_former" << endl;
        exit(1);
    }

    if (root->get_kind() < AstType::AstMinimum ||
        root->get_kind() > AstType::AstMaximum)
    {
        handle_error(redg, root, ctx);
    }

    if ((redg.debug_flags & DebugType::DebugAstHandlers) != 0)
    {
        cout << "ReduceGenerator " 
             << redg.prsi.get_grammar_kind_string(root->get_kind()) << ": " 
             << former_handler_name[root->get_kind()] << endl;
    }

    former_handler[root->get_kind()](redg, root, ctx);

}

//
//  handle_error                                                     
//  ------------                                                     
//                                                                   
//  This should never be called. It means there is a path we haven't 
//  accomodated. It's not a user error, it's a logic error.          
//

#line 418 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
void ReduceGenerator::handle_error(ReduceGenerator& redg,
                                   Ast* root,
                                   Context& ctx)
#line 419 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{
    cout << "No ReduceGenerator::former handler for Ast!" << endl << endl;
    redg.prsi.dump_grammar_ast(root);
    exit(1);
}

//
//  handle_ast_former                                                   
//  -----------------                                                   
//                                                                      
//  Create an Ast from a list of Ast items. We want to do this in 2     
//  passes. First we create the children on the stack. Then we form the 
//  Ast and go back and fill in the other Ast data.                     
//

void ReduceGenerator::handle_ast_former(ReduceGenerator& redg,
                                        Ast* root,
                                        Context& ctx)
#line 436 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{

    //
    //  If we're in the wrong phase, return.
    //

    if (ctx.phase != PhaseType::PhaseChildren &&
        ctx.phase != PhaseType::PhaseTop)
    {
        return;
    }

    //
    //  Set a marker at the current stack top. 
    //

    Context cctx;
    cctx.rule = ctx.rule;
    cctx.base_ptr = ctx.base_ptr;
    cctx.phase = PhaseType::PhaseChildren;

    ICodeRegister* top_ptr = redg.code.get_temporary();

    redg.code.emit(OpcodeType::OpcodeAstNew,
                   root->get_location(),
                   ICodeOperand(top_ptr));
    
    //
    //  Generate code for the children placing each on the stack. 
    //

    for (int i = 0; i < root->get_num_children(); i++)
    {
        handle_former(redg, root->get_child(i), cctx);
    }

    //
    //  Form the Ast with children. 
    //

    redg.code.emit(OpcodeType::OpcodeAstForm,
                   root->get_location(),
                   ICodeOperand(ctx.base_ptr),
                   ICodeOperand(top_ptr),
                   ICodeOperand(ctx.rule->rhs.size()));

    redg.code.free_temporary(top_ptr);
    
    //
    //  Fill in the other Ast data. 
    //

    cctx.phase = PhaseType::PhaseData;
    for (int i = 0; i < root->get_num_children(); i++)
    {
        handle_former(redg, root->get_child(i), cctx);
    }

    if (cctx.processed_set.find(AstType::AstIdentifier) == cctx.processed_set.end() &&
        cctx.processed_set.find(AstType::AstAstKind) == cctx.processed_set.end())
    {

        int64_t kind = redg.prsd.get_kind_force(ctx.rule->lhs->symbol_name);

        redg.code.emit(OpcodeType::OpcodeAstKindNum,
                       root->get_location(),
                       ICodeOperand(kind));
    
    }

}

//
//  handle_ast_child                                                       
//  ----------------                                                       
//                                                                         
//  Handle Ast child references. These can occur at the top level to hoist 
//  an existing Ast or as part of an Ast Former.                           
//

void ReduceGenerator::handle_ast_child(ReduceGenerator& redg,
                                       Ast* root,
                                       Context& ctx)
#line 518 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{

    //
    //  If the expression occurs at the top level copy the Ast and move 
    //  the pointers around.                                            
    //

    if (ctx.phase == PhaseType::PhaseTop)
    {

        ctx.ast_ptr = redg.code.get_ast_operand();

        handle_former(redg, root->get_child(0), ctx);

        redg.code.emit(OpcodeType::OpcodeAstChild,
                       root->get_location(),
                       ICodeOperand(ctx.ast_ptr));

        redg.code.free_ast_operand(ctx.ast_ptr);

        return;

    }

    //
    //  If we're in the non-child data phase, return. 
    //

    if (ctx.phase != PhaseType::PhaseChildren)
    {
        return;
    }

    //
    //  Build the child(ren). Note here that we might have one child or a 
    //  slice, and they may be from the stack or nested.                  
    //

    Ast* child = root->get_child(0);
    if (child == nullptr || child->get_kind() == AstType::AstNull)
    {
        ctx.ast_ptr = nullptr;
    }
    else
    {
        handle_former(redg, child, ctx);
    }
    
    child = root->get_child(1);
    if (child == nullptr || child->get_kind() == AstType::AstNull)
    {
        redg.code.emit(OpcodeType::OpcodeAstChild,
                       root->get_location(),
                       ICodeOperand(ctx.ast_ptr));
    }
    else
    {
        handle_former(redg, child, ctx);
    }

    if (ctx.ast_ptr != nullptr)
    {
        redg.code.free_ast_operand(ctx.ast_ptr);
    }

}

//
//  handle_ast_dot                                                         
//  --------------                                                         
//                                                                         
//  A dot expression starts with the stack and descends down a subtree     
//  until it reaches the desired node. Note that we can find invalid       
//  values on the stack here, but must wait until runtime to check indices 
//  in nodes. Maybe someday I'll include a deeper analysis to move this to 
//  compile time.                                                          
//

void ReduceGenerator::handle_ast_dot(ReduceGenerator& redg,
                                     Ast* root,
                                     Context& ctx)
#line 598 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{
    
    if (root->get_num_children() == 0)
    {
        ctx.ast_ptr = nullptr;
        return;
    }

    //
    //  Use the first index to load from the stack. 
    //

    handle_former(redg, root->get_child(0), ctx);
    int64_t child_num = ctx.integer_value;

    if (child_num < 0)
    {
        child_num = ctx.rule->rhs.size() + 1 + child_num;
    }

    if (child_num < 1 || child_num > ctx.rule->rhs.size())
    {

        ostringstream ost;
        ost << "Child index must be within rule: 1 to " 
            << ctx.rule->rhs.size();

        redg.errh.add_error(ErrorType::ErrorAstIndex,
                            root->get_location(),
                            ost.str());

        child_num = 0;

    }

    ctx.ast_ptr = redg.code.get_ast_operand();
    redg.code.emit(OpcodeType::OpcodeAstLoad,
                   root->get_location(),
                   ICodeOperand(ctx.ast_ptr),
                   ICodeOperand(ctx.base_ptr),
                   ICodeOperand(child_num - ctx.rule->rhs.size() - 1));

    //
    //  Remaining indices descend the subtree. 
    //

    for (int i = 1; i < root->get_num_children(); i++)
    {

        handle_former(redg, root->get_child(i), ctx);

        child_num = ctx.integer_value;
        if (child_num > 0)
        {
            child_num--;
        }

        redg.code.emit(OpcodeType::OpcodeAstIndex,
                       root->get_location(),
                       ICodeOperand(ctx.ast_ptr),
                       ICodeOperand(child_num));

    }

}

//
//  handle_ast_slice                                                      
//  ----------------                                                      
//                                                                        
//  Copy a range of Ast's to the stack. Note that they may also be coming 
//  from the stack.                                                       
//

void ReduceGenerator::handle_ast_slice(ReduceGenerator& redg,
                                       Ast* root,
                                       Context& ctx)
#line 674 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{
    
    //
    //  Get the two indices. 
    //

    handle_former(redg, root->get_child(0), ctx);
    int64_t first = ctx.integer_value;

    handle_former(redg, root->get_child(1), ctx);
    int64_t last = ctx.integer_value;

    //
    //  If there is no Ast pointer these must be on the stack. 
    //

    if (ctx.ast_ptr == nullptr)
    {

        if (first < 0)
        {
            first = ctx.rule->rhs.size() + 1 + first;
        }

        if (first < 1 || first > ctx.rule->rhs.size())
        {

            ostringstream ost;
            ost << "Child index must be within rule: 1 to " 
                << ctx.rule->rhs.size() + 1;

            redg.errh.add_error(ErrorType::ErrorAstIndex,
                                root->get_child(0)->get_location(),
                                ost.str());

            first = 0;

        }

        if (last < 0)
        {
            last = ctx.rule->rhs.size() + 1 + last;
        }

        if (last < 1 || last > ctx.rule->rhs.size())
        {

            ostringstream ost;
            ost << "Child index must be within rule: 1 to " 
                << ctx.rule->rhs.size() + 1;

            redg.errh.add_error(ErrorType::ErrorAstIndex,
                                root->get_child(1)->get_location(),
                                ost.str());

            last = 0;

        }

        for (int64_t i = first; i <= last; i++)
        {
            
            ICodeAst* ast_ptr = redg.code.get_ast_operand();

            redg.code.emit(OpcodeType::OpcodeAstLoad,
                           root->get_location(),
                           ICodeOperand(ast_ptr),
                           ICodeOperand(ctx.base_ptr),
                           ICodeOperand(i - ctx.rule->rhs.size() - 1));

            redg.code.emit(OpcodeType::OpcodeAstChild,
                           root->get_location(),
                           ICodeOperand(ast_ptr));

            redg.code.free_ast_operand(ast_ptr);

        }

        return;

    }

    //
    //  Get the result from the last Ast. 
    //

    if (first > 0)
    {
        first--;
    }

    if (last > 0)
    {
        last--;
    }

    redg.code.emit(OpcodeType::OpcodeAstChildSlice,
                   root->get_location(),
                   ICodeOperand(ctx.ast_ptr),
                   ICodeOperand(first),
                   ICodeOperand(last));

}

//
//  handle_identifier                                                   
//  -----------------                                                   
//                                                                      
//  A raw identifier in this context is an Ast kind. We decode the type 
//  and emit the instruction.                                           
//

void ReduceGenerator::handle_identifier(ReduceGenerator& redg,
                                        Ast* root,
                                        Context& ctx)
#line 788 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{

    if (ctx.phase != PhaseType::PhaseData)
    {
        return;
    }

    if (ctx.processed_set.find(AstType::AstIdentifier) != ctx.processed_set.end() ||
        ctx.processed_set.find(AstType::AstAstKind) != ctx.processed_set.end())
    {
        redg.errh.add_error(ErrorType::ErrorDupAstItem,
                            root->get_location(),
                            "Duplicate Ast kind");
        return;
    }

    int64_t kind = redg.prsd.get_kind_force(root->get_lexeme());

    redg.code.emit(OpcodeType::OpcodeAstKindNum,
                   root->get_location(),
                   ICodeOperand(kind));
    
    ctx.processed_set.insert(AstType::AstIdentifier);

}

//
//  handle_ast_kind                     
//  ---------------                     
//                                      
//  Copy the kind from an existing Ast. 
//

void ReduceGenerator::handle_ast_kind(ReduceGenerator& redg,
                                      Ast* root,
                                      Context& ctx)
#line 823 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{

    if (ctx.phase != PhaseType::PhaseData)
    {
        return;
    }

    if (ctx.processed_set.find(AstType::AstIdentifier) != ctx.processed_set.end() ||
        ctx.processed_set.find(AstType::AstAstKind) != ctx.processed_set.end())
    {
        redg.errh.add_error(ErrorType::ErrorDupAstItem,
                            root->get_location(),
                            "Duplicate Ast kind");
        return;
    }

    ctx.ast_ptr = nullptr;
    handle_former(redg, root->get_child(0), ctx);
    
    redg.code.emit(OpcodeType::OpcodeAstKind,
                   root->get_location(),
                   ICodeOperand(ctx.ast_ptr));
    
    if (ctx.ast_ptr != nullptr)
    {
        redg.code.free_ast_operand(ctx.ast_ptr);
    }

    ctx.processed_set.insert(AstType::AstAstKind);

}

//
//  handle_ast_location                     
//  -------------------                     
//                                      
//  Copy the location from an existing Ast. 
//

void ReduceGenerator::handle_ast_location(ReduceGenerator& redg,
                                          Ast* root,
                                          Context& ctx)
#line 864 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{

    if (ctx.phase != PhaseType::PhaseData)
    {
        return;
    }

    if (ctx.processed_set.find(AstType::AstAstLocation) != ctx.processed_set.end() ||
        ctx.processed_set.find(AstType::AstAstLocationString) != ctx.processed_set.end())
    {
        redg.errh.add_error(ErrorType::ErrorDupAstItem,
                            root->get_location(),
                            "Duplicate Ast location");
        return;
    }

    ctx.ast_ptr = nullptr;
    handle_former(redg, root->get_child(0), ctx);
    
    redg.code.emit(OpcodeType::OpcodeAstLocation,
                   root->get_location(),
                   ICodeOperand(ctx.ast_ptr));
    
    if (ctx.ast_ptr != nullptr)
    {
        redg.code.free_ast_operand(ctx.ast_ptr);
    }

    ctx.processed_set.insert(AstType::AstAstLocation);

}

//
//  handle_ast_location_string                     
//  --------------------------                     
//                                      
//  Copy the location from a location string.
//

void ReduceGenerator::handle_ast_location_string(ReduceGenerator& redg,
                                                 Ast* root,
                                                 Context& ctx)
#line 905 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{

    if (ctx.phase != PhaseType::PhaseData)
    {
        return;
    }

    if (ctx.processed_set.find(AstType::AstAstLocation) != ctx.processed_set.end() ||
        ctx.processed_set.find(AstType::AstAstLocationString) != ctx.processed_set.end())
    {
        redg.errh.add_error(ErrorType::ErrorDupAstItem,
                            root->get_location(),
                            "Duplicate Ast location");
        return;
    }

    string lexeme = root->get_child(0)->get_lexeme();
    int64_t num = atol(lexeme.substr(1, lexeme.length() - 2).c_str());

    redg.code.emit(OpcodeType::OpcodeAstLocationNum,
                   root->get_location(),
                   ICodeOperand(num));

    ctx.processed_set.insert(AstType::AstAstLocationString);

}

//
//  handle_ast_lexeme                     
//  -----------------                     
//                                      
//  Copy the lexeme from an existing Ast. 
//

void ReduceGenerator::handle_ast_lexeme(ReduceGenerator& redg,
                                        Ast* root,
                                        Context& ctx)
#line 941 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{

    if (ctx.phase != PhaseType::PhaseData)
    {
        return;
    }

    if (ctx.processed_set.find(AstType::AstAstLexeme) != ctx.processed_set.end() ||
        ctx.processed_set.find(AstType::AstAstLexemeString) != ctx.processed_set.end())
    {
        redg.errh.add_error(ErrorType::ErrorDupAstItem,
                            root->get_location(),
                            "Duplicate Ast lexeme");
        return;
    }

    ctx.ast_ptr = nullptr;
    handle_former(redg, root->get_child(0), ctx);
    
    redg.code.emit(OpcodeType::OpcodeAstLexeme,
                   root->get_location(),
                   ICodeOperand(ctx.ast_ptr));
    
    if (ctx.ast_ptr != nullptr)
    {
        redg.code.free_ast_operand(ctx.ast_ptr);
    }

    ctx.processed_set.insert(AstType::AstAstLexeme);

}

//
//  handle_ast_lexeme_string
//  ------------------------                     
//                                      
//  Copy the lexeme from a literal string.
//

void ReduceGenerator::handle_ast_lexeme_string(ReduceGenerator& redg,
                                               Ast* root,
                                               Context& ctx)
#line 982 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{

    if (ctx.phase != PhaseType::PhaseData)
    {
        return;
    }

    if (ctx.processed_set.find(AstType::AstAstLexeme) != ctx.processed_set.end() ||
        ctx.processed_set.find(AstType::AstAstLexemeString) != ctx.processed_set.end())
    {
        redg.errh.add_error(ErrorType::ErrorDupAstItem,
                            root->get_location(),
                            "Duplicate Ast lexeme");
        return;
    }

    string lexeme = root->get_child(0)->get_lexeme();
    string* string_ptr = redg.code.get_string(lexeme.substr(1, lexeme.length() - 2));

    redg.code.emit(OpcodeType::OpcodeAstLexemeString,
                   root->get_location(),
                   ICodeOperand(string_ptr));
    
    ctx.processed_set.insert(AstType::AstAstLexemeString);

}

//
//  handle_integer                                      
//  --------------                                      
//                                                      
//  Extract the integer value to be used by the caller. 
//

void ReduceGenerator::handle_integer(ReduceGenerator& redg,
                                     Ast* root,
                                     Context& ctx)
#line 1018 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{
    ctx.integer_value = atol(root->get_lexeme().c_str());
}

void ReduceGenerator::handle_negative_integer(ReduceGenerator& redg,
                                              Ast* root,
                                              Context& ctx)
#line 1024 "u:\\hoshi\\raw\\ReduceGenerator.cpp"
{
    ctx.integer_value = -atol(root->get_lexeme().c_str());
}

//
//  save_parser_data                                                     
//  ----------------                                                     
//                                                                       
//  This function is called *after* the code generator stores vm code in 
//  the parser. At this point labels have been given addresses so it's   
//  safe to store labels in the parser.                                  
//

void ReduceGenerator::save_parser_data()
{

    prsd.rule_pc = new int64_t[rule_label.size()];

    for (size_t i = 0; i < rule_label.size(); i++)
    {

        if (rule_label[i] == nullptr)
        {
            prsd.rule_pc[i] = -1;
        }
        else
        {
            prsd.rule_pc[i] = rule_label[i]->pc;
        }

    }

}

} // namespace hoshi



