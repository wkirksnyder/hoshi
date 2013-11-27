//
//  ActionGenerator                                                        
//  ---------------                                                        
//                                                                         
//  Generate intermediate code for token actions, reduce actions and guard 
//  conditions.                                                            
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
#include "Parser.H"
#include "CodeGenerator.H"
#include "ActionGenerator.H"

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

void (*ActionGenerator::statement_handler[])(ActionGenerator& actg,
                                             Ast* root,
                                             Context& ctx)
{
    handle_statement_error,             // Unknown
    handle_statement_error,             // Null
    handle_statement_error,             // Grammar
    handle_statement_error,             // OptionList
    handle_statement_error,             // TokenList
    handle_statement_error,             // RuleList
    handle_statement_error,             // Lookaheads
    handle_statement_error,             // ErrorRecovery
    handle_statement_error,             // Conflicts
    handle_statement_error,             // KeepWhitespace
    handle_statement_error,             // CaseSensitive
    handle_statement_error,             // TokenDeclaration
    handle_statement_error,             // TokenOptionList
    handle_statement_error,             // TokenTemplate
    handle_statement_error,             // TokenDescription
    handle_statement_error,             // TokenRegexList
    handle_statement_error,             // TokenRegex
    handle_statement_error,             // TokenPrecedence
    handle_statement_error,             // TokenAction
    handle_statement_error,             // TokenLexeme
    handle_statement_error,             // TokenIgnore
    handle_statement_error,             // TokenError
    handle_statement_error,             // Rule
    handle_statement_error,             // RuleRhsList
    handle_statement_error,             // RuleRhs
    handle_statement_error,             // Optional
    handle_statement_error,             // ZeroClosure
    handle_statement_error,             // OneClosure
    handle_statement_error,             // Group
    handle_statement_error,             // RulePrecedence
    handle_statement_error,             // RulePrecedenceList
    handle_statement_error,             // RulePrecedenceSpec
    handle_statement_error,             // RuleLeftAssoc
    handle_statement_error,             // RuleRightAssoc
    handle_statement_error,             // RuleOperatorList
    handle_statement_error,             // RuleOperatorSpec
    handle_statement_error,             // TerminalReference
    handle_statement_error,             // NonterminalReference
    handle_statement_error,             // Empty
    handle_statement_error,             // AstFormer
    handle_statement_error,             // AstItemList
    handle_statement_error,             // AstChild
    handle_statement_error,             // AstKind
    handle_statement_error,             // AstLocation
    handle_statement_error,             // AstLocationString
    handle_statement_error,             // AstLexeme
    handle_statement_error,             // AstLexemeString
    handle_statement_error,             // AstLocator
    handle_statement_error,             // AstDot
    handle_statement_error,             // AstSlice
    handle_statement_error,             // Token
    handle_statement_error,             // Options
    handle_statement_error,             // ReduceActions
    handle_statement_error,             // RegexString
    handle_statement_error,             // CharsetString
    handle_statement_error,             // MacroString
    handle_statement_error,             // Identifier
    handle_statement_error,             // Integer
    handle_statement_error,             // NegativeInteger
    handle_statement_error,             // String
    handle_statement_error,             // TripleString
    handle_statement_error,             // True
    handle_statement_error,             // False
    handle_statement_error,             // Regex
    handle_statement_error,             // RegexOr
    handle_statement_error,             // RegexList
    handle_statement_error,             // RegexOptional
    handle_statement_error,             // RegexZeroClosure
    handle_statement_error,             // RegexOneClosure
    handle_statement_error,             // RegexChar
    handle_statement_error,             // RegexWildcard
    handle_statement_error,             // RegexWhitespace
    handle_statement_error,             // RegexNotWhitespace
    handle_statement_error,             // RegexDigits
    handle_statement_error,             // RegexNotDigits
    handle_statement_error,             // RegexEscape
    handle_statement_error,             // RegexAltNewline
    handle_statement_error,             // RegexNewline
    handle_statement_error,             // RegexCr
    handle_statement_error,             // RegexVBar
    handle_statement_error,             // RegexStar
    handle_statement_error,             // RegexPlus
    handle_statement_error,             // RegexQuestion
    handle_statement_error,             // RegexPeriod
    handle_statement_error,             // RegexDollar
    handle_statement_error,             // RegexSpace
    handle_statement_error,             // RegexLeftParen
    handle_statement_error,             // RegexRightParen
    handle_statement_error,             // RegexLeftBracket
    handle_statement_error,             // RegexRightBracket
    handle_statement_error,             // RegexLeftBrace
    handle_statement_error,             // RegexRightBrace
    handle_statement_error,             // Charset
    handle_statement_error,             // CharsetInvert
    handle_statement_error,             // CharsetRange
    handle_statement_error,             // CharsetChar
    handle_statement_error,             // CharsetWhitespace
    handle_statement_error,             // CharsetNotWhitespace
    handle_statement_error,             // CharsetDigits
    handle_statement_error,             // CharsetNotDigits
    handle_statement_error,             // CharsetEscape
    handle_statement_error,             // CharsetAltNewline
    handle_statement_error,             // CharsetNewline
    handle_statement_error,             // CharsetCr
    handle_statement_error,             // CharsetCaret
    handle_statement_error,             // CharsetDash
    handle_statement_error,             // CharsetDollar
    handle_statement_error,             // CharsetLeftBracket
    handle_statement_error,             // CharsetRightBracket
    handle_statement_statement_list,    // ActionStatementList
    handle_statement_assign,            // ActionAssign
    handle_statement_error,             // ActionEqual
    handle_statement_error,             // ActionNotEqual
    handle_statement_error,             // ActionLessThan
    handle_statement_error,             // ActionLessEqual
    handle_statement_error,             // ActionGreaterThan
    handle_statement_error,             // ActionGreaterEqual
    handle_statement_error,             // ActionAdd
    handle_statement_error,             // ActionSubtract
    handle_statement_error,             // ActionMultiply
    handle_statement_error,             // ActionDivide
    handle_statement_error,             // ActionUnaryMinus
    handle_statement_error,             // ActionAnd
    handle_statement_error,             // ActionOr
    handle_statement_error,             // ActionNot
    handle_statement_dump_stack,        // ActionDumpStack
    handle_statement_error              // ActionTokenCount
};

