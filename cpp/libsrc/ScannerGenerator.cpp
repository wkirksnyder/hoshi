//
//  ScannerGenerator                                                       
//  ----------------                                                       
//                                                                         
//  Create the scanner part of the parser from token information in the    
//  grammar source. Each token type has a regular expression or is used as 
//  a literal in the grammar. From these we generate a DFA that can be     
//  called by the parser to scan the input source.                         
//                                                                         
//  The theory behind all this is fairly well known. My favorite reference 
//  is Introduction to AUtomata Theory, Languages and Computation by       
//  Hopcraft and Ullman. The 1979 first edition may still be the best one. 
//

#include <cstdint>
#include <cstring>
#include <cctype>
#include <memory>
#include <cstdlib>
#include <functional>
#include <string>
#include <queue>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "AstType.H"
#include "OpcodeType.H"
#include "Parser.H"
#include "ParserImpl.H"
#include "ParserData.H"
#include "ErrorHandler.H"
#include "Grammar.H"
#include "CodeGenerator.H"
#include "ActionGenerator.H"
#include "ScannerGenerator.H"

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
//  Wiring tables                                
//  -------------                                
//                                               
//  Tables that help us route nodes to handlers. 
//

void (*ScannerGenerator::build_nfa_handler[])(ScannerGenerator& scan, Ast* root, Context& ctx)
{
    handle_error,                     // Unknown
    handle_error,                     // Null
    handle_error,                     // Grammar
    handle_error,                     // OptionList
    handle_error,                     // TokenList
    handle_error,                     // RuleList
    handle_error,                     // Lookaheads
    handle_error,                     // ErrorRecovery
    handle_error,                     // Conflicts
    handle_error,                     // KeepWhitespace
    handle_error,                     // CaseSensitive
    handle_error,                     // TokenDeclaration
    handle_error,                     // TokenOptionList
    handle_error,                     // TokenTemplate
    handle_error,                     // TokenDescription
    handle_error,                     // TokenRegexList
    handle_error,                     // TokenRegex
    handle_error,                     // TokenPrecedence
    handle_error,                     // TokenAction
    handle_error,                     // TokenLexeme
    handle_error,                     // TokenIgnore
    handle_error,                     // TokenError
    handle_error,                     // Rule
    handle_error,                     // RuleRhsList
    handle_error,                     // RuleRhs
    handle_error,                     // Optional
    handle_error,                     // ZeroClosure
    handle_error,                     // OneClosure
    handle_error,                     // Group
    handle_error,                     // RulePrecedence
    handle_error,                     // RulePrecedenceList
    handle_error,                     // RulePrecedenceSpec
    handle_error,                     // RuleLeftAssoc
    handle_error,                     // RuleRightAssoc
    handle_error,                     // RuleOperatorList
    handle_error,                     // RuleOperatorSpec
    handle_error,                     // TerminalReference
    handle_error,                     // NonterminalReference
    handle_error,                     // Empty
    handle_error,                     // AstFormer
    handle_error,                     // AstItemList
    handle_error,                     // AstChild
    handle_error,                     // AstKind
    handle_error,                     // AstLocation
    handle_error,                     // AstLocationString
    handle_error,                     // AstLexeme
    handle_error,                     // AstLexemeString
    handle_error,                     // AstLocator
    handle_error,                     // AstDot
    handle_error,                     // AstSlice
    handle_error,                     // Token
    handle_error,                     // Options
    handle_error,                     // ReduceActions
    handle_error,                     // RegexString
    handle_error,                     // CharsetString
    handle_error,                     // MacroString
    handle_error,                     // Identifier
    handle_error,                     // Integer
    handle_error,                     // NegativeInteger
    handle_error,                     // String
    handle_error,                     // TripleString
    handle_error,                     // True
    handle_error,                     // False
    handle_regex,                     // Regex
    handle_regex_or,                  // RegexOr
    handle_regex_list,                // RegexList
    handle_regex_optional,            // RegexOptional
    handle_regex_zero_closure,        // RegexZeroClosure
    handle_regex_one_closure,         // RegexOneClosure
    handle_regex_char,                // RegexChar
    handle_regex_wildcard,            // RegexWildcard
    handle_regex_whitespace,          // RegexWhitespace
    handle_regex_not_whitespace,      // RegexNotWhitespace
    handle_regex_digits,              // RegexDigits
    handle_regex_not_digits,          // RegexNotDigits
    handle_regex_escape,              // RegexEscape
    handle_regex_alt_newline,         // RegexAltNewline
    handle_regex_newline,             // RegexNewline
    handle_regex_cr,                  // RegexCr
    handle_regex_v_bar,               // RegexVBar
    handle_regex_star,                // RegexStar
    handle_regex_plus,                // RegexPlus
    handle_regex_question,            // RegexQuestion
    handle_regex_period,              // RegexPeriod
    handle_regex_dollar,              // RegexDollar
    handle_regex_space,               // RegexSpace
    handle_regex_left_paren,          // RegexLeftParen
    handle_regex_right_paren,         // RegexRightParen
    handle_regex_left_bracket,        // RegexLeftBracket
    handle_regex_right_bracket,       // RegexRightBracket
    handle_regex_left_brace,          // RegexLeftBrace
    handle_regex_right_brace,         // RegexRightBrace
    handle_charset,                   // Charset
    handle_charset_invert,            // CharsetInvert
    handle_charset_range,             // CharsetRange
    handle_charset_char,              // CharsetChar
    handle_charset_whitespace,        // CharsetWhitespace
    handle_charset_not_whitespace,    // CharsetNotWhitespace
    handle_charset_digits,            // CharsetDigits
    handle_charset_not_digits,        // CharsetNotDigits
    handle_charset_escape,            // CharsetEscape
    handle_charset_alt_newline,       // CharsetAltNewline
    handle_charset_newline,           // CharsetNewline
    handle_charset_cr,                // CharsetCr
    handle_charset_caret,             // CharsetCaret
    handle_charset_dash,              // CharsetDash
    handle_charset_dollar,            // CharsetDollar
    handle_charset_left_bracket,      // CharsetLeftBracket
    handle_charset_right_bracket,     // CharsetRightBracket
    handle_error,                     // ActionStatementList
    handle_error,                     // ActionAssign
    handle_error,                     // ActionEqual
    handle_error,                     // ActionNotEqual
    handle_error,                     // ActionLessThan
    handle_error,                     // ActionLessEqual
    handle_error,                     // ActionGreaterThan
    handle_error,                     // ActionGreaterEqual
    handle_error,                     // ActionAdd
    handle_error,                     // ActionSubtract
    handle_error,                     // ActionMultiply
    handle_error,                     // ActionDivide
    handle_error,                     // ActionUnaryMinus
    handle_error,                     // ActionAnd
    handle_error,                     // ActionOr
    handle_error,                     // ActionNot
    handle_error,                     // ActionDumpStack
    handle_error                      // ActionTokenCount
};

