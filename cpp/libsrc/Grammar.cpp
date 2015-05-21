//
//  Grammar                                                               
//  -------                                                               
//                                                                        
//  Traverse an Ast of the input source file assembling the grammar in a  
//  more manageable form. We need lists of symbols and rules but with     
//  subtrees attached to them in various places.                          
//                                                                        
//  We're going to keep largish sets and maps of the items here all over  
//  the place, so we are going to use a flyweight pattern for storage. As 
//  we allocate these things we'll keep pointers in sets, maps and lists  
//  here and return naked pointers. When an object of this class is       
//  destroyed we'll clean up everything we allocated.                     
//

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "AstType.H"
#include "LibraryToken.H"
#include "Parser.H"
#include "ParserImpl.H"
#include "ParserData.H"
#include "Grammar.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;

//
//  Wiring tables                                
//  -------------                                
//                                               
//  Tables that help us route nodes to handlers. 
//

void (*Grammar::extract_handler[])(Grammar& gram, Ast* root, Context& ctx)
{
    handle_error,                    // Unknown
    handle_error,                    // Null
    handle_list,                     // Grammar
    handle_list,                     // OptionList
    handle_list,                     // TokenList
    handle_list,                     // RuleList
    handle_lookaheads,               // Lookaheads
    handle_error_recovery,           // ErrorRecovery
    handle_conflicts,                // Conflicts
    handle_keep_whitespace,          // KeepWhitespace
    handle_case_sensitive,           // CaseSensitive
    handle_token_declaration,        // TokenDeclaration
    handle_token_option_list,        // TokenOptionList
    handle_token_template,           // TokenTemplate
    handle_token_description,        // TokenDescription
    handle_token_regex_list,         // TokenRegexList
    handle_error,                    // TokenRegex
    handle_token_precedence,         // TokenPrecedence
    handle_token_action,             // TokenAction
    handle_token_lexeme,             // TokenLexeme
    handle_token_ignore,             // TokenIgnore
    handle_token_error,              // TokenError
    handle_rule,                     // Rule
    handle_list,                     // RuleRhsList
    handle_rule_rhs,                 // RuleRhs
    handle_optional,                 // Optional
    handle_zero_closure,             // ZeroClosure
    handle_one_closure,              // OneClosure
    handle_group,                    // Group
    handle_rule_precedence,          // RulePrecedence
    handle_list,                     // RulePrecedenceList
    handle_rule_precedence_spec,     // RulePrecedenceSpec
    handle_rule_left_assoc,          // RuleLeftAssoc
    handle_rule_right_assoc,         // RuleRightAssoc
    handle_list,                     // RuleOperatorList
    handle_rule_operator_spec,       // RuleOperatorSpec
    handle_terminal_reference,       // TerminalReference
    handle_nonterminal_reference,    // NonterminalReference
    handle_empty,                    // Empty
    handle_error,                    // AstFormer
    handle_error,                    // AstItemList
    handle_error,                    // AstChild
    handle_error,                    // AstKind
    handle_error,                    // AstLocation
    handle_error,                    // AstLocationString
    handle_error,                    // AstLexeme
    handle_error,                    // AstLexemeString
    handle_error,                    // AstLocator
    handle_error,                    // AstDot
    handle_error,                    // AstSlice
    handle_error,                    // Token
    handle_error,                    // Options
    handle_error,                    // ReduceActions
    handle_error,                    // RegexString
    handle_error,                    // CharsetString
    handle_error,                    // MacroString
    handle_identifier,               // Identifier
    handle_integer,                  // Integer
    handle_error,                    // NegativeInteger
    handle_string,                   // String
    handle_triple_string,            // TripleString
    handle_true,                     // True
    handle_false,                    // False
    handle_error,                    // Regex
    handle_error,                    // RegexOr
    handle_error,                    // RegexList
    handle_error,                    // RegexOptional
    handle_error,                    // RegexZeroClosure
    handle_error,                    // RegexOneClosure
    handle_error,                    // RegexChar
    handle_error,                    // RegexWildcard
    handle_error,                    // RegexWhitespace
    handle_error,                    // RegexNotWhitespace
    handle_error,                    // RegexDigits
    handle_error,                    // RegexNotDigits
    handle_error,                    // RegexEscape
    handle_error,                    // RegexAltNewline
    handle_error,                    // RegexNewline
    handle_error,                    // RegexCr
    handle_error,                    // RegexVBar
    handle_error,                    // RegexStar
    handle_error,                    // RegexPlus
    handle_error,                    // RegexQuestion
    handle_error,                    // RegexPeriod
    handle_error,                    // RegexDollar
    handle_error,                    // RegexSpace
    handle_error,                    // RegexLeftParen
    handle_error,                    // RegexRightParen
    handle_error,                    // RegexLeftBracket
    handle_error,                    // RegexRightBracket
    handle_error,                    // RegexLeftBrace
    handle_error,                    // RegexRightBrace
    handle_error,                    // Charset
    handle_error,                    // CharsetInvert
    handle_error,                    // CharsetRange
    handle_error,                    // CharsetChar
    handle_error,                    // CharsetWhitespace
    handle_error,                    // CharsetNotWhitespace
    handle_error,                    // CharsetDigits
    handle_error,                    // CharsetNotDigits
    handle_error,                    // CharsetEscape
    handle_error,                    // CharsetAltNewline
    handle_error,                    // CharsetNewline
    handle_error,                    // CharsetCr
    handle_error,                    // CharsetCaret
    handle_error,                    // CharsetDash
    handle_error,                    // CharsetDollar
    handle_error,                    // CharsetLeftBracket
    handle_error,                    // CharsetRightBracket
    handle_error,                    // ActionStatementList
    handle_error,                    // ActionAssign
    handle_error,                    // ActionEqual
    handle_error,                    // ActionNotEqual
    handle_error,                    // ActionLessThan
    handle_error,                    // ActionLessEqual
    handle_error,                    // ActionGreaterThan
    handle_error,                    // ActionGreaterEqual
    handle_error,                    // ActionAdd
    handle_error,                    // ActionSubtract
    handle_error,                    // ActionMultiply
    handle_error,                    // ActionDivide
    handle_error,                    // ActionUnaryMinus
    handle_error,                    // ActionAnd
    handle_error,                    // ActionOr
    handle_error,                    // ActionNot
    handle_error,                    // ActionDumpStack
    handle_error                     // ActionTokenCount
};