char *ActionGenerator::statement_handler_name[]
{
    "handle_statement_error",           // Unknown
    "handle_statement_error",           // Null
    "handle_statement_error",           // Grammar
    "handle_statement_error",           // OptionList
    "handle_statement_error",           // TokenList
    "handle_statement_error",           // RuleList
    "handle_statement_error",           // Lookaheads
    "handle_statement_error",           // ErrorRecovery
    "handle_statement_error",           // Conflicts
    "handle_statement_error",           // KeepWhitespace
    "handle_statement_error",           // CaseSensitive
    "handle_statement_error",           // TokenDeclaration
    "handle_statement_error",           // TokenOptionList
    "handle_statement_error",           // TokenTemplate
    "handle_statement_error",           // TokenDescription
    "handle_statement_error",           // TokenRegexList
    "handle_statement_error",           // TokenRegex
    "handle_statement_error",           // TokenPrecedence
    "handle_statement_error",           // TokenAction
    "handle_statement_error",           // TokenLexeme
    "handle_statement_error",           // TokenIgnore
    "handle_statement_error",           // TokenError
    "handle_statement_error",           // Rule
    "handle_statement_error",           // RuleRhsList
    "handle_statement_error",           // RuleRhs
    "handle_statement_error",           // Optional
    "handle_statement_error",           // ZeroClosure
    "handle_statement_error",           // OneClosure
    "handle_statement_error",           // Group
    "handle_statement_error",           // RulePrecedence
    "handle_statement_error",           // RulePrecedenceList
    "handle_statement_error",           // RulePrecedenceSpec
    "handle_statement_error",           // RuleLeftAssoc
    "handle_statement_error",           // RuleRightAssoc
    "handle_statement_error",           // RuleOperatorList
    "handle_statement_error",           // RuleOperatorSpec
    "handle_statement_error",           // TerminalReference
    "handle_statement_error",           // NonterminalReference
    "handle_statement_error",           // Empty
    "handle_statement_error",           // AstFormer
    "handle_statement_error",           // AstItemList
    "handle_statement_error",           // AstChild
    "handle_statement_error",           // AstKind
    "handle_statement_error",           // AstLocation
    "handle_statement_error",           // AstLocationString
    "handle_statement_error",           // AstLexeme
    "handle_statement_error",           // AstLexemeString
    "handle_statement_error",           // AstLocator
    "handle_statement_error",           // AstDot
    "handle_statement_error",           // AstSlice
    "handle_statement_error",           // Token
    "handle_statement_error",           // Options
    "handle_statement_error",           // ReduceActions
    "handle_statement_error",           // RegexString
    "handle_statement_error",           // CharsetString
    "handle_statement_error",           // MacroString
    "handle_statement_error",           // Identifier
    "handle_statement_error",           // Integer
    "handle_statement_error",           // NegativeInteger
    "handle_statement_error",           // String
    "handle_statement_error",           // TripleString
    "handle_statement_error",           // True
    "handle_statement_error",           // False
    "handle_statement_error",           // Regex
    "handle_statement_error",           // RegexOr
    "handle_statement_error",           // RegexList
    "handle_statement_error",           // RegexOptional
    "handle_statement_error",           // RegexZeroClosure
    "handle_statement_error",           // RegexOneClosure
    "handle_statement_error",           // RegexChar
    "handle_statement_error",           // RegexWildcard
    "handle_statement_error",           // RegexWhitespace
    "handle_statement_error",           // RegexNotWhitespace
    "handle_statement_error",           // RegexDigits
    "handle_statement_error",           // RegexNotDigits
    "handle_statement_error",           // RegexEscape
    "handle_statement_error",           // RegexAltNewline
    "handle_statement_error",           // RegexNewline
    "handle_statement_error",           // RegexCr
    "handle_statement_error",           // RegexVBar
    "handle_statement_error",           // RegexStar
    "handle_statement_error",           // RegexPlus
    "handle_statement_error",           // RegexQuestion
    "handle_statement_error",           // RegexPeriod
    "handle_statement_error",           // RegexDollar
    "handle_statement_error",           // RegexSpace
    "handle_statement_error",           // RegexLeftParen
    "handle_statement_error",           // RegexRightParen
    "handle_statement_error",           // RegexLeftBracket
    "handle_statement_error",           // RegexRightBracket
    "handle_statement_error",           // RegexLeftBrace
    "handle_statement_error",           // RegexRightBrace
    "handle_statement_error",           // Charset
    "handle_statement_error",           // CharsetInvert
    "handle_statement_error",           // CharsetRange
    "handle_statement_error",           // CharsetChar
    "handle_statement_error",           // CharsetWhitespace
    "handle_statement_error",           // CharsetNotWhitespace
    "handle_statement_error",           // CharsetDigits
    "handle_statement_error",           // CharsetNotDigits
    "handle_statement_error",           // CharsetEscape
    "handle_statement_error",           // CharsetAltNewline
    "handle_statement_error",           // CharsetNewline
    "handle_statement_error",           // CharsetCr
    "handle_statement_error",           // CharsetCaret
    "handle_statement_error",           // CharsetDash
    "handle_statement_error",           // CharsetDollar
    "handle_statement_error",           // CharsetLeftBracket
    "handle_statement_error",           // CharsetRightBracket
    "handle_statement_statement_list",  // ActionStatementList
    "handle_statement_assign",          // ActionAssign
    "handle_statement_error",           // ActionEqual
    "handle_statement_error",           // ActionNotEqual
    "handle_statement_error",           // ActionLessThan
    "handle_statement_error",           // ActionLessEqual
    "handle_statement_error",           // ActionGreaterThan
    "handle_statement_error",           // ActionGreaterEqual
    "handle_statement_error",           // ActionAdd
    "handle_statement_error",           // ActionSubtract
    "handle_statement_error",           // ActionMultiply
    "handle_statement_error",           // ActionDivide
    "handle_statement_error",           // ActionUnaryMinus
    "handle_statement_error",           // ActionAnd
    "handle_statement_error",           // ActionOr
    "handle_statement_error",           // ActionNot
    "handle_statement_dump_stack",      // ActionDumpStack
    "handle_statement_error"            // ActionTokenCount
};

//
//  Expression wiring tables                                
//  ------------------------                                
//                                               
//  Tables that help us route nodes to handlers. 
//