char *ScannerGenerator::build_nfa_handler_name[]
{
    "handle_error",                   // Unknown
    "handle_error",                   // Null
    "handle_error",                   // Grammar
    "handle_error",                   // OptionList
    "handle_error",                   // TokenList
    "handle_error",                   // RuleList
    "handle_error",                   // Lookaheads
    "handle_error",                   // ErrorRecovery
    "handle_error",                   // Conflicts
    "handle_error",                   // KeepWhitespace
    "handle_error",                   // CaseSensitive
    "handle_error",                   // TokenDeclaration
    "handle_error",                   // TokenOptionList
    "handle_error",                   // TokenTemplate
    "handle_error",                   // TokenDescription
    "handle_error",                   // TokenRegexList
    "handle_error",                   // TokenRegex
    "handle_error",                   // TokenPrecedence
    "handle_error",                   // TokenAction
    "handle_error",                   // TokenLexeme
    "handle_error",                   // TokenIgnore
    "handle_error",                   // TokenError
    "handle_error",                   // Rule
    "handle_error",                   // RuleRhsList
    "handle_error",                   // RuleRhs
    "handle_error",                   // Optional
    "handle_error",                   // ZeroClosure
    "handle_error",                   // OneClosure
    "handle_error",                   // Group
    "handle_error",                   // RulePrecedence
    "handle_error",                   // RulePrecedenceList
    "handle_error",                   // RulePrecedenceSpec
    "handle_error",                   // RuleLeftAssoc
    "handle_error",                   // RuleRightAssoc
    "handle_error",                   // RuleOperatorList
    "handle_error",                   // RuleOperatorSpec
    "handle_error",                   // TerminalReference
    "handle_error",                   // NonterminalReference
    "handle_error",                   // Empty
    "handle_error",                   // AstFormer
    "handle_error",                   // AstItemList
    "handle_error",                   // AstChild
    "handle_error",                   // AstKind
    "handle_error",                   // AstLocation
    "handle_error",                   // AstLocationString
    "handle_error",                   // AstLexeme
    "handle_error",                   // AstLexemeString
    "handle_error",                   // AstLocator
    "handle_error",                   // AstDot
    "handle_error",                   // AstSlice
    "handle_error",                   // Token
    "handle_error",                   // Options
    "handle_error",                   // ReduceActions
    "handle_error",                   // RegexString
    "handle_error",                   // CharsetString
    "handle_error",                   // MacroString
    "handle_error",                   // Identifier
    "handle_error",                   // Integer
    "handle_error",                   // NegativeInteger
    "handle_error",                   // String
    "handle_error",                   // TripleString
    "handle_error",                   // True
    "handle_error",                   // False
    "handle_regex",                   // Regex
    "handle_regex_or",                // RegexOr
    "handle_regex_list",              // RegexList
    "handle_regex_optional",          // RegexOptional
    "handle_regex_zero_closure",      // RegexZeroClosure
    "handle_regex_one_closure",       // RegexOneClosure
    "handle_regex_char",              // RegexChar
    "handle_regex_wildcard",          // RegexWildcard
    "handle_regex_whitespace",        // RegexWhitespace
    "handle_regex_not_whitespace",    // RegexNotWhitespace
    "handle_regex_digits",            // RegexDigits
    "handle_regex_not_digits",        // RegexNotDigits
    "handle_regex_escape",            // RegexEscape
    "handle_regex_alt_newline",       // RegexAltNewline
    "handle_regex_newline",           // RegexNewline
    "handle_regex_cr",                // RegexCr
    "handle_regex_v_bar",             // RegexVBar
    "handle_regex_star",              // RegexStar
    "handle_regex_plus",              // RegexPlus
    "handle_regex_question",          // RegexQuestion
    "handle_regex_period",            // RegexPeriod
    "handle_regex_dollar",            // RegexDollar
    "handle_regex_space",             // RegexSpace
    "handle_regex_left_paren",        // RegexLeftParen
    "handle_regex_right_paren",       // RegexRightParen
    "handle_regex_left_bracket",      // RegexLeftBracket
    "handle_regex_right_bracket",     // RegexRightBracket
    "handle_regex_left_brace",        // RegexLeftBrace
    "handle_regex_right_brace",       // RegexRightBrace
    "handle_charset",                 // Charset
    "handle_charset_invert",          // CharsetInvert
    "handle_charset_range",           // CharsetRange
    "handle_charset_char",            // CharsetChar
    "handle_charset_whitespace",      // CharsetWhitespace
    "handle_charset_not_whitespace",  // CharsetNotWhitespace
    "handle_charset_digits",          // CharsetDigits
    "handle_charset_not_digits",      // CharsetNotDigits
    "handle_charset_escape",          // CharsetEscape
    "handle_charset_alt_newline",     // CharsetAltNewline
    "handle_charset_newline",         // CharsetNewline
    "handle_charset_cr",              // CharsetCr
    "handle_charset_caret",           // CharsetCaret
    "handle_charset_dash",            // CharsetDash
    "handle_charset_dollar",          // CharsetDollar
    "handle_charset_left_bracket",    // CharsetLeftBracket
    "handle_charset_right_bracket",   // CharsetRightBracket
    "handle_error",                   // ActionStatementList
    "handle_error",                   // ActionAssign
    "handle_error",                   // ActionEqual
    "handle_error",                   // ActionNotEqual
    "handle_error",                   // ActionLessThan
    "handle_error",                   // ActionLessEqual
    "handle_error",                   // ActionGreaterThan
    "handle_error",                   // ActionGreaterEqual
    "handle_error",                   // ActionAdd
    "handle_error",                   // ActionSubtract
    "handle_error",                   // ActionMultiply
    "handle_error",                   // ActionDivide
    "handle_error",                   // ActionUnaryMinus
    "handle_error",                   // ActionAnd
    "handle_error",                   // ActionOr
    "handle_error",                   // ActionNot
    "handle_error",                   // ActionDumpStack
    "handle_error"                    // ActionTokenCount
};

//
//  destructor                        
//  ----------                        
//                                    
//  Free all the states we allocated. 
//

ScannerGenerator::~ScannerGenerator()
{

    for (auto p: allocated_states)
    {
        delete p;
    }

}

//
//  generate                                                               
//  --------                                                               
//                                                                         
//  Generate the scanner code for the parser. This is the external entry   
//  point; the caller should create a ScannerGenerator, call this function 
//  and destroy it.                                                        
//                                                                         
//  This is a facade, calling other functions to perform the various steps 
//  of scanner construction.                                               
//

void ScannerGenerator::generate()
{

    if ((debug_flags & DebugFlags::DebugProgress) != 0)
    {
        cout << "Beginning scanner generation: "
             << prsi.elapsed_time_string() << endl;
    }

    //
    //  Construct an NFA with e-moves from the Ast's or literal strings 
    //  stored with the tokens.                                       
    //

    construct_nfa();
    if ((debug_flags & DebugFlags::DebugScanner) != 0)
    {
        prsi.log_heading("NFA scanner: " + prsi.elapsed_time_string());
        dump_automaton(nfa_start_state);
    }

    //
    //  Convert the NFA with e-moves into a DFA. 
    //

    nfa_to_dfa();
    if ((debug_flags & DebugFlags::DebugScanner) != 0)
    {
        prsi.log_heading("DFA scanner: " + prsi.elapsed_time_string());
        dump_automaton(dfa_start_state);
    }

    //
    //  Minimize the size of the DFA. 
    //

    minimize_dfa();
    if ((debug_flags & DebugFlags::DebugScanner) != 0)
    {
        prsi.log_heading("DFA scanner after optimizing: " + prsi.elapsed_time_string());
        dump_automaton(dfa_start_state);
    }

    //
    //  Create the intermediate code for the scanner and save it in the 
    //  ParserData.                                                      
    //

    create_vmcode();
    if ((debug_flags & DebugFlags::DebugProgress) != 0)
    {
        cout << "Finished scanner generation: " << prsi.elapsed_time_string() << endl;
    }

}

//
//  construct_nfa                                                    
//  -------------                                                    
//                                                                    
//  Construct an NFA with e-moves from regex Ast's stored with the   
//  tokens. For tokens without Ast's build one from the literal 
//  string.                                                           
//

void ScannerGenerator::construct_nfa()
{

    nfa_start_state = get_new_state();

    for (auto mp: gram.symbol_map)
    {

        Symbol* token = mp.second;

        if (!token->is_scanned)
        {
            continue;
        }

        Context ctx;
        ctx.start_state = get_new_state();
        ctx.final_state = ctx.start_state;
        nfa_start_state->e_moves.insert(ctx.start_state);

        for (int i = 0; i < token->regex_list_ast->get_num_children(); i++)
        {

            Ast* guard_ast = token->regex_list_ast->get_child(i)->get_child(0);
            Ast* regex_ast = token->regex_list_ast->get_child(i)->get_child(1);

            if ((debug_flags & DebugFlags::DebugAstHandlers) != 0)
            {
                prsi.dump_grammar_ast(regex_ast);
            }

            handle_build_nfa(*this, regex_ast, ctx);
            ctx.start_state->accept_actions.insert(AcceptAction(token, guard_ast));

        }

    }

}