const char* Grammar::extract_handler_name[]
{
    "handle_error",                  // Unknown
    "handle_error",                  // Null
    "handle_list",                   // Grammar
    "handle_list",                   // OptionList
    "handle_list",                   // TokenList
    "handle_list",                   // RuleList
    "handle_lookaheads",             // Lookaheads
    "handle_error_recovery",         // ErrorRecovery
    "handle_conflicts",              // Conflicts
    "handle_keep_whitespace",        // KeepWhitespace
    "handle_case_sensitive",         // CaseSensitive
    "handle_token_declaration",      // TokenDeclaration
    "handle_token_option_list",      // TokenOptionList
    "handle_token_template",         // TokenTemplate
    "handle_token_description",      // TokenDescription
    "handle_token_regex_list",       // TokenRegexList
    "handle_error",                  // TokenRegex
    "handle_token_precedence",       // TokenPrecedence
    "handle_token_action",           // TokenAction
    "handle_token_lexeme",           // TokenLexeme
    "handle_token_ignore",           // TokenIgnore
    "handle_token_error",            // TokenError
    "handle_rule",                   // Rule
    "handle_list",                   // RuleRhsList
    "handle_rule_rhs",               // RuleRhs
    "handle_optional",               // Optional
    "handle_zero_closure",           // ZeroClosure
    "handle_one_closure",            // OneClosure
    "handle_group",                  // Group
    "handle_rule_precedence",        // RulePrecedence
    "handle_list",                   // RulePrecedenceList
    "handle_rule_precedence_spec",   // RulePrecedenceSpec
    "handle_rule_left_assoc",        // RuleLeftAssoc
    "handle_rule_right_assoc",       // RuleRightAssoc
    "handle_list",                   // RuleOperatorList
    "handle_rule_operator_spec",     // RuleOperatorSpec
    "handle_terminal_reference",     // TerminalReference
    "handle_nonterminal_reference",  // NonterminalReference
    "handle_empty",                  // Empty
    "handle_error",                  // AstFormer
    "handle_error",                  // AstItemList
    "handle_error",                  // AstChild
    "handle_error",                  // AstKind
    "handle_error",                  // AstLocation
    "handle_error",                  // AstLocationString
    "handle_error",                  // AstLexeme
    "handle_error",                  // AstLexemeString
    "handle_error",                  // AstLocator
    "handle_error",                  // AstDot
    "handle_error",                  // AstSlice
    "handle_error",                  // Token
    "handle_error",                  // Options
    "handle_error",                  // ReduceActions
    "handle_error",                  // RegexString
    "handle_error",                  // CharsetString
    "handle_error",                  // MacroString
    "handle_identifier",             // Identifier
    "handle_integer",                // Integer
    "handle_error",                  // NegativeInteger
    "handle_string",                 // String
    "handle_triple_string",          // TripleString
    "handle_true",                   // True
    "handle_false",                  // False
    "handle_error",                  // Regex
    "handle_error",                  // RegexOr
    "handle_error",                  // RegexList
    "handle_error",                  // RegexOptional
    "handle_error",                  // RegexZeroClosure
    "handle_error",                  // RegexOneClosure
    "handle_error",                  // RegexChar
    "handle_error",                  // RegexWildcard
    "handle_error",                  // RegexWhitespace
    "handle_error",                  // RegexNotWhitespace
    "handle_error",                  // RegexDigits
    "handle_error",                  // RegexNotDigits
    "handle_error",                  // RegexEscape
    "handle_error",                  // RegexAltNewline
    "handle_error",                  // RegexNewline
    "handle_error",                  // RegexCr
    "handle_error",                  // RegexVBar
    "handle_error",                  // RegexStar
    "handle_error",                  // RegexPlus
    "handle_error",                  // RegexQuestion
    "handle_error",                  // RegexPeriod
    "handle_error",                  // RegexDollar
    "handle_error",                  // RegexSpace
    "handle_error",                  // RegexLeftParen
    "handle_error",                  // RegexRightParen
    "handle_error",                  // RegexLeftBracket
    "handle_error",                  // RegexRightBracket
    "handle_error",                  // RegexLeftBrace
    "handle_error",                  // RegexRightBrace
    "handle_error",                  // Charset
    "handle_error",                  // CharsetInvert
    "handle_error",                  // CharsetRange
    "handle_error",                  // CharsetChar
    "handle_error",                  // CharsetWhitespace
    "handle_error",                  // CharsetNotWhitespace
    "handle_error",                  // CharsetDigits
    "handle_error",                  // CharsetNotDigits
    "handle_error",                  // CharsetEscape
    "handle_error",                  // CharsetAltNewline
    "handle_error",                  // CharsetNewline
    "handle_error",                  // CharsetCr
    "handle_error",                  // CharsetCaret
    "handle_error",                  // CharsetDash
    "handle_error",                  // CharsetDollar
    "handle_error",                  // CharsetLeftBracket
    "handle_error",                  // CharsetRightBracket
    "handle_error",                  // ActionStatementList
    "handle_error",                  // ActionAssign
    "handle_error",                  // ActionEqual
    "handle_error",                  // ActionNotEqual
    "handle_error",                  // ActionLessThan
    "handle_error",                  // ActionLessEqual
    "handle_error",                  // ActionGreaterThan
    "handle_error",                  // ActionGreaterEqual
    "handle_error",                  // ActionAdd
    "handle_error",                  // ActionSubtract
    "handle_error",                  // ActionMultiply
    "handle_error",                  // ActionDivide
    "handle_error",                  // ActionUnaryMinus
    "handle_error",                  // ActionAnd
    "handle_error",                  // ActionOr
    "handle_error",                  // ActionNot
    "handle_error",                  // ActionDumpStack
    "handle_error"                   // ActionTokenCount
};

//
//  constructor & destructor                                               
//  ------------------------                                               
//                                                                         
//  This class manages storage for flyweight objects. 
//

Grammar::Grammar(ParserImpl& prsi,
                 ErrorHandler& errh,
                 ParserData& prsd,
                 Ast* root,
                 int64_t debug_flags)
    : prsi(prsi), errh(errh), prsd(prsd), root(root), debug_flags(debug_flags)
{

    eof_symbol = create_symbol("*eof*");
    eof_symbol->is_terminal = true;

    error_symbol = create_symbol("*error*");
    error_symbol->is_terminal = true;

    accept_symbol = create_symbol("*accept*");
    accept_symbol->is_nonterminal = true;

    epsilon_symbol = create_symbol("*epsilon*");
    epsilon_symbol->is_terminal = true;

}

Grammar::~Grammar()
{

    for (auto mp: symbol_map)
    {
        delete mp.second;
    }

    for (Rule* rule: rule_list)
    {
        delete rule;
    }

}

//
//  Symbols                                                                
//  -------                                                                
//                                                                         
//  We have to provide an interface to get and create symbols so we store  
//  a map of them on this object. This map will be used to delete them all 
//  when the object is destroyed.                                          
//

Symbol* Grammar::get_symbol(const string& name)
{

    if (symbol_map.find(name) == symbol_map.end())
    {
        return nullptr;
    }

    return symbol_map[name];

}

Symbol* Grammar::create_symbol(const string& name)
{

    if (symbol_map.find(name) != symbol_map.end())
    {
        return nullptr;
    }

    Symbol* symbol = new Symbol();

    symbol->symbol_num = 0;
    symbol->symbol_name = name;

    symbol_map[name] = symbol;

    return symbol;

}

void Grammar::delete_symbol(Symbol* symbol)
{
    symbol_map.erase(symbol->symbol_name);
    delete symbol;
}

//
//  Rules                                                                 
//  -----                                                                 
//                                                                        
//  All we need do is allocate rules. There will be lots of copies in the 
//  parser generation classes, so a flyweight model makes sense here too. 
//

Rule* Grammar::add_rule()
{

    Rule* rule = new Rule();
    rule->rule_num = rule_list.size();
    rule_list.push_back(rule);

    return rule;

}

void Grammar::delete_rule(Rule* rule)
{

    for (auto it = rule_list.begin(); it != rule_list.end(); )
    {

        if (*it == rule)
        {
            it = rule_list.erase(it);
        }
        else
        {
            it++;
        }

    }

    delete rule;

}

//
//  extract                                                              
//  --------                                                              
//                                                                        
//  An external entry point. Here we traverse the entire tree extracting 
//  the grammar.                                                          
//