void (*ActionGenerator::expression_handler[])(ActionGenerator& actg,
                                              Ast* root,
                                              Context& ctx)
{
    handle_expression_error,          // Unknown
    handle_expression_error,          // Null
    handle_expression_error,          // Grammar
    handle_expression_error,          // OptionList
    handle_expression_error,          // TokenList
    handle_expression_error,          // RuleList
    handle_expression_error,          // Lookaheads
    handle_expression_error,          // ErrorRecovery
    handle_expression_error,          // Conflicts
    handle_expression_error,          // KeepWhitespace
    handle_expression_error,          // CaseSensitive
    handle_expression_error,          // TokenDeclaration
    handle_expression_error,          // TokenOptionList
    handle_expression_error,          // TokenTemplate
    handle_expression_error,          // TokenDescription
    handle_expression_error,          // TokenRegexList
    handle_expression_error,          // TokenRegex
    handle_expression_error,          // TokenPrecedence
    handle_expression_error,          // TokenAction
    handle_expression_error,          // TokenLexeme
    handle_expression_error,          // TokenIgnore
    handle_expression_error,          // TokenError
    handle_expression_error,          // Rule
    handle_expression_error,          // RuleRhsList
    handle_expression_error,          // RuleRhs
    handle_expression_error,          // Optional
    handle_expression_error,          // ZeroClosure
    handle_expression_error,          // OneClosure
    handle_expression_error,          // Group
    handle_expression_error,          // RulePrecedence
    handle_expression_error,          // RulePrecedenceList
    handle_expression_error,          // RulePrecedenceSpec
    handle_expression_error,          // RuleLeftAssoc
    handle_expression_error,          // RuleRightAssoc
    handle_expression_error,          // RuleOperatorList
    handle_expression_error,          // RuleOperatorSpec
    handle_expression_error,          // TerminalReference
    handle_expression_error,          // NonterminalReference
    handle_expression_error,          // Empty
    handle_expression_error,          // AstFormer
    handle_expression_error,          // AstItemList
    handle_expression_error,          // AstChild
    handle_expression_error,          // AstKind
    handle_expression_error,          // AstLocation
    handle_expression_error,          // AstLocationString
    handle_expression_error,          // AstLexeme
    handle_expression_error,          // AstLexemeString
    handle_expression_error,          // AstLocator
    handle_expression_error,          // AstDot
    handle_expression_error,          // AstSlice
    handle_expression_error,          // Token
    handle_expression_error,          // Options
    handle_expression_error,          // ReduceActions
    handle_expression_error,          // RegexString
    handle_expression_error,          // CharsetString
    handle_expression_error,          // MacroString
    handle_expression_identifier,     // Identifier
    handle_expression_integer,        // Integer
    handle_expression_error,          // NegativeInteger
    handle_expression_error,          // String
    handle_expression_error,          // TripleString
    handle_expression_error,          // True
    handle_expression_error,          // False
    handle_expression_error,          // Regex
    handle_expression_error,          // RegexOr
    handle_expression_error,          // RegexList
    handle_expression_error,          // RegexOptional
    handle_expression_error,          // RegexZeroClosure
    handle_expression_error,          // RegexOneClosure
    handle_expression_error,          // RegexChar
    handle_expression_error,          // RegexWildcard
    handle_expression_error,          // RegexWhitespace
    handle_expression_error,          // RegexNotWhitespace
    handle_expression_error,          // RegexDigits
    handle_expression_error,          // RegexNotDigits
    handle_expression_error,          // RegexEscape
    handle_expression_error,          // RegexAltNewline
    handle_expression_error,          // RegexNewline
    handle_expression_error,          // RegexCr
    handle_expression_error,          // RegexVBar
    handle_expression_error,          // RegexStar
    handle_expression_error,          // RegexPlus
    handle_expression_error,          // RegexQuestion
    handle_expression_error,          // RegexPeriod
    handle_expression_error,          // RegexDollar
    handle_expression_error,          // RegexSpace
    handle_expression_error,          // RegexLeftParen
    handle_expression_error,          // RegexRightParen
    handle_expression_error,          // RegexLeftBracket
    handle_expression_error,          // RegexRightBracket
    handle_expression_error,          // RegexLeftBrace
    handle_expression_error,          // RegexRightBrace
    handle_expression_error,          // Charset
    handle_expression_error,          // CharsetInvert
    handle_expression_error,          // CharsetRange
    handle_expression_error,          // CharsetChar
    handle_expression_error,          // CharsetWhitespace
    handle_expression_error,          // CharsetNotWhitespace
    handle_expression_error,          // CharsetDigits
    handle_expression_error,          // CharsetNotDigits
    handle_expression_error,          // CharsetEscape
    handle_expression_error,          // CharsetAltNewline
    handle_expression_error,          // CharsetNewline
    handle_expression_error,          // CharsetCr
    handle_expression_error,          // CharsetCaret
    handle_expression_error,          // CharsetDash
    handle_expression_error,          // CharsetDollar
    handle_expression_error,          // CharsetLeftBracket
    handle_expression_error,          // CharsetRightBracket
    handle_expression_error,          // ActionStatementList
    handle_expression_error,          // ActionAssign
    handle_expression_relation,       // ActionEqual
    handle_expression_relation,       // ActionNotEqual
    handle_expression_relation,       // ActionLessThan
    handle_expression_relation,       // ActionLessEqual
    handle_expression_relation,       // ActionGreaterThan
    handle_expression_relation,       // ActionGreaterEqual
    handle_expression_add,            // ActionAdd
    handle_expression_subtract,       // ActionSubtract
    handle_expression_multiply,       // ActionMultiply
    handle_expression_divide,         // ActionDivide
    handle_expression_unary_minus,    // ActionUnaryMinus
    handle_expression_relation,       // ActionAnd
    handle_expression_relation,       // ActionOr
    handle_expression_relation,       // ActionNot
    handle_expression_error,          // ActionDumpStack
    handle_expression_token_count     // ActionTokenCount
};