//
//  handle_build_nfa                                                         
//  ----------------                                                         
//                                                                         
//  Route a call to the appropriate handler. This function is the only one 
//  that should know about our routing table.                              
//

void ScannerGenerator::handle_build_nfa(ScannerGenerator& scan, Ast* root, Context& ctx)
{
    
    if (root == nullptr)
    {
        cout << "Nullptr in ScannerGenerator::handle_build_nfa" << endl;
        exit(1);
    }

    if (root->get_kind() < AstType::AstMinimum ||
        root->get_kind() > AstType::AstMaximum)
    {
        handle_error(scan, root, ctx);
    }

    if ((scan.debug_flags & DebugFlags::DebugAstHandlers) != 0)
    {
        cout << "ScannerGenerator "
             << scan.prsi.get_grammar_kind_string(root->get_kind()) << ": " 
             << build_nfa_handler_name[root->get_kind()] << endl;
    }

    build_nfa_handler[root->get_kind()](scan, root, ctx);

}

//
//  handle_error                                                     
//  ------------                                                     
//                                                                   
//  This should never be called. It means there is a path we haven't 
//  accomodated. It's not a user error, it's a logic error.          
//

void ScannerGenerator::handle_error(ScannerGenerator& scan,
                                    Ast* root,
                                    Context& ctx)
{
    cout << "No ScannerGenerator::build_nfa handler for Ast!" << endl << endl;
    scan.prsi.dump_grammar_ast(root);
    exit(1);
}

//
//  handle_regex                                                      
//  ------------                                                      
//                                                                    
//  The root regex node doesn't do anything. Because of macros it can 
//  appear pretty much anywhere.                                      
//

void ScannerGenerator::handle_regex(ScannerGenerator& scan,
                                    Ast* root,
                                    Context& ctx)
{

    for (int i = 0; i < root->get_num_children(); i++)
    {
        handle_build_nfa(scan, root->get_child(i), ctx);
        ctx.start_state = ctx.final_state;
    }

}

//
//  handle_regex_or                     
//  ---------------                     
//                                      
//  Generate the NFA for an or clause. 
//