void Grammar::extract()
{

    //
    //  Log progress. 
    //

    if ((debug_flags & DebugType::DebugProgress) != 0)
    {
        cout << "Beginning grammar extraction: "
             << prsi.elapsed_time_string()
             << endl;
    }

    if ((debug_flags & DebugType::DebugAstHandlers) != 0)
    {
        prsi.dump_grammar_ast(root);
    }

    //
    //  Create an augmented grammar by adding a start rule and extract all 
    //  the other symbols and rules.                                       
    //

    start_rule = add_rule();

    Context ctx;
    handle_extract(*this, root, ctx);

    start_rule->lhs = accept_symbol;
    if (rule_list.size() > 1)
    {
        start_rule->rhs.push_back(rule_list[1]->lhs);
    }

    //
    //  Remove the epsilons from all the rules. 
    //

    for (Rule* rule: rule_list)
    {

        for (auto it = rule->rhs.begin(); it != rule->rhs.end(); )
        {

            if (*it == epsilon_symbol)
            {
                it = rule->rhs.erase(it);
            }
            else
            {
                it++;
            }

        }

    }

    //
    //  Most likely the language accepted by the parser ignores 
    //  whitespace. We'll make that the default.                
    //

    if (!keep_whitespace &&
        symbol_map.find("<whitespace>") == symbol_map.end())
    {

        LibraryToken* token = LibraryToken::get_library_token("whitespace");
        if (token == nullptr)
        {
            cerr << "Missing whitespace library!" << endl;
            exit(1);
        }

        Symbol* symbol = create_symbol("<whitespace>");
        symbol->is_ignored = token->is_ignored;
        symbol->is_scanned = true;
        symbol->description = token->description;
        symbol->precedence = token->precedence;
        symbol->lexeme_needed = token->lexeme_needed;

        symbol->is_ast_synthesized = true;

        Ast* token_regex_list_ast = new Ast(1); 
        token_regex_list_ast->set_kind(AstType::AstTokenRegexList);
        token_regex_list_ast->set_location(-1);
        symbol->regex_list_ast = token_regex_list_ast;

        Ast* token_regex_ast = new Ast(2); 
        token_regex_ast->set_kind(AstType::AstTokenRegex);
        token_regex_ast->set_location(-1);
        token_regex_list_ast->set_child(0, token_regex_ast);

        Ast* token_regex_guard_ast = new Ast(0); 
        token_regex_guard_ast->set_kind(AstType::AstNull);
        token_regex_guard_ast->set_location(-1);
        token_regex_ast->set_child(0, token_regex_guard_ast);

        token_regex_ast->set_child(1, prsi.parse_library_regex(token->regex_string));

    }

    //
    //  Add default regex's where they are missing.
    //

    for (auto mp: symbol_map)
    {

        Symbol* token = mp.second;

        if (!token->is_scanned)
        {
            continue;
        }

        if (token->regex_list_ast != nullptr &&
            token->regex_list_ast->get_kind() != AstType::AstNull)
        {
            continue;
        }

        token->is_ast_synthesized = true;

        const string& symbol = token->string_value;

        Ast* token_regex_list_ast = new Ast(1); 
        token_regex_list_ast->set_kind(AstType::AstTokenRegexList);
        token_regex_list_ast->set_location(token->location);
        token->regex_list_ast = token_regex_list_ast;

        Ast* token_regex_ast = new Ast(2); 
        token_regex_ast->set_kind(AstType::AstTokenRegex);
        token_regex_ast->set_location(token->location);
        token_regex_list_ast->set_child(0, token_regex_ast);

        Ast* token_regex_guard_ast = new Ast(0); 
        token_regex_guard_ast->set_kind(AstType::AstNull);
        token_regex_guard_ast->set_location(token->location);
        token_regex_ast->set_child(0, token_regex_guard_ast);

        Ast* regex_ast = new Ast(1); 
        regex_ast->set_kind(AstType::AstRegex);
        regex_ast->set_location(token->location);
        token_regex_ast->set_child(1, regex_ast);

        Ast* list_ast = new Ast(symbol.length()); 
        list_ast->set_kind(AstType::AstRegexList);
        list_ast->set_location(token->location);
        regex_ast->set_child(0, list_ast);

        for (int i = 0; i < symbol.length(); i++)
        {

            if (case_sensitive || tolower(symbol[i]) == toupper(symbol[i]))
            {

                Ast* char_ast = new Ast(0);
                char_ast->set_kind(AstType::AstRegexChar);
                char_ast->set_location(token->location);
                char_ast->set_lexeme(symbol.substr(i, 1));
                list_ast->set_child(i, char_ast);

            }
            else
            {

                Ast* charset_ast = new Ast(2);
                charset_ast->set_kind(AstType::AstCharset);
                charset_ast->set_location(token->location);
                list_ast->set_child(i, charset_ast);
                
                Ast* charset_range_ast = new Ast(1);
                charset_range_ast->set_kind(AstType::AstCharsetRange);
                charset_range_ast->set_location(token->location);
                charset_ast->set_child(0, charset_range_ast);

                Ast* char_ast = new Ast(0);
                char_ast->set_kind(AstType::AstCharsetChar);
                char_ast->set_location(token->location);
                char_ast->set_lexeme(string(1, tolower(symbol[i])));
                charset_range_ast->set_child(0, char_ast);

                charset_range_ast = new Ast(1);
                charset_range_ast->set_kind(AstType::AstCharsetRange);
                charset_range_ast->set_location(token->location);
                charset_ast->set_child(1, charset_range_ast);

                char_ast = new Ast(0);
                char_ast->set_kind(AstType::AstCharsetChar);
                char_ast->set_location(token->location);
                char_ast->set_lexeme(string(1, toupper(symbol[i])));
                charset_range_ast->set_child(0, char_ast);

            }

        }

    }

    //
    //  Add default Ast formers where they are missing. 
    //

    for (Rule* rule: rule_list)
    {

        if (rule->ast_former_ast != nullptr &&
            rule->ast_former_ast->get_kind() != AstType::AstNull)
        {
            continue;
        }

        if (rule->rhs.size() == 1)
        {
            continue;
        }

        rule->is_ast_synthesized = true;

        Ast* former_ast = new Ast(rule->rhs.size() + 1); 
        former_ast->set_kind(AstType::AstAstFormer);
        former_ast->set_location(rule->location);
        rule->ast_former_ast = former_ast;

        Ast* kind_ast = new Ast(0);
        kind_ast->set_kind(AstType::AstIdentifier);
        kind_ast->set_location(rule->location);
        kind_ast->set_lexeme(rule->lhs->symbol_name);
        former_ast->set_child(0, kind_ast);

        for (int i = 0; i < rule->rhs.size(); i++)
        {

            Ast* child_ast = new Ast(2);
            child_ast->set_kind(AstType::AstAstChild);
            child_ast->set_location(rule->location);
            former_ast->set_child(i + 1, child_ast);

            Ast* dot_ast = new Ast(1);
            dot_ast->set_kind(AstType::AstAstDot);
            dot_ast->set_location(rule->location);
            child_ast->set_child(0, dot_ast);

            Ast* integer_ast = new Ast(0);
            integer_ast->set_kind(AstType::AstInteger);
            integer_ast->set_location(rule->location);
            integer_ast->set_lexeme(to_string(i + 1));
            dot_ast->set_child(0, integer_ast);

            Ast* slice_ast = new Ast(0);
            slice_ast->set_kind(AstType::AstNull);
            slice_ast->set_location(rule->location);
            child_ast->set_child(1, slice_ast);

        }

    }

    //
    //  Precompute the maximum symbol width for the debugging code. 
    //

    symbol_width = 0;
    for (auto mp: symbol_map)
    {
        if (symbol_width < mp.second->symbol_name.length())
        {
            symbol_width = mp.second->symbol_name.length();
        }
    }

    symbol_width += 2;

    //
    //  Dump all our tables if desired. 
    //

    if ((debug_flags & DebugType::DebugGrammar) != 0)
    {
        dump_grammar();
    }

    if ((debug_flags & DebugType::DebugProgress) != 0)
    {
        cout << "Finished grammar extraction: "
             << prsi.elapsed_time_string()
             << endl;
    }

}

//
//  handle_extract                                                         
//  --------------                                                         
//                                                                         
//  Route a call to the appropriate handler. This function is the only one 
//  that should know about our routing table.                              
//

void Grammar::handle_extract(Grammar& gram, Ast* root, Context& ctx)
{
    
    if (root == nullptr)
    {
        cout << "Nullptr in handle_extract" << endl;
        exit(1);
    }

    if (root->get_kind() < AstType::AstMinimum ||
        root->get_kind() > AstType::AstMaximum)
    {
        handle_error(gram, root, ctx);
    }
  
    if ((gram.debug_flags & DebugType::DebugAstHandlers) != 0)
    {
        cout << "Grammar handler: "
             << gram.prsi.get_grammar_kind_string(root->get_kind()) << ": " 
             << extract_handler_name[root->get_kind()] << endl;
    }

    extract_handler[root->get_kind()](gram, root, ctx);

}

//
//  handle_error                                                     
//  ------------                                                     
//                                                                   
//  This should never be called. It means there is a path we haven't 
//  accomodated. It's not a user error, it's a logic error.          
//

void Grammar::handle_error(Grammar& gram, Ast* root, Context& ctx)
{
    cout << "No extract handler for Ast!" << endl << endl;
    gram.prsi.dump_grammar_ast(root);
    exit(1);
}

//
//  handle_list                                                           
//  -----------                                                           
//                                                                        
//  Various kinds of lists don't need anything special. We just make sure 
//  we handle all the children.                                           
//

void Grammar::handle_list(Grammar& gram, Ast* root, Context& ctx)
{

    for (int i = 0; i < root->get_num_children(); i++)
    {

        if (i == 0)
        {
            ctx.first = true;
        }
        else
        {
            ctx.first = false;
        }

        if (i == root->get_num_children() - 1)
        {
            ctx.last = true;
        }
        else
        {
            ctx.last = false;
        }

        handle_extract(gram, root->get_child(i), ctx);

    }

}

//
//  handle_lookaheads                                                 
//  -----------------                                                 
//                                                                    
//  Lookaheads are the `k' in LALR(k). It's just an integer (hopefully a 
//  small one) that we set in the parser generator.                   
//

void Grammar::handle_lookaheads(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstLookaheads) != ctx.processed_set.end())
    {
        gram.errh.add_error(ErrorType::ErrorDupGrammarOption,
                            root->get_location(),
                            "Duplicate lookaheads option");
        return;
    }

    ctx.processed_set.insert(AstType::AstLookaheads);
    handle_extract(gram, root->get_child(0), ctx);
    gram.max_lookaheads = ctx.integer_value;

}

//
//  handle_conflicts                                                      
//  ----------------                                                      
//                                                                        
//  The user is allowed to specify a maximum number of conflicts          
//  acceptable. Hopefully this is 0 because the resulting parser is a bit 
//  shaky if this option is used. But Yacc allows it so we might as well. 
//

void Grammar::handle_conflicts(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstConflicts) != ctx.processed_set.end())
    {
        gram.errh.add_error(ErrorType::ErrorDupGrammarOption,
                            root->get_location(),
                            "Duplicate conflicts option");
        return;
    }

    ctx.processed_set.insert(AstType::AstConflicts);
    handle_extract(gram, root->get_child(0), ctx);
    gram.expected_conflicts = ctx.integer_value;

}

//
//  handle_error_recovery                                                  
//  ---------------------                                                  
//                                                                         
//  Error recovery determines whether we automatically recover from syntax 
//  errors.                                                                
//