char *ActionGenerator::expression_handler_name[]
{
    "handle_expression_error",        // Unknown
    "handle_expression_error",        // Null
    "handle_expression_error",        // Grammar
    "handle_expression_error",        // OptionList
    "handle_expression_error",        // TokenList
    "handle_expression_error",        // RuleList
    "handle_expression_error",        // Lookaheads
    "handle_expression_error",        // ErrorRecovery
    "handle_expression_error",        // Conflicts
    "handle_expression_error",        // KeepWhitespace
    "handle_expression_error",        // CaseSensitive
    "handle_expression_error",        // TokenDeclaration
    "handle_expression_error",        // TokenOptionList
    "handle_expression_error",        // TokenTemplate
    "handle_expression_error",        // TokenDescription
    "handle_expression_error",        // TokenRegexList
    "handle_expression_error",        // TokenRegex
    "handle_expression_error",        // TokenPrecedence
    "handle_expression_error",        // TokenAction
    "handle_expression_error",        // TokenLexeme
    "handle_expression_error",        // TokenIgnore
    "handle_expression_error",        // TokenError
    "handle_expression_error",        // Rule
    "handle_expression_error",        // RuleRhsList
    "handle_expression_error",        // RuleRhs
    "handle_expression_error",        // Optional
    "handle_expression_error",        // ZeroClosure
    "handle_expression_error",        // OneClosure
    "handle_expression_error",        // Group
    "handle_expression_error",        // RulePrecedence
    "handle_expression_error",        // RulePrecedenceList
    "handle_expression_error",        // RulePrecedenceSpec
    "handle_expression_error",        // RuleLeftAssoc
    "handle_expression_error",        // RuleRightAssoc
    "handle_expression_error",        // RuleOperatorList
    "handle_expression_error",        // RuleOperatorSpec
    "handle_expression_error",        // TerminalReference
    "handle_expression_error",        // NonterminalReference
    "handle_expression_error",        // Empty
    "handle_expression_error",        // AstFormer
    "handle_expression_error",        // AstItemList
    "handle_expression_error",        // AstChild
    "handle_expression_error",        // AstKind
    "handle_expression_error",        // AstLocation
    "handle_expression_error",        // AstLocationString
    "handle_expression_error",        // AstLexeme
    "handle_expression_error",        // AstLexemeString
    "handle_expression_error",        // AstLocator
    "handle_expression_error",        // AstDot
    "handle_expression_error",        // AstSlice
    "handle_expression_error",        // Token
    "handle_expression_error",        // Options
    "handle_expression_error",        // ReduceActions
    "handle_expression_error",        // RegexString
    "handle_expression_error",        // CharsetString
    "handle_expression_error",        // MacroString
    "handle_expression_identifier",   // Identifier
    "handle_expression_integer",      // Integer
    "handle_expression_error",        // NegativeInteger
    "handle_expression_error",        // String
    "handle_expression_error",        // TripleString
    "handle_expression_error",        // True
    "handle_expression_error",        // False
    "handle_expression_error",        // Regex
    "handle_expression_error",        // RegexOr
    "handle_expression_error",        // RegexList
    "handle_expression_error",        // RegexOptional
    "handle_expression_error",        // RegexZeroClosure
    "handle_expression_error",        // RegexOneClosure
    "handle_expression_error",        // RegexChar
    "handle_expression_error",        // RegexWildcard
    "handle_expression_error",        // RegexWhitespace
    "handle_expression_error",        // RegexNotWhitespace
    "handle_expression_error",        // RegexDigits
    "handle_expression_error",        // RegexNotDigits
    "handle_expression_error",        // RegexEscape
    "handle_expression_error",        // RegexAltNewline
    "handle_expression_error",        // RegexNewline
    "handle_expression_error",        // RegexCr
    "handle_expression_error",        // RegexVBar
    "handle_expression_error",        // RegexStar
    "handle_expression_error",        // RegexPlus
    "handle_expression_error",        // RegexQuestion
    "handle_expression_error",        // RegexPeriod
    "handle_expression_error",        // RegexDollar
    "handle_expression_error",        // RegexSpace
    "handle_expression_error",        // RegexLeftParen
    "handle_expression_error",        // RegexRightParen
    "handle_expression_error",        // RegexLeftBracket
    "handle_expression_error",        // RegexRightBracket
    "handle_expression_error",        // RegexLeftBrace
    "handle_expression_error",        // RegexRightBrace
    "handle_expression_error",        // Charset
    "handle_expression_error",        // CharsetInvert
    "handle_expression_error",        // CharsetRange
    "handle_expression_error",        // CharsetChar
    "handle_expression_error",        // CharsetWhitespace
    "handle_expression_error",        // CharsetNotWhitespace
    "handle_expression_error",        // CharsetDigits
    "handle_expression_error",        // CharsetNotDigits
    "handle_expression_error",        // CharsetEscape
    "handle_expression_error",        // CharsetAltNewline
    "handle_expression_error",        // CharsetNewline
    "handle_expression_error",        // CharsetCr
    "handle_expression_error",        // CharsetCaret
    "handle_expression_error",        // CharsetDash
    "handle_expression_error",        // CharsetDollar
    "handle_expression_error",        // CharsetLeftBracket
    "handle_expression_error",        // CharsetRightBracket
    "handle_expression_error",        // ActionStatementList
    "handle_expression_error",        // ActionAssign
    "handle_expression_relation",     // ActionEqual
    "handle_expression_relation",     // ActionNotEqual
    "handle_expression_relation",     // ActionLessThan
    "handle_expression_relation",     // ActionLessEqual
    "handle_expression_relation",     // ActionGreaterThan
    "handle_expression_relation",     // ActionGreaterEqual
    "handle_expression_add",          // ActionAdd
    "handle_expression_subtract",     // ActionSubtract
    "handle_expression_multiply",     // ActionMultiply
    "handle_expression_divide",       // ActionDivide
    "handle_expression_unary_minus",  // ActionUnaryMinus
    "handle_expression_relation",     // ActionAnd
    "handle_expression_relation",     // ActionOr
    "handle_expression_relation",     // ActionNot
    "handle_expression_error",        // ActionDumpStack
    "handle_expression_token_count"   // ActionTokenCount
};

//
//  Condition wiring tables                                
//  -----------------------                                
//                                               
//  Tables that help us route nodes to handlers. 
//