void ScannerGenerator::handle_regex_or(ScannerGenerator& scan,
                                       Ast* root,
                                       Context& ctx)
{

    ctx.final_state = scan.get_new_state();

    for (int i = 0; i < root->get_num_children(); i++)
    {

        Context cctx;
        cctx.start_state = scan.get_new_state();
        cctx.final_state = cctx.start_state;
        ctx.start_state->e_moves.insert(cctx.start_state);

        handle_build_nfa(scan, root->get_child(i), cctx);

        cctx.final_state->e_moves.insert(ctx.final_state);

    }

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_list                                                   
//  -----------------                                                   
//                                                                      
//  For a list of clauses we generate the NFA for each clause and link 
//  the final to start states.                                          
//

void ScannerGenerator::handle_regex_list(ScannerGenerator& scan,
                                         Ast* root,
                                         Context& ctx)
{

    for (int i = 0; i < root->get_num_children(); i++)
    {
        handle_build_nfa(scan, root->get_child(i), ctx);
        ctx.start_state = ctx.final_state;
    }

}

//
//  handle_regex_optional                                               
//  ---------------------                                               
//                                                                      
//  Optional (denoted t?). We generate an NFA to recognize zero or one 
//  instances of a pattern.                                             
//

void ScannerGenerator::handle_regex_optional(ScannerGenerator& scan,
                                             Ast* root,
                                             Context& ctx)
{

    Context cctx;
    cctx.start_state = scan.get_new_state();
    cctx.final_state = cctx.start_state;
    ctx.start_state->e_moves.insert(cctx.start_state);

    handle_build_nfa(scan, root->get_child(0), cctx);

    ctx.final_state = scan.get_new_state();
    cctx.final_state->e_moves.insert(ctx.final_state);

    ctx.start_state->e_moves.insert(ctx.final_state);
    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_zero_closure                                             
//  -------------------------                                             
//                                                                        
//  Kleene closure (denoted t*). We generate an NFA to recognize zero or 
//  more instances of a pattern.                                          
//

void ScannerGenerator::handle_regex_zero_closure(ScannerGenerator& scan,
                                                 Ast* root,
                                                 Context& ctx)
{

    Context cctx;
    cctx.start_state = scan.get_new_state();
    cctx.final_state = cctx.start_state;
    ctx.start_state->e_moves.insert(cctx.start_state);
 
    handle_build_nfa(scan, root->get_child(0), cctx);

    ctx.final_state = scan.get_new_state();
    cctx.final_state->e_moves.insert(ctx.final_state);
    cctx.final_state->e_moves.insert(ctx.start_state);

    ctx.start_state->e_moves.insert(ctx.final_state);

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_one_closure                                             
//  ------------------------                                             
//                                                                        
//  Kleene closure (denoted t+). We generate an NFA to recognize one or 
//  more instances of a pattern.                                          
//

void ScannerGenerator::handle_regex_one_closure(ScannerGenerator& scan,
                                                Ast* root,
                                                Context& ctx)
{

    Context cctx;
    cctx.start_state = scan.get_new_state();
    cctx.final_state = cctx.start_state;
    ctx.start_state->e_moves.insert(cctx.start_state);
 
    handle_build_nfa(scan, root->get_child(0), cctx);

    ctx.final_state = scan.get_new_state();
    cctx.final_state->e_moves.insert(ctx.final_state);
    cctx.final_state->e_moves.insert(ctx.start_state);

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_wildcard
//  ---------------------
//  
//  The special character set 'Wildcard' denoted by '.'.
//  

void ScannerGenerator::handle_regex_wildcard(ScannerGenerator& scan,
                                             Ast* root,
                                             Context& ctx)
{

    static char32_t ranges[][2] = { {0x00000000, 0xffffffff} };

    ctx.final_state = scan.get_new_state();

    for (int i = 0; i < LENGTH(ranges); i++)
    {
        ctx.start_state->transitions.insert(
            Transition(ranges[i][0], ranges[i][1], ctx.final_state));
    }

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_whitespace
//  -----------------------
//  
//  The special character set 'Whitespace' denoted by '\s'.
//  

void ScannerGenerator::handle_regex_whitespace(ScannerGenerator& scan,
                                               Ast* root,
                                               Context& ctx)
{

    static char32_t ranges[][2] = { {'\t', '\r'}, {' ', ' '} };

    ctx.final_state = scan.get_new_state();

    for (int i = 0; i < LENGTH(ranges); i++)
    {
        ctx.start_state->transitions.insert(
            Transition(ranges[i][0], ranges[i][1], ctx.final_state));
    }

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_not_whitespace
//  ---------------------------
//  
//  The special character set 'NotWhitespace' denoted by '\S'.
//  

void ScannerGenerator::handle_regex_not_whitespace(ScannerGenerator& scan,
                                                   Ast* root,
                                                   Context& ctx)
{

    static char32_t ranges[][2] =
    {
        {0x00000000, 0x00000008},   {0x0000000e, 0x0000001f},   {'!', 0xffffffff}
    };

    ctx.final_state = scan.get_new_state();

    for (int i = 0; i < LENGTH(ranges); i++)
    {
        ctx.start_state->transitions.insert(
            Transition(ranges[i][0], ranges[i][1], ctx.final_state));
    }

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_digits
//  -------------------
//  
//  The special character set 'Digits' denoted by '\d'.
//  

void ScannerGenerator::handle_regex_digits(ScannerGenerator& scan,
                                           Ast* root,
                                           Context& ctx)
{

    static char32_t ranges[][2] = { {'0', '9'} };

    ctx.final_state = scan.get_new_state();

    for (int i = 0; i < LENGTH(ranges); i++)
    {
        ctx.start_state->transitions.insert(
            Transition(ranges[i][0], ranges[i][1], ctx.final_state));
    }

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_not_digits
//  -----------------------
//  
//  The special character set 'NotDigits' denoted by '\D'.
//  

void ScannerGenerator::handle_regex_not_digits(ScannerGenerator& scan,
                                               Ast* root,
                                               Context& ctx)
{

    static char32_t ranges[][2] = { {0x00000000, '/'}, {':', 0xffffffff} };

    ctx.final_state = scan.get_new_state();

    for (int i = 0; i < LENGTH(ranges); i++)
    {
        ctx.start_state->transitions.insert(
            Transition(ranges[i][0], ranges[i][1], ctx.final_state));
    }

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_char                                                  
//  -----------------                                                  
//                                                                     
//  A standalone character in a regular expression. Create a new final 
//  state and a transition to it.                                      
//

void ScannerGenerator::handle_regex_char(ScannerGenerator& scan,
                                         Ast* root,
                                         Context& ctx)
{

    char32_t character = root->get_lexeme()[0];
    ctx.final_state = scan.get_new_state();

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;  

}

//
//  handle_regex_escape
//  -------------------
//  
//  The escape character 'RegexEscape' denoted by '\\'.
//  

void ScannerGenerator::handle_regex_escape(ScannerGenerator& scan,
                                           Ast* root,
                                           Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '\\';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_alt_newline
//  ------------------------
//  
//  The escape character 'RegexAltNewline' denoted by '$'.
//  

void ScannerGenerator::handle_regex_alt_newline(ScannerGenerator& scan,
                                                Ast* root,
                                                Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '\n';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_newline
//  --------------------
//  
//  The escape character 'RegexNewline' denoted by '\n'.
//  

void ScannerGenerator::handle_regex_newline(ScannerGenerator& scan,
                                            Ast* root,
                                            Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '\n';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_cr
//  ---------------
//  
//  The escape character 'RegexCr' denoted by '\r'.
//  

void ScannerGenerator::handle_regex_cr(ScannerGenerator& scan,
                                       Ast* root,
                                       Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '\r';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_v_bar
//  ------------------
//  
//  The escape character 'RegexVBar' denoted by '\|'.
//  

void ScannerGenerator::handle_regex_v_bar(ScannerGenerator& scan,
                                          Ast* root,
                                          Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '|';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_star
//  -----------------
//  
//  The escape character 'RegexStar' denoted by '\*'.
//  

void ScannerGenerator::handle_regex_star(ScannerGenerator& scan,
                                         Ast* root,
                                         Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '*';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_plus
//  -----------------
//  
//  The escape character 'RegexPlus' denoted by '\+'.
//  

void ScannerGenerator::handle_regex_plus(ScannerGenerator& scan,
                                         Ast* root,
                                         Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '+';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_question
//  ---------------------
//  
//  The escape character 'RegexQuestion' denoted by '\?'.
//  

void ScannerGenerator::handle_regex_question(ScannerGenerator& scan,
                                             Ast* root,
                                             Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '?';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_period
//  -------------------
//  
//  The escape character 'RegexPeriod' denoted by '\.'.
//  

void ScannerGenerator::handle_regex_period(ScannerGenerator& scan,
                                           Ast* root,
                                           Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '.';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_dollar
//  -------------------
//  
//  The escape character 'RegexDollar' denoted by '\$'.
//  

void ScannerGenerator::handle_regex_dollar(ScannerGenerator& scan,
                                           Ast* root,
                                           Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '$';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_space
//  ------------------
//  
//  The escape character 'RegexSpace' denoted by '\b'.
//  

void ScannerGenerator::handle_regex_space(ScannerGenerator& scan,
                                          Ast* root,
                                          Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = ' ';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_left_paren
//  -----------------------
//  
//  The escape character 'RegexLeftParen' denoted by '\('.
//  

void ScannerGenerator::handle_regex_left_paren(ScannerGenerator& scan,
                                               Ast* root,
                                               Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '(';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_right_paren
//  ------------------------
//  
//  The escape character 'RegexRightParen' denoted by '\)'.
//  

void ScannerGenerator::handle_regex_right_paren(ScannerGenerator& scan,
                                                Ast* root,
                                                Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = ')';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_left_bracket
//  -------------------------
//  
//  The escape character 'RegexLeftBracket' denoted by '\['.
//  

void ScannerGenerator::handle_regex_left_bracket(ScannerGenerator& scan,
                                                 Ast* root,
                                                 Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '[';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_right_bracket
//  --------------------------
//  
//  The escape character 'RegexRightBracket' denoted by '\]'.
//  

void ScannerGenerator::handle_regex_right_bracket(ScannerGenerator& scan,
                                                  Ast* root,
                                                  Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = ']';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_left_brace
//  -----------------------
//  
//  The escape character 'RegexLeftBrace' denoted by '\{'.
//  

void ScannerGenerator::handle_regex_left_brace(ScannerGenerator& scan,
                                               Ast* root,
                                               Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '{';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_regex_right_brace
//  ------------------------
//  
//  The escape character 'RegexRightBrace' denoted by '\}'.
//  

void ScannerGenerator::handle_regex_right_brace(ScannerGenerator& scan,
                                                Ast* root,
                                                Context& ctx)
{

    ctx.final_state = scan.get_new_state();
    char32_t character = '}';

    ctx.start_state->transitions.insert(
        Transition(character, character, ctx.final_state));

    ctx.start_state = ctx.final_state;

}

//
//  handle_charset
//  --------------
//                                                  
//  Generate code for character sets.
//

void ScannerGenerator::handle_charset(ScannerGenerator& scan,
                                      Ast* root,
                                      Context& ctx)
{

    Context cctx;
    cctx.start_state = scan.get_new_state();
    cctx.final_state = scan.get_new_state();
    ctx.start_state->e_moves.insert(cctx.start_state);
       
    for (int i = 0; i < root->get_num_children(); i++)
    {
        handle_build_nfa(scan, root->get_child(i), cctx);
    }

    ctx.final_state = cctx.final_state;
    ctx.start_state = ctx.final_state;

}

//
//  handle_charset_invert
//  ---------------------
//                                                  
//  Generate code for inverted character sets.
//

void ScannerGenerator::handle_charset_invert(ScannerGenerator& scan,
                                             Ast* root,
                                             Context& ctx)
{

    Context cctx;
    cctx.start_state = scan.get_new_state();
    cctx.final_state = scan.get_new_state();
    ctx.start_state->e_moves.insert(cctx.start_state);
       
    for (int i = 0; i < root->get_num_children(); i++)
    {
        handle_build_nfa(scan, root->get_child(i), cctx);
    }

    set<Transition> new_transitions;
    int64_t last_character = 0;

    for (auto transition: cctx.start_state->transitions)
    {
    
        int64_t range_start = transition.range_start;
        int64_t range_end = transition.range_end;

        if (range_start > last_character)
        {
            new_transitions.insert(Transition(static_cast<char32_t>(last_character),
                                              static_cast<char32_t>(range_start - 1),
                                              cctx.final_state));
        }                             

        if (range_end >= last_character)
        {
            last_character = range_end + 1;
        }

    }

    if (last_character <= numeric_limits<char32_t>::max())
    {
        new_transitions.insert(Transition(static_cast<char32_t>(last_character),
                                          numeric_limits<char32_t>::max(),
                                          cctx.final_state));
    }

    cctx.start_state->transitions = new_transitions;

    ctx.final_state = cctx.final_state;
    ctx.start_state = ctx.final_state;

}

//
//  handle_charset_range                            
//  --------------------                            
//                                                  
//  Generate code for ranges within character sets. 
//

void ScannerGenerator::handle_charset_range(ScannerGenerator& scan,
                                            Ast* root,
                                            Context& ctx)
{

    handle_build_nfa(scan, root->get_child(0), ctx);
    char32_t range_start = ctx.character;
    char32_t range_end = ctx.character;

    if (root->get_num_children() > 1)
    {
        handle_build_nfa(scan, root->get_child(1), ctx);
        range_end = ctx.character;
    }

    if (range_end < range_start)
    {
        scan.errh.add_error(ErrorType::ErrorCharacterRange,
                            root->get_location(),
                            "Invalid character range");
        return;
    }

    ctx.start_state->transitions.insert(
        Transition(range_start, range_end, ctx.final_state));

}

//
//  handle_charset_whitespace
//  -------------------------
//  
//  The special character set 'Whitespace' denoted by '\s'.
//  

void ScannerGenerator::handle_charset_whitespace(ScannerGenerator& scan,
                                                 Ast* root,
                                                 Context& ctx)
{

    static char32_t ranges[][2] = { {'\t', '\r'}, {' ', ' '} };

    for (int i = 0; i < LENGTH(ranges); i++)
    {
        ctx.start_state->transitions.insert(
            Transition(ranges[i][0], ranges[i][1], ctx.final_state));
    }

}

//
//  handle_charset_not_whitespace
//  -----------------------------
//  
//  The special character set 'NotWhitespace' denoted by '\S'.
//  

void ScannerGenerator::handle_charset_not_whitespace(ScannerGenerator& scan,
                                                     Ast* root,
                                                     Context& ctx)
{

    static char32_t ranges[][2] =
    {
        {0x00000000, 0x00000008},   {0x0000000e, 0x0000001f},   {'!', 0xffffffff}
    };

    for (int i = 0; i < LENGTH(ranges); i++)
    {
        ctx.start_state->transitions.insert(
            Transition(ranges[i][0], ranges[i][1], ctx.final_state));
    }

}

//
//  handle_charset_digits
//  ---------------------
//  
//  The special character set 'Digits' denoted by '\d'.
//  

void ScannerGenerator::handle_charset_digits(ScannerGenerator& scan,
                                             Ast* root,
                                             Context& ctx)
{

    static char32_t ranges[][2] = { {'0', '9'} };

    for (int i = 0; i < LENGTH(ranges); i++)
    {
        ctx.start_state->transitions.insert(
            Transition(ranges[i][0], ranges[i][1], ctx.final_state));
    }

}

//
//  handle_charset_not_digits
//  -------------------------
//  
//  The special character set 'NotDigits' denoted by '\D'.
//  

void ScannerGenerator::handle_charset_not_digits(ScannerGenerator& scan,
                                                 Ast* root,
                                                 Context& ctx)
{

    static char32_t ranges[][2] = { {0x00000000, '/'}, {':', 0xffffffff} };

    for (int i = 0; i < LENGTH(ranges); i++)
    {
        ctx.start_state->transitions.insert(
            Transition(ranges[i][0], ranges[i][1], ctx.final_state));
    }

}

//
//  handle_charset_char
//  -------------------
//                                                  
//  Generate code for single characters in character sets.
//

void ScannerGenerator::handle_charset_char(ScannerGenerator& scan,
                                           Ast* root,
                                           Context& ctx)
{
    ctx.character = root->get_lexeme()[0];
}

//
//  handle_charset_escape
//  ---------------------
//  
//  The escape character 'CharsetEscape' denoted by '\\'.
//  

void ScannerGenerator::handle_charset_escape(ScannerGenerator& scan,
                                             Ast* root,
                                             Context& ctx)
{
    ctx.character = '\\';
}

//
//  handle_charset_alt_newline
//  --------------------------
//  
//  The escape character 'CharsetAltNewline' denoted by '$'.
//  

void ScannerGenerator::handle_charset_alt_newline(ScannerGenerator& scan,
                                                  Ast* root,
                                                  Context& ctx)
{
    ctx.character = '\n';
}

//
//  handle_charset_newline
//  ----------------------
//  
//  The escape character 'CharsetNewline' denoted by '\n'.
//  

void ScannerGenerator::handle_charset_newline(ScannerGenerator& scan,
                                              Ast* root,
                                              Context& ctx)
{
    ctx.character = '\n';
}

//
//  handle_charset_cr
//  -----------------
//  
//  The escape character 'CharsetCr' denoted by '\r'.
//  

void ScannerGenerator::handle_charset_cr(ScannerGenerator& scan,
                                         Ast* root,
                                         Context& ctx)
{
    ctx.character = '\r';
}

//
//  handle_charset_caret
//  --------------------
//  
//  The escape character 'CharsetCaret' denoted by '\^'.
//  

void ScannerGenerator::handle_charset_caret(ScannerGenerator& scan,
                                            Ast* root,
                                            Context& ctx)
{
    ctx.character = '^';
}

//
//  handle_charset_dash
//  -------------------
//  
//  The escape character 'CharsetDash' denoted by '\-'.
//  

void ScannerGenerator::handle_charset_dash(ScannerGenerator& scan,
                                           Ast* root,
                                           Context& ctx)
{
    ctx.character = '-';
}

//
//  handle_charset_dollar
//  ---------------------
//  
//  The escape character 'CharsetDollar' denoted by '\$'.
//  

void ScannerGenerator::handle_charset_dollar(ScannerGenerator& scan,
                                             Ast* root,
                                             Context& ctx)
{
    ctx.character = '$';
}

//
//  handle_charset_left_bracket
//  ---------------------------
//  
//  The escape character 'CharsetLeftBracket' denoted by '\['.
//  

void ScannerGenerator::handle_charset_left_bracket(ScannerGenerator& scan,
                                                   Ast* root,
                                                   Context& ctx)
{
    ctx.character = '[';
}

//
//  handle_charset_right_bracket
//  ----------------------------
//  
//  The escape character 'CharsetRightBracket' denoted by '\]'.
//  

void ScannerGenerator::handle_charset_right_bracket(ScannerGenerator& scan,
                                                    Ast* root,
                                                    Context& ctx)
{
    ctx.character = ']';
}

//
//  nfa_to_dfa                                                         
//  ----------
//                                                                      
//  Convert the NFA with e-moves into a DFA. This is described well in 
//  Hopcraft and Ullman's automata book.                           
//

void ScannerGenerator::nfa_to_dfa()
{

    nfa_to_dfa_map.clear();
    dfa_to_nfa_map.clear();

    StateSet state_set;
    state_set.get().insert(nfa_start_state);
    find_e_closure(state_set);
    dfa_start_state = get_dfa_state(state_set);

    while (workpile.size() > 0)
    {
        State* state = workpile.front();
        workpile.pop();
        collapse_dfa_state(state);
    }

}

//
//  find_e_closure                                                 
//  --------------                                                 
//                                                                 
//  Add to a state set all other states reachable through e-moves. 
//

void ScannerGenerator::find_e_closure(StateSet& closure)
{

    vector<State*> additions(closure.get().begin(), closure.get().end());

    while (additions.size() > 0)
    {

        State* state = additions.back();
        additions.pop_back();

        for (auto next_state: state->e_moves)
        {
        
            if (closure.get().find(next_state) == closure.get().end())
            {
                closure.get().insert(next_state);
                additions.push_back(next_state);
            }

        }

    }

}

//
//  get_dfa_state                                
//  -------------                                
//                                               
//  Find the DFA state for a set of NFA states. 
//

ScannerGenerator::State* ScannerGenerator::get_dfa_state(StateSet& state_set)
{

    if (nfa_to_dfa_map.find(state_set) != nfa_to_dfa_map.end())
    {
        return nfa_to_dfa_map[state_set];
    }

    State* state = get_new_state();
    nfa_to_dfa_map[state_set] = state;
    dfa_to_nfa_map[state] = state_set;
    workpile.push(state);

    return state;

}

//
//  collapse_dfa_state                                                     
//  ------------------                                                     
//                                                                         
//  Collapse the transitions for a set of NFA states into DFA transitions. 
//

void ScannerGenerator::collapse_dfa_state(State* state)
{

    //
    //  Gather up a merged transition set. 
    //

    set<Transition> transitions;

    for (auto nfa_state: dfa_to_nfa_map[state].get())
    {

        for (auto transition: nfa_state->transitions)
        {
            transitions.insert(transition);
        }

        for (auto accept_action: nfa_state->accept_actions)
        {
            state->accept_actions.insert(accept_action);
        }

    }

    multimap<int, AcceptAction> conflict_map;
    set<int> key_set;
    for (auto accept_action: state->accept_actions)
    {
        conflict_map.insert(make_pair(accept_action.token->precedence, accept_action));
        key_set.insert(accept_action.token->precedence);
    }

    for (auto key: key_set)
    {

        if (conflict_map.count(key) > 1)
        {

            ostringstream ost;

            ost << "Token regex conflict ";
            if (conflict_map.count(key) > 2)
            {
                ost << "among ";
            }
            else
            {
                ost << "between ";
            }

            int count = conflict_map.count(key);
            bool first = true;

            auto range = conflict_map.equal_range(key);
            for (auto it = range.first; it != range.second; it++)
            {

                if (first)
                {
                    first = false;
                }
                else if (--count > 1)
                {
                    ost << ", ";
                }
                else
                {
                    ost << " and ";
                }
                
                ost << it->second.token->symbol_name;

            }

            errh.add_error(ErrorType::ErrorRegexConflict,
                           -1,
                           ost.str());

        }

    }

    //
    //  We're going to proceed by ranges. We keep a heap of transitions in 
    //  the current range.                                                 
    //

    auto compare = [](const Transition& left, const Transition& right) -> bool
    {

        if (left.range_end > right.range_end)
        {
            return true;
        }
        else if (left.range_end < right.range_end)
        {
            return false;
        }

        if (left.range_start > right.range_start)
        {
            return true;
        }
        else if (left.range_start < right.range_start)
        {
            return false;
        }

        return left.target_state > right.target_state;

    };

    priority_queue<Transition, vector<Transition>, decltype(compare)> included_heap(compare);
    set<Transition> included_set;
        
    //
    //  Build up the ranges. We traverse the set of transitions in `start' 
    //  order. For each we see create a range preceeding it. As we pass    
    //  the starting point insert the range into a heap by 'end' order. As 
    //  we pop things off the end create a range for the range we          
    //  complete.                                                          
    //

    int64_t last_character = -1;
    auto next_transition = transitions.begin();

    while (next_transition != transitions.end() || !included_heap.empty())
    {

        if (included_heap.empty() ||
            (next_transition != transitions.end() &&
             next_transition->range_start <= included_heap.top().range_end))
        {

            int64_t next_character = next_transition->range_start;

            if (!included_heap.empty())
            {

                StateSet state_set;
                for (auto transition: included_set)
                {
                    state_set.get().insert(transition.target_state);
                }

                find_e_closure(state_set);
                State* next_state = get_dfa_state(state_set);

                state->transitions.insert(Transition(
                    static_cast<char32_t>(last_character + 1),
                    static_cast<char32_t>(next_character - 1),
                    next_state));

            }

            last_character =
                static_cast<int64_t>(next_character) - 1;

        }
        else if (next_transition == transitions.end() ||
                 (!included_heap.empty() &&
                  next_transition->range_start > included_heap.top().range_end))
        {

            int64_t next_character = included_heap.top().range_end;

            if (!included_heap.empty())
            {

                StateSet state_set;
                for (auto transition: included_set)
                {
                    state_set.get().insert(transition.target_state);
                }

                find_e_closure(state_set);
                State* next_state = get_dfa_state(state_set);

                state->transitions.insert(Transition(
                    static_cast<char32_t>(last_character + 1),
                    static_cast<char32_t>(next_character),
                    next_state));

            }

            last_character =
                static_cast<int64_t>(next_character);

        }

        while (next_transition != transitions.end() &&
               next_transition->range_start <= last_character + 1)
        {
            included_heap.push(*next_transition);
            included_set.insert(*next_transition);
            next_transition++;
        }
     
        while (!included_heap.empty() &&
               included_heap.top().range_end <= last_character)
        {
            included_set.erase(included_heap.top());
            included_heap.pop();
        }

    }

}

//
//  minimize_dfa                                                         
//  ------------                                                         
//                                                                       
//  Rebuild the dfa into an equivalent one with a minimal number of      
//  states. I doubt if this will actually improve the automaton much but 
//  it seems like we should try.                                         
//                                                                       
//  The algorithm is again described in Hopcraft and Ullman.        
//

void ScannerGenerator::minimize_dfa()
{

    //
    //  SimilarKey                                                         
    //  ----------                                                         
    //                                                                     
    //  This is a map key facilitating an efficiency gimmick. On each pass 
    //  of this procedure we need to determine if each pair of states is   
    //  distinguishable. We can cut this down by only comparing states     
    //  that are similar, where two states are definitely similar if they  
    //  are indistinguishable. Here we define similar as having the same     
    //  accepted tokens and the same transition domain.                    
    //

    class SimilarKey
    {

    public:    

        explicit SimilarKey(State* state) : state(state) {}

        bool operator<(const SimilarKey& rhs) const
        {

            State* left = state;
            State* right = rhs.state;

            auto left_accept = left->accept_actions.begin();
            auto right_accept = right->accept_actions.begin();

            while (left_accept != left->accept_actions.end() ||
                   right_accept != right->accept_actions.end())
            {

                if (left_accept == left->accept_actions.end())
                {
                    return true;
                }
                else if (right_accept == right->accept_actions.end())
                {
                    return false;
                }

                if (left_accept->token < right_accept->token)
                {
                    return true;
                }
                else if (left_accept->token > right_accept->token)
                {
                    return false;
                }
                 
                left_accept++;
                right_accept++;

            } 

            auto left_transition = left->transitions.begin();
            auto right_transition = right->transitions.begin();

            while (left_transition != left->transitions.end() ||
                   right_transition != right->transitions.end())
            {

                if (left_transition == left->transitions.end())
                {
                    return true;
                }
                else if (right_transition == right->transitions.end())
                {
                    return false;
                }

                if (left_transition->range_start < right_transition->range_start)
                {
                    return true;
                }
                else if (left_transition->range_start > right_transition->range_start)
                {
                    return false;
                }
                 
                if (left_transition->range_end < right_transition->range_end)
                {
                    return true;
                }
                else if (left_transition->range_end > right_transition->range_end)
                {
                    return false;
                }
                 
                left_transition++;
                right_transition++;

            } 

            return false;

        }

        bool operator>(const SimilarKey& rhs) const 
        {
            return rhs < *this;
        } 

        bool operator==(const SimilarKey& rhs)
        {

            State* left = state;
            State* right = rhs.state;

            auto left_accept = left->accept_actions.begin();
            auto right_accept = right->accept_actions.begin();

            while (left_accept != left->accept_actions.end() ||
                   right_accept != right->accept_actions.end())
            {

                if (left_accept == left->accept_actions.end() ||
                    right_accept == right->accept_actions.end() ||
                    left_accept->token != right_accept->token)
                {
                    return false;
                }
                 
                left_accept++;
                right_accept++;

            } 

            auto left_transition = left->transitions.begin();
            auto right_transition = right->transitions.begin();

            while (left_transition != left->transitions.end() ||
                   right_transition != right->transitions.end())
            {

                if (left_transition == left->transitions.end() ||
                    right_transition == right->transitions.end() ||
                    left_transition->range_start != right_transition->range_start ||
                    left_transition->range_end != right_transition->range_end)
                {
                    return false;
                }
                 
                left_transition++;
                right_transition++;

            } 

            return true;

        }   

    private:

        State* state;

    };

    //
    //  Common maps used throughout. 
    //

    vector<set<State*>> bucket_list;
    map<State*, size_t> bucket_map;
    map<SimilarKey, vector<State*>> similar_states;
    set<State*> rebuilt_states;

    //
    //  initialize_state                                                
    //  ----------------                                                
    //                                                                  
    //  Insert a state and all reachable states into our initial bucket 
    //  list and similar state map.                                     
    //

    function<void(State*)> initialize_state = [&](State* state) -> void
    {

        if (bucket_map.find(state) != bucket_map.end())
        {
            return;
        }
 
        set<State*> bucket;
        bucket.insert(state);
        bucket_map[state] = bucket_list.size();
        bucket_list.push_back(bucket); 

        SimilarKey key(state);

        if (similar_states.find(key) == similar_states.end())
        {
            similar_states[key] = vector<State*>();
        }

        similar_states[key].push_back(state);

        for (auto transition: state->transitions)
        {
            initialize_state(transition.target_state);
        }
    
    };

    //
    //  merge_buckets                                  
    //  -------------                                  
    //                                                 
    //  Merge two buckets of indistinguishable states. 
    //

    function<void(int, int)> merge_buckets = [&](int left, int right) -> void
    {

        for (auto state: bucket_list[right])
        {
            bucket_list[left].insert(state);
            bucket_map[state] = left;
        }
        
        bucket_list[right].clear();

    };

    //
    //  identical_asts                       
    //  --------------                       
    //                                       
    //  Test whether two asts are identical. 
    //

    function<bool(Ast*, Ast*)> identical_asts = [&](Ast* left, Ast* right) -> bool
    {

        if (left == right)
        {
            return true;
        }

        if (left == nullptr || right == nullptr)
        {
            return false;
        }

        if (left->get_kind() != right->get_kind() ||
            left->get_lexeme() != right->get_lexeme() ||
            left->get_num_children() != right->get_num_children())
        {
            return false;
        }
        
        for (int i = 0; i < left->get_num_children(); i++)
        {

            if (!identical_asts(left->get_child(i), right->get_child(i)))
            {
                return false;
            }

        }

        return true;

    };

    //
    //  indistinguishable                              
    //  -----------------                              
    //                                                 
    //  Test whether two states are indistinguishable. 
    //

    function<bool(State*, State*)> indistinguishable = [&](State* left, State* right) -> bool
    {

        auto left_accept = left->accept_actions.begin();
        auto right_accept = right->accept_actions.begin();

        while (left_accept != left->accept_actions.end() ||
               right_accept != right->accept_actions.end())
        {

            if (left_accept == left->accept_actions.end() ||
                right_accept == right->accept_actions.end() ||
                left_accept->token != right_accept->token ||
                !identical_asts(left_accept->guard_ast, right_accept->guard_ast))
            {
                return false;
            }

            left_accept++;
            right_accept++;

        } 

        auto left_transition = left->transitions.begin();
        auto right_transition = right->transitions.begin();

        while (left_transition != left->transitions.end() ||
               right_transition != right->transitions.end())
        {

            if (left_transition == left->transitions.end() ||
                right_transition == right->transitions.end() ||
                left_transition->range_start != right_transition->range_start ||
                left_transition->range_end != right_transition->range_end ||
                bucket_map[left_transition->target_state] != bucket_map[right_transition->target_state])
            {
                return false;
            }

            left_transition++;
            right_transition++;

        } 

        return true;

    };
    
    //
    //  rebuild_state                                                      
    //  -------------                                                      
    //                                                                     
    //  Rebuild the transitions of a state and reachable states, replacing 
    //  all target states with those of the minimized automaton.           
    //

    function<void(State*)> rebuild_state = [&](State* state) -> void
    {

        if (rebuilt_states.find(state) != rebuilt_states.end())
        {
            return;
        }

        rebuilt_states.insert(state);

        decltype(state->transitions) new_transitions;
        for (auto transition: state->transitions)
        {
            new_transitions.insert(Transition(transition.range_start,
                                              transition.range_end,
                                              *(bucket_list[bucket_map[transition.target_state]].begin())));
        }

        state->transitions = new_transitions;

        for (auto transition: state->transitions)
        {
            rebuild_state(transition.target_state);
        }
    
    };

    //
    //  minimize_dfa
    //  ------------
    //                                 
    //  The function body begins here. 
    //

    //
    //  Setup our similar states map and initialize the buckets of 
    //  indistinguishable states.                                  
    //

    initialize_state(dfa_start_state);

    //
    //  Use a fixpoint algorithm to merge indistinguishable states until 
    //  we can't find any more to merge.                                 
    //

    bool any_changes = true;
    while (any_changes)
    {

        any_changes = false;
        for (auto sp: similar_states)
        {

            vector<State*> state_list = sp.second;
            for (int i = 0; i < state_list.size(); i++)
            {

                for (int j = i + 1; j < state_list.size(); j++)
                {

                    if (bucket_map[state_list[i]] != bucket_map[state_list[j]] &&
                        indistinguishable(*(bucket_list[bucket_map[state_list[i]]].begin()),
                                          *(bucket_list[bucket_map[state_list[j]]].begin())))
                    {
                        merge_buckets(bucket_map[state_list[i]], bucket_map[state_list[j]]);
                        any_changes = true;
                    }

                }

            }

        }

    }                          

    //
    //  All the states are arranged in buckets. Rebuild the transitions to 
    //  use the new states.                                                
    //

    rebuild_state(dfa_start_state);

}

//
//  create_vmcode                                    
//  -------------                                    
//                                                   
//  Create the virtual machine code for the scanner. 
//

void ScannerGenerator::create_vmcode()
{

    map<State*, ICodeLabel*> state_label_map;
    set<State*> state_coded_set;
    map<Symbol*, ICodeLabel*> symbol_action_map;

    //
    //  state_label                                
    //  -----------                                
    //                                             
    //  Create a code label for a state.
    //

    function<ICodeLabel*(State*)> state_label = [&](State* state) -> ICodeLabel*
    {

        if (state_label_map.find(state) == state_label_map.end())
        {
            state_label_map[state] = code.get_label();
        }

        return state_label_map[state];

    };

    //
    //  encode_state                             
    //  ------------                             
    //                                           
    //  Generate the VM code for a single state. 
    //

    function<void(State*)> encode_state = [&](State* state) -> void
    {

        if (state_coded_set.find(state) != state_coded_set.end())
        {
            return;
        }
 
        state_coded_set.insert(state);
        code.emit(OpcodeType::OpcodeLabel, -1, ICodeOperand(state_label(state)));

        //
        //  Encode guard conditions for all accepted tokens. 
        //

        map<int, AcceptAction> accept_actions;
        for (auto accept_action: state->accept_actions)
        {
            accept_actions.insert(make_pair(accept_action.token->precedence, accept_action));
        }
        
        for (auto it = accept_actions.rbegin(); it != accept_actions.rend(); it++)
        {
          
            Symbol* token = it->second.token;
            Ast* guard_ast = it->second.guard_ast;

            ICodeLabel* true_label = nullptr;
            ICodeLabel* false_label = nullptr;

            if (guard_ast != nullptr && guard_ast->get_kind() != AstType::AstNull)
            {

                true_label = code.get_label();
                false_label = code.get_label();

                actg.generate_condition(guard_ast, true_label, false_label);
                code.emit(OpcodeType::OpcodeLabel, token->location,
                          ICodeOperand(true_label));

            }
                
            code.emit(OpcodeType::OpcodeScanAccept, token->location,
                      ICodeOperand(static_cast<int64_t>(it->second.token->symbol_num)),
                      ICodeOperand(symbol_action_map[token]));

            if (false_label == nullptr)
            {
                break;
            }

            code.emit(OpcodeType::OpcodeLabel, token->location,
                      ICodeOperand(false_label));
            
        }

        //
        //  Encode the character transitions. 
        //

        vector<ICodeOperand> operands;
        operands.push_back(ICodeOperand(state->transitions.size()));

        for (auto transition: state->transitions)
        {
            operands.push_back(ICodeOperand(transition.range_start));
            operands.push_back(ICodeOperand(transition.range_end));
            operands.push_back(ICodeOperand(state_label(transition.target_state)));
        }

        code.emit(OpcodeScanChar, -1, operands);

        //
        //  Process all the states reachable from this one. 
        //

        for (auto transition: state->transitions)
        {
            encode_state(transition.target_state);
        }
    
    };

    //
    //  create_vmcode
    //  -------------
    //                                 
    //  The function body begins here. 
    //

    scan_label = code.get_label("Scan");
    scan_label->is_extern = true;

    //
    //  Create an action label for each symbol. 
    //

    ICodeLabel* default_accept_label = code.get_label();
    ICodeLabel* default_ignore_label = code.get_label();

    for (auto mp: gram.symbol_map)
    {
    
        Symbol* token = mp.second;
        if (token->is_nonterminal)
        {
            continue;
        }

        if (token->action_ast == nullptr ||
            token->action_ast->get_kind() == AstType::AstNull)
        {

            if (token->is_ignored)
            {
                symbol_action_map[token] = default_ignore_label;
                continue;
            }

            if (!token->is_error)
            {
                symbol_action_map[token] = default_accept_label;
                continue;
            }

        }

        symbol_action_map[token] = code.get_label();

    }

    //
    //  Emit the scanner prolog and encode each state.
    //

    code.emit(OpcodeType::OpcodeLabel, -1, ICodeOperand(scan_label));
    code.emit(OpcodeType::OpcodeScanStart, -1);

    encode_state(dfa_start_state);

    //
    //  Default accept (no actions).
    //

    code.emit(OpcodeType::OpcodeLabel, -1, ICodeOperand(default_accept_label));

    code.emit(OpcodeType::OpcodeScanToken, -1);

    code.emit(OpcodeType::OpcodeAdd, -1,
              ICodeOperand(code.get_register("token_count", 0)),
              ICodeOperand(code.get_register("token_count", 0)),
              ICodeOperand(code.get_register("1", 1)));

    code.emit(OpcodeType::OpcodeReturn, -1);

    //
    //  Default ignore (no actions).
    //

    code.emit(OpcodeType::OpcodeLabel, -1, ICodeOperand(default_ignore_label));
    code.emit(OpcodeType::OpcodeBranch, -1, ICodeOperand(scan_label));

    //
    //  Generate accept actions for the other symbols. 
    //

    for (auto mp: symbol_action_map)
    {
          
        if (mp.second == default_accept_label || mp.second == default_ignore_label)
        {
            continue;
        }

        Symbol* token = mp.first;
        code.emit(OpcodeType::OpcodeLabel, token->location,
                  ICodeOperand(mp.second));

        if (token->action_ast != nullptr &&
            token->action_ast->get_kind() != AstType::AstNull)
        {
            actg.generate_action(token->action_ast);
        }

        if (token->is_ignored)
        {
            code.emit(OpcodeType::OpcodeBranch, token->location,
                      ICodeOperand(scan_label));
            continue;
        }

        if (token->is_error)
        {
            code.emit(OpcodeType::OpcodeScanError, token->location,
                      ICodeOperand(code.get_string(token->error_message)));
            code.emit(OpcodeType::OpcodeReturn, token->location);
            continue;
        }

        code.emit(OpcodeType::OpcodeScanToken, token->location);

        code.emit(OpcodeType::OpcodeAdd, token->location,
                  ICodeOperand(code.get_register("token_count", 0)),
                  ICodeOperand(code.get_register("token_count", 0)),
                  ICodeOperand(code.get_register("1", 1)));

        code.emit(OpcodeType::OpcodeReturn, token->location);

    }

}

//
//  save_parser_data                                                     
//  ----------------                                                     
//                                                                       
//  This function is called *after* the code generator stores vm code in 
//  the parser. At this point labels have been given addresses so it's   
//  safe to store labels in the parser.                                  
//

void ScannerGenerator::save_parser_data()
{
    prsd.scanner_pc = scan_label->pc;
}

//
//  dump_automaton
//  --------------
//                                                                  
//  Dump out an automaton (NFA or DFA) rooted at a provided state. 
//

void ScannerGenerator::dump_automaton(State* start_state, ostream& os, int indent) const
{

    map<State*, string> state_label_map;

    //
    //  character_label                           
    //  ---------------                           
    //                                            
    //  Create a printable label for a character. 
    //

    function<string(char32_t)> character_label = [&](char32_t character) -> string
    {

        switch (character)
        {

            case '\n': return "'\\n'";
            case '\r': return "'\\r'";
            case '\t': return "'\\t'";

            default:
            {

                ostringstream ost;

                if (character >= ' ' && character < 128)
                {
                    ost << "'" << static_cast<char>(character) << "'";    
                }
                else
                {
                    ost << setfill('0') << setw(8) << hex << character;
                }

                return ost.str();

            }
               
        }   

    };

    //
    //  transition_label                           
    //  ----------------                           
    //                                             
    //  Create a printable label for a transition. 
    //

    function<string(Transition&)> transition_label = [&](Transition& transition) -> string
    {

        ostringstream ost;

        ost << "[" << character_label(transition.range_start) << ", "
            << character_label(transition.range_end) << "]: "
            << state_label_map[transition.target_state];

        return ost.str();

    };

    //
    //  dump_state                                                       
    //  ----------                                                       
    //                                                                   
    //  Dump a single state on the console. This is the workhorse of the 
    //  state dumping mechanism.                                         
    //

    function<void(State*)> dump_state = [&](State* state) -> void
    {

        os << setw(indent) << setfill(' ') << "" << setw(0)
           << "State " << state_label_map[state] << endl 
           << setw(indent) << setfill(' ') << "" << setw(0) 
           << setw(gram.line_width - indent) << setfill('-') << ""
           << setw(0) << setfill(' ')
           << endl;
        
        if (state->accept_actions.size() > 0)
        {

            os << setw(indent) << setfill(' ') << "" << setw(0)
               << "Accepts:" << endl;

            os << setw(indent + 2) << setfill(' ') << "" << setw(0);
            int width = indent + 2;
            string comma = "";
            for (auto accept_action: state->accept_actions)
            {

                if (width + accept_action.token->symbol_name.length() + comma.length() >
                    gram.line_width)
                {
                    os << endl << setw(indent + 2) << setfill(' ') << "" << setw(0);
                    width = indent + 2;
                }

                os << comma << accept_action.token->symbol_name;
                width = width + comma.length() + accept_action.token->symbol_name.length();
                comma = ", ";

            }

            os << endl << endl;

        }
        
        if (state->e_moves.size() > 0)
        {

            os << setw(indent) << setfill(' ') << "" << setw(0)
               << "E-Moves:" << endl;

            os << setw(indent + 2) << setfill(' ') << "" << setw(0);
            int printed = 0;
            for (auto target: state->e_moves)
            {

                if (printed > 12)
                {
                    os << endl << setw(indent + 2) << setfill(' ') << "" << setw(0);
                    printed = 0;
                }
              
                os << right << setw(6) << state_label_map[target]
                   << left << setw(0);

                printed++;

            }

            os << endl << endl;

        }

        if (state->transitions.size() > 0)
        {

            os << setw(indent) << setfill(' ') << "" << setw(0)
               << "Transitions:" << endl;

            os << setw(indent + 2) << setfill(' ') << "" << setw(0);
            int printed = 0;
            for (auto transition: state->transitions)
            {

                if (printed > 2)
                {
                    os << endl << setw(indent + 2) << setfill(' ') << "" << setw(0);
                    printed = 0;
                }
              
                os << left << setw(28) << transition_label(transition) << setw(0);
                printed++;

            }

            os << endl << endl;

        }

    };

    //
    //  dump_automaton
    //  --------------
    //                                 
    //  The function body begins here. 
    //

    vector<State*> state_list;
    set<State*> state_listed_set;
    queue<State*> workpile;
    workpile.push(start_state);
    
    while (workpile.size() > 0)
    {

        State* state = workpile.front();
        workpile.pop();

        if (state_listed_set.find(state) != state_listed_set.end())
        {
            continue;
        }

        state_label_map[state] = to_string(state_listed_set.size());
        state_listed_set.insert(state);
        state_list.push_back(state);

        for (auto target_state: state->e_moves)
        {
            workpile.push(target_state);
        }

        for (auto transition: state->transitions)
        {
            workpile.push(transition.target_state);
        }
    
    }

    for (auto state: state_list)
    {
        dump_state(state);
    }

}

} // namespace hoshi