void Grammar::handle_error_recovery(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstErrorRecovery) != ctx.processed_set.end())
    {
        gram.errh.add_error(ErrorType::ErrorDupGrammarOption,
                            root->get_location(),
                            "Duplicate error_recovery option");
        return;
    }

    ctx.processed_set.insert(AstType::AstErrorRecovery);
    handle_extract(gram, root->get_child(0), ctx);
    gram.error_recovery = ctx.bool_value;

}

//
//  handle_keep_whitespace                                          
//  ----------------------                                          
//                                                                 
//  Keep whitespace disables automatic whitespace skipping.
//

void Grammar::handle_keep_whitespace(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstKeepWhitespace) != ctx.processed_set.end())
    {
        gram.errh.add_error(ErrorType::ErrorDupGrammarOption,
                            root->get_location(),
                            "Duplicate keep_whitespace option");
        return;
    }

    ctx.processed_set.insert(AstType::AstKeepWhitespace);
    handle_extract(gram, root->get_child(0), ctx);
    gram.keep_whitespace = ctx.bool_value;

}

//
//  handle_case_sensitive                                          
//  ---------------------                                          
//                                                                 
//  Case sensitive determines whether keywords are case sensitive. 
//

void Grammar::handle_case_sensitive(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstCaseSensitive) != ctx.processed_set.end())
    {
        gram.errh.add_error(ErrorType::ErrorDupGrammarOption,
                            root->get_location(),
                            "Duplicate case_sensitive option");
        return;
    }

    ctx.processed_set.insert(AstType::AstCaseSensitive);
    handle_extract(gram, root->get_child(0), ctx);
    gram.case_sensitive = ctx.bool_value;

}

//
//  handle_token_declaration                                       
//  ------------------------
//                                                                     
//  Install the token in the generator and process the option list. 
//

void Grammar::handle_token_declaration(Grammar& gram, Ast* root, Context& ctx)
{

    handle_extract(gram, root->get_child(0), ctx);

    if (gram.get_symbol(ctx.lexeme) != nullptr)
    {

        ostringstream ost;
        ost << "Duplicate declaration of token " << ctx.lexeme;

        gram.errh.add_error(ErrorType::ErrorDupToken,
                            root->get_child(0)->get_location(),
                            ost.str());

        return;

    }

    Context cctx;
    cctx.symbol = gram.create_symbol(ctx.lexeme);

    cctx.symbol->string_value = ctx.string_value;
    cctx.symbol->location = ctx.location;

    handle_extract(gram, root->get_child(1), cctx);

    cctx.symbol->is_scanned = true;
    if (!cctx.symbol->is_ignored && !cctx.symbol->is_error)
    {
        cctx.symbol->is_terminal = true;
    }

    if (cctx.processed_set.find(AstType::AstTokenLexeme) == cctx.processed_set.end())
    {

        if (cctx.symbol->regex_list_ast != nullptr &&
            cctx.symbol->regex_list_ast->get_kind() != AstType::AstNull)
        {
            cctx.symbol->lexeme_needed = true;
        }
        else
        {
            cctx.symbol->lexeme_needed = false;
        }

    }

}

//
//  handle_token_option_list                                            
//  ------------------------                                            
//                                                                      
//  We have to process token option lists twice. Once for templates and 
//  once for everything else.                                           
//

void Grammar::handle_token_option_list(Grammar& gram, Ast* root, Context& ctx)
{

    for (int i = 0; i < root->get_num_children(); i++)
    {

        if (i == 0)
        {
            ctx.first = true;
        }
        else
        {
            ctx.first = false;
        }

        if (i == root->get_num_children() - 1)
        {
            ctx.last = true;
        }
        else
        {
            ctx.last = false;
        }

        if (root->get_child(i)->get_kind() == AstType::AstTokenTemplate)
        {
            handle_extract(gram, root->get_child(i), ctx);
        }

    }

    for (int i = 0; i < root->get_num_children(); i++)
    {

        if (i == 0)
        {
            ctx.first = true;
        }
        else
        {
            ctx.first = false;
        }

        if (i == root->get_num_children() - 1)
        {
            ctx.last = true;
        }
        else
        {
            ctx.last = false;
        }

        if (root->get_child(i)->get_kind() != AstType::AstTokenTemplate)
        {
            handle_extract(gram, root->get_child(i), ctx);
        }

    }

}

//
//  handle_token_template
//  ---------------------
//                                          
//  Load template parameters into the token.
//

void Grammar::handle_token_template(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstTokenTemplate) != ctx.processed_set.end())
    {

        ostringstream ost;
        ost << "Duplicate template declaration for token "
            << ctx.symbol->symbol_name;

        gram.errh.add_error(ErrorType::ErrorDupTokenOption,
                            root->get_location(),
                            ost.str());

        return;

    }

    ctx.processed_set.insert(AstType::AstTokenTemplate);

    handle_extract(gram, root->get_child(0), ctx);

    string name = ctx.lexeme.substr(1, ctx.lexeme.length() - 2);
    LibraryToken* token = LibraryToken::get_library_token(name);

    if (token == nullptr)
    {

        ostringstream ost;
        ost << "Unknown regex macro: " << name << ".";

        gram.errh.add_error(ErrorType::ErrorUnknownMacro,
                            root->get_child(0)->get_location(),
                            ost.str());

        return;

    }

    ctx.symbol->is_ignored = token->is_ignored;
    ctx.symbol->is_terminal = !token->is_ignored;
    ctx.symbol->is_scanned = true;
    ctx.symbol->description = token->description;
    ctx.symbol->precedence = token->precedence;
    ctx.symbol->lexeme_needed = token->lexeme_needed;

    ctx.symbol->is_ast_synthesized = true;

    Ast* token_regex_list_ast = new Ast(1); 
    token_regex_list_ast->set_kind(AstType::AstTokenRegexList);
    token_regex_list_ast->set_location(-1);
    ctx.symbol->regex_list_ast = token_regex_list_ast;

    Ast* token_regex_ast = new Ast(2); 
    token_regex_ast->set_kind(AstType::AstTokenRegex);
    token_regex_ast->set_location(-1);
    token_regex_list_ast->set_child(0, token_regex_ast);

    Ast* token_regex_guard_ast = new Ast(0); 
    token_regex_guard_ast->set_kind(AstType::AstNull);
    token_regex_guard_ast->set_location(-1);
    token_regex_ast->set_child(0, token_regex_guard_ast);

    token_regex_ast->set_child(1, gram.prsi.parse_library_regex(token->regex_string));

}

//
//  handle_token_description              
//  ------------------------
//                                          
//  Store the description with the token.
//

void Grammar::handle_token_description(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstTokenDescription) != ctx.processed_set.end())
    {

        ostringstream ost;
        ost << "Duplicate description declaration for token "
            << ctx.symbol->symbol_name;

        gram.errh.add_error(ErrorType::ErrorDupTokenOption,
                            root->get_location(),
                            ost.str());

        return;

    }

    ctx.processed_set.insert(AstType::AstTokenDescription);
    handle_extract(gram, root->get_child(0), ctx);
    ctx.symbol->description = ctx.string_value;

}

//
//  handle_token_regex_list                                       
//  -----------------------                                       
//                                                                
//  We'll handle the regular expressions later so for now just save the Ast. 
//

void Grammar::handle_token_regex_list(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstTokenRegexList) != ctx.processed_set.end())
    {

        ostringstream ost;
        ost << "Duplicate regex declaration for token "
            << ctx.symbol->symbol_name;

        gram.errh.add_error(ErrorType::ErrorDupTokenOption,
                            root->get_location(),
                            ost.str());

        return;

    }

    ctx.processed_set.insert(AstType::AstTokenRegexList);
    delete ctx.symbol->regex_list_ast;
    ctx.symbol->regex_list_ast = root;

}

//
//  handle_token_precedence              
//  -----------------------
//                                          
//  Store the precedence with the token. 
//

void Grammar::handle_token_precedence(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstTokenPrecedence) != ctx.processed_set.end())
    {

        ostringstream ost;
        ost << "Duplicate precedence declaration for token "
            << ctx.symbol->symbol_name;

        gram.errh.add_error(ErrorType::ErrorDupTokenOption,
                            root->get_location(),
                            ost.str());

        return;

    }

    ctx.processed_set.insert(AstType::AstTokenPrecedence);
    handle_extract(gram, root->get_child(0), ctx);
    ctx.symbol->precedence = ctx.integer_value;

}

//
//  handle_token_action                                       
//  -------------------                                       
//                                                                
//  We'll handle actions later so for now just save the Ast. 
//

void Grammar::handle_token_action(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstTokenAction) != ctx.processed_set.end())
    {

        ostringstream ost;
        ost << "Duplicate action declaration for token "
            << ctx.symbol->symbol_name;

        gram.errh.add_error(ErrorType::ErrorDupTokenOption,
                            root->get_location(),
                            ost.str());

        return;

    }

    ctx.processed_set.insert(AstType::AstTokenAction);
    ctx.symbol->action_ast = root->get_child(0);

}