void (*ActionGenerator::condition_handler[])(ActionGenerator& actg,
                                             Ast* root,
                                             Context& ctx)
{
    handle_condition_error,            // Unknown
    handle_condition_error,            // Null
    handle_condition_error,            // Grammar
    handle_condition_error,            // OptionList
    handle_condition_error,            // TokenList
    handle_condition_error,            // RuleList
    handle_condition_error,            // Lookaheads
    handle_condition_error,            // ErrorRecovery
    handle_condition_error,            // Conflicts
    handle_condition_error,            // KeepWhitespace
    handle_condition_error,            // CaseSensitive
    handle_condition_error,            // TokenDeclaration
    handle_condition_error,            // TokenOptionList
    handle_condition_error,            // TokenTemplate
    handle_condition_error,            // TokenDescription
    handle_condition_error,            // TokenRegexList
    handle_condition_error,            // TokenRegex
    handle_condition_error,            // TokenPrecedence
    handle_condition_error,            // TokenAction
    handle_condition_error,            // TokenLexeme
    handle_condition_error,            // TokenIgnore
    handle_condition_error,            // TokenError
    handle_condition_error,            // Rule
    handle_condition_error,            // RuleRhsList
    handle_condition_error,            // RuleRhs
    handle_condition_error,            // Optional
    handle_condition_error,            // ZeroClosure
    handle_condition_error,            // OneClosure
    handle_condition_error,            // Group
    handle_condition_error,            // RulePrecedence
    handle_condition_error,            // RulePrecedenceList
    handle_condition_error,            // RulePrecedenceSpec
    handle_condition_error,            // RuleLeftAssoc
    handle_condition_error,            // RuleRightAssoc
    handle_condition_error,            // RuleOperatorList
    handle_condition_error,            // RuleOperatorSpec
    handle_condition_error,            // TerminalReference
    handle_condition_error,            // NonterminalReference
    handle_condition_error,            // Empty
    handle_condition_error,            // AstFormer
    handle_condition_error,            // AstItemList
    handle_condition_error,            // AstChild
    handle_condition_error,            // AstKind
    handle_condition_error,            // AstLocation
    handle_condition_error,            // AstLocationString
    handle_condition_error,            // AstLexeme
    handle_condition_error,            // AstLexemeString
    handle_condition_error,            // AstLocator
    handle_condition_error,            // AstDot
    handle_condition_error,            // AstSlice
    handle_condition_error,            // Token
    handle_condition_error,            // Options
    handle_condition_error,            // ReduceActions
    handle_condition_error,            // RegexString
    handle_condition_error,            // CharsetString
    handle_condition_error,            // MacroString
    handle_condition_math,             // Identifier
    handle_condition_math,             // Integer
    handle_condition_error,            // NegativeInteger
    handle_condition_error,            // String
    handle_condition_error,            // TripleString
    handle_condition_error,            // True
    handle_condition_error,            // False
    handle_condition_error,            // Regex
    handle_condition_error,            // RegexOr
    handle_condition_error,            // RegexList
    handle_condition_error,            // RegexOptional
    handle_condition_error,            // RegexZeroClosure
    handle_condition_error,            // RegexOneClosure
    handle_condition_error,            // RegexChar
    handle_condition_error,            // RegexWildcard
    handle_condition_error,            // RegexWhitespace
    handle_condition_error,            // RegexNotWhitespace
    handle_condition_error,            // RegexDigits
    handle_condition_error,            // RegexNotDigits
    handle_condition_error,            // RegexEscape
    handle_condition_error,            // RegexAltNewline
    handle_condition_error,            // RegexNewline
    handle_condition_error,            // RegexCr
    handle_condition_error,            // RegexVBar
    handle_condition_error,            // RegexStar
    handle_condition_error,            // RegexPlus
    handle_condition_error,            // RegexQuestion
    handle_condition_error,            // RegexPeriod
    handle_condition_error,            // RegexDollar
    handle_condition_error,            // RegexSpace
    handle_condition_error,            // RegexLeftParen
    handle_condition_error,            // RegexRightParen
    handle_condition_error,            // RegexLeftBracket
    handle_condition_error,            // RegexRightBracket
    handle_condition_error,            // RegexLeftBrace
    handle_condition_error,            // RegexRightBrace
    handle_condition_error,            // Charset
    handle_condition_error,            // CharsetInvert
    handle_condition_error,            // CharsetRange
    handle_condition_error,            // CharsetChar
    handle_condition_error,            // CharsetWhitespace
    handle_condition_error,            // CharsetNotWhitespace
    handle_condition_error,            // CharsetDigits
    handle_condition_error,            // CharsetNotDigits
    handle_condition_error,            // CharsetEscape
    handle_condition_error,            // CharsetAltNewline
    handle_condition_error,            // CharsetNewline
    handle_condition_error,            // CharsetCr
    handle_condition_error,            // CharsetCaret
    handle_condition_error,            // CharsetDash
    handle_condition_error,            // CharsetDollar
    handle_condition_error,            // CharsetLeftBracket
    handle_condition_error,            // CharsetRightBracket
    handle_condition_error,            // ActionStatementList
    handle_condition_error,            // ActionAssign
    handle_condition_equal,            // ActionEqual
    handle_condition_not_equal,        // ActionNotEqual
    handle_condition_less_than,        // ActionLessThan
    handle_condition_less_equal,       // ActionLessEqual
    handle_condition_greater_than,     // ActionGreaterThan
    handle_condition_greater_equal,    // ActionGreaterEqual
    handle_condition_math,             // ActionAdd
    handle_condition_math,             // ActionSubtract
    handle_condition_math,             // ActionMultiply
    handle_condition_math,             // ActionDivide
    handle_condition_math,             // ActionUnaryMinus
    handle_condition_and,              // ActionAnd
    handle_condition_or,               // ActionOr
    handle_condition_not,              // ActionNot
    handle_condition_error,            // ActionDumpStack
    handle_condition_math              // ActionTokenCount
};

char *ActionGenerator::condition_handler_name[]
{
    "handle_condition_error",          // Unknown
    "handle_condition_error",          // Null
    "handle_condition_error",          // Grammar
    "handle_condition_error",          // OptionList
    "handle_condition_error",          // TokenList
    "handle_condition_error",          // RuleList
    "handle_condition_error",          // Lookaheads
    "handle_condition_error",          // ErrorRecovery
    "handle_condition_error",          // Conflicts
    "handle_condition_error",          // KeepWhitespace
    "handle_condition_error",          // CaseSensitive
    "handle_condition_error",          // TokenDeclaration
    "handle_condition_error",          // TokenOptionList
    "handle_condition_error",          // TokenTemplate
    "handle_condition_error",          // TokenDescription
    "handle_condition_error",          // TokenRegexList
    "handle_condition_error",          // TokenRegex
    "handle_condition_error",          // TokenPrecedence
    "handle_condition_error",          // TokenAction
    "handle_condition_error",          // TokenLexeme
    "handle_condition_error",          // TokenIgnore
    "handle_condition_error",          // TokenError
    "handle_condition_error",          // Rule
    "handle_condition_error",          // RuleRhsList
    "handle_condition_error",          // RuleRhs
    "handle_condition_error",          // Optional
    "handle_condition_error",          // ZeroClosure
    "handle_condition_error",          // OneClosure
    "handle_condition_error",          // Group
    "handle_condition_error",          // RulePrecedence
    "handle_condition_error",          // RulePrecedenceList
    "handle_condition_error",          // RulePrecedenceSpec
    "handle_condition_error",          // RuleLeftAssoc
    "handle_condition_error",          // RuleRightAssoc
    "handle_condition_error",          // RuleOperatorList
    "handle_condition_error",          // RuleOperatorSpec
    "handle_condition_error",          // TerminalReference
    "handle_condition_error",          // NonterminalReference
    "handle_condition_error",          // Empty
    "handle_condition_error",          // AstFormer
    "handle_condition_error",          // AstItemList
    "handle_condition_error",          // AstChild
    "handle_condition_error",          // AstKind
    "handle_condition_error",          // AstLocation
    "handle_condition_error",          // AstLocationString
    "handle_condition_error",          // AstLexeme
    "handle_condition_error",          // AstLexemeString
    "handle_condition_error",          // AstLocator
    "handle_condition_error",          // AstDot
    "handle_condition_error",          // AstSlice
    "handle_condition_error",          // Token
    "handle_condition_error",          // Options
    "handle_condition_error",          // ReduceActions
    "handle_condition_error",          // RegexString
    "handle_condition_error",          // CharsetString
    "handle_condition_error",          // MacroString
    "handle_condition_math",           // Identifier
    "handle_condition_math",           // Integer
    "handle_condition_error",          // NegativeInteger
    "handle_condition_error",          // String
    "handle_condition_error",          // TripleString
    "handle_condition_error",          // True
    "handle_condition_error",          // False
    "handle_condition_error",          // Regex
    "handle_condition_error",          // RegexOr
    "handle_condition_error",          // RegexList
    "handle_condition_error",          // RegexOptional
    "handle_condition_error",          // RegexZeroClosure
    "handle_condition_error",          // RegexOneClosure
    "handle_condition_error",          // RegexChar
    "handle_condition_error",          // RegexWildcard
    "handle_condition_error",          // RegexWhitespace
    "handle_condition_error",          // RegexNotWhitespace
    "handle_condition_error",          // RegexDigits
    "handle_condition_error",          // RegexNotDigits
    "handle_condition_error",          // RegexEscape
    "handle_condition_error",          // RegexAltNewline
    "handle_condition_error",          // RegexNewline
    "handle_condition_error",          // RegexCr
    "handle_condition_error",          // RegexVBar
    "handle_condition_error",          // RegexStar
    "handle_condition_error",          // RegexPlus
    "handle_condition_error",          // RegexQuestion
    "handle_condition_error",          // RegexPeriod
    "handle_condition_error",          // RegexDollar
    "handle_condition_error",          // RegexSpace
    "handle_condition_error",          // RegexLeftParen
    "handle_condition_error",          // RegexRightParen
    "handle_condition_error",          // RegexLeftBracket
    "handle_condition_error",          // RegexRightBracket
    "handle_condition_error",          // RegexLeftBrace
    "handle_condition_error",          // RegexRightBrace
    "handle_condition_error",          // Charset
    "handle_condition_error",          // CharsetInvert
    "handle_condition_error",          // CharsetRange
    "handle_condition_error",          // CharsetChar
    "handle_condition_error",          // CharsetWhitespace
    "handle_condition_error",          // CharsetNotWhitespace
    "handle_condition_error",          // CharsetDigits
    "handle_condition_error",          // CharsetNotDigits
    "handle_condition_error",          // CharsetEscape
    "handle_condition_error",          // CharsetAltNewline
    "handle_condition_error",          // CharsetNewline
    "handle_condition_error",          // CharsetCr
    "handle_condition_error",          // CharsetCaret
    "handle_condition_error",          // CharsetDash
    "handle_condition_error",          // CharsetDollar
    "handle_condition_error",          // CharsetLeftBracket
    "handle_condition_error",          // CharsetRightBracket
    "handle_condition_error",          // ActionStatementList
    "handle_condition_error",          // ActionAssign
    "handle_condition_equal",          // ActionEqual
    "handle_condition_not_equal",      // ActionNotEqual
    "handle_condition_less_than",      // ActionLessThan
    "handle_condition_less_equal",     // ActionLessEqual
    "handle_condition_greater_than",   // ActionGreaterThan
    "handle_condition_greater_equal",  // ActionGreaterEqual
    "handle_condition_math",           // ActionAdd
    "handle_condition_math",           // ActionSubtract
    "handle_condition_math",           // ActionMultiply
    "handle_condition_math",           // ActionDivide
    "handle_condition_math",           // ActionUnaryMinus
    "handle_condition_and",            // ActionAnd
    "handle_condition_or",             // ActionOr
    "handle_condition_not",            // ActionNot
    "handle_condition_error",          // ActionDumpStack
    "handle_condition_math"            // ActionTokenCount
};