//
//  handle_token_lexeme
//  -------------------
//                                              
//  Store whether the token needs a lexeme.
//

void Grammar::handle_token_lexeme(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstTokenLexeme) != ctx.processed_set.end())
    {

        ostringstream ost;
        ost << "Duplicate lexeme declaration for token "
            << ctx.symbol->symbol_name;

        gram.errh.add_error(ErrorType::ErrorDupTokenOption,
                            root->get_location(),
                            ost.str());

        return;

    }

    ctx.processed_set.insert(AstType::AstTokenLexeme);
    handle_extract(gram, root->get_child(0), ctx);
    ctx.symbol->lexeme_needed = ctx.bool_value;

}

//
//  handle_token_ignore                      
//  -------------------
//                                              
//  Store whether the token should be ignored. 
//

void Grammar::handle_token_ignore(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstTokenIgnore) != ctx.processed_set.end())
    {

        ostringstream ost;
        ost << "Duplicate ignore declaration for token "
            << ctx.symbol->symbol_name;

        gram.errh.add_error(ErrorType::ErrorDupTokenOption,
                            root->get_location(),
                            ost.str());

        return;

    }

    ctx.processed_set.insert(AstType::AstTokenIgnore);
    handle_extract(gram, root->get_child(0), ctx);
    ctx.symbol->is_ignored = ctx.bool_value;

}

//
//  handle_token_error
//  ------------------
//                                              
//  Store whether the token should return an error.
//

void Grammar::handle_token_error(Grammar& gram, Ast* root, Context& ctx)
{

    if (ctx.processed_set.find(AstType::AstTokenError) != ctx.processed_set.end())
    {

        ostringstream ost;
        ost << "Duplicate error declaration for token "
            << ctx.symbol->symbol_name;

        gram.errh.add_error(ErrorType::ErrorDupTokenOption,
                            root->get_location(),
                            ost.str());

        return;

    }

    ctx.processed_set.insert(AstType::AstTokenError);
    handle_extract(gram, root->get_child(0), ctx);
    ctx.symbol->is_error = true;
    ctx.symbol->error_message = ctx.string_value;

}

//
//  handle_rule                                         
//  -----------                                         
//                                                       
//  Convert from EBNF to BNF and store the rule in the Grammar. 
//

void Grammar::handle_rule(Grammar& gram, Ast* root, Context& ctx)
{

    Context cctx;

    cctx.location = root->get_location();
    handle_extract(gram, root->get_child(0), cctx);
    cctx.lhs = cctx.symbol;

    cctx.ast_former_ast = root->get_child(2);
    cctx.action_ast = root->get_child(3);

    handle_extract(gram, root->get_child(1), cctx);

}

//
//  handle_rule_rhs                                                 
//  ---------------                                                 
//                                                                        
//  Handling of or expressions is weird in EBNF. They basically slurp up  
//  as much as they can in all directions. So we call an rhs expression a 
//  list of symbols, and an rhs expression list several such lists        
//  separated by or's.                                                    
//                                                                        
//  The rhs expression lists can be handled by the pass-through function. 
//  Here we have an rhs expression.                                       
//

void Grammar::handle_rule_rhs(Grammar& gram, Ast* root, Context& ctx)
{

    ctx.rule = gram.add_rule();
    ctx.rule->location = ctx.location;
    ctx.rule->lhs = ctx.lhs;
    ctx.rule->ast_former_ast = ctx.ast_former_ast;
    ctx.rule->action_ast = ctx.action_ast;

    for (int i = 0; i < root->get_num_children(); i++)
    {
        handle_extract(gram, root->get_child(i), ctx);
        ctx.rule->rhs.push_back(ctx.symbol);
    }

}

//
//  handle_optional                                                    
//  ---------------                                                    
//                                                                     
//  For optional terms (like t?) create a synthetic lhs with two rules 
//  yielding either t or epsilon.                                      
//

void Grammar::handle_optional(Grammar& gram, Ast* root, Context& ctx)
{

    Context cctx;

    for (int i = 1; ; i++)
    {

        ostringstream ost;
        auto n = ctx.lhs->symbol_name.find(":");

        ost << ((n == string::npos) ? ctx.lhs->symbol_name :
                ctx.lhs->symbol_name.substr(0, n))
            << ":" << i;

        if (gram.get_symbol(ost.str()) == nullptr)
        {
            cctx.lhs = gram.create_symbol(ost.str());
            cctx.lhs->is_nonterminal = true;
            break;
        }

    }

    cctx.ast_former_ast = nullptr;
    cctx.action_ast = nullptr;

    cctx.rule = gram.add_rule();
    cctx.rule->location = cctx.location;
    cctx.rule->ast_former_ast = nullptr;
    cctx.rule->lhs = cctx.lhs;
    cctx.rule->action_ast = nullptr;

    handle_extract(gram, root->get_child(0), cctx);
    cctx.rule->rhs.push_back(cctx.symbol);

    cctx.rule = gram.add_rule();
    cctx.rule->location = cctx.location;
    cctx.rule->lhs = cctx.lhs;
    cctx.rule->action_ast = nullptr;
    cctx.rule->rhs.push_back(gram.epsilon_symbol);

    Ast* former_ast = new Ast(1); 
    former_ast->set_kind(AstType::AstAstFormer);
    former_ast->set_location(cctx.rule->location);
    cctx.rule->ast_former_ast = former_ast;
    cctx.rule->is_ast_synthesized = true;

    Ast* kind_ast = new Ast(0);
    kind_ast->set_kind(AstType::AstIdentifier);
    kind_ast->set_location(cctx.rule->location);
    kind_ast->set_lexeme("Null");
    former_ast->set_child(0, kind_ast);

    ctx.symbol = cctx.lhs;
    
}

//
//  handle_zero_closure                                                  
//  -------------------                                                  
//                                                                       
//  Kleene closure (denoted t*). We create rules which give zero or more 
//  copies of the operand.                                               
//

void Grammar::handle_zero_closure(Grammar& gram, Ast* root, Context& ctx)
{

    Context cctx;

    for (int i = 1; ; i++)
    {

        ostringstream ost;
        auto n = ctx.lhs->symbol_name.find(":");

        ost << ((n == string::npos) ? ctx.lhs->symbol_name :
                ctx.lhs->symbol_name.substr(0, n))
            << ":" << i;

        if (gram.get_symbol(ost.str()) == nullptr)
        {
            cctx.lhs = gram.create_symbol(ost.str());
            cctx.lhs->is_nonterminal = true;
            break;
        }

    }

    cctx.ast_former_ast = nullptr;
    cctx.action_ast = nullptr;

    cctx.rule = gram.add_rule();
    cctx.rule->location = ctx.location;
    cctx.rule->lhs = cctx.lhs;
    cctx.rule->action_ast = nullptr;

    cctx.rule->rhs.push_back(cctx.lhs);
    handle_extract(gram, root->get_child(0), cctx);
    cctx.rule->rhs.push_back(cctx.symbol);

    Ast* former_ast = new Ast(3); 
    former_ast->set_kind(AstType::AstAstFormer);
    former_ast->set_location(cctx.rule->location);
    cctx.rule->ast_former_ast = former_ast;
    cctx.rule->is_ast_synthesized = true;

    Ast* kind_ast = new Ast(0);
    kind_ast->set_kind(AstType::AstIdentifier);
    kind_ast->set_location(cctx.rule->location);
    kind_ast->set_lexeme("Unknown");
    former_ast->set_child(0, kind_ast);

    Ast* child_ast = new Ast(2);
    child_ast->set_kind(AstType::AstAstChild);
    child_ast->set_location(cctx.rule->location);
    former_ast->set_child(1, child_ast);

    Ast* dot_ast = new Ast(1);
    dot_ast->set_kind(AstType::AstAstDot);
    dot_ast->set_location(cctx.rule->location);
    child_ast->set_child(0, dot_ast);

    Ast* integer_ast = new Ast(0);
    integer_ast->set_kind(AstType::AstInteger);
    integer_ast->set_location(cctx.rule->location);
    integer_ast->set_lexeme("1");
    dot_ast->set_child(0, integer_ast);

    Ast* slice_ast = new Ast(2);
    slice_ast->set_kind(AstType::AstAstSlice);
    slice_ast->set_location(cctx.rule->location);
    child_ast->set_child(1, slice_ast);

    integer_ast = new Ast(0);
    integer_ast->set_kind(AstType::AstInteger);
    integer_ast->set_location(cctx.rule->location);
    integer_ast->set_lexeme("1");
    slice_ast->set_child(0, integer_ast);

    integer_ast = new Ast(0);
    integer_ast->set_kind(AstType::AstNegativeInteger);
    integer_ast->set_location(cctx.rule->location);
    integer_ast->set_lexeme("1");
    slice_ast->set_child(1, integer_ast);

    child_ast = new Ast(2);
    child_ast->set_kind(AstType::AstAstChild);
    child_ast->set_location(cctx.rule->location);
    former_ast->set_child(2, child_ast);

    dot_ast = new Ast(1);
    dot_ast->set_kind(AstType::AstAstDot);
    dot_ast->set_location(cctx.rule->location);
    child_ast->set_child(0, dot_ast);

    integer_ast = new Ast(0);
    integer_ast->set_kind(AstType::AstInteger);
    integer_ast->set_location(cctx.rule->location);
    integer_ast->set_lexeme("2");
    dot_ast->set_child(0, integer_ast);

    slice_ast = new Ast(0);
    slice_ast->set_kind(AstType::AstNull);
    slice_ast->set_location(cctx.rule->location);
    child_ast->set_child(1, slice_ast);

    cctx.rule = gram.add_rule();
    cctx.rule->location = ctx.location;
    cctx.rule->lhs = cctx.lhs;
    cctx.rule->action_ast = nullptr;
    cctx.rule->rhs.push_back(gram.epsilon_symbol);

    former_ast = new Ast(1); 
    former_ast->set_kind(AstType::AstAstFormer);
    former_ast->set_location(cctx.rule->location);
    cctx.rule->ast_former_ast = former_ast;
    cctx.rule->is_ast_synthesized = true;

    kind_ast = new Ast(0);
    kind_ast->set_kind(AstType::AstIdentifier);
    kind_ast->set_location(cctx.rule->location);
    kind_ast->set_lexeme("Unknown");
    former_ast->set_child(0, kind_ast);

    ctx.symbol = cctx.lhs;
    
}