//
//  generate_action                                             
//  ---------------                                             
//                                                              
//  Generate code for either a token action or a reduce action. 
//

void ActionGenerator::generate_action(Ast* root)
{

    if ((debug_flags & DebugFlags::DebugAstHandlers) != 0)
    {
        prsi.dump_grammar_ast(root);
    }

    Context ctx;
    handle_statement(*this, root, ctx);

}

//
//  generate_condition
//  ------------------                                             
//                                                              
//  Generate code for a token guard condition
//

void ActionGenerator::generate_condition(Ast* root,
                                         ICodeLabel* true_label,
                                         ICodeLabel* false_label)
{

    if ((debug_flags & DebugFlags::DebugAstHandlers) != 0)
    {
        prsi.dump_grammar_ast(root);
    }

    Context ctx;
    ctx.true_label = true_label;
    ctx.false_label = false_label;
    handle_condition(*this, root, ctx);

}

//
//  handle_statement                                                         
//  ----------------                                                         
//                                                                         
//  Route a call to the appropriate handler. This function is the only one 
//  that should know about our routing table.                              
//

void ActionGenerator::handle_statement(ActionGenerator& actg,
                                       Ast* root,
                                       Context& ctx)
{
    
    if (root == nullptr)
    {
        cout << "Nullptr in ActionGenerator::handle_statement" << endl;
        exit(1);
    }

    if (root->get_kind() < AstType::AstMinimum ||
        root->get_kind() > AstType::AstMaximum)
    {
        handle_statement_error(actg, root, ctx);
    }

    if ((actg.debug_flags & DebugFlags::DebugActions) != 0)
    {
        cout << "ActionGenerator "
             << actg.prsi.get_grammar_kind_string(root->get_kind()) << ": " 
             << statement_handler_name[root->get_kind()] << endl;
    }

    statement_handler[root->get_kind()](actg, root, ctx);

}

//
//  handle_statement_error                                                     
//  -----------------------                                                     
//                                                                   
//  This should never be called. It means there is a path we haven't 
//  accomodated. It's not a user error, it's a logic error.          
//

void ActionGenerator::handle_statement_error(ActionGenerator& actg,
                                             Ast* root,
                                             Context& ctx)
{
    cout << "No ActionGenerator::statement handler for Ast!" << endl << endl;
    actg.prsi.dump_grammar_ast(root);
    exit(1);
}

//
//  handle_statement_list                          
//  ---------------------                          
//                                                 
//  Encode statement lists by encoding each child. 
//

void ActionGenerator::handle_statement_statement_list(ActionGenerator& actg,
                                                      Ast* root,
                                                      Context& ctx)
{

    for (int i = 0; i < root->get_num_children(); i++)
    {
        handle_statement(actg, root->get_child(i), ctx);
    }

}

//
//  handle_assign                                        
//  -------------                                        
//                                                       
//  Copy the right hand value to the left hand register. 
//

void ActionGenerator::handle_statement_assign(ActionGenerator& actg,
                                              Ast* root,
                                              Context& ctx)
{

    Context cctx;
    cctx.target_register = nullptr;
    
    handle_expression(actg, root->get_child(0), cctx);
    ICodeRegister* lhs_register = cctx.target_register;

    handle_expression(actg, root->get_child(1), cctx);
    
    if (cctx.target_register != lhs_register)
    {
        actg.code.emit(OpcodeType::OpcodeAssign,
                       root->get_location(),
                       ICodeOperand(lhs_register),
                       ICodeOperand(cctx.target_register));
    }

    if (actg.code.is_temporary(cctx.target_register))
    {
        actg.code.free_temporary(cctx.target_register);
    }

}

//
//  handle_dump_stack
//  -----------------                                        
//                                                       
//  Generate code to dump the stack.
//

void ActionGenerator::handle_statement_dump_stack(ActionGenerator& actg,
                                                  Ast* root,
                                                  Context& ctx)
{
    actg.code.emit(OpcodeType::OpcodeDumpStack, root->get_location());
}

//
//  handle_expression                                                         
//  -----------------                                                         
//                                                                         
//  Route a call to the appropriate handler. This function is the only one 
//  that should know about our routing table.                              
//

void ActionGenerator::handle_expression(ActionGenerator& actg,
                                        Ast* root,
                                        Context& ctx)
{
    
    if (root == nullptr)
    {
        cout << "Nullptr in ActionGenerator::handle_expression" << endl;
        exit(1);
    }

    if (root->get_kind() < AstType::AstMinimum ||
        root->get_kind() > AstType::AstMaximum)
    {
        handle_expression_error(actg, root, ctx);
    }

    if ((actg.debug_flags & DebugFlags::DebugActions) != 0)
    {
        cout << "ActionGenerator "
             << actg.prsi.get_grammar_kind_string(root->get_kind()) << ": " 
             << expression_handler_name[root->get_kind()] << endl;
    }

    expression_handler[root->get_kind()](actg, root, ctx);

}

//
//  handle_expression_error                                                     
//  -----------------------                                                     
//                                                                   
//  This should never be called. It means there is a path we haven't 
//  accomodated. It's not a user error, it's a logic error.          
//

void ActionGenerator::handle_expression_error(ActionGenerator& actg,
                                              Ast* root,
                                              Context& ctx)
{
    cout << "No ActionGenerator::expression handler for Ast!" << endl << endl;
    actg.prsi.dump_grammar_ast(root);
    exit(1);
}

//
//  handle_expression_math_binop                                         
//  ----------------------------                                         
//                                                                  
//  This function factors out the common code for binary arithmetic 
//  operators. Those should just call this one after filling in the 
//  opcode.                                                         
//

void ActionGenerator::handle_expression_math_binop(ActionGenerator& actg,
                                                   Ast* root,
                                                   Context& ctx)
{

    Context cctx;

    cctx.target_register = nullptr;
    handle_expression(actg, root->get_child(0), cctx);
    ICodeOperand left(cctx.target_register);

    cctx.target_register = nullptr;
    handle_expression(actg, root->get_child(1), cctx);
    ICodeOperand right(cctx.target_register);

    ICodeOperand target;
    if (ctx.target_register == nullptr)
    {
        target.register_ptr = actg.code.get_temporary();
        ctx.target_register = target.register_ptr;
    }
    else
    {
        target.register_ptr = ctx.target_register;
    }

    actg.code.emit(ctx.opcode,
                   root->get_location(),
                   target,
                   left,
                   right);

    if (actg.code.is_temporary(left.register_ptr))
    {
        actg.code.free_temporary(left.register_ptr);
    }

    if (actg.code.is_temporary(right.register_ptr))
    {
        actg.code.free_temporary(right.register_ptr);
    }
  
}

void ActionGenerator::handle_expression_add(ActionGenerator& actg,
                                            Ast* root,
                                            Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeAdd;
    handle_expression_math_binop(actg, root, ctx);
}

void ActionGenerator::handle_expression_subtract(ActionGenerator& actg,
                                                 Ast* root,
                                                 Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeSubtract;
    handle_expression_math_binop(actg, root, ctx);
}

void ActionGenerator::handle_expression_multiply(ActionGenerator& actg,
                                                 Ast* root,
                                                 Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeMultiply;
    handle_expression_math_binop(actg, root, ctx);
}

void ActionGenerator::handle_expression_divide(ActionGenerator& actg,
                                               Ast* root,
                                               Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeDivide;
    handle_expression_math_binop(actg, root, ctx);
}

//
//  handle_expression_math_unop                                         
//  ---------------------------                                         
//                                                                  
//  This function factors out the common code for unary arithmetic 
//  operators. Those should just call this one after filling in the 
//  opcode.                                                         
//

void ActionGenerator::handle_expression_math_unop(ActionGenerator& actg,
                                                  Ast* root,
                                                  Context& ctx)
{

    Context cctx;

    cctx.target_register = nullptr;
    handle_expression(actg, root->get_child(0), cctx);
    ICodeOperand left(cctx.target_register);

    ICodeOperand target;
    if (ctx.target_register == nullptr)
    {
        target.register_ptr = actg.code.get_temporary();
        ctx.target_register = target.register_ptr;
    }
    else
    {
        target.register_ptr = ctx.target_register;
    }

    actg.code.emit(ctx.opcode,
                   root->get_location(),
                   target,
                   left);

    if (actg.code.is_temporary(left.register_ptr))
    {
        actg.code.free_temporary(left.register_ptr);
    }

}

void ActionGenerator::handle_expression_unary_minus(ActionGenerator& actg,
                                                    Ast* root,
                                                    Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeUnaryMinus;
    handle_expression_math_unop(actg, root, ctx);
}

//
//  handle_expression_relation                                          
//  --------------------------                                          
//                                                                      
//  This function factors out the common code for relational operators. 
//

void ActionGenerator::handle_expression_relation(ActionGenerator& actg,
                                                 Ast* root,
                                                 Context& ctx)
{

    Context cctx;

    ctx.true_label = actg.code.get_label();
    ctx.false_label = actg.code.get_label();
    ICodeLabel* next_label = actg.code.get_label();

    ICodeOperand target;
    if (ctx.target_register == nullptr)
    {
        target.register_ptr = actg.code.get_temporary();
        ctx.target_register = target.register_ptr;
    }
    else
    {
        target.register_ptr = ctx.target_register;
    }

    handle_condition(actg, root, ctx);

    actg.code.emit(OpcodeType::OpcodeLabel,
                   root->get_location(),
                   ICodeOperand(ctx.true_label));

    actg.code.emit(OpcodeType::OpcodeAssign,
                   root->get_location(),
                   target,
                   ICodeOperand(actg.code.get_register("1", 1)));

    actg.code.emit(OpcodeType::OpcodeBranch,
                   root->get_location(),
                   ICodeOperand(next_label));

    actg.code.emit(OpcodeType::OpcodeLabel,
                   root->get_location(),
                   ICodeOperand(ctx.false_label));

    actg.code.emit(OpcodeType::OpcodeAssign,
                   root->get_location(),
                   target,
                   ICodeOperand(actg.code.get_register("0", 0)));

    actg.code.emit(OpcodeType::OpcodeLabel,
                   root->get_location(),
                   ICodeOperand(next_label));

}

//
//  handle_expression_identifier                                 
//  ----------------------------                                 
//                                                               
//  Identifiers should yield either an existing or new register. 
//

void ActionGenerator::handle_expression_identifier(ActionGenerator& actg,
                                                   Ast* root,
                                                   Context& ctx)
{
    ctx.target_register = actg.code.get_register(root->get_lexeme(), 0);
}

//
//  handle_expression_integer
//  -------------------------
//                                                               
//  An identifier yields a literal integer register.
//

void ActionGenerator::handle_expression_integer(ActionGenerator& actg,
                                                Ast* root,
                                                Context& ctx)
{
    ctx.target_register = actg.code.get_register(root->get_lexeme(),
                                                 atol(root->get_lexeme().c_str()));
}