//
//  handle_one_closure                                             
//  ------------------                                             
//                                                                       
//  Kleene closure (denoted t+). We create rules which give one or 
//  more copies of the operand.                                    
//

void Grammar::handle_one_closure(Grammar& gram, Ast* root, Context& ctx)
{

    Context cctx;

    for (int i = 1; ; i++)
    {

        ostringstream ost;
        auto n = ctx.lhs->symbol_name.find(":");

        ost << ((n == string::npos) ? ctx.lhs->symbol_name :
                ctx.lhs->symbol_name.substr(0, n))
            << ":" << i;

        if (gram.get_symbol(ost.str()) == nullptr)
        {
            cctx.lhs = gram.create_symbol(ost.str());
            cctx.lhs->is_nonterminal = true;
            break;
        }

    }

    cctx.ast_former_ast = nullptr;
    cctx.action_ast = nullptr;

    cctx.rule = gram.add_rule();
    cctx.rule->location = ctx.location;
    cctx.rule->lhs = cctx.lhs;
    cctx.rule->action_ast = nullptr;

    cctx.rule->rhs.push_back(cctx.lhs);
    handle_extract(gram, root->get_child(0), cctx);
    cctx.rule->rhs.push_back(cctx.symbol);

    Ast* former_ast = new Ast(3); 
    former_ast->set_kind(AstType::AstAstFormer);
    former_ast->set_location(cctx.rule->location);
    cctx.rule->ast_former_ast = former_ast;
    cctx.rule->is_ast_synthesized = true;

    Ast* kind_ast = new Ast(0);
    kind_ast->set_kind(AstType::AstIdentifier);
    kind_ast->set_location(cctx.rule->location);
    kind_ast->set_lexeme("Unknown");
    former_ast->set_child(0, kind_ast);

    Ast* child_ast = new Ast(2);
    child_ast->set_kind(AstType::AstAstChild);
    child_ast->set_location(cctx.rule->location);
    former_ast->set_child(1, child_ast);

    Ast* dot_ast = new Ast(1);
    dot_ast->set_kind(AstType::AstAstDot);
    dot_ast->set_location(cctx.rule->location);
    child_ast->set_child(0, dot_ast);

    Ast* integer_ast = new Ast(0);
    integer_ast->set_kind(AstType::AstInteger);
    integer_ast->set_location(cctx.rule->location);
    integer_ast->set_lexeme("1");
    dot_ast->set_child(0, integer_ast);

    Ast* slice_ast = new Ast(2);
    slice_ast->set_kind(AstType::AstAstSlice);
    slice_ast->set_location(cctx.rule->location);
    child_ast->set_child(1, slice_ast);

    integer_ast = new Ast(0);
    integer_ast->set_kind(AstType::AstInteger);
    integer_ast->set_location(cctx.rule->location);
    integer_ast->set_lexeme("1");
    slice_ast->set_child(0, integer_ast);

    integer_ast = new Ast(0);
    integer_ast->set_kind(AstType::AstNegativeInteger);
    integer_ast->set_location(cctx.rule->location);
    integer_ast->set_lexeme("1");
    slice_ast->set_child(1, integer_ast);

    child_ast = new Ast(2);
    child_ast->set_kind(AstType::AstAstChild);
    child_ast->set_location(cctx.rule->location);
    former_ast->set_child(2, child_ast);

    dot_ast = new Ast(1);
    dot_ast->set_kind(AstType::AstAstDot);
    dot_ast->set_location(cctx.rule->location);
    child_ast->set_child(0, dot_ast);

    integer_ast = new Ast(0);
    integer_ast->set_kind(AstType::AstInteger);
    integer_ast->set_location(cctx.rule->location);
    integer_ast->set_lexeme("2");
    dot_ast->set_child(0, integer_ast);

    slice_ast = new Ast(0);
    slice_ast->set_kind(AstType::AstNull);
    slice_ast->set_location(cctx.rule->location);
    child_ast->set_child(1, slice_ast);

    cctx.rule = gram.add_rule();
    cctx.rule->location = ctx.location;
    cctx.rule->lhs = cctx.lhs;
    cctx.rule->action_ast = nullptr;
    cctx.rule->rhs.push_back(cctx.symbol);

    former_ast = new Ast(2); 
    former_ast->set_kind(AstType::AstAstFormer);
    former_ast->set_location(cctx.rule->location);
    cctx.rule->ast_former_ast = former_ast;
    cctx.rule->is_ast_synthesized = true;

    kind_ast = new Ast(0);
    kind_ast->set_kind(AstType::AstIdentifier);
    kind_ast->set_location(cctx.rule->location);
    kind_ast->set_lexeme("Unknown");
    former_ast->set_child(0, kind_ast);

    child_ast = new Ast(2);
    child_ast->set_kind(AstType::AstAstChild);
    child_ast->set_location(cctx.rule->location);
    former_ast->set_child(1, child_ast);

    dot_ast = new Ast(1);
    dot_ast->set_kind(AstType::AstAstDot);
    dot_ast->set_location(cctx.rule->location);
    child_ast->set_child(0, dot_ast);

    integer_ast = new Ast(0);
    integer_ast->set_kind(AstType::AstInteger);
    integer_ast->set_location(cctx.rule->location);
    integer_ast->set_lexeme("1");
    dot_ast->set_child(0, integer_ast);

    slice_ast = new Ast(0);
    slice_ast->set_kind(AstType::AstNull);
    slice_ast->set_location(cctx.rule->location);
    child_ast->set_child(1, slice_ast);

    ctx.symbol = cctx.lhs;
    
}

//
//  handle_group            
//  ------------            
//                          
//  Brace delimited groups. 
//

void Grammar::handle_group(Grammar& gram, Ast* root, Context& ctx)
{

    Context cctx;

    for (int i = 1; ; i++)
    {

        ostringstream ost;
        auto n = ctx.lhs->symbol_name.find(":");

        ost << ((n == string::npos) ? ctx.lhs->symbol_name :
                ctx.lhs->symbol_name.substr(0, n))
            << ":" << i;

        if (gram.get_symbol(ost.str()) == nullptr)
        {
            cctx.lhs = gram.create_symbol(ost.str());
            cctx.lhs->is_nonterminal = true;
            break;
        }

    }
    
    cctx.ast_former_ast = root->get_child(1);
    cctx.action_ast = root->get_child(2);

    handle_extract(gram, root->get_child(0), cctx);

    ctx.symbol = cctx.lhs;
    
}

//
//  handle_rule_precedence                               
//  ----------------------                               
//                                                       
//  Generate tiered rules to handle operator precedence. 
//

void Grammar::handle_rule_precedence(Grammar& gram, Ast* root, Context& ctx)
{

    Context cctx;

    handle_extract(gram, root->get_child(0), cctx);
    cctx.lhs = cctx.symbol;

    handle_extract(gram, root->get_child(1), cctx);
    cctx.rhs_term = cctx.symbol;

    handle_extract(gram, root->get_child(2), cctx);

}

//
//  handle_rule_precedence_spec                                            
//  ---------------------------                                            
//                                                                         
//  Handle a precedence level, which is defined by a LHS symbol. Note that 
//  we have to do something slightly different on the last level and that  
//  must be provided by our caller.                                        
//