//
//  handle_expression_token_count
//  -----------------------------                                 
//                                                               
//  This is essentially a pre-defined register.
//

void ActionGenerator::handle_expression_token_count(ActionGenerator& actg,
                                                    Ast* root,
                                                    Context& ctx)
{
    ctx.target_register = actg.code.get_register("token_count", 0);
}

//
//  handle_condition                                                         
//  ----------------                                                         
//                                                                         
//  Route a call to the appropriate handler. This function is the only one 
//  that should know about our routing table.                              
//

void ActionGenerator::handle_condition(ActionGenerator& actg,
                                       Ast* root,
                                       Context& ctx)
{
    
    if (root == nullptr)
    {
        cout << "Nullptr in ActionGenerator::handle_condition" << endl;
        exit(1);
    }

    if (root->get_kind() < AstType::AstMinimum ||
        root->get_kind() > AstType::AstMaximum)
    {
        handle_condition_error(actg, root, ctx);
    }

    if ((actg.debug_flags & DebugFlags::DebugActions) != 0)
    {
        cout << "ActionGenerator "
             << actg.prsi.get_grammar_kind_string(root->get_kind()) << ": " 
             << condition_handler_name[root->get_kind()] << endl;
    }

    condition_handler[root->get_kind()](actg, root, ctx);

}

//
//  handle_condition_error                                                     
//  -----------------------                                                     
//                                                                   
//  This should never be called. It means there is a path we haven't 
//  accomodated. It's not a user error, it's a logic error.          
//

void ActionGenerator::handle_condition_error(ActionGenerator& actg,
                                             Ast* root,
                                             Context& ctx)
{
    cout << "No ActionGenerator::condition handler for Ast!" << endl << endl;
    actg.prsi.dump_grammar_ast(root);
    exit(1);
}

//
//  handle_condition_math                                               
//  ---------------------                                               
//                                                                      
//  This function factors out the common code for arithmetic operators. 
//

void ActionGenerator::handle_condition_math(ActionGenerator& actg,
                                            Ast* root,
                                            Context& ctx)
{

    Context cctx;

    cctx.target_register = nullptr;
    handle_expression(actg, root, cctx);

    actg.code.emit(OpcodeType::OpcodeBranchNotEqual,
                   root->get_location(),
                   ICodeOperand(ctx.true_label),
                   ICodeOperand(cctx.target_register),
                   ICodeOperand(actg.code.get_register("0", 0)));

    actg.code.emit(OpcodeType::OpcodeBranch,
                   root->get_location(),
                   ICodeOperand(ctx.false_label));

    if (actg.code.is_temporary(cctx.target_register))
    {
        actg.code.free_temporary(cctx.target_register);
    }

}

//
//  handle_condition_relation                                               
//  -------------------------                                               
//                                                                      
//  This function factors out the common code for relational operators. 
//  Those should just call this one after filling in the opcode.        
//

void ActionGenerator::handle_condition_relation(ActionGenerator& actg,
                                                Ast* root,
                                                Context& ctx)
{

    Context cctx;

    cctx.target_register = nullptr;
    handle_expression(actg, root->get_child(0), cctx);
    ICodeOperand left(cctx.target_register);

    cctx.target_register = nullptr;
    handle_expression(actg, root->get_child(1), cctx);
    ICodeOperand right(cctx.target_register);

    actg.code.emit(ctx.opcode,
                   root->get_location(),
                   ICodeOperand(ctx.true_label),
                   left,
                   right);

    actg.code.emit(OpcodeType::OpcodeBranch,
                   root->get_location(),
                   ICodeOperand(ctx.false_label));

    if (actg.code.is_temporary(left.register_ptr))
    {
        actg.code.free_temporary(left.register_ptr);
    }

    if (actg.code.is_temporary(right.register_ptr))
    {
        actg.code.free_temporary(right.register_ptr);
    }
  
}

void ActionGenerator::handle_condition_equal(ActionGenerator& actg,
                                             Ast* root,
                                             Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeBranchEqual;
    handle_condition_relation(actg, root, ctx);
}

void ActionGenerator::handle_condition_not_equal(ActionGenerator& actg,
                                                 Ast* root,
                                                 Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeBranchNotEqual;
    handle_condition_relation(actg, root, ctx);
}

void ActionGenerator::handle_condition_less_than(ActionGenerator& actg,
                                                 Ast* root,
                                                 Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeBranchLessThan;
    handle_condition_relation(actg, root, ctx);
}

void ActionGenerator::handle_condition_less_equal(ActionGenerator& actg,
                                                  Ast* root,
                                                  Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeBranchLessEqual;
    handle_condition_relation(actg, root, ctx);
}

void ActionGenerator::handle_condition_greater_than(ActionGenerator& actg,
                                                    Ast* root,
                                                    Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeBranchGreaterThan;
    handle_condition_relation(actg, root, ctx);
}

void ActionGenerator::handle_condition_greater_equal(ActionGenerator& actg,
                                                     Ast* root,
                                                     Context& ctx)
{
    ctx.opcode = OpcodeType::OpcodeBranchGreaterEqual;
    handle_condition_relation(actg, root, ctx);
}

//
//  handle_condition_and                                
//  --------------------                                
//                                                      
//  Generate code for logical and. We short circuit it. 
//

void ActionGenerator::handle_condition_and(ActionGenerator& actg,
                                           Ast* root,
                                           Context& ctx)
{
  
    Context cctx;

    cctx.true_label = actg.code.get_label();
    cctx.false_label = ctx.false_label;

    handle_condition(actg, root->get_child(0), cctx);

    actg.code.emit(OpcodeType::OpcodeLabel,
                   root->get_location(),
                   ICodeOperand(cctx.true_label));

    cctx.true_label = ctx.true_label;

    handle_condition(actg, root->get_child(1), cctx);
    
}

//
//  handle_condition_or                                
//  --------------------                                
//                                                      
//  Generate code for logical and. We short circuit it. 
//

void ActionGenerator::handle_condition_or(ActionGenerator& actg,
                                          Ast* root,
                                          Context& ctx)
{
  
    Context cctx;

    cctx.true_label = ctx.true_label;
    cctx.false_label = actg.code.get_label();

    handle_condition(actg, root->get_child(0), cctx);

    actg.code.emit(OpcodeType::OpcodeLabel,
                   root->get_location(),
                   ICodeOperand(cctx.false_label));

    cctx.false_label = ctx.false_label;

    handle_condition(actg, root->get_child(1), cctx);
    
}

//
//  handle_condition_not                                
//  --------------------                                
//                                                      
//  Generate code for logical not.
//

void ActionGenerator::handle_condition_not(ActionGenerator& actg,
                                           Ast* root,
                                           Context& ctx)
{
  
    Context cctx;

    cctx.true_label = ctx.false_label;
    cctx.false_label = ctx.true_label;

    handle_condition(actg, root->get_child(0), cctx);

}

} // namespace hoshi