void Grammar::handle_rule_precedence_spec(Grammar& gram, Ast* root, Context& ctx)
{

    Context cctx;
    cctx.lhs = ctx.lhs;

    if (ctx.last)
    {
        cctx.rhs_term = ctx.rhs_term;
    }
    else
    {

        for (int i = 1; ; i++)
        {

            ostringstream ost;
            auto n = ctx.lhs->symbol_name.find(":");

            ost << ((n == string::npos) ? ctx.lhs->symbol_name :
                    ctx.lhs->symbol_name.substr(0, n))
                << ":" << i;

            if (gram.get_symbol(ost.str()) == nullptr)
            {
                cctx.rhs_term = gram.create_symbol(ost.str());
                cctx.rhs_term->is_nonterminal = true;
                break;
            }

        }

    }

    handle_extract(gram, root->get_child(0), cctx);
    handle_extract(gram, root->get_child(1), cctx);

    cctx.rule = gram.add_rule();
    cctx.rule->location = ctx.location;
    cctx.rule->lhs = cctx.lhs;
    cctx.rule->rhs.push_back(cctx.rhs_term);

    cctx.rule->ast_former_ast = nullptr;
    cctx.rule->action_ast = nullptr;

    ctx.lhs = cctx.rhs_term;

}

//
//  handle_rule_left_assoc                                                 
//  ----------------------                                                 
//                                                                         
//  Store a flag indicating whether we are associating to left or right at 
//  this level.                                                            
//

void Grammar::handle_rule_left_assoc(Grammar& gram, Ast* root, Context& ctx)
{
    ctx.left_assoc = true;
}

//
//  handle_rule_right_assoc                                                
//  -----------------------                                                
//                                                                         
//  Store a flag indicating whether we are associating to left or right at 
//  this level.                                                            
//

void Grammar::handle_rule_right_assoc(Grammar& gram, Ast* root, Context& ctx)
{
    ctx.left_assoc = false;
}

//
//  handle_rule_operator_spec                                     
//  -------------------------                                     
//                                                                
//  A single operator in an operator list. Create a left or right 
//  associative rule.                                             
//

void Grammar::handle_rule_operator_spec(Grammar& gram, Ast* root, Context& ctx)
{

    Context cctx;

    cctx.rule = gram.add_rule();
    cctx.rule->location = ctx.location;
    cctx.rule->lhs = ctx.lhs;

    handle_extract(gram, root->get_child(0), cctx);
    
    if (ctx.left_assoc)
    {
        cctx.rule->rhs.push_back(cctx.rule->lhs);
        cctx.rule->rhs.push_back(cctx.symbol);
        cctx.rule->rhs.push_back(ctx.rhs_term);
    }
    else
    {
        cctx.rule->rhs.push_back(ctx.rhs_term);
        cctx.rule->rhs.push_back(cctx.symbol);
        cctx.rule->rhs.push_back(cctx.rule->lhs);
    }

    cctx.rule->ast_former_ast = root->get_child(1);
    cctx.rule->action_ast = root->get_child(2);

}

//
//  handle_nonterminal_reference                                         
//  ----------------------------                                         
//                                                                       
//  A nonterminal is found in a right hand side context. We use what has 
//  been declared if one is available, otherwise we create one.          
//

void Grammar::handle_nonterminal_reference(Grammar& gram, Ast* root, Context& ctx)
{

    handle_extract(gram, root->get_child(0), ctx);

    ctx.symbol = gram.get_symbol(ctx.lexeme);
    if (ctx.symbol == nullptr)
    {
        ctx.symbol = gram.create_symbol(ctx.lexeme);
        ctx.symbol->is_nonterminal = true;
        ctx.symbol->location = ctx.location;
    }

}

//
//  handle_terminal_reference                                         
//  -------------------------                                         
//                                                                       
//  A terminal is found in a right hand side context. We use what has 
//  been declared if one is available, otherwise we create one.          
//

void Grammar::handle_terminal_reference(Grammar& gram, Ast* root, Context& ctx)
{

    handle_extract(gram, root->get_child(0), ctx);

    ctx.symbol = gram.get_symbol(ctx.lexeme);
    if (ctx.symbol == nullptr)
    {

        //
        //  If the token isn't declared try to find it in our library 
        //  tokens.                                                   
        //

        string name = ctx.lexeme.substr(1, ctx.lexeme.length() - 2);
        LibraryToken* token = LibraryToken::get_library_token(name);

        if (token != nullptr)
        {

            ctx.symbol = gram.create_symbol(ctx.lexeme);
            ctx.symbol->is_ignored = false;
            ctx.symbol->is_terminal = true;
            ctx.symbol->is_scanned = true;
            ctx.symbol->description = token->description;
            ctx.symbol->precedence = token->precedence;
            ctx.symbol->lexeme_needed = token->lexeme_needed;

            ctx.symbol->is_ast_synthesized = true;

            Ast* token_regex_list_ast = new Ast(1); 
            token_regex_list_ast->set_kind(AstType::AstTokenRegexList);
            token_regex_list_ast->set_location(-1);
            ctx.symbol->regex_list_ast = token_regex_list_ast;

            Ast* token_regex_ast = new Ast(2); 
            token_regex_ast->set_kind(AstType::AstTokenRegex);
            token_regex_ast->set_location(-1);
            token_regex_list_ast->set_child(0, token_regex_ast);

            Ast* token_regex_guard_ast = new Ast(0); 
            token_regex_guard_ast->set_kind(AstType::AstNull);
            token_regex_guard_ast->set_location(-1);
            token_regex_ast->set_child(0, token_regex_guard_ast);

            token_regex_ast->set_child(1, gram.prsi.parse_library_regex(token->regex_string));

        }
        else
        {

            //
            //  If it hasn't been declared yet then auto-declare it. 
            //

            ctx.symbol = gram.create_symbol(ctx.lexeme);
            ctx.symbol->is_terminal = true;
            ctx.symbol->is_scanned = true;
            ctx.symbol->string_value = ctx.string_value;
            ctx.symbol->location = ctx.location;

        }

    }

}

//
//  handle_empty
//  ------------
//                                                                       
//  Return an epsilon.
//

void Grammar::handle_empty(Grammar& gram, Ast* root, Context& ctx)
{
    ctx.symbol = gram.epsilon_symbol;
}

//
//  handle_identifier                                   
//  -----------------                                   
//                                                      
//  Copy the identifier string into the context record. 
//

void Grammar::handle_identifier(Grammar& gram, Ast* root, Context& ctx)
{
    ctx.lexeme = root->get_lexeme();
    ctx.location = root->get_location();
    ctx.string_value = root->get_lexeme();
}

//
//  handle_integer                                                      
//  --------------                                                      
//                                                                      
//  Copy the literal value of an integer into the context record. Saves 
//  repeating this in a lot of actions.                                 
//

void Grammar::handle_integer(Grammar& gram, Ast* root, Context& ctx)
{
    ctx.lexeme = root->get_lexeme();
    ctx.location = root->get_location();
    ctx.integer_value = atol(root->get_lexeme().c_str());
}

//
//  handle_string                               
//  -------------                               
//                                              
//  Convert string literals into internal form. 
//

void Grammar::handle_string(Grammar& gram, Ast* root, Context& ctx)
{

    ctx.lexeme = root->get_lexeme();
    ctx.location = root->get_location();

    ostringstream ost;

    const string lexeme = root->get_lexeme();
    for (const char* p = lexeme.c_str() + 1;
         p < lexeme.c_str() + lexeme.size() - 1;
         p++)
    {

        if (*p != '\\')
        {
            ost << *p;
            continue;
        }

        p++;

        switch (*p)
        {

            case '\'':
                ost << '\'';
                break;

            case '\"':
                ost << '\"';
                break;

            case '\\':
                ost << '\\';
                break;

            case '0':
                ost << '\0';
                break;

            case 'b':
                ost << '\b';
                break;

            case 'f':
                ost << '\f';
                break;

            case 'n':
                ost << '\n';
                break;

            case 'r':
                ost << '\r';
                break;

            case 't':
                ost << '\t';
                break;

            default:
                ost << *p;
                break;

        }

    }

    ctx.string_value = ost.str();

}

//
//  handle_triple_string                               
//  --------------------                               
//                                              
//  Convert triple string literals into internal form. 
//

void Grammar::handle_triple_string(Grammar& gram, Ast* root, Context& ctx)
{
    ctx.lexeme = root->get_lexeme();
    ctx.location = root->get_location();
    ctx.string_value = root->get_lexeme().substr(3, root->get_lexeme().size() - 6);
}

//
//  handle_true/false                      
//  -----------------                      
//                                         
//  Save the literal value in the context. 
//

void Grammar::handle_true(Grammar& gram, Ast* root, Context& ctx)
{
    ctx.bool_value = true;
    ctx.location = root->get_location();
}

void Grammar::handle_false(Grammar& gram, Ast* root, Context& ctx)
{
    ctx.bool_value = false;
    ctx.location = root->get_location();
}

//
//  save_parser_data                                                     
//  ----------------                                                     
//                                                                       
//  This is called sometime after we've finished updating the rule list. 
//  We store the rules in the ParserData object.                         
//

void Grammar::save_parser_data()
{

    //
    //  Build the token information tables. 
    //

    prsd.lookaheads = max_lookaheads;
    prsd.error_recovery = error_recovery;
    prsd.error_symbol_num = error_symbol->symbol_num;
    prsd.eof_symbol_num = eof_symbol->symbol_num;

    int max_symbol_num = 0;
    for (auto mp: symbol_map)
    {

        Symbol* token = mp.second;
        if (token->is_nonterminal)
        {
            continue;    
        }

        if (token->symbol_num > max_symbol_num)
        {
            max_symbol_num = token->symbol_num;
        }

    }

    prsd.token_count = max_symbol_num + 1;
    prsd.token_name_list = new string[prsd.token_count];
    prsd.token_is_terminal = new bool[prsd.token_count];
    prsd.token_kind = new int[prsd.token_count];
    prsd.token_lexeme_needed = new bool[prsd.token_count];

    for (auto mp: symbol_map)
    {

        Symbol* token = mp.second;
        if (token->is_nonterminal)
        {
            continue;    
        }

        prsd.token_name_list[token->symbol_num] = token->symbol_name;
        prsd.token_is_terminal[token->symbol_num] = token->is_terminal;
        prsd.token_kind[token->symbol_num] = prsd.get_kind_force(token->symbol_name);
        prsd.token_lexeme_needed[token->symbol_num] = token->lexeme_needed;

    }
    
    //
    //  Build the rule information tables. 
    //

    vector<int> rule_size;
    vector<int> rule_lhs;
    vector<string> rule_text;

    for (Rule* rule: rule_list)
    {

        rule_size.push_back(rule->rhs.size());
        rule_lhs.push_back(rule->lhs->symbol_num);

        ostringstream ost;
        ost << rule->lhs->symbol_name << " ::=";
        if (rule->rhs.size() == 0)
        {
            ost << " " << epsilon_symbol->symbol_name;
        }
        else
        {
            for (Symbol* symbol: rule->rhs)
            {
                ost << " " << symbol->symbol_name;
            }
        }

        rule_text.push_back(ost.str());

    }

    prsd.rule_count = rule_size.size();

    prsd.rule_size = new int[rule_size.size()];
    memcpy(static_cast<void *>(prsd.rule_size),
           static_cast<void *>(rule_size.data()),
           rule_size.size() * sizeof(int));

    prsd.rule_lhs = new int[rule_lhs.size()];
    memcpy(static_cast<void *>(prsd.rule_lhs),
           static_cast<void *>(rule_lhs.data()),
           rule_lhs.size() * sizeof(int));

    prsd.rule_text = new string[rule_text.size()];
    for (int i = 0; i < rule_text.size(); i++)
    {
        prsd.rule_text[i] = rule_text[i];
    }

}

//
//  dump_grammar                                        
//  ------------                                        
//                                                      
//  Dump the grammar after it's extracted from the Ast. 
//

void Grammar::dump_grammar(std::ostream& os, int indent) const
{

    //
    //  Global options. 
    //

    if (indent > 0)
    {
        os << setw(indent) << "" << setw(0);
    }

    os << "Global options" << endl;

    if (indent > 0)
    {
        os << setw(indent) << "" << setw(0);
    }

    os << setw(20) << left << "  Lookaheads:"
       << setw(5) << right << max_lookaheads
       << setw(0) << right << endl;

    if (indent > 0)
    {
        os << setw(indent) << "" << setw(0);
    }

    os << setw(20) << left << "  Conflicts:"
       << setw(5) << right << expected_conflicts
       << setw(0) << right << endl;

    if (indent > 0)
    {
        os << setw(indent) << "" << setw(0);
    }

    os << setw(20) << left << "  ErrorRecovery:"
       << setw(5) << right << ((error_recovery) ? "true" : "false")
       << setw(0) << right << endl;

    if (indent > 0)
    {
        os << setw(indent) << "" << setw(0);
    }

    os << setw(20) << left << "  KeepWhitespace:"
       << setw(5) << right << ((keep_whitespace) ? "true" : "false")
       << setw(0) << right << endl;

    os << setw(20) << left << "  CaseSensitive:"
       << setw(5) << right << ((case_sensitive) ? "true" : "false")
       << setw(0) << right << endl;

    os << endl;

    //
    //  Grammar.
    //

    dump_tokens(os, indent);
    dump_nonterminals(os, indent);
    dump_rules(os, indent);

}

//
//  dump_tokens                             
//  -----------                             
//                                             
//  Dump the list of tokens in the gramamr. 
//

void Grammar::dump_tokens(std::ostream& os, int indent) const
{

    if (indent > 0)
    {
        os << setw(indent) << "" << setw(0);
    }

    os << "Tokens" << endl;

    for (auto mp: symbol_map)
    {

        Symbol* symbol = mp.second;
        if (symbol->is_nonterminal)
        {
            continue;
        }

        if (indent > 0)
        {
            os << setw(indent) << "" << setw(0);
        }

        os << "  " << setw(symbol_width) << left << symbol->symbol_name;
         
        if (symbol->is_scanned)
        {
            os << " scan";
        }

        if (symbol->is_ignored)
        {
            os << " ignore";
        }

        if (symbol->is_error)
        {
            os << " error=\"" << symbol->error_message << "\"";
        }

        if (symbol->precedence != 100)
        {
            os << " precedence=" << symbol->precedence;
        }

        if (symbol->description != "")
        {
            os << " description=\"" << symbol->description << "\"";
        }

        os << endl;

        if ((debug_flags & DebugType::DebugGrammarAst) != 0 &&
            symbol->regex_list_ast != nullptr &&
            symbol->regex_list_ast->get_kind() != AstType::AstNull)
        {
            prsi.dump_grammar_ast(symbol->regex_list_ast, os, indent + 8);
            os << endl;
        }

        if ((debug_flags & DebugType::DebugGrammarAst) != 0 &&
            symbol->action_ast != nullptr &&
            symbol->action_ast->get_kind() != AstType::AstNull)
        {
            prsi.dump_grammar_ast(symbol->action_ast, os, indent + 8);
            os << endl;
        }

    }

    os << endl;

}

//
//  dump_nonterminals                             
//  -----------------                             
//                                             
//  Dump the list of nonnonterminals in the gramamr. 
//

void Grammar::dump_nonterminals(std::ostream& os, int indent) const
{

    if (indent > 0)
    {
        os << setw(indent) << "" << setw(0);
    }

    os << "Nonterminals" << endl;

    for (auto mp: symbol_map)
    {

        Symbol* symbol = mp.second;
        if (!symbol->is_nonterminal)
        {
            continue;
        }

        if (indent > 0)
        {
            os << setw(indent) << "" << setw(0);
        }

        os << "  " << symbol->symbol_name << endl;

    }

    os << endl;

}

//
//  dump_rules                           
//  ----------                           
//                                       
//  Dump the rules and associated Ast's. 
//

void Grammar::dump_rules(std::ostream& os, int indent) const
{

    if (indent > 0)
    {
        os << setw(indent) << "" << setw(0);
    }

    os << "Rules" << endl;

    for (Rule* rule: rule_list)
    {

        if (indent > 0)
        {
            os << setw(indent) << "" << setw(0);
        }

        os << "  " 
           << right << setw(4) << setfill(' ') << rule->rule_num << " "
           << left << setw(symbol_width)
           << rule->lhs->symbol_name
           << setw(0) << right << " ::=";

        if (rule->rhs.size() == 0)
        {
            os << " " << epsilon_symbol->symbol_name;
        }
        else
        {

            int width = symbol_width + 6;

            for (Symbol* symbol: rule->rhs)
            {

                if (width + symbol->symbol_name.length() > line_width - indent)
                {

                    os << setw(indent) << setfill(' ') << "" << setw(0) 
                       << endl << setw(symbol_width + 6) << " ";

                    width = symbol_width + 6;

                }

                os << " " << symbol->symbol_name;
                width = width + symbol->symbol_name.length() + 1;

            }

        }

        os << endl;

        if ((debug_flags & DebugType::DebugGrammarAst) != 0 &&
            rule->ast_former_ast != nullptr &&
            rule->ast_former_ast->get_kind() != AstType::AstNull)
        {
            prsi.dump_grammar_ast(rule->ast_former_ast, os, indent + 8);
            os << endl;
        }

        if ((debug_flags & DebugType::DebugGrammarAst) != 0 &&
            rule->action_ast != nullptr &&
            rule->action_ast->get_kind() != AstType::AstNull)
        {
            prsi.dump_grammar_ast(rule->action_ast, os, indent + 8);
            os << endl;
        }

    }

}

} // namespace hoshi

