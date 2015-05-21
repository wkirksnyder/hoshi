//
//  ParserImpl                                                             
//  ----------                                                             
//                                                                         
//  This is the counterpart to Parser. It contains all the implementation  
//  details that we would like to hide from clients.                       
//                                                                         
//  Note that the Parser class is very state-dependent. It is initialized  
//  in an Invalid state. We generate the parser via a source file, which   
//  either yields a parser or an error list. If there are no errors we can 
//  parse a source file in the described language.                         
//                                                                         
//  This class is basically a facade. If we have finished generating a     
//  parser and want to parse a source stream we call ParserEngine. If we   
//  want to generate a parser we call a sequence of helper classes to do   
//  the real work.                                                         
//

#include <cstdint>
#include <memory>
#include <exception>
#include <functional>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "Parser.H"
#include "ParserData.H"
#include "ParserEngine.H"
#include "ParserImpl.H"
#include "ErrorHandler.H"
#include "Grammar.H"
#include "Editor.H"
#include "LalrGenerator.H"
#include "CodeGenerator.H"
#include "ReduceGenerator.H"
#include "ActionGenerator.H"
#include "ScannerGenerator.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;
using namespace std::chrono;

//
//  A macro to find the size of an array.
//

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

//
//  Missing argument flag. 
//

map<string, int> ParserImpl::kind_map_missing;
string ParserImpl::string_missing{""};

//
//  Serialized grammar parsers                                         
//  --------------------------                                         
//                                                                     
//  To parse grammars we need parsers generated in a prior generation. 
//

static const char* grammar_str =
{
    "|!|}!|'!'|#\"|'$'|$\"|'%'|%\"|'&'|&\"|'('|'\"|')'|(\"|'*'|)\"|'+'|*\"|'"
    ",'|+\"|'-'|,\"|'.'|-\"|'/'|.\"|'/='|/\"|':'|0\"|'::='|1\"|'::^'|2\"|':="
    "'|3\"|';'|4\"|'<'|5\"|'<<'|6\"|'<='|7\"|'='|8\"|'=>'|9\"|'>'|:\"|'>='|;"
    "\"|'>>'|<\"|'?'|=\"|'@'|>\"|'['|?\"|']'|@\"|'_'|A\"|'action'|B\"|'case_"
    "sensitive'|C\"|'conflicts'|D\"|'description'|E\"|'dump_stack'|F\"|'empt"
    "y'|G\"|'error'|H\"|'false'|I\"|'ignore'|J\"|'keep_whitespace'|K\"|'lexe"
    "me'|L\"|'lookaheads'|M\"|'options'|N\"|'precedence'|O\"|'regex'|P\"|'ru"
    "les'|Q\"|'template'|R\"|'token_count'|S\"|'tokens'|T\"|'true'|U\"|'{','"
    "|V\"|*eof*|W\"|*epsilon*|X\"|*error*|Y\"|<bracketstring>|Z\"|<comment>|"
    "[\"|<identifier>|\\\"|<integer>|]\"|<string>|^\"|<stringerror>|_\"|<tri"
    "plestring>| #|<triplestringerror>|!#|<whitespace>|\"#|AstActionAdd|U!|A"
    "stActionAnd|Z!|AstActionAssign|N!|AstActionDivide|X!|AstActionDumpStack"
    "|]!|AstActionEqual|O!|AstActionGreaterEqual|T!|AstActionGreaterThan|S!|"
    "AstActionLessEqual|R!|AstActionLessThan|Q!|AstActionMultiply|W!|AstActi"
    "onNot|\\!|AstActionNotEqual|P!|AstActionOr|[!|AstActionStatementList|M!"
    "|AstActionSubtract|V!|AstActionTokenCount|^!|AstActionUnaryMinus|Y!|Ast"
    "AstChild|I|AstAstDot|P|AstAstFormer|G|AstAstItemList|H|AstAstKind|J|Ast"
    "AstLexeme|M|AstAstLexemeString|N|AstAstLocation|K|AstAstLocationString|"
    "L|AstAstLocator|O|AstAstSlice|Q|AstCaseSensitive|*|AstCharset|<!|AstCha"
    "rsetAltNewline|E!|AstCharsetCaret|H!|AstCharsetChar|?!|AstCharsetCr|G!|"
    "AstCharsetDash|I!|AstCharsetDigits|B!|AstCharsetDollar|J!|AstCharsetEsc"
    "ape|D!|AstCharsetInvert|=!|AstCharsetLeftBracket|K!|AstCharsetNewline|F"
    "!|AstCharsetNotDigits|C!|AstCharsetNotWhitespace|A!|AstCharsetRange|>!|"
    "AstCharsetRightBracket|L!|AstCharsetString|V|AstCharsetWhitespace|@!|As"
    "tConflicts|(|AstEmpty|F|AstErrorRecovery|'|AstFalse|^|AstFormer:2|\"\"|"
    "AstGrammar|\"|AstGroup|<|AstIdentifier|X|AstInteger|Y|AstKeepWhitespace"
    "|)|AstLookaheads|&|AstMacroString|W|AstNegativeInteger|Z|AstNonterminal"
    "Reference|E|AstNull|!|AstOneClosure|;|AstOptionList|#|AstOptional|9|Ast"
    "Options|S|AstReduceActions|T|AstRegex|_|AstRegexAltNewline|,!|AstRegexC"
    "har|%!|AstRegexCr|.!|AstRegexDigits|)!|AstRegexDollar|4!|AstRegexEscape"
    "|+!|AstRegexLeftBrace|:!|AstRegexLeftBracket|8!|AstRegexLeftParen|6!|As"
    "tRegexList|!!|AstRegexNewline|-!|AstRegexNotDigits|*!|AstRegexNotWhites"
    "pace|(!|AstRegexOneClosure|$!|AstRegexOptional|\"!|AstRegexOr| !|AstReg"
    "exPeriod|3!|AstRegexPlus|1!|AstRegexQuestion|2!|AstRegexRightBrace|;!|A"
    "stRegexRightBracket|9!|AstRegexRightParen|7!|AstRegexSpace|5!|AstRegexS"
    "tar|0!|AstRegexString|U|AstRegexVBar|/!|AstRegexWhitespace|'!|AstRegexW"
    "ildcard|&!|AstRegexZeroClosure|#!|AstRule|6|AstRuleLeftAssoc|@|AstRuleL"
    "ist|%|AstRuleOperatorList|B|AstRuleOperatorSpec|C|AstRulePrecedence|=|A"
    "stRulePrecedenceList|>|AstRulePrecedenceSpec|?|AstRuleRhs|8|AstRuleRhsL"
    "ist|7|AstRuleRightAssoc|A|AstString|[|AstTerminalReference|D|AstToken|R"
    "|AstTokenAction|2|AstTokenDeclaration|+|AstTokenDescription|.|AstTokenE"
    "rror|5|AstTokenIgnore|4|AstTokenLexeme|3|AstTokenList|$|AstTokenOptionL"
    "ist|,|AstTokenPrecedence|1|AstTokenRegex|0|AstTokenRegexList|/|AstToken"
    "Template|-|AstTripleString|\\|AstTrue|]|AstUnknown||AstZeroClosure|:|Nu"
    "ll| \"|ReduceActions|!\"|Unknown|_!|}\"|//{ -{ *//  Main Grammar{ -{ */"
    "/  ------------{ -{ *//{ -{ *//  Grammar file for the main part of the "
    "grammar.{ -{ *//{ -{ *{ -{ *options{ -{ *{ -{ *    lookaheads = 4{ -{ *"
    "    conflicts = 0{ -{ *    case_sensitive = true{ -{ *{ -{ *tokens{ -{ "
    "*{ -{ *    <comment>            : regex = ''' {'+cpp_comment{'- '''{ -{"
    " *                           ignore = true{ -{ *{ -{ *    <whitespace> "
    "        : regex = ''' {'+whitespace{'- '''{ -{ *                       "
    "    ignore = true{ -{ *{ -{ *    <integer>            : regex = ''' [0-"
    "9]+ '''{ -{ *{ -{ *    <identifier>         : regex = ''' [A-Za-z][a-zA"
    "-Z0-9_]* '''{ -{ *                           precedence = 50{ -{ *{ -{ "
    "*    <bracketstring>      : regex = [ !in_guard ] => ''' <[a-zA-Z][a-zA"
    "-Z0-9_]*> '''{ -{ *{ -{ *    <string>             : regex = ''' ' ( \\\\"
    " [^\\n] {', [^'\\\\\\n] )* ' {',{ -{ *                                 "
    "      \" ( \\\\ [^\\n] {', [^\"\\\\\\n] )* \" '''{ -{ *{ -{ *    <strin"
    "gerror>        : regex = ''' ' ( \\\\ [^\\n] {', [^'\\\\\\n] )* \\n {',"
    "{ -{ *                                       \" ( \\\\ [^\\n] {', [^\"\\"
    "\\\\n] )* \\n '''{ -{ *                           error = \"Missing clo"
    "sing quote on string literal\"{ -{ *{ -{ *    <triplestring>       : re"
    "gex = \"''' ( [^'] {', '[^'] {', ''[^'] )* '''\"{ -{ *{ -{ *    <triple"
    "stringerror>  : regex = \"''' ( [^'] {', '[^'] {', ''[^'] )*\"{ -{ *   "
    "                        error = \"Missing closing quote on triple quote"
    "d string literal\"{ -{ *{ -{ *    '['                  : action = [ in_"
    "guard := 1; ]{ -{ *{ -{ *    ']'                  : action = [ in_guard"
    " := 0; ]{ -{ *{ -{ *    '<'                  : regex = [ in_guard ] => "
    "'<'{ -{ *{ -{ *rules{ -{ *{ -{ *    //{ -{ *    //  Overall Structure{ "
    "-{ *    //  -----------------{ -{ *    //{ -{ *{ -{ *    Grammar       "
    "       ::= OptionSection?{ -{ *                             TokenSectio"
    "n?{ -{ *                             RuleSection{ -{ *                 "
    "        :   (AstGrammar, @3,{ -{ *                                 (Ast"
    "OptionList, @1, $1._),{ -{ *                                 (AstTokenL"
    "ist, @2, $2._),{ -{ *                                 (AstRuleList, @3,"
    " $3._){ -{ *                             ){ -{ *{ -{ *    //{ -{ *    /"
    "/  Option Sublanguage{ -{ *    //  ------------------{ -{ *    //{ -{ *"
    "{ -{ *    OptionSection        ::= 'options' OptionSpec*{ -{ *         "
    "                :   (AstOptionList, $2._){ -{ *{ -{ *    OptionSpec    "
    "       ::= 'lookaheads' '=' IntegerValue{ -{ *                         "
    ":   (AstLookaheads, $3){ -{ *{ -{ *    OptionSpec           ::= 'confli"
    "cts' '=' IntegerValue{ -{ *                         :   (AstConflicts, "
    "$3){ -{ *{ -{ *    OptionSpec           ::= 'keep_whitespace' '=' Boole"
    "anValue{ -{ *                         :   (AstKeepWhitespace, $3){ -{ *"
    "{ -{ *    OptionSpec           ::= 'case_sensitive' '=' BooleanValue{ -"
    "{ *                         :   (AstCaseSensitive, $3){ -{ *{ -{ *    /"
    "/{ -{ *    //  Token Sublanguage{ -{ *    //  -----------------{ -{ *  "
    "  //{ -{ *{ -{ *    TokenSection         ::= 'tokens' TokenSpec*{ -{ * "
    "                        :   (AstTokenList, $2._){ -{ *{ -{ *    TokenSp"
    "ec            ::= TerminalSymbol{ -{ *                         :   (Ast"
    "TokenDeclaration, $1, (AstTokenOptionList)){ -{ *{ -{ *    TokenSpec   "
    "         ::= TerminalSymbol ':' TokenOption*{ -{ *                     "
    "    :   (AstTokenDeclaration, $1, (AstTokenOptionList, @3, $3._)){ -{ *"
    "{ -{ *    TokenOption          ::= 'template' '=' TerminalSymbol{ -{ * "
    "                        :   (AstTokenTemplate, $3){ -{ *{ -{ *    Token"
    "Option          ::= 'description' '=' StringValue{ -{ *                "
    "         :   (AstTokenDescription, $3){ -{ *{ -{ *    TokenOption      "
    "    ::= 'regex' '=' RegexString{ -{ *                         :   (AstT"
    "okenRegexList, (AstTokenRegex, (AstNull, @\"-1\"), $3)){ -{ *{ -{ *    "
    "TokenOption          ::= 'regex' '=' TokenRegex+{ -{ *                 "
    "        :   (AstTokenRegexList, $3._){ -{ *{ -{ *    TokenRegex        "
    "   ::= '[' ActionExpression ']' '=>' RegexString{ -{ *                 "
    "        :   (AstTokenRegex, $2, $5){ -{ *{ -{ *    RegexString         "
    " ::= StringValue{ -{ *                         :   (AstRegexString, $1)"
    "{ -{ *{ -{ *    TokenOption          ::= 'precedence' '=' IntegerValue{"
    " -{ *                         :   (AstTokenPrecedence, $3){ -{ *{ -{ * "
    "   TokenOption          ::= 'action' '=' '[' ActionStatementList ']'{ -"
    "{ *                         :   (AstTokenAction, $4){ -{ *{ -{ *    Tok"
    "enOption          ::= 'lexeme' '=' BooleanValue{ -{ *                  "
    "       :   (AstTokenLexeme, $3){ -{ *{ -{ *    TokenOption          ::="
    " 'ignore' '=' BooleanValue{ -{ *                         :   (AstTokenI"
    "gnore, $3){ -{ *{ -{ *    TokenOption          ::= 'error' '=' StringVa"
    "lue{ -{ *                         :   (AstTokenError, $3){ -{ *{ -{ *  "
    "  //{ -{ *    //  Rule Sublanguage{ -{ *    //  ----------------{ -{ * "
    "   //{ -{ *{ -{ *    RuleSection          ::= 'rules'? Rule+{ -{ *     "
    "                    :   (AstRuleList, $2._){ -{ *{ -{ *    Rule        "
    "         ::= NonterminalReference '::=' RuleRhsList ReduceActions{ -{ *"
    "                         :   (AstRule, @2, $1, $3, $4.1, $4.2){ -{ *{ -"
    "{ *    ReduceActions        ::= ':' ReduceAstAction ':' ReduceGuardActi"
    "on{ -{ *                         :   ($2, $4){ -{ *{ -{ *    ReduceActi"
    "ons        ::= ':' ReduceAstAction{ -{ *                         :   ($"
    "2, (AstNull, @\"-1\")){ -{ *{ -{ *    ReduceActions        ::= empty{ -"
    "{ *                         :   ((AstNull, @\"-1\"), (AstNull, @\"-1\")"
    "){ -{ *{ -{ *    RuleRhsList          ::= RuleRhs ( '{',' RuleRhs : $2 "
    ")*{ -{ *                         :   (AstRuleRhsList, $1, $2._){ -{ *{ "
    "-{ *    RuleRhs              ::= RuleSequenceTerm+{ -{ *               "
    "          :   (AstRuleRhs, $1._){ -{ *{ -{ *    RuleSequenceTerm     ::"
    "= RuleUnopTerm '*'{ -{ *                         :   (AstZeroClosure, @"
    "2, $1){ -{ *{ -{ *    RuleSequenceTerm     ::= RuleUnopTerm '+'{ -{ *  "
    "                       :   (AstOneClosure, @2, $1){ -{ *{ -{ *    RuleS"
    "equenceTerm     ::= RuleUnopTerm '?'{ -{ *                         :   "
    "(AstOptional, @2, $1){ -{ *{ -{ *    RuleSequenceTerm     ::= RuleUnopT"
    "erm{ -{ *{ -{ *    RuleUnopTerm         ::= SymbolReference{ -{ *{ -{ *"
    "    RuleUnopTerm         ::= 'empty'{ -{ *                         :   "
    "(AstEmpty){ -{ *{ -{ *    RuleUnopTerm         ::= '(' RuleRhsList Redu"
    "ceActions ')'{ -{ *                         :   (AstGroup, $2, $3.1, $3"
    ".2){ -{ *{ -{ *    Rule                 ::= NonterminalReference '::^' "
    "SymbolReference PrecedenceSpec+{ -{ *                         :   (AstR"
    "ulePrecedence, @2, $1, $3, (AstRulePrecedenceList, @4, $4._)){ -{ *{ -{"
    " *    PrecedenceSpec       ::= Assoc Operator+{ -{ *                   "
    "      :   (AstRulePrecedenceSpec, $1, (AstRuleOperatorList, @2, $2._)){"
    " -{ *{ -{ *    Assoc                ::= '<<'{ -{ *                     "
    "    :   (AstRuleLeftAssoc){ -{ *{ -{ *    Assoc                ::= '>>'"
    "{ -{ *                         :   (AstRuleRightAssoc){ -{ *{ -{ *    O"
    "perator             ::= SymbolReference ReduceActions{ -{ *            "
    "             :   (AstRuleOperatorSpec, $1, $2.1, $2.2){ -{ *{ -{ *    S"
    "ymbolReference      ::= TerminalReference{ -{ *{ -{ *    SymbolReferenc"
    "e      ::= NonterminalReference{ -{ *{ -{ *    TerminalReference    ::="
    " TerminalSymbol{ -{ *                         :   (AstTerminalReference"
    ", $1){ -{ *{ -{ *    NonterminalReference ::= NonterminalSymbol{ -{ *  "
    "                       :   (AstNonterminalReference, $1){ -{ *{ -{ *   "
    " //{ -{ *    //  Ast Former Sublanguage{ -{ *    //  ------------------"
    "----{ -{ *    //{ -{ *{ -{ *    ReduceAstAction      ::= AstFormer{ -{ "
    "*{ -{ *    ReduceAstAction      ::= AstChild{ -{ *{ -{ *    ReduceAstAc"
    "tion      ::= empty{ -{ *                         :   (AstNull){ -{ *{ "
    "-{ *    AstFormer            ::= '(' ( AstItem ( ','? AstItem : $2 )* :"
    " ($1, $2._) )? ')'{ -{ *                         :   (AstAstFormer, $2."
    "_){ -{ *{ -{ *    AstItem              ::= AstChild{ -{ *{ -{ *    AstC"
    "hild             ::= '$' AstChildSpec '.' AstSliceSpec{ -{ *           "
    "              :   (AstAstChild, $2, $4){ -{ *{ -{ *    AstChild        "
    "     ::= '$' AstChildSpec{ -{ *                         :   (AstAstChil"
    "d, $2, (AstNull, @\"-1\")){ -{ *{ -{ *    AstChild             ::= '$' "
    "AstSliceSpec{ -{ *                         :   (AstAstChild, (AstNull, "
    "@\"-1\"), $2){ -{ *{ -{ *    AstItem              ::= Identifier{ -{ * "
    "                        :   (AstIdentifier, &1){ -{ *{ -{ *    AstItem "
    "             ::= '%' AstChildSpec{ -{ *                         :   (As"
    "tAstKind, $2){ -{ *{ -{ *    AstItem              ::= '@' AstChildSpec{"
    " -{ *                         :   (AstAstLocation, $2){ -{ *{ -{ *    A"
    "stItem              ::= '@' StringValue{ -{ *                         :"
    "   (AstAstLocationString, $2){ -{ *{ -{ *    AstItem              ::= '"
    "&' AstChildSpec{ -{ *                         :   (AstAstLexeme, $2){ -"
    "{ *{ -{ *    AstItem              ::= '&' StringValue{ -{ *            "
    "             :   (AstAstLexemeString, $2){ -{ *{ -{ *    AstItem       "
    "       ::= AstFormer{ -{ *{ -{ *    AstChildSpec         ::= AstChildNu"
    "mber ( '.' AstChildNumber : $2 )*{ -{ *                         :   (As"
    "tAstDot, $1, $2._){ -{ *{ -{ *    AstSliceSpec         ::= AstFirstChil"
    "dNumber '_' AstLastChildNumber{ -{ *                         :   (AstAs"
    "tSlice, $1, $3){ -{ *{ -{ *    AstFirstChildNumber  ::= AstChildNumber{"
    " -{ *{ -{ *    AstFirstChildNumber  ::= empty{ -{ *                    "
    "     :   (AstInteger, &\"1\"){ -{ *{ -{ *    AstLastChildNumber   ::= A"
    "stChildNumber{ -{ *{ -{ *    AstLastChildNumber   ::= empty{ -{ *      "
    "                   :   (AstNegativeInteger, &\"1\"){ -{ *{ -{ *    AstC"
    "hildNumber       ::= '-' <integer>{ -{ *                         :   (A"
    "stNegativeInteger, &2){ -{ *{ -{ *    AstChildNumber       ::= <integer"
    ">{ -{ *                         :   (AstInteger, &1){ -{ *{ -{ *    Red"
    "uceGuardAction    ::= '[' ActionStatementList ']'{ -{ *                "
    "         :   $2{ -{ *{ -{ *    //{ -{ *    //  Action Sublanguage{ -{ *"
    "    //  ------------------{ -{ *    //{ -{ *{ -{ *    ActionStatementLi"
    "st  ::= ActionStatement*{ -{ *                         :   (AstActionSt"
    "atementList, $1._){ -{ *{ -{ *    ActionStatement      ::= Identifier '"
    ":=' ActionExpression ';'{ -{ *                         :   (AstActionAs"
    "sign, @2, $1, $3){ -{ *{ -{ *    ActionStatement      ::= 'dump_stack' "
    "';'{ -{ *                         :   (AstActionDumpStack){ -{ *{ -{ * "
    "   ActionExpression     ::^ ActionUnopExpression << '{','  : (AstAction"
    "Or, $1, $2){ -{ *                                                  << '"
    "&'  : (AstActionAnd, $1, $2){ -{ *                                     "
    "             << '='  : (AstActionEqual, $1, $2){ -{ *                  "
    "                                   '/=' : (AstActionNotEqual, $1, $2){ "
    "-{ *                                                     '<'  : (AstAct"
    "ionLessThan, $1, $2){ -{ *                                             "
    "        '<=' : (AstActionLessEqual, $1, $2){ -{ *                      "
    "                               '>'  : (AstActionGreaterThan, $1, $2){ -"
    "{ *                                                     '>=' : (AstActi"
    "onGreaterEqual, $1, $2){ -{ *                                          "
    "        << '+'  : (AstActionAdd, $1, $2){ -{ *                         "
    "                            '-'  : (AstActionSubtract, $1, $2){ -{ *   "
    "                                               << '*'  : (AstActionMult"
    "iply, $1, $2){ -{ *                                                    "
    " '/'  : (AstActionDivide, $1, $2){ -{ *{ -{ *    ActionUnopExpression :"
    ":= '-' ActionUnopTerm{ -{ *                         :   (AstActionUnary"
    "Minus, $2){ -{ *{ -{ *    ActionUnopExpression ::= '!' ActionUnopTerm{ "
    "-{ *                         :   (AstActionNot, $2){ -{ *{ -{ *    Acti"
    "onUnopExpression ::= ActionUnopTerm{ -{ *{ -{ *    ActionUnopTerm      "
    " ::= '(' ActionExpression ')'{ -{ *                         :   $2{ -{ "
    "*{ -{ *    ActionUnopTerm       ::= IntegerValue{ -{ *{ -{ *    ActionU"
    "nopTerm       ::= Identifier{ -{ *{ -{ *    ActionUnopTerm       ::= 't"
    "oken_count'{ -{ *                         :   (AstActionTokenCount){ -{"
    " *{ -{ *    //{ -{ *    //  Literal Wrappers{ -{ *    //  -------------"
    "---{ -{ *    //{ -{ *{ -{ *    TerminalSymbol       ::= <string>{ -{ * "
    "                        :   (AstString, &1){ -{ *{ -{ *    TerminalSymb"
    "ol       ::= <bracketstring>{ -{ *                         :   (AstStri"
    "ng, &1){ -{ *{ -{ *    NonterminalSymbol    ::= <identifier>{ -{ *     "
    "                    :   (AstIdentifier, &1){ -{ *{ -{ *    IntegerValue"
    "         ::= <integer>{ -{ *                         :   (AstInteger, &"
    "1){ -{ *{ -{ *    BooleanValue         ::= 'true'{ -{ *                "
    "         :   (AstTrue){ -{ *{ -{ *    BooleanValue         ::= 'false'{"
    " -{ *                         :   (AstFalse){ -{ *{ -{ *    StringValue"
    "          ::= <string>{ -{ *                         :   (AstString, &1"
    "){ -{ *{ -{ *    StringValue          ::= <triplestring>{ -{ *         "
    "                :   (AstTripleString, &1){ -{ *{ -{ *    Identifier    "
    "       ::= <identifier>{ -{ *                         :   (AstIdentifie"
    "r, &1){ -{ *|}#|$|}$|!|}%|]!|}&|%|}'|$\"|}(|<identifier>|')'|<string>|<"
    "bracketstring>|'('|*eof*|'&'|'{','|':'|'-'|'>>'|'<<'|'$'|']'|'%'|'rules"
    "'|'@'|'+'|'='|','|'*'|';'|<integer>|'>'|'>='|'/='|'<'|'<='|'ignore'|'ac"
    "tion'|'description'|'lexeme'|'precedence'|'regex'|'template'|'error'|'e"
    "mpty'|'/'|'token_count'|'!'|'?'|'['|'tokens'|'_'|'.'|'conflicts'|'keep_"
    "whitespace'|'case_sensitive'|'lookaheads'|'dump_stack'|<triplestring>|'"
    "::^'|'::='|'true'|'false'|':='|'=>'|'options'||||||||||||||||||||||||||"
    "||||||||||||||||||||||||||||||||||||||||||*error*||<comment>|<whitespac"
    "e>|<stringerror>|<triplestringerror>|*epsilon*|})|!|!|!|!|!|!|!|!|!|!|!"
    "|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|"
    "!|!|!|!|!|!|!|!|!|!|!|!|||||||?!||||||||@!|J#|/!||||||A!||||||||A!|||||"
    "|||P\"|F#|+\"||||||B!||||||||Z!||||||||@!|J#|/!|||!||||||!|}*|\\\"|(\"|"
    "^\"|Z\"|'\"|W\"|&\"|V\"|0\"|,\"|<\"|6\"|$\"|@\"|%\"|Q\"|>\"|*\"|8\"|+\""
    "|)\"|4\"|]\"|:\"|;\"|/\"|5\"|7\"|J\"|B\"|E\"|L\"|O\"|P\"|R\"|H\"|G\"|.\""
    "|S\"|#\"|=\"|?\"|T\"|A\"|-\"|D\"|K\"|C\"|M\"|F\"| #|2\"|1\"|U\"|I\"|3\""
    "|9\"|N\"|I$||||||%||!~!~||||\"||!~!~||||&||0D\"||||||%||!~!~||||\"||!~!"
    "~||||%||!~!~||||\"||!~!~||||&||<,|||Y\"||[\"|\"#|_\"|!#|X\"|}+|!||!|!||"
    "|||||||||||||||||!||||!||||||||||||||||||||||||!||||||||||||||?!|||||||"
    "|@!|J#|/!||||||A!||||||||A!||||||||@#|R|0!||||||B!||||||||Z!||||||||@!|"
    "J#|/!|||||!|!|!|!||},|.\"|}-|!|#|!||!||\"|\"||#|#|#|#|\"|\"||!|#|\"||#|"
    "#|#|#|\"|!|%|!|#|%|#|#|#|\"|!||\"|!|$|$|\"||\"|\"|\"||!|\"|!|\"|\"|\"|!"
    "|!|!|$|$|\"|!|\"|\"|!|!|!|\"|!|!|!|!|!|!||#|!|\"|\"|\"|!||||!|$|\"|\"|!"
    "|\"|\"|\"|\"|\"|!|\"|\"|\"||#|!||!||\"|!|#|!|\"||$|\"|#|!|#|!|#|#|#|#|#"
    "|#|!|#|#|!|#|#|!|\"|\"|!|#|!|!|!|!|!|!|!|!|!|!|!|!|}.|^!|L!|M!|M!|O!|O!"
    "|Q!|R!|R!|S!|S!|S!|S!|Y!|Z!|Z!|[!|[!|;!|;!|V!|V!|V!|V!|H!|H!|B!|8!|V!|V"
    "!|V!|V!|V!|N!|U!|U!|P!|P!|C!|0!|0!|0!|3!|F!|\\!|F!|.!|-!|-!|+!|+!|+!|+!"
    "|*!|*!|*!|C!|D!|D!|@!|W!|W!|:!|:!|G!|$!|$!|%!|!!|6!|6!|6!|,!|=!|<!|E!|I"
    "!|J!|J!|E!|=!|5!|(!|(!|(!|5!|5!|5!|5!|5!|5!|5!|2!|A!|X!|A!|>!|?!|?!|K!|"
    "K!|'!|'!|9!|7!|4!|4!|T!|T!|/!|/!|&!|&!|#!|#!|#!|#!|#!|#!|#!|_|_|_|^|^|^"
    "|]|]|]|\\|\\|\\|\\| !| !|\"!|[|1!|1!|)!|)!|Z|}/|*accept* ::= Grammar|Gr"
    "ammar ::= Grammar:1 Grammar:2 RuleSection|Grammar:1 ::= OptionSection|G"
    "rammar:1 ::= *epsilon*|Grammar:2 ::= TokenSection|Grammar:2 ::= *epsilo"
    "n*|OptionSection ::= 'options' OptionSection:1|OptionSection:1 ::= Opti"
    "onSection:1 OptionSpec|OptionSection:1 ::= *epsilon*|OptionSpec ::= 'lo"
    "okaheads' '=' IntegerValue|OptionSpec ::= 'conflicts' '=' IntegerValue|"
    "OptionSpec ::= 'keep_whitespace' '=' BooleanValue|OptionSpec ::= 'case_"
    "sensitive' '=' BooleanValue|TokenSection ::= 'tokens' TokenSection:1|To"
    "kenSection:1 ::= TokenSection:1 TokenSpec|TokenSection:1 ::= *epsilon*|"
    "TokenSpec ::= TerminalSymbol|TokenSpec ::= TerminalSymbol ':' TokenSpec"
    ":1|TokenSpec:1 ::= TokenSpec:1 TokenOption|TokenSpec:1 ::= *epsilon*|To"
    "kenOption ::= 'template' '=' TerminalSymbol|TokenOption ::= 'descriptio"
    "n' '=' StringValue|TokenOption ::= 'regex' '=' RegexString|TokenOption "
    "::= 'regex' '=' TokenOption:1|TokenOption:1 ::= TokenOption:1 TokenRege"
    "x|TokenOption:1 ::= TokenRegex|TokenRegex ::= '[' ActionExpression ']' "
    "'=>' RegexString|RegexString ::= StringValue|TokenOption ::= 'precedenc"
    "e' '=' IntegerValue|TokenOption ::= 'action' '=' '[' ActionStatementLis"
    "t ']'|TokenOption ::= 'lexeme' '=' BooleanValue|TokenOption ::= 'ignore"
    "' '=' BooleanValue|TokenOption ::= 'error' '=' StringValue|RuleSection "
    "::= RuleSection:1 RuleSection:2|RuleSection:1 ::= 'rules'|RuleSection:1"
    " ::= *epsilon*|RuleSection:2 ::= RuleSection:2 Rule|RuleSection:2 ::= R"
    "ule|Rule ::= NonterminalReference '::=' RuleRhsList ReduceActions|Reduc"
    "eActions ::= ':' ReduceAstAction ':' ReduceGuardAction|ReduceActions ::"
    "= ':' ReduceAstAction|ReduceActions ::= *epsilon*|RuleRhsList ::= RuleR"
    "hs RuleRhsList:1|RuleRhsList:1 ::= RuleRhsList:1 RuleRhsList:2|RuleRhsL"
    "ist:2 ::= '{',' RuleRhs|RuleRhsList:1 ::= *epsilon*|RuleRhs ::= RuleRhs"
    ":1|RuleRhs:1 ::= RuleRhs:1 RuleSequenceTerm|RuleRhs:1 ::= RuleSequenceT"
    "erm|RuleSequenceTerm ::= RuleUnopTerm '*'|RuleSequenceTerm ::= RuleUnop"
    "Term '+'|RuleSequenceTerm ::= RuleUnopTerm '?'|RuleSequenceTerm ::= Rul"
    "eUnopTerm|RuleUnopTerm ::= SymbolReference|RuleUnopTerm ::= 'empty'|Rul"
    "eUnopTerm ::= '(' RuleRhsList ReduceActions ')'|Rule ::= NonterminalRef"
    "erence '::^' SymbolReference Rule:1|Rule:1 ::= Rule:1 PrecedenceSpec|Ru"
    "le:1 ::= PrecedenceSpec|PrecedenceSpec ::= Assoc PrecedenceSpec:1|Prece"
    "denceSpec:1 ::= PrecedenceSpec:1 Operator|PrecedenceSpec:1 ::= Operator"
    "|Assoc ::= '<<'|Assoc ::= '>>'|Operator ::= SymbolReference ReduceActio"
    "ns|SymbolReference ::= TerminalReference|SymbolReference ::= Nontermina"
    "lReference|TerminalReference ::= TerminalSymbol|NonterminalReference ::"
    "= NonterminalSymbol|ReduceAstAction ::= AstFormer|ReduceAstAction ::= A"
    "stChild|ReduceAstAction ::= *epsilon*|AstFormer ::= '(' AstFormer:1 ')'"
    "|AstFormer:1 ::= AstFormer:2|AstFormer:2 ::= AstItem AstFormer:3|AstFor"
    "mer:3 ::= AstFormer:3 AstFormer:4|AstFormer:4 ::= AstFormer:5 AstItem|A"
    "stFormer:5 ::= ','|AstFormer:5 ::= *epsilon*|AstFormer:3 ::= *epsilon*|"
    "AstFormer:1 ::= *epsilon*|AstItem ::= AstChild|AstChild ::= '$' AstChil"
    "dSpec '.' AstSliceSpec|AstChild ::= '$' AstChildSpec|AstChild ::= '$' A"
    "stSliceSpec|AstItem ::= Identifier|AstItem ::= '%' AstChildSpec|AstItem"
    " ::= '@' AstChildSpec|AstItem ::= '@' StringValue|AstItem ::= '&' AstCh"
    "ildSpec|AstItem ::= '&' StringValue|AstItem ::= AstFormer|AstChildSpec "
    "::= AstChildNumber AstChildSpec:1|AstChildSpec:1 ::= AstChildSpec:1 Ast"
    "ChildSpec:2|AstChildSpec:2 ::= '.' AstChildNumber|AstChildSpec:1 ::= *e"
    "psilon*|AstSliceSpec ::= AstFirstChildNumber '_' AstLastChildNumber|Ast"
    "FirstChildNumber ::= AstChildNumber|AstFirstChildNumber ::= *epsilon*|A"
    "stLastChildNumber ::= AstChildNumber|AstLastChildNumber ::= *epsilon*|A"
    "stChildNumber ::= '-' <integer>|AstChildNumber ::= <integer>|ReduceGuar"
    "dAction ::= '[' ActionStatementList ']'|ActionStatementList ::= ActionS"
    "tatementList:1|ActionStatementList:1 ::= ActionStatementList:1 ActionSt"
    "atement|ActionStatementList:1 ::= *epsilon*|ActionStatement ::= Identif"
    "ier ':=' ActionExpression ';'|ActionStatement ::= 'dump_stack' ';'|Acti"
    "onExpression ::= ActionExpression '{',' ActionExpression:1|ActionExpres"
    "sion ::= ActionExpression:1|ActionExpression:1 ::= ActionExpression:1 '"
    "&' ActionExpression:2|ActionExpression:1 ::= ActionExpression:2|ActionE"
    "xpression:2 ::= ActionExpression:2 '=' ActionExpression:3|ActionExpress"
    "ion:2 ::= ActionExpression:2 '/=' ActionExpression:3|ActionExpression:2"
    " ::= ActionExpression:2 '<' ActionExpression:3|ActionExpression:2 ::= A"
    "ctionExpression:2 '<=' ActionExpression:3|ActionExpression:2 ::= Action"
    "Expression:2 '>' ActionExpression:3|ActionExpression:2 ::= ActionExpres"
    "sion:2 '>=' ActionExpression:3|ActionExpression:2 ::= ActionExpression:"
    "3|ActionExpression:3 ::= ActionExpression:3 '+' ActionExpression:4|Acti"
    "onExpression:3 ::= ActionExpression:3 '-' ActionExpression:4|ActionExpr"
    "ession:3 ::= ActionExpression:4|ActionExpression:4 ::= ActionExpression"
    ":4 '*' ActionUnopExpression|ActionExpression:4 ::= ActionExpression:4 '"
    "/' ActionUnopExpression|ActionExpression:4 ::= ActionUnopExpression|Act"
    "ionUnopExpression ::= '-' ActionUnopTerm|ActionUnopExpression ::= '!' A"
    "ctionUnopTerm|ActionUnopExpression ::= ActionUnopTerm|ActionUnopTerm ::"
    "= '(' ActionExpression ')'|ActionUnopTerm ::= IntegerValue|ActionUnopTe"
    "rm ::= Identifier|ActionUnopTerm ::= 'token_count'|TerminalSymbol ::= <"
    "string>|TerminalSymbol ::= <bracketstring>|NonterminalSymbol ::= <ident"
    "ifier>|IntegerValue ::= <integer>|BooleanValue ::= 'true'|BooleanValue "
    "::= 'false'|StringValue ::= <string>|StringValue ::= <triplestring>|Ide"
    "ntifier ::= <identifier>|}0|!~:&|!~W&|!~]&|#'|+'|5'|;'|C'|K'|S'|['|#(|-"
    "(|3(|>(|M(|W(|](|%)|-)|<)|D)|N)|V)| *|(*|0*|8*|@*|H*|P*|!~X*|^*|(+|0+|B"
    "+|L+|X+|&,|0,|:,|?,|E,|M,|W,|_,|)-|3-|!~!~=-|C-|Q-|$.|..|6.|E.|O.|W.|]."
    "|#/|!~!~1/|9/|!~!~A/|G/|!~O/|Y/|#0|!~(0|.0|40|!~:0|D0|P0|\\0|$1|,1|41|<"
    "1|D1|!~L1|V1| 2|%2|+2|!~52|!~<2|C2|K2|S2|X2| 3|*3|03|<3|B3|!~L3|!~V3| 4"
    "|*4|44|>4|H4|!~R4|\\4|!~&5|05|!~:5|B5|!~J5|!~!~O5|U5|]5|%6|-6|56|;6|A6|"
    "I6|Q6|}1|\"|}2||}3|<#|}4|;$|}5|D>|\"%|S<|?$|O]|0)|[&|::|#@|T&|6#|$#|N.|"
    "L2|%#|V#|'%|@%|&<|I2|X$|R!|;<|3)|6.|D%|\"2|M#|L*|<-|X&|L,|[>|'R|EN|H%|L"
    "7|@U|?W|R,|09|>_|!M|AK|V%|6-|&:|7L|_Y|JY|,R|U?|NI|,,|QP|A+|5,|O8|+7|\"7"
    "|X/|'.|&'|.6|I(|0\\|A\"|2#|^Z|E(|D(|3(|#(|!(|P'|8'|7'|UN|@P|!Q|51|,V|R2"
    "|84|+)|E,|9V|^S|:B|/)|?%|G*|O#|J!|\\'|>%|&%|C%|-0|\"W|C[|6%|%$|!N|E^|S7"
    "|&E|._|D=|!I|EJ|89|@(|:%|\\*|;F|P$|5+|1V|]1|%3|)V|E@|9S|!X|[M|NS|7X|D/|"
    "LX|\"Y|7Y|+-|/(|YJ|?S|B_|0=|N^|S_|9_|(_|^X|L:|I6|^=|[A|+'|%B|&C|#)|AC|)"
    "A|4\\|]R|\\C|J5|22|5?|9R|#&|P4|;'|_^|M=|C-|-K|*#|M,|&G|5I|XK|\\$|>I|*+|"
    "&@|PQ|!W|[V|UV|(Z|\"X|6Z|-\\|V\\|/]|6]|@B|5Z|>&|O$|Y%|U&|4^|MA|!,|IB| A"
    "|VI| P|DO|)O|O<|MU|2U|*H|OG|4G|PE|JE|@3|NT|6$|U(|,L|//|X(|M;|Q#|9;||H/|"
    ":*|.5|9&|O3|W:|B$|0$|S'|Q7|:6|B[|<,|X-|$-|'4|#8|W*|]!|<(|]3|4$|G[|2T|RD"
    "|5%|-E|7D|CL|^8|RF|8M|;Q|LM|V9|Y0|\"'|2>|M>|-*|L0|<P|F'|.;|@)|RT|A2|97|"
    "_.|BQ|NO|N\"|&+|@4|'?|A1|&)|?#|EH|X3|0N|.+|}6|X!!|}7| 0  '|!$ *'|\"$ @'"
    "|#$ N|$$ ('|%4|&$ ^&|'$ <'|($ 2'|)$ 0'|*$ &\"|+$ $\"|,$ B\"|-$ :'|.$ 2#"
    "|/$ 6|0$ 4#|1$ .'|2$ 6'|3$ *%|4$ ,'|5$ 4'|6$ >'|7$ J%|8$ L%|9$ F%|:$ B%"
    "|;$ H%|<$ 6\"|=$ 2\"|>$ ,\"|?$ 4\"|@$ 0\"|A$ .\"|B$ *\"|C$ 8\"|D$ D!|E$"
    " T%|F$ \\$|G$ Z$|H$ Z!|I$ 8'|J$ .|K$ @$|L0 $'|M$ >|N$ @|O$ B|P$ <|Q$ ^%"
    "|R$ P#|S$ \"!|T$  !|U$ .!|V$ 0!|W$ B&|X$ &&|Y$ (|Z,  (|[, \"(|\\, P'|],"
    " N'|^, J'|_, H'| - :(|!- $(|\"- L|#- F'|$- 8(|%- H!|&- D'|'- T'|(- R'|)"
    "- 6(|*- @!|+- 4(|,- X'|-- <!|.- 0(|/- B'|0- *(|1- ^'|2- V'|3- 2(|4- \"%"
    "|5- Z'|6- :\"|7- L'|8- ,(|9- *$|:- \"\"|;- L!|<- ,#|=- *#|>- \\'|?- @#|"
    "@- ((|A- >$|B- <(|C- .(|D- ^!|E- .$|F- R!|G- &(|H- Z#|I- &%|J- (%|K- 6%"
    "|L- \"|M- $|N- 2|O- *|P- F|Q- &|R- 0|S- :|T- \\%|U- 4|V- (\"|W- N\"|X- "
    "2%|Y- ,|Z- 8|[- R|\\- D\"| 0  '|!0 *'|\"0 P|#0 N|$0 <\"|%0 X&|&0 ^&|'($"
    "!0K|(0 \"#|)($! N|*0 &\"|+0 $\"|,0 B\"|-($! K|.0 2#|/($!P\"|00 4#|1($!P"
    "M|2($!@L|30 *%|4($!0N|5($!0U|I$  $|7($!0M|8($!@M|9($!PL|:($!0L|;($! M|<"
    "($!P2|=($!02|>($!@1|?($!@2|@($! 2|A($!P1|B($!01|C($! 3| (? @$|E($!@N|\""
    "(?  &|#(? P%|S$ \"!|T$  !|J($!P!|K0 @$|L0 $'|M($!P#|N($! $|O($!0$|P($!@"
    "#| 0  '|!0 *'|\"0 P|#0 N|$(S @3|%(S  W|&0 ^&|'($!0K|((S 08|)($! N|*(S P"
    "0|+(S @0|,(S 04|-($! K|.(S 0:|/($!P\"|0(S @:|1($!PM|2($!@L|3(S 0I|4($!0"
    "N|5($!0U| (1 @$|7($!0M|8($!@M|9($!PL|:($!0L|;($! M|<($!P2|=($!02|>($!@1"
    "|?($!@2|@($! 2|A($!P1|B($!01|C($! 3| H? @$|E($!@N|\"H?  &|#H? P%| $ D|2"
    "$ V|J($!P!|K(S  D|L(S @X|M($!P#|N($! $|O($!0$|P($!@#| 0  '|!0 *'|\"0 P|"
    "#0 N|$HR @3|%HR  W|&0 ^&|'($!0K|(HR 08|)($! N|*HR P0|+HR @0|,HR 04|-($!"
    " K|.HR 0:|/($!P\"|0HR @:|1($!PM|2($!@L|3HR 0I|4($!0N|5($!0U|KHP  D|7($!"
    "0M|8($!@M|9($!PL|:($!0L|;($! M|<($!P2|=($!02|>($!@1|?($!@2|@($! 2|A($!P"
    "1|B($!01|C($! 3|6$ &!|E($!@N|6$ &!|!HD @A|6  V&|2$ X|J($!P!|KHR  D|LHR "
    "@X|M($!P#|N($! $|O($!0$|P($!@#| $  '|!(H @A|\"$ P|#$ N|$$ ('|!0 *&|&$ 6"
    "#|!- J|\"- L|)$ L$|&$ @%|'0 :%|,$ B\"|'$ :%|.$ 2#| (! @$|0$ 4#|-0 8%| 0"
    " D|!HS P<|\"0 P|#0 N|6$ &!|%HS  W|[, (!|50 J&|[, ^#|5$ J&|*HS P0|+HS @0"
    "|/(! P\"| HT @8| (@ @$|/H. P\"|\"(@  &|#(@ P%|D$ D!|%(@  W|F$ \\$|G$ Z$"
    "| H2 @$|C- H|*(@ P0|+(@ @0|-HT PX|%H2  W|<H. P2|=H. 02|>H. @1|?H. @2|@H"
    ".  2|AH. P1|BH. 01|CH.  3|P- F|%4|\"$ N#|J(! P!|Z, H(|[, D$|\\, X$|], N"
    "$|^, V$|_, T$| - 4!|!- 6!|\"- L|#- R$|$- B!|%- H!|&- P$|!$ ,$|(- (#|2$ "
    "Z|*- @!|+- >!|,- &#|-- <!|.- :!|/- <%|QHT PO|\"$ N#|-$ &'|3- \\!| (' @$"
    "|5- .#|\"('  &|#(' P%| ($!@$|!($!0Q|\"($! &|#($!P%|<- ,#|=- *#|&($! L|'"
    "($!0K|6$ B$|)($! N|2$ \\|/(' P\"|X$ &&|-($! K| H4 @$|/($!P\"|R$ P#|1($!"
    "PM|2($!@L|%H4  W|4($!0N|5($!0U|($ N!|7($!0M|8($!@M|9($!PL|:($!0L|;($! M"
    "|<($!P2|=($!02|>($!@1|?($!@2|@($! 2|A($!P1|B($!01|C($! 3|I$ ($|E($!@N|5"
    "$ D&|)- &$|-$ X%|R$ P#|J($!P!|U$ .!|V$ 0!|M($!P#|N($! $|O($!0$|P($!@#| "
    "0  '|!0 *'|\"H#! &|#H#!P%|$0 B(|%H#! W|&H&!PW|'0 <'|(H#!P-|)H&! N|*H#!P"
    "0|+H#!@0|,H&!04|-H&! K|.H&!0:|)- R#|0H&!@:|10 .'|2H&!@L|3H&!0I|40 ,'|5H"
    "&!0U|1- $$|7H&!0M|8H&!@M|9H&!PL|:H&!0L|;H&! M| HS @$|!HS P<|\"HS  &|#HS"
    " P%|%H   W|%HS  W| (\" @$|9- *$|DH#!@,|EH&!@N|*HS P0|+HS @0|HH#!0/| 0 D"
    "|!(!!0Q|\"(.  &|#(. P%|0- P!|!H4 P<|&(!! L|'(!!0K|/(\" P\"|)(!! N|SH#!0"
    "(|TH#! (|($ N!|-(!! K|WH&!0T|/0 6|'$ :%|1(!!PM|2(!!@L|W$ B&|4(!!0N|5(!!"
    "0U|-$ 8%|7(!!0M|8(!!@M|9(!!PL|:(!!0L|;(!! M|<(. P2|=(. 02|>(. @1|?(. @2"
    "|@(.  2|A(. P1|B(. 01|C(.  3| 0 D|E(!!@N|-$  &|2$  #|2$ ^\"|%0 X&|J0 .|"
    "U$ .!|V$ 0!|M0 >|N0 @|O0 B|P0 <| 0  '|!0 *'|\"0 P|#0 N|$0 B(|%0 X&|&0 ^"
    "&|'0 <'|(0 D(|)H ! N|*(D P0|+(D @0|,(D 04|-H ! K|.(D 0:|2$ \\\"|0(D @:|"
    "10 .'|2H !@L|3(D 0I|40 ,'|5H !0U|1- 2!|7H !0M|8H !@M|9H !PL|:H !0L|;H !"
    " M| H3 @$|!H3 P<|\"H3  &|#H3 P%|2$ Z\"|%H3  W|2$ X\"|0- H\"|DH; @,|EH !"
    "@N|*H3 P0|+H3 @0|HH; 0/| 0  '|!0 \"'|\"0 P|#0 N|$0 B(|%H\"! W|&H%!P:|'H"
    "\"!P4|(H\"!0W|2$ V\"|*H\"!P0|+H\"!@0|,H%!04| (= @$|.H%!0:|/0 6|0H%!@:|1"
    "H\"! /|%(=  W|3H%!0I|4H\"!P.|U$ .!|V$ 0!|*(= P0|+(= @0| HU @8|2$ T\"|2$"
    " R\"|<0 6\"|=0 2\"|>0 ,\"|?0 4\"|@0 0\"|A0 .\"|B0 *\"|C0 8\"|DH\"!@,|) "
    " T&|-HU PX| $ $#|HH\"!0/|IH%!@>| $  '|$$ H$|\"$ P|#$ N|$$ J(|K$ @$|!$ F"
    "#|1- \"$|6  R&|)$ L$| H1 @$|\"$ P|#$ N| (#!@$|!(#!P<|\"(#! &|#(#!P%|$(#"
    "!P,|%(#! W|6$ &!|'(#!P4|((#!0W|6$ &!|*(#!P0|+(#!@0|/$ 6| H4 @$|!H4 P<|/"
    "(#!P\"|K(N @C|1(#! /|%H4  W|QHU PO|4(#!P.|($ N!|F$ \\$|D$ D!|!~F$ \\$|G"
    "$ Z$|!~<(#!P2|=(#!02|>(#!@1|?(#!@2|@(#! 2|A(#!P1|B(#!01|C(#! 3|D(#!@,|!"
    "~!~!~H(#!0/|!~Z, J$|[, D$|\\, >%|Z, J$|[, D$|\\, X$|], N$|^, V$|_, T$| "
    "- 4!|!- 6!|\"- L|#- R$|$- B!|%- H!|&- P$| 0 D|!~ - L#|*- @!|+- >!|%0 X&"
    "|-- <!|.- :!|/- <%|!~*0 &\"|+0 $\"|3- \\!| (D  X|!0 *'|\"(D  &|#(D P%|$"
    "(D @3|%(D  W|&0 ^&|'H !0K|((D 08|)H ! N|*(D P0|+(D @0|,(D 04|-H ! K|.(D"
    " 0:|\"$ N#|0(D @:|1H !PM|2H !@L|3(D 0I|4H !0N|5H !0U|0- T(|7H !0M|8H !@"
    "M|9H !PL|:H !0L|;H ! M|!~ $  '|!~\"$ P|#$ N|$$ J(| (6 @$|!(6 P<|N- 2|EH"
    " !@N|)$ L$|%(6  W|6$ &!|'(6 P4|((6 P-|U- 4| $  '|!(H @A|\"$ P|#$ N|$$ B"
    "(|!$ *&|&$ 6#|6$ &!| (3 @$|!$ F#|I$ T#|'$ :%|,$ B\"|%(3  W|.$ 2#| (> @$"
    "|0$ 4#|\"(>  &|#(> P%|R$ P#|%(>  W|D$ D!|!~F$ \\$|G$ Z$|*(> P0|+(> @0| "
    "HC @$|!HC P<|\"HC  &|#HC P%|$$ <\"|%HC  W|[, *!|!~(HC 08|D$ D!|*HC P0|+"
    "HC @0|,$ B\"|!~!~)- V#|Z, J$|[, D$|\\, X$|], N$|^, V$|_, T$| - 4!|!- 6!"
    "|\"- L|#- R$|$- B!|%- H!|&- (&|!~8- X#|Z, 0#|*- @!|+- >!|\"$ N#|-- <!|."
    "- D#| - 4!|!- 6!|\"- L|B- \\#|$- B!|%- H!|!~!~(- (#|H- Z#|*- @!|+- >!|,"
    "- &#|-- <!|.- :!| (3 @$|*$ &\"|+$ $\"|!~3- \\!|%(3  W|5- .#| 0 D|!HC P<"
    "|\"0 P|#0 N|$$ <\"|%HC  W|<- ,#|=- *#|(HC 08| H< @$|*HC P0|+HC @0|,$ B\""
    "|(- @\"|%H<  W|/H) P\"| (2 @$|,- >\"|!~*H< P0|+H< @0|%(2  W| H$!@$|R$ P"
    "#|\"H$! &|#H$!P%|)$ B#|6- :\"|<H) P2|=H) 02|>H) @1|?H) @2|@H)  2|AH) P1"
    "|BH) 01|CH)  3|!~/H$!P\"|!~6$ 8#| $ $#|I$ ($|\"$ @'|#$ N|$$ H$|!~)- V#|"
    " (U @8|!~)$ L$|<H$!P2|=H$!02|>H$!@1|?H$!@2|@H$! 2|AH$!P1|BH$!01|CH$! 3|"
    " H6 @$|!H6 P<|-(U PX|8- F&|6$ &!|%H6  W|JH$!P!|'H6 P4|(H6 P-|MH$!P#|NH$"
    "! $|OH$!0$|PH$!@#| HF @8|(- @\"|!~:- \"\"|$HF @3|,- >\"|&HF P:|F$ \\$|G"
    "$ Z$|@-  \"|I$ P(|!~,HF 04|D- ^!|.HF 0:|6- :\"|0HF @:|'- $&|9- *$|R$ P#"
    "|;- L!| 0 $#|U$ .!|V$ 0!|!~Q(U PO|!~Z, J$|[, \"(|\\, X$|], N$|^, V$|_, "
    "0&| - L#|-0 &'|!~ H9 @$|!H9 P<|\"H9  &|#H9 P%|$H9 P,|%H9  W|)- R(|'H9 P"
    "4|(H9 P-|!~U$ .!|V$ 0!|!~!~1- ^'| H\"!@$|!H\"!P<|\"H\"! &|#H\"!P%|$H\"!"
    "P,|%H\"! W|8- X#|'H\"!P4|(H\"!0W|4- \"%|*H\"!P0|+H\"!@0|7- $%|!~!~/H\"!"
    "P\"|B- \\#|1H\"! /|!~Q0 ^%|4H\"!P.|DH9 @,|H- Z#|1- ,!| H& @$|!~\"$ P|#$"
    " N|<H\"!P2|=H\"!02|>H\"!@1|?H\"!@2|@H\"! 2|AH\"!P1|BH\"!01|CH\"! 3|DH\""
    "!@,|!~F- R!|/H& P\"|HH\"!0/| 0 D|!HA P<|\"0 P|#0 N|$HA P,|%HA  W|!~'HA "
    "P4|(0 Z&|!~*HA P0|+HA @0|!~!~!~/0 6| $ D|1HA  /|\"$ D|#$ D|4HA P.|%$ D|"
    "!~!~($ D|!~*$ D|+$ D|<(* P2|=(* 02|>(* @1|?(* @2|@(*  2|A(* P1|B(* 01|C"
    "(*  3|DHA @,| (U @8|!~!~HHA 0/| 0 D|!HC P<|\"0 P|#0 N|$$ <\"|%HC  W| - "
    "T|!~(HC 08|-(U PX|*HC P0|+HC @0|,$ B\"|!~!~/H) P\"| (9 @$|!(9 P<|\"(9  "
    "&|#(9 P%|$(9 P,|%(9  W|!~'(9 P4|((9 P-|!~SH= @$|TH= @$|<H) P2|=H) 02|>H"
    ") @1|?H) @2|@H)  2|AH) P1|BH) 01|CH)  3|!~ H&!@8|!H&!@W|!~!~$H&!@3|!~&H"
    "&!PW|'H&!0K|Q(U PO|)H&! N|!~!~,H&!04|-H&! K|.H&!0:|D(9 @,|0H&!@:|1H&!PM"
    "|2H&!@L|3H&!0I|4H&!0N|5H&!0U|!~7H&!0M|8H&!@M|9H&!PL|:H&!0L|;H&! M|[- R|"
    "!~!~ 0 D|!$ F#|\"(@  &|#(@ P%|(- @\"|%0 X&|EH&!@N|!~,- >\"|!~*(@ P0|+(@"
    " @0|4- \"%| HJ @8|!0 \\&|7-  %|!~$HJ @3|6- :\"|&0 ^&|'H!!0K|!~)H!! N|;-"
    " L!|WH&!0T|,HJ 04|-H!! K|.HJ 0:|!~0HJ @:|1H!!PM|2H!!@L|3HJ 0I|4H!!0N|5H"
    "!!0U|!~7H!!0M|8H!!@M|9H!!PL|:H!!0L|;H!! M| (C @$|!(C P<|\"(C  &|#(C P%|"
    "!~%(C  W|!~!~((C 08|EH!!@N|*(C P0|+(C @0| HJ @8|!0 \\&|!~!~$HJ @3|!~&0 "
    "^&|'H!!0K|!~)H!! N|!~!~,HJ 04|-H!! K|.HJ 0:|W$ B&|0HJ @:|1H!!PM|2H!!@L|"
    "3HJ 0I|4H!!0N|5H!!0U|!~7H!!0M|8H!!@M|9H!!PL|:H!!0L|;H!! M| H%! X|!H%!@A"
    "|\"H%! &|#H%!P%|$H%!@3| (( @$|&H%!P:|\"((  &|#(( P%|EH!!@N|!~!~,H%!04|("
    "$ $!|.H%!0:|/H%!P\"|0H%!@:|!~!~3H%!0I|/(( P\"| $ $#|!~!~!~$$ H$|!~!~<H%"
    "!P2|=H%!02|>H%!@1|?H%!@2|@H%! 2|AH%!P1|BH%!01|CH%! 3| 0  '|!0 ,$|\"0 P|"
    "#0 N|$0 <\"|IH%!@>|&0 6#|6$ &!| $ D|!~!~ H# @$|,0 B\"|%H0  W|.0 2#|/0 6"
    "|00 4#| H5 @$|!H5 P<|30 *%|!~!~%H5  W|F$ \\$|'H5 P4|(H5 P-|/H# P\"|!~<0"
    " 6\"|=0 2\"|>0 ,\"|?0 4\"|@0 0\"|A0 .\"|B0 *\"|C0 8\"| (&! X|!(&!@A|\"("
    "&! &|#(&!P%|$(&!@3|IH- @>|&(&!P:|Z, J$|[, D$|\\, V%|!~!~,(&!04|!~.(&!0:"
    "|/(&!P\"|0(&!@:|JH# P!|!~3(&!0I|MH# P#|NH#  $|OH# 0$|PH# @#|!~!~!~ (V @"
    "8|<(&!P2|=(&!02|>(&!@1|?(&!@2|@(&! 2|A(&!P1|B(&!01|C(&! 3|!~!- J|\"- L|"
    "!~-(V PX|I(&!@>| 0  '|!0 \"'|\"0 P|#0 N|$0 B(|%0 X&|&(D P:|'H; P4|(0 D("
    "| 0 $#|*(D P0|+(D @0|,(D 04|$$ H$|.(D 0:|!$ *&|0(D @:|1H;  /|)$ L$|3(D "
    "0I|4H; P.|'$ :%|-(U PX|!~ 0 $#|!~!~-$ 8%|$$ H$|C- ^|!~6$ &!|!~)$ L$|Q(V"
    " PO|5$ J&|DH; @,|-(U PX|!~!~HH; 0/| $ D|!~\"$ P|#$ N|$$ F!|6$ &!|F$ \\$"
    "|G$ Z$| HC @$|!HC P<|\"HC  &|#HC P%|$$ <\"|%HC  W|!~!~(HC 08|Q(U PO|*HC"
    " P0|+HC @0|,$ B\"|F$ \\$|G$ Z$|!~ $ $#|!~Z, J$|[, D$|\\, X$|], N$|^, V$"
    "|_, T$|Q(U PO|!~!~#- R$|D$ D!|-(T PX|&- P$|!~!~Z, J$|[, D$|\\, X$|], N$"
    "|^, V$|_, T$|/- F$|!~I$ ($|#- R$|!~4- \"%|&- P$| $ $#|7-  %|\"$ N#|!~$$"
    " H$|!~!~!~/- F$|)$ >(| - 4!|!- 6!|\"- L|4- \"%|$- B!|%- H!|7- L'|!~!~Q$"
    " ^%|*- @!|+- >!|6$ @(|-- <!|.- D#|!~(- @\"|!~Z, Z%|!( !0Q|,- >\"|!~!~!~"
    "&( ! L|'( !0K|!~)( ! N|F$ \\$|G$ Z$|6- :\"|-( ! K|!~9- *$|!~1( !PM|2( !"
    "@L|!~4( !0N|5( !0U|R$ P#|7( !0M|8( !@M|9( !PL|:( !0L|;( ! M|!~!~Z, J$|["
    ", D$|\\, X$|], N$|^, V$|_, T$| (< @$|E( !@N|!~#- ,&|!~%(<  W|!~'- 2$|!~"
    ")- 8$|*$ &\"|+$ $\"| 0  '|!(9 P<|\"(9  &|#(9 P%|$0 J(|%(9  W|2- :$|'(9 "
    "P4|((9 P-|)$ L$|T- \\%|!~!~!~!~ (R  X|!(R 0X|\"(R  &|#(R P%|$(R @3|%(R "
    " W|&(R P:|6$ &!|((R 08|)$ B#|*(R P0|+(R @0|,(R 04|!~.(R 0:|!~0(R @:|!~!"
    "~3(R 0I|!~D(9 @,|6$ 8#|F$ \\$|G$ Z$| H8 @$|!H8 P<|\"H8  &|#H8 P%|$H8 P,"
    "|%H8  W|!~'H8 P4|(H8 P-| H7 @$|!H7 P<|\"H7  &|#H7 P%|$H7 P,|%H7  W|!~'H"
    "7 P4|(H7 P-|Z, J$|[, D$|\\, X$|], N$|^, :&| 0 D|!H: P<|\"0 P|#0 N|$H: P"
    ",|%0 X&|!~'H: P4|(0 N!|!~*0 &\"|+0 $\"|!~DH8 @,|!~:- \"\"|!~1H:  /|!~ H"
    "$ @$|4H: P.|@- J\"|DH7 @,|'- 4%| 0  '|!H8 P<|\"H8  &|#H8 P%|$0 J(|%H8  "
    "W|!~'H8 P4|(H8 P-|)$ L$|/H$ P\"|)$ B#|DH: @,|!~!~!~HH: 0/|!~ 0 D|!HS P<"
    "|\"0 P|#0 N|6$ &!|%HS  W|6$ 8#|!~!~!~*HS P0|+HS @0|!~!~!~/H. P\"|!~K- 6"
    "%|DH8 @,|JH$ P!|F$ \\$|G$ Z$|MH$ P#|NH$  $|OH$ 0$|PH$ @#|!~!~<H. P2|=H."
    " 02|>H. @1|?H. @2|@H.  2|AH. P1|BH. 01|CH.  3|!~!~0- H#|!~Z, J$|[, D$|\\"
    ", X$|], >&| (5 @$|!(5 P<|!~!~:- \"\"|%(5  W|!~'$ F\"|((5 P-|!~@-  \"|'-"
    " 2$|X$ &&|!~D- ^!| 0  '|!0 \"'|\"0 P|#0 N|$0 <\"|%0 X&|&0 6#|2- 0$|(0 \""
    "#|!~*0 &\"|+0 $\"|,0 B\"|!~.0 2#|!~00 4#|!~ (%!@$|30 *%|\"(%! &|#(%!P%|"
    "!~!~!~!~ HO  X|!HO 0X|\"HO  &|#HO P%|$HO @3|%HO  W|&HO P:|/(%!P\"|(HO 0"
    "8|!~*HO P0|+HO @0|,HO 04|!~.HO 0:|!~0HO @:|KHP  D|L0 $'|3HO 0I|<(%!P2|="
    "(%!02|>(%!@1|?(%!@2|@(%! 2|A(%!P1|B(%!01|C(%! 3|!~!~ 0 D|!~\"0 P|#0 N|J"
    "(%!P!|!~!~M(%!P#|N(%! $|O(%!0$|P(%!@#|!~!~KHP  D|LHO @X|/0 6|   L&|!(7 "
    "P<|\"$ P|#$ N|$$ F!|%(7  W|!~'(7 P4|((7 P-|!~!~!~<0 6\"|=0 2\"|>0 ,\"|?"
    "0 4\"|@0 0\"|A0 .\"|B0 *\"|C0 8\"| H' @$|!~\"H'  &|#H' P%|!~A- >$|J0 .|"
    "!~!~M0 >|N0 @|O0 B|P0 <|!~!~/H' P\"|D$ D!|\\- D\"| (N  X|!(N 0X|\"(N  &"
    "|#(N P%|$(N @3|%(N  W|&(N P:|!~((N 08|!~*(N P0|+(N @0|,(N 04|A- >$|.(N "
    "0:|!~0(N @:|!~!~3(N 0I|)$ B#|!~!~!~!~!~ - 4!|!- 6!|\"- L|!~$- B!|%- H!|"
    "!~6$ 8#| 0 D|!0 F#|*- @!|+- T!|!~%0 X&|!~'0 F\"|(0 N!|!~L  P&| $ 0%|!$ "
    "0%|\"$ 0%|#$ 0%|$$ 0%|%$ 0%|&$ 0%|!~($ 0%|K(Q  D|*$ 0%|+$ 0%|,$ 0%|!~.$"
    " 0%|!~0$ 0%|!~!~3$ 0%| $ 0%|!$ 0%|\"$ 0%|#$ 0%|$$ 0%|%$ 0%|&$ 0%|!~($ 0"
    "%|!~*$ 0%|+$ 0%|,$ 0%|!~.$ 0%|!~0$ 0%|'- F(|!~3$ 0%|!~!~!~K(N @C|L$ 0%|"
    " H#!@$|!H#!P<|\"H#! &|#H#!P%|$H#!P,|%H#! W|!~'H#!P4|(H#!P-|!~*H#!P0|+H#"
    "!@0|!~!~Z- 8|>- ,%|?- @#|1H#! /|K(N @C|L$ 0%|4H#!P.| (B @$|!(B P<|\"(B "
    " &|#(B P%|$(B P,|%(B  W|!~'(B P4|((B P-|X- 2%|*(B P0|+(B @0|!~!~!~DH#!@"
    ",|!~1(B  /|!~HH#!0/|4(B P.|!(Z 0Q|!~!~ H\" @$|F- R!|&(Z  L|'(Z 0K|!~)$ "
    "P%|SH#!0(|TH#! (|!~-(Z  K|!~!~D(B @,|1$ N%|2(Z @L|/H\" P\"|H(B 0/|5(Z 0"
    "U|!~7(Z 0M|8(Z @M|9(Z PL|:(Z 0L|;(Z  M|!~!~!~S(B 0(|T(B  (| HO  X|!HO 0"
    "X|\"HO  &|#HO P%|$HO @3|%HO  W|&HO P:|!~(HO 08|!~*HO P0|+HO @0|,HO 04|J"
    "$ .|.HO 0:|!~0HO @:|!~!~3HO 0I| (S  X|!(S 0X|\"(S  &|#(S P%|$(S @3|%(S "
    " W|&(S P:|!~((S 08| $ $#|*(S P0|+(S @0|,(S 04|$$ <\"|.(S 0:|&$ 6#|0(S @"
    ":|!~!~3(S 0I|!~,$ B\"|!~.$ 2#|LHO @X|0$ 4#| HR  X|!HR 0X|\"HR  &|#HR P%"
    "|$HR @3|%HR  W|&HR P:|!~(HR 08|!~*HR P0|+HR @0|,HR 04|!~.HR 0:|!~0HR @:"
    "|K(S  D|L(S @X|3HR 0I| (A @$|!(A P<|\"(A  &|#(A P%|$(A P,|%(A  W|!~'(A "
    "P4|((A P-|!~*(A P0|+(A @0|!~!~!~!~O- *|1(A  /| H! @$|!~4(A P.|Z, 0#|!~K"
    "HR  D|LHR @X|!~Y- ,| 0 D|!~\"0 P|#0 N|A- >$|%0 X&|/H! P\"|!~(- (#|D(A @"
    ",|*0 &\"|+0 $\"|,- &#|H(A 0/| $ D|!~\"$ P|#$ N|$$ F!|!~!~5- \"&|!~!~S$ "
    "\"!|T$  !| (O  X|!(O 0X|\"(O  &|#(O P%|$(O @3|%(O  W|&(O P:|JH! P!|((O "
    "08|!~*(O P0|+(O @0|,(O 04|!~.(O 0:|!(\"!0Q|0(O @:|!~!~3(O 0I|&(\"! L|'("
    "\"!0K|Y$ (|)(\"! N|D$ D!|!~!~-(\"! K|!~!~!~1(\"!PM|2(\"!@L|!~4(\"!0N|5("
    "\"!0U|!~7(\"!0M|8(\"!@M|9(\"!PL|:(\"!0L|;(\"! M|!~KHP  D|L(O @X|!~ $ D|"
    "!~\"$ P|#$ N|$$ F!|E(\"!@N| - 4!|!- 6!|\"- L|!~$- B!|%- H!|!~!~ (# @$|!"
    "~*- @!|+- >!|!(_ 0Q|-- <!|.- :!|!~!~&(_  L|'(_ 0K|3- 8!|)(_  N|L- \"|M-"
    " $|/(# P\"|-(_  K|!~Q- &|!~1(_ PM|2(_ @L|D$ D!|4(_ 0N|5(_ 0U|!~7(_ 0M|8"
    "(_ @M|9(_ PL|:(_ 0L|;(_  M|!~!~!~ $ $#|!~!~!~$$ H$|!~E(_ @N|!~J(# P!|)$"
    " L$|!~M$ >|N$ @|O$ B|P$ <|!~ - 4!|!- 6!|\"- L|!~$- B!|%- H!|6$ &!|!~!~!"
    "~*- @!|+- >!|!H !0Q|-- <!|.- :!|!~!~&H ! L|'H !0K|3- \\!|)H ! N|!(W 0Q|"
    "F$ \\$|G$ Z$|-H ! K|!~&$ @%|'(W 0K|1H !PM|2H !@L|!~4H !0N|5H !0U|-(W  K"
    "|7H !0M|8H !@M|9H !PL|:H !0L|;H ! M|!~!~5(W 0U|Z, J$|[, D$|\\, X$|], N$"
    "|^, V$|_, T$|EH !@N|!~!~#- R$|!~!~&- P$|!~ (O  X|!(O 0X|\"(O  &|#(O P%|"
    "$(O @3|%(O  W|&(O P:|/- F$|((O 08|S- :|*(O P0|+(O @0|,(O 04|!~.(O 0:|!("
    "!!0Q|0(O @:|!~!~3(O 0I|&(!! L|'(!!0K|!~)(!! N| $ $#|!~!~-(!! K|$$ H$|!~"
    "!~1(!!PM|2(!!@L|)$ L$|4(!!0N|5(!!0U|!~7(!!0M|8(!!@M|9(!!PL|:(!!0L|;(!! "
    "M|!~!~L(O @X| H> @$|6$ &!|\"H>  &|#H> P%|!~%H>  W|E(!!@N|!H_ 0Q|!~!~*H>"
    " P0|+H> @0|&H_  L|'H_ 0K|!~)H_  N|!HV 0Q|F$ \\$|G$ Z$|-H_  K|!~&$ @%|'H"
    "V 0K|1H_ PM|2H_ @L|!~4H_ 0N|5H_ 0U|-HV  K|7H_ 0M|8H_ @M|9H_ PL|:H_ 0L|;"
    "H_  M|!~!~5HV 0U|Z, J$|[, D$|\\, X$|], N$|^, V$|_, T$|EH_ @N|!~!H!!0Q|#"
    "- R$|!~!~&- P$|&H!! L|'H!!0K|!~)H!! N|!~!~!~-H!! K|/- <%|!~!~1H!!PM|2H!"
    "!@L|!~4H!!0N|5H!!0U|!~7H!!0M|8H!!@M|9H!!PL|:H!!0L|;H!! M|!H^ 0Q|!~!~!~!"
    "~&H^  L|'H^ 0K|!~)H^  N|EH!!@N|!~!~-H^  K|!~!~!~1H^ PM|2H^ @L|!~4H^ 0N|"
    "5H^ 0U|!~7H^ 0M|8H^ @M|9H^ PL|:H^ 0L|;H^  M|!(] 0Q|!~!~!~!~&(]  L|'(] 0"
    "K|!~)(]  N|EH^ @N|!~!~-(]  K|!~!~!~1(] PM|2(] @L|!~4$ R%|5(] 0U|!~7(] 0"
    "M|8(] @M|9(] PL|:(] 0L|;(]  M|!0 *&|!~!~!~!~&0 @%|'0 :%|!~)0 P%|E$ T%|!"
    "~!~-0 8%|!~!~!~10 N%|20 D%|!~40 R%|50 J&|!~70 J%|80 L%|90 F%|:0 B%|;0 H"
    "%|!0 *&|!~!~!~!~&0 @%|'0 :%|!~)0 P%|E0 T%|!~!~-0 8%|!~!~!~10 N%|20 D%|!"
    "~4$ R%|50 J&|\"$ N#|70 J%|80 L%|90 F%|:0 B%|;0 H%|!0 *&|)$ B#|!~!~!~&0 "
    "@%|'0 :%|!~)0 P%|E$ T%|!~!~-0 8%|!~6$ 8#|!~10 N%|20 D%|!~40 R%|50 J&|!~"
    "70 J%|80 L%|90 F%|:0 B%|;0 H%|!~ $ $#|!~!~!~$$ H$|!~!~!(^ 0Q|E0 T%|)$ L"
    "$|!~!~&(^  L|'(^ 0K|R$ P#|)(^  N|!~!~!~-(^  K|!~!~6$ &!|1(^ PM|2(^ @L|!"
    "~4(^ 0N|5(^ 0U|!~7(^ 0M|8(^ @M|9(^ PL|:(^ 0L|;(^  M|!~'- 2$|!~)- 4$|F$ "
    "\\$|G$ Z$|!~!~!~E(^ @N|!~!~2- 6$|!~!~ H; @$|!H; P<|\"H;  &|#H; P%|$H; P"
    ",|%H;  W|!~'H; P4|(H; P-|Z, J$|[, D$|\\, X$|], N$|^, V$|_, T$|!~!~1H;  "
    "/|#- R$|!~4H; P.|&- P$|!~ 0  '|!0 \"'|\"HI  &|#HI P%|$0 <\"|%HI  W|&0 6"
    "#|/- H&|(HI 08|!~*HI P0|+HI @0|,0 B\"|DH; @,|.0 2#|!~00 4#|HH; 0/|!~30 "
    "*%| HN  X|!HN 0X|\"HN  &|#HN P%|$HN @3|%HN  W|&HN P:|!~(HN 08|!~*HN P0|"
    "+HN @0|,HN 04|!~.HN 0:|!H] 0Q|0HN @:|!~!~3HN 0I|&H]  L|'H] 0K|!~)H]  N|"
    "L$ <$|!~!~-H]  K|!~!~!~1H] PM|2H] @L|!~4H] 0N|5H] 0U|!~7H] 0M|8H] @M|9H"
    "] PL|:H] 0L|;H]  M|!H\\ 0Q|!~LHN @X|!~!~&H\\  L|'H\\ 0K|!~)H\\  N|EH] @"
    "N|!~!~-H\\  K|!~!~!~1H\\ PM|2H\\ @L|!~4$ R%|5H\\ 0U|!~7H\\ 0M|8H\\ @M|9"
    "H\\ PL|:H\\ 0L|;H\\  M|!(\\ 0Q|!~!~!~!~&(\\  L|'(\\ 0K|!~)(\\  N|E$ T%|"
    "!~!~-(\\  K|!~!~!~1(\\ PM|2(\\ @L|!~4$ R%|5(\\ 0U|!~7(\\ 0M|8(\\ @M|9(\\"
    " PL|:(\\ 0L|;(\\  M|!0 *&|!~!~!~!~&0 @%|'0 :%|!~)0 P%|E$ T%|!~!~-0 8%|!"
    "~!~!~10 N%|20 D%|!~40 R%|50 J&|!~70 J%|80 L%|90 F%|:0 B%|;0 H%| HI  X|!"
    "HI 0X|\"HI  &|#HI P%|$HI @3|%HI  W|&HI P:|!~(HI 08|E0 T%|*HI P0|+HI @0|"
    ",HI 04|!~.HI 0:|!~0HI @:|!~!~3HI 0I| HQ  X|!HQ 0X|\"HQ  &|#HQ P%|$HQ @3"
    "|%HQ  W|&HQ P:|!~(HQ 08| $ $#|*HQ P0|+HQ @0|,HQ 04|$$ H$|.HQ 0:|!~0HQ @"
    ":|!~)$ L$|3HQ 0I|!~!~!~!~L$ <$| H@ @$|!H@ P<|\"H@  &|#H@ P%|$H@ P,|%H@ "
    " W|6$ &!|'H@ P4|(H@ P-|!HW 0Q|*H@ P0|+H@ @0|!~!~&HW  L|'HW 0K|!~1H@  /|"
    "!~!~4H@ P.|-HW  K|F$ \\$|G$ Z$|!~!~2$ D%|!~!~5HW 0U|!~7$ J%|8$ L%|9$ F%"
    "|:$ B%|;$ H%|DH@ @,|!~!~!~HH@ 0/|!~Z, J$|[, D$|\\, X$|], N$|^, V$|_, T$"
    "|!~!~!~#- R$|!~!~&- (&| (J  X|!(J 0X|\"(J  &|#(J P%|$(J @3|%(J  W|&(J P"
    ":|!~((J 08|!~*(J P0|+(J @0|,(J 04|!~.(J 0:|!~0(J @:|!~!~3(J 0I| (D  X|!"
    "(D 0X|\"(D  &|#(D P%|$(D @3|%(D  W|&(D P:|!~((D 08|!~*(D P0|+(D @0|,(D "
    "04|!~.(D 0:|!~0(D @:|!~!~3(D 0I| (I  X|!(I 0X|\"(I  &|#(I P%|$(I @3|%(I"
    "  W|&(I P:|!~((I 08|!~*(I P0|+(I @0|,(I 04|!~.(I 0:|!~0(I @:|!~!~3(I 0I"
    "| (A @$|!(A P<|\"(A  &|#(A P%|$(A P,|%(A  W|!~'(A P4|((A P-|!~*(A P0|+("
    "A @0|!~!~!~!~!~1(A  /|!~!~4(A P.|!~!~ (P  X|!(P 0X|\"(P  &|#(P P%|$(P @"
    "3|%(P  W|&(P P:|!~((P 08|!~*(P P0|+(P @0|,(P 04|D(A @,|.(P 0:|!~0(P @:|"
    "H(A 0/|!~3(P 0I| $ D|!$ D|\"$ D|#$ D|$$ D|%$ D|!~'$ D|($ D|!~!~ (8 @$|!"
    "(8 P<|\"(8  &|#(8 P%|$(8 P,|%(8  W|1$ D|'(8 P4|((8 P-|4$ D|!~!~ 0  '|!0"
    " \"'|\"(C  &|#(C P%|$HH @3|%(C  W|&HH P:|!~((C 08|!~*(C P0|+(C @0|,HH 0"
    "4|D$ D|.HH 0:|!~0HH @:|H$ D|!~3HH 0I|!~!~!~!~D(8 @,|!~!~!~S(7 @$|T(7 @$"
    "| HA @$|!HA P<|\"HA  &|#HA P%|$HA P,|%HA  W|!~'HA P4|(HA P-|!~*HA P0|+H"
    "A @0|!~!~!~!~!~1HA  /|!~!~4HA P.|!~!~ 0  '|!0 \"'|\"HB  &|#HB P%|$HM @3"
    "|%HB  W|&HM P:|!~(HB 08|!~*HB P0|+HB @0|,HM 04|DHA @,|.HM 0:|!~0HM @:|H"
    "HA 0/|!~3HM 0I| 0  '|!0 \"'|\"0 P|#0 N|$0 <\"|%0 X&|&0 6#|!~(0 \"#|!~*0"
    " &\"|+0 $\"|,0 B\"|!~.0 2#| H+ @$|00 4#|\"H+  &|#H+ P%|30 *%|!~ HG @8|!"
    "HG @A|!~!~$HG @3|!~&HG P:|!~!~/H+ P\"|!~!~,HG 04|!~.HG 0:| 0 D|0HG @:|\""
    "0 P|#0 N|3HG 0I|!~!~<H+ P2|=H+ 02|>H+ @1|?H+ @2|@H+  2|AH+ P1|BH+ 01|CH"
    "+  3|/0 6|!~!~!~!~I$ T#| H) @$|!~\"H)  &|#H) P%|!~!~!~<0 6\"|=0 2\"|>0 "
    ",\"|?0 4\"|@0 0\"|A0 .\"|B0 *\"|C0 8\"|/H) P\"| (4 @$|!(4 P<|\"(4  &|#("
    "4 P%|IH- @>|%(4  W|!~!~($ \"#|!~*(4 P0|+(4 @0|<H) P2|=H) 02|>H) @1|?H) "
    "@2|@H)  2|AH) P1|BH) 01|CH)  3|!~!(Y 0Q|!~!~!~!~&(Y  L|'(Y 0K|!~)$ P%|!"
    "~!~!~-(Y  K|!~!~!~1$ N%|2(Y @L|!~B- ^$|5(Y 0U|!~7(Y 0M|8(Y @M|9(Y PL|:("
    "Y 0L|;(Y  M|!HX 0Q|E- .$|!~!~!~&HX  L|'HX 0K|!~)$ P%| $ $#|!~!~-HX  K|$"
    "$ H$|!~!~1$ N%|2HX @L|!~!~5HX 0U|!~7HX 0M|8HX @M|9HX PL|:HX 0L|;HX  M|;"
    "- L!|!HY 0Q|!~!~6$ L(|!~&HY  L|'HY 0K|!~)$ P%|!~!~!~-HY  K|!~!~!~1$ N%|"
    "2HY @L|!~F$ \\$|5HY 0U|!~7HY 0M|8HY @M|9HY PL|:HY 0L|;HY  M| 0 D|!~\"0 "
    "P|#0 N| $ $#|!(H @A|!~!~$$ <\"|!~&$ 6#|!~Z, J$|[, D$|\\, >%|/0 6|,$ B\""
    "|!~.$ 2#|!~0$ 4#| H( @$|!~\"H(  &|#H( P%|!~!~!~<0 6\"|=0 2\"|>0 ,\"|?0 "
    "4\"|@0 0\"|A0 .\"|B0 *\"|C0 8\"|/H( P\"| HB @$|!HB P<|\"HB  &|#HB P%|I("
    "- @>|%HB  W|!~!~(HB 08|!~*HB P0|+HB @0|<$ 6\"|=$ 2\"|>$ ,\"|?$ 4\"|@$ 0"
    "\"|A$ .\"|B$ *\"|C$ 8\"|!~!~!~!~!~Z, 0#| 0 $#|!0 ,$|!~!~$0 <\"|!~&0 6#|"
    " 0 D|!~\"0 P|#0 N|!~,0 B\"|(- (#|.0 2#|!~00 4#|,- &#|!~30 *%|!~ $ $#|/0"
    " 6|!~!~$$ H$|5- .#|!~!~!~)$ L$|!~!~<- ,#|=- *#|<0 6\"|=0 2\"|>0 ,\"|?0 "
    "4\"|@0 0\"|A0 .\"|B0 *\"|C0 8\"|6$ &!| $ D|!~\"$ P|#$ N|I0 T#| (; @$|!("
    "; P<|\"(;  &|#(; P%|$(; P,|%(;  W|!~'(; P4|((; P-|!~F$ \\$|G$ Z$|!~ (, "
    "@$|!~\"(,  &|#(, P%|1(;  /|!~!~4(; P.|!~!~!~!~!~!~V- (\"|/(, P\"|!~Z, J"
    "$|[, D$|\\, X$|], N$|^, V$|_, T$|D(; @,|!~!~#- ,&|H(; 0/|<(, P2|=(, 02|"
    ">(, @1|?(, @2|@(,  2|A(, P1|B(, 01|C(,  3|!~!H[ 0Q|!~E- .$|!~I(, @>|&H["
    "  L|'H[ 0K|!~)$ P%| - 4!|!- 6!|\"- L|-H[  K|$- J!|%- H!|!~1$ N%|2H[ @L|"
    "!~!~5H[ 0U|!~7H[ 0M|8H[ @M|9H[ PL|:H[ 0L|;H[  M| H- @$|!~\"H-  &|#H- P%"
    "|!~!~ (G @8|!(E @A|!~!~$(G @3|!~&(G P:|!~!~/H- P\"|!~!~,(G 04|!~.(G 0:|"
    " H, @$|0(G @:|\"H,  &|#H, P%|3$ *%|!~!~<H- P2|=H- 02|>H- @1|?H- @2|@H- "
    " 2|AH- P1|BH- 01|CH-  3|/H, P\"|   N&|!~\"$ P|#$ N|IH- @>|%H=  W|!~!~!~"
    "!~*H= P0|+H= @0|<H, P2|=H, 02|>H, @1|?H, @2|@H,  2|AH, P1|BH, 01|CH,  3"
    "|!~!0 *&|!~!~!~IH, @>|&0 @%|'0 :%|!~)$ P%|!~!~!~-0 8%|!~!~!~1$ N%|20 D%"
    "|!~!~50 J&|!~70 J%|80 L%|90 F%|:0 B%|;0 H%| (- @$|!~\"(-  &|#(- P%| 0 D"
    "|!0 F#|\"0 P|#0 N|$0 F!|%0 X&|!~'0 F\"|(0 N!|!~!~/(- P\"| - 4!|!- 6!|\""
    "- L|!~$- L\"|%- H!|!~!~!~!~I- &%|J- (%|<(- P2|=(- 02|>(- @1|?(- @2|@(- "
    " 2|A(- P1|B(- 01|C(-  3|!~!([ 0Q|!~!~D0 D!|I(- @>|&([  L|'([ 0K|!~)$ P%"
    "|!~!~!~-([  K| (% @$|!~!~1$ N%|2([ @L|G- J#|!~5([ 0U|!~7([ 0M|8([ @M|9("
    "[ PL|:([ 0L|;([  M|!HZ 0Q|/(% P\"|!~!~!~&HZ  L|'HZ 0K|!~)$ P%|!~!~!~-HZ"
    "  K|!~!~!~1$ N%|2HZ @L|!~!~5HZ 0U|!~7HZ 0M|8HZ @M|9HZ PL|:HZ 0L|;HZ  M|"
    " H* @$|J(% P!|\"H*  &|#H* P%|M(% P#|N(%  $|O(% 0$|P(% @#| (* @$|!~\"(* "
    " &|#(* P%|)$ B#|!~!~/H* P\"| H4 @$|!~\"H4  &|#H4 P%|!~%H4  W|!~/(* P\"|"
    "($ N!|6$ 8#|*H4 P0|+H4 @0|<H* P2|=H* 02|>H* @1|?H* @2|@H*  2|AH* P1|BH*"
    " 01|CH*  3|<(* P2|=(* 02|>(* @1|?(* @2|@(*  2|A(* P1|B(* 01|C(*  3| $ $"
    "#|!~K(Q  D|!~$$ H$|!~ $ $#|!~!~)$ L$|$$ H$|!~ $ $#| HM @8|!HM @A|)$ L$|"
    "$$ H$|$HM @3|!~&HM P:|!~)$ L$|6$ &!|!~!~,HM 04|!~.HM 0:|6$ &!|0HM @:|'-"
    " >#|!~3HM 0I|!~6$ &!|!~!~!~F$ \\$|G$ Z$|!~2- :#| H% @$|!~F$ \\$|G$ Z$|!"
    "~!~!~!~F$ \\$|G$ Z$|0- H#|>- <#|?- @#|!~!~/H% P\"|Z, J$|[, D$|\\, X$|],"
    " N$|^, V$|_, 2&|Z, J$|[, D$|\\, X$|], N$|^, V$|_, 0&|Z, J$|[, D$|\\, X$"
    "|], N$|^, V$|_, .&| (+ @$| $ $#|\"(+  &|#(+ P%|!~$$ H$|!~!~JH% P!|!~)$ "
    "L$|MH% P#|NH%  $|OH% 0$|PH% @#|/(+ P\"|!~!~!~!~!~!~ (. @$|6$ &!|\"(.  &"
    "|#(. P%|!~!~<(+ P2|=(+ 02|>(+ @1|?(+ @2|@(+  2|A(+ P1|B(+ 01|C(+  3|!~/"
    "(. P\"|!~F$ \\$|G$ Z$|!~!~ (/ @$|!~\"(/  &|#(/ P%|!~!~!~<(. P2|=(. 02|>"
    "(. @1|?(. @2|@(.  2|A(. P1|B(. 01|C(.  3|/(/ P\"|Z, J$|[, D$|\\, X$|], "
    "N$|^, V$|_, 6&| H/ @$|!~\"H/  &|#H/ P%|!~)$ B#|<(/ P2|=(/ 02|>(/ @1|?(/"
    " @2|@(/  2|A(/ P1|B(/ 01|C(/  3|!~/H/ P\"|!~!~6$ 8#|!~!~ (0 @$|!~\"(0  "
    "&|#(0 P%|!~!~!~<H/ P2|=H/ 02|>H/ @1|?H/ @2|@H/  2|AH/ P1|BH/ 01|CH/  3|"
    "/(0 P\"|!~!~K(Q  D| H: @$|!H: P<|\"H:  &|#H: P%|$H: P,|%H:  W|!~'H: P4|"
    "(H: P-|<(0 P2|=(0 02|>(0 @1|?(0 @2|@(0  2|A(0 P1|B(0 01|C(0  3|1H:  /|!"
    "~!~4H: P.| (: @$|!(: P<|\"(:  &|#(: P%|$(: P,|%(:  W|'- .%|'(: P4|((: P"
    "-| $ $#|!~!~!~$$ H$|!~DH: @,|!~1$ X!|)$ L$|HH: 0/|4$ V!|!~ H. @$| $ $#|"
    "\"H.  &|#H. P%|!~$$ H$|!~>- ,%|?- @#|6$ &!|)$ L$|!~!~!~D(: @,|/H. P\"|!"
    "~!~H$ Z!|!~!~!~!~6$ &!|!~F$ \\$|G$ Z$|!~<H. P2|=H. 02|>H. @1|?H. @2|@H."
    "  2|AH. P1|BH. 01|CH.  3|!~!~!~F$ \\$|G$ Z$| () @$|!~\"()  &|#() P%|Z, "
    "J$|[, D$|\\, X$|], N$|^, V$|_, 4&|!~!~!~!~!~/() P\"|!~!~Z, J$|[, D$|\\,"
    " X$|], N$|^, V$|_, 8&|!~!~!~!~<() P2|=() 02|>() @1|?() @2|@()  2|A() P1"
    "|B() 01|C()  3| $ $#| HH @8|!HH @A|!~$$ H$|$HH @3|!0 *&|&HH P:|!~)$ L$|"
    "!~&0 @%|'0 :%|,HH 04|!~.HH 0:|!~0HH @:|-0 8%|!~3HH 0I|!~6$ L(|2$ D%|!~!"
    "~50 J&|!~7$ J%|8$ L%|9$ F%|:$ B%|;$ H%|!~!~!~!~!~F$ \\$|G$ Z$|!~!~!~ $ "
    "$#|!~!~ $ D|$$ H$|\"$ P|#$ N|!~!(X 0Q|)$ L$|!~!~!~&(X  L|'(X 0K|Z, J$|["
    ", D$|\\, N(|], N$|^, <&|-(X  K|!~6$ &!|!~!~2$ D%|!~!~5(X 0U|!~7$ J%|8$ "
    "L%|9$ F%|:$ B%|;$ H%|!~!~!~F$ \\$|G$ Z$|!~ $ $#|!~!~!~$$ H$|!~!~!~!~)$ "
    "L$|!~!~!~!~!~!~!~Z, J$|[, D$|\\, X$|], N$|^, :&|6$ &!|!~!~ $ $#| - 4!|!"
    "- 6!|\"- L|$$ H$|$- L\"|%- H!| $ $#|!~)$ L$|!~$$ H$|!~F$ \\$|G$ Z$|!~)$"
    " L$|!~!~!~!~!~6$ &!|!~!~!~!~!~!~6$ &!|!~!~ ($ @$|Z, J$|[, D$|\\, X$|], "
    "N$|^, <&|F$ \\$|G$ Z$|G- P\"|!~!~!~!~F$ \\$|G$ Z$|/($ P\"|!~!~!~!~!~!~!"
    "~!~W- N\"|!~Z, J$|[, D$|\\, X$|], >&|!~!~!~Z, J$|[, D$|\\, X$|], @&| (F"
    " @8|!(F @A|!~!~$(F @3|J($ P!|&(F P:|!~M($ P#|N($  $|O($ 0$|P($ @#|,(F 0"
    "4|!~.(F 0:|!~0(F @:| HJ @8|!HJ @A|3(F 0I|!~$HJ @3|!~&HJ P:|!~!~ (L @8|!"
    "(L @A|!~,HJ 04|$(L @3|.HJ 0:|&(L P:|0HJ @:|!~!~3HJ 0I|!~,(L 04|!~.(L 0:"
    "|!~0(L @:| HE @8|!HE @A|3(L 0I|!~$HE @3|!~&HE P:|!~!~ HL @8|!HL @A|!~,H"
    "E 04|$HL @3|.HE 0:|&HL P:|0HE @:|\"$ N#|!~3HE 0I|!~,HL 04|!~.HL 0:|)$ B"
    "#|0HL @:| (M @8|!(M @A|3HL 0I|!~$(M @3| (& @$|&(M P:|!~R- 0| (K @8|!(K "
    "@A|6$ 8#|,(M 04|$(K @3|.(M 0:|&(K P:|0(M @:|!~!~3(M 0I|/(& P\"|,(K 04|!"
    "~.(K 0:|!~0(K @:| HK @8|!HK @A|3(K 0I|!~$HK @3|!~&HK P:|!~!~!~!~!~,HK 0"
    "4|R$ P#|.HK 0:|!~0HK @:|!~!~3HK 0I|!~J(& P!|!~!~M(& P#|N(&  $|O(& 0$|P("
    "& @#|!~!~!~!~!~!~'- 2$|!~)- 8$|!~!~!~!~!~!~!~!~2- :$|!~!~!~!~!~!~!~!~!~"
    "!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!"
    "~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~"
    "!~!~!~!~!~!~!~}8|!|}9||}:||};|_#|}<||}=|(|}>|'|}?||}@|+|}A|_#|}B||}C|3|"
    "}D|_'|}E||}F|<|}G|_'|}H|Null||Halt|!|Label|\"|Call|#|ScanStart|$|ScanCh"
    "ar|%|ScanAccept|&|ScanToken|'|ScanError|(|AstStart|)|AstFinish|*|AstNew"
    "|+|AstForm|,|AstLoad|-|AstIndex|.|AstChild|/|AstChildSlice|0|AstKind|1|"
    "AstKindNum|2|AstLocation|3|AstLocationNum|4|AstLexeme|5|AstLexemeString"
    "|6|Assign|7|DumpStack|8|Add|9|Subtract|:|Multiply|;|Divide|<|UnaryMinus"
    "|=|Return|>|Branch|?|BranchEqual|@|BranchNotEqual|A|BranchLessThan|B|Br"
    "anchLessEqual|C|BranchGreaterThan|D|BranchGreaterEqual|E|}I|Y6|}J|E.!|}"
    "K|7|!~|'||>|!~\"|$|!~\"|%|!~\"|P|)|-|$|@|@|$|A|A|&|B|B|(|D|D|.|E|E|0|F|"
    "F|2|G|G|4|H|H|?|I|I|A|J|J|C|K|K|E|L|L|G|M|M|I|N|N|K|O|O|M|P|Y|W|Z|Z|Y|["
    "|[|\"!|\\|\\|$!|]|]|/!|^|^|3!|_|_|9!| !| !|;!|!!|:!|=!|;!|;!|?!|=!|=!|A"
    "!|?!|?!|C!|A!|A!|E!|B!|B!|=!|C!|C!|Q!|D!|D!|=\"|E!|E!|%#|F!|F!|7#|G!|H!"
    "|=!|I!|I!|A#|J!|J!|=!|K!|K!|M#|L!|L!|+$|M!|N!|=!|O!|O!|I$|P!|P!|W$|Q!|Q"
    "!|=!|R!|R!|+%|S!|S!|=!|T!|T!|=%|U!|Z!|=!|\\!|\\!|)&|&|I$|3\"| \"|\"|%|!"
    "~5\"|\"|)|-|$|@|@|$|&|0D\"|<\"|G|+&|%|!~>\"||%|!~?\"|'||)|(|*|*|)|+|A|("
    "|B|B|+|C|;!|(|<!|<!|-|=!|_____#|(|&|<,|U\"|!\"|.&|%|!~W\"||&|**|X\"|\"|"
    "+&|%|!~Z\"||%|!~[\"|\"||)|(|+|_____#|(|&|@O!|\"#|,|+&|%|!~$#||&|9V!|%#|"
    ".|+&|%|!~'#||&|&[!|(#|&|+&|%|!~*#||%|!~+#|'||)|5|*|*|)|+|F|5|G|G|7|H|;!"
    "|5|<!|<!|6|=!|_____#|5|%|!~A#|'||)|5|*|*|)|+|F|5|G|G|+|H|;!|5|<!|<!|6|="
    "!|_____#|5|%|!~W#|\"||)|5|+|_____#|5|&|**|^#|\"|+&|%|!~ $|!|G|G|9|&|)1|"
    "$$|\"\"|0&|%|!~&$|#||F|9|G|G|;|H|_____#|9|%|!~0$|#||F|9|G|G|<|H|_____#|"
    "9|%|!~:$|#||F|9|G|G|=|H|_____#|9|&|^/|D$|R|+&|%|!~F$||&|*8!|G$|$|+&|%|!"
    "~I$||&|H8!|J$|!|+&|%|!~L$||&|U0!|M$|4|+&|%|!~O$||&|=2!|P$|1|+&|%|!~R$||"
    "&|[L!|S$|3|+&|%|!~U$||&|,'\"|V$|)|+&|%|!~X$||&|QO!|Y$|L|+&|%|!~[$||&|DA"
    "\"|\\$|E|+&|%|!~^$|#|J|J|O|O|O|S|]|]|U|%|!~(%|#||I|O|J|J|P|K|_____#|O|%"
    "|!~2%|%||I|O|J|J|P|K|N|O|O|O|Q|P|_____#|O|&|##|B%|_!|\"|%|!~D%||&|##|E%"
    "|_!|\"|%|!~G%|\"||)|S|+|_____#|S|&|96\"|N%|9|+&|%|!~P%||&|.&|Q%|6|+&|%|"
    "!~S%|!|P|Y|W|&|(M|W%|(|+&|%|!~Y%|\"|Z|Z|[|]|]| !|%|!~ &|\"|]|]|\\|>!|>!"
    "|^|&|+&!|'&|T|+&|%|!~)&||&|::!|*&|S|+&|%|!~,&||&|J.\"|-&|W|+&|%|!~/&||&"
    "| /\"|0&|5|+&|%|!~2&||@|!6|3&|&!|&||&|@5|6&|:|+&|%|!~8&|$|\\|\\|'!|]|]|"
    ")!|!!|:!|+!|A!|Z!|+!|&|,?!|E&|+|+&|%|!~G&||&|+9\"|H&|;|+&|%|!~J&||%|!~K"
    "&|%|P|Y|+!|^|^|,!|!!|:!|+!|?!|?!|+!|A!|Z!|+!|A|5)|[&|.!|&||&|S(|^&|#|+&"
    "|%|!~ '||&|,A|!'|2|+&|%|!~#'|!|^|^|1!|&|4W|''|X|+&|%|!~)'||&|E:\"|*'|7|"
    "+&|%|!~,'|\"|]|]|5!|^|^|7!|&|!<\"|3'|8|+&|%|!~5'||&|B@!|6'|*|+&|%|!~8'|"
    "|&|$4!|9'|H|+&|%|!~;'||&|YW!|<'|0|+&|%|!~>'||&|#'|?'||+&|%|!~A'|$|P|Y|="
    "!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|L3|N'|I|2&|%|!~P'||&|F4|Q'|-|6&|%|!~S'||"
    "&|-!\"|T'|K|+&|%|!~V'||&|#'|W'||+&|%|!~Y'|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!"
    "|B!|=!|C!|C!|G!|D!|Z!|=!|&|#'|,(||+&|%|!~.(|&|P|Y|=!|!!|:!|=!|?!|?!|=!|"
    "A!|S!|=!|T!|T!|I!|U!|Z!|=!|&|#'|A(||+&|%|!~C(|&|P|Y|=!|!!|:!|=!|?!|?!|="
    "!|A!|H!|=!|I!|I!|K!|J!|Z!|=!|&|#'|V(||+&|%|!~X(|&|P|Y|=!|!!|:!|=!|?!|?!"
    "|=!|A!|N!|=!|O!|O!|M!|P!|Z!|=!|&|#'|+)||+&|%|!~-)|&|P|Y|=!|!!|:!|=!|?!|"
    "?!|=!|A!|M!|=!|N!|N!|O!|O!|Z!|=!|&|/\\|@)|=|+&|%|!~B)|$|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|Z!|=!|&|#'|O)||+&|%|!~Q)|'|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|A!|"
    "S!|B!|N!|=!|O!|O!|-\"|P!|Z!|=!|&|#'|'*||+&|%|!~)*|&|P|Y|=!|!!|:!|=!|?!|"
    "?!|=!|A!|R!|=!|S!|S!|U!|T!|Z!|=!|&|#'|<*||+&|%|!~>*|&|P|Y|=!|!!|:!|=!|?"
    "!|?!|=!|A!|D!|=!|E!|E!|W!|F!|Z!|=!|&|#'|Q*||+&|%|!~S*|$|P|Y|=!|!!|:!|=!"
    "|?!|?!|Y!|A!|Z!|=!|&|#'| +||+&|%|!~\"+|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R!"
    "|=!|S!|S!|[!|T!|Z!|=!|&|#'|5+||+&|%|!~7+|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|"
    "D!|=!|E!|E!|]!|F!|Z!|=!|&|#'|J+||+&|%|!~L+|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A"
    "!|M!|=!|N!|N!|_!|O!|Z!|=!|&|#'|_+||+&|%|!~!,|&|P|Y|=!|!!|:!|=!|?!|?!|=!"
    "|A!|R!|=!|S!|S!|!\"|T!|Z!|=!|&|#'|4,||+&|%|!~6,|&|P|Y|=!|!!|:!|=!|?!|?!"
    "|=!|A!|H!|=!|I!|I!|#\"|J!|Z!|=!|&|#'|I,||+&|%|!~K,|&|P|Y|=!|!!|:!|=!|?!"
    "|?!|=!|A!|S!|=!|T!|T!|%\"|U!|Z!|=!|&|#'|^,||+&|%|!~ -|&|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|H!|=!|I!|I!|'\"|J!|Z!|=!|&|#'|3-||+&|%|!~5-|&|P|Y|=!|!!|:!"
    "|=!|?!|?!|=!|A!|U!|=!|V!|V!|)\"|W!|Z!|=!|&|#'|H-||+&|%|!~J-|&|P|Y|=!|!!"
    "|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|+\"|F!|Z!|=!|&|6F|]-|O|+&|%|!~_-|$|P|Y|="
    "!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|,.||+&|%|!~..|&|P|Y|=!|!!|:!|=!|?!|?!"
    "|=!|A!|M!|=!|N!|N!|/\"|O!|Z!|=!|&|#'|A.||+&|%|!~C.|&|P|Y|=!|!!|:!|=!|?!"
    "|?!|=!|A!|E!|=!|F!|F!|1\"|G!|Z!|=!|&|#'|V.||+&|%|!~X.|&|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|K!|=!|L!|L!|3\"|M!|Z!|=!|&|#'|+/||+&|%|!~-/|&|P|Y|=!|!!|:!"
    "|=!|?!|?!|=!|A!|H!|=!|I!|I!|5\"|J!|Z!|=!|&|#'|@/||+&|%|!~B/|&|P|Y|=!|!!"
    "|:!|=!|?!|?!|=!|A!|B!|=!|C!|C!|7\"|D!|Z!|=!|&|#'|U/||+&|%|!~W/|&|P|Y|=!"
    "|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|9\"|U!|Z!|=!|&|#'|*0||+&|%|!~,0|&|P|Y"
    "|=!|!!|:!|=!|?!|?!|=!|A!|R!|=!|S!|S!|;\"|T!|Z!|=!|&|OB|?0|M|+&|%|!~A0|$"
    "|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|N0||+&|%|!~P0|(|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|D!|=!|E!|E!|?\"|F!|T!|=!|U!|U!|S\"|V!|Z!|=!|&|#'|)1||+&|%|"
    "!~+1|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R!|=!|S!|S!|A\"|T!|Z!|=!|&|#'|>1||+&"
    "|%|!~@1|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|B!|=!|C!|C!|C\"|D!|Z!|=!|&|#'|S1|"
    "|+&|%|!~U1|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Q!|=!|R!|R!|E\"|S!|Z!|=!|&|#'|"
    "(2||+&|%|!~*2|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|H!|=!|I!|I!|G\"|J!|Z!|=!|&|"
    "#'|=2||+&|%|!~?2|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|O!|=!|P!|P!|I\"|Q!|Z!|=!"
    "|&|#'|R2||+&|%|!~T2|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|K\"|U!|Z!"
    "|=!|&|#'|'3||+&|%|!~)3|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|H!|=!|I!|I!|M\"|J!"
    "|Z!|=!|&|#'|<3||+&|%|!~>3|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|N!|=!|O!|O!|O\""
    "|P!|Z!|=!|&|#'|Q3||+&|%|!~S3|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|N!|"
    "Q\"|O!|Z!|=!|&|$Q|&4|>|+&|%|!~(4|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|"
    "#'|54||+&|%|!~74|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|L!|=!|M!|M!|U\"|N!|Z!|=!"
    "|&|#'|J4||+&|%|!~L4|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|O!|=!|P!|P!|W\"|Q!|Z!"
    "|=!|&|#'|_4||+&|%|!~!5|$|P|Y|=!|!!|:!|=!|?!|?!|Y\"|A!|Z!|=!|&|#'|.5||+&"
    "|%|!~05|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R!|=!|S!|S!|[\"|T!|Z!|=!|&|#'|C5|"
    "|+&|%|!~E5|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|]\"|U!|Z!|=!|&|#'|"
    "X5||+&|%|!~Z5|%|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|A!|_\"|B!|Z!|=!|&|#'|*6||+&"
    "|%|!~,6|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|B!|=!|C!|C!|!#|D!|Z!|=!|&|#'|?6||"
    "+&|%|!~A6|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|J!|=!|K!|K!|##|L!|Z!|=!|&|@0\"|"
    "T6|Q|+&|%|!~V6|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|#7||+&|%|!~%7|("
    "|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|L!|=!|M!|M!|'#|N!|Q!|=!|R!|R!|/#|S!|Z!|=!|"
    "&|#'|>7||+&|%|!~@7|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|O!|=!|P!|P!|)#|Q!|Z!|="
    "!|&|#'|S7||+&|%|!~U7|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|+#|U!|Z!"
    "|=!|&|#'|(8||+&|%|!~*8|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|X!|=!|Y!|Y!|-#|Z!|"
    "Z!|=!|&|Y6!|=8|D|+&|%|!~?8|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|L8|"
    "|+&|%|!~N8|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Q!|=!|R!|R!|1#|S!|Z!|=!|&|#'|!"
    "9||+&|%|!~#9|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|N!|=!|O!|O!|3#|P!|Z!|=!|&|#'"
    "|69||+&|%|!~89|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Q!|=!|R!|R!|5#|S!|Z!|=!|&|"
    "E!!|K9|C|+&|%|!~M9|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|Z9||+&|%|!~"
    "\\9|%|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|A!|9#|B!|Z!|=!|&|#'|,:||+&|%|!~.:|&|P"
    "|Y|=!|!!|:!|=!|?!|?!|=!|A!|K!|=!|L!|L!|;#|M!|Z!|=!|&|#'|A:||+&|%|!~C:|&"
    "|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R!|=!|S!|S!|=#|T!|Z!|=!|&|#'|V:||+&|%|!~X:"
    "|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|?#|F!|Z!|=!|&|YR\"|+;|V|+&|%"
    "|!~-;|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|:;||+&|%|!~<;|&|P|Y|=!|!"
    "!|:!|=!|?!|?!|=!|A!|F!|=!|G!|G!|C#|H!|Z!|=!|&|#'|O;||+&|%|!~Q;|&|P|Y|=!"
    "|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|N!|E#|O!|Z!|=!|&|#'|$<||+&|%|!~&<|&|P|Y|"
    "=!|!!|:!|=!|?!|?!|=!|A!|N!|=!|O!|O!|G#|P!|Z!|=!|&|#'|9<||+&|%|!~;<|&|P|"
    "Y|=!|!!|:!|=!|?!|?!|=!|A!|Q!|=!|R!|R!|I#|S!|Z!|=!|&|#'|N<||+&|%|!~P<|&|"
    "P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|K#|F!|Z!|=!|&|X_|#=|<|+&|%|!~%="
    "|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|2=||+&|%|!~4=|&|P|Y|=!|!!|:!|"
    "=!|?!|?!|=!|A!|D!|=!|E!|E!|O#|F!|Z!|=!|&|#'|G=||+&|%|!~I=|&|P|Y|=!|!!|:"
    "!|=!|?!|?!|=!|A!|D!|=!|E!|E!|Q#|F!|Z!|=!|&|#'|\\=||+&|%|!~^=|&|P|Y|=!|!"
    "!|:!|=!|?!|?!|=!|A!|O!|=!|P!|P!|S#|Q!|Z!|=!|&|#'|1>||+&|%|!~3>|$|P|Y|=!"
    "|!!|:!|=!|?!|?!|U#|A!|Z!|=!|&|#'|@>||+&|%|!~B>|&|P|Y|=!|!!|:!|=!|?!|?!|"
    "=!|A!|V!|=!|W!|W!|W#|X!|Z!|=!|&|#'|U>||+&|%|!~W>|&|P|Y|=!|!!|:!|=!|?!|?"
    "!|=!|A!|G!|=!|H!|H!|Y#|I!|Z!|=!|&|#'|*?||+&|%|!~,?|&|P|Y|=!|!!|:!|=!|?!"
    "|?!|=!|A!|H!|=!|I!|I!|[#|J!|Z!|=!|&|#'|??||+&|%|!~A?|&|P|Y|=!|!!|:!|=!|"
    "?!|?!|=!|A!|S!|=!|T!|T!|]#|U!|Z!|=!|&|#'|T?||+&|%|!~V?|&|P|Y|=!|!!|:!|="
    "!|?!|?!|=!|A!|D!|=!|E!|E!|_#|F!|Z!|=!|&|#'|)@||+&|%|!~+@|&|P|Y|=!|!!|:!"
    "|=!|?!|?!|=!|A!|R!|=!|S!|S!|!$|T!|Z!|=!|&|#'|>@||+&|%|!~@@|&|P|Y|=!|!!|"
    ":!|=!|?!|?!|=!|A!|O!|=!|P!|P!|#$|Q!|Z!|=!|&|#'|S@||+&|%|!~U@|%|P|Y|=!|!"
    "!|:!|=!|?!|?!|=!|A!|A!|%$|B!|Z!|=!|&|#'|%A||+&|%|!~'A|&|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|B!|=!|C!|C!|'$|D!|Z!|=!|&|#'|:A||+&|%|!~<A|&|P|Y|=!|!!|:!|"
    "=!|?!|?!|=!|A!|D!|=!|E!|E!|)$|F!|Z!|=!|&|=D|OA|N|+&|%|!~QA|$|P|Y|=!|!!|"
    ":!|=!|?!|?!|=!|A!|Z!|=!|&|#'|^A||+&|%|!~ B|(|P|Y|=!|!!|:!|=!|?!|?!|=!|A"
    "!|D!|=!|E!|E!|-$|F!|N!|=!|O!|O!|7$|P!|Z!|=!|&|#'|9B||+&|%|!~;B|&|P|Y|=!"
    "|!!|:!|=!|?!|?!|=!|A!|W!|=!|X!|X!|/$|Y!|Z!|=!|&|#'|NB||+&|%|!~PB|&|P|Y|"
    "=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|1$|F!|Z!|=!|&|#'|#C||+&|%|!~%C|&|P|"
    "Y|=!|!!|:!|=!|?!|?!|=!|A!|L!|=!|M!|M!|3$|N!|Z!|=!|&|#'|8C||+&|%|!~:C|&|"
    "P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|5$|F!|Z!|=!|&|+^|MC|?|+&|%|!~OC"
    "|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|\\C||+&|%|!~^C|&|P|Y|=!|!!|:!"
    "|=!|?!|?!|=!|A!|N!|=!|O!|O!|9$|P!|Z!|=!|&|#'|1D||+&|%|!~3D|&|P|Y|=!|!!|"
    ":!|=!|?!|?!|=!|A!|J!|=!|K!|K!|;$|L!|Z!|=!|&|#'|FD||+&|%|!~HD|%|P|Y|=!|!"
    "!|:!|=!|?!|?!|=!|A!|A!|=$|B!|Z!|=!|&|#'|XD||+&|%|!~ZD|&|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|G!|=!|H!|H!|?$|I!|Z!|=!|&|#'|-E||+&|%|!~/E|&|P|Y|=!|!!|:!|"
    "=!|?!|?!|=!|A!|D!|=!|E!|E!|A$|F!|Z!|=!|&|#'|BE||+&|%|!~DE|%|P|Y|=!|!!|:"
    "!|=!|?!|?!|=!|A!|A!|C$|B!|Z!|=!|&|#'|TE||+&|%|!~VE|&|P|Y|=!|!!|:!|=!|?!"
    "|?!|=!|A!|C!|=!|D!|D!|E$|E!|Z!|=!|&|#'|)F||+&|%|!~+F|&|P|Y|=!|!!|:!|=!|"
    "?!|?!|=!|A!|R!|=!|S!|S!|G$|T!|Z!|=!|&|_@|>F|P|+&|%|!~@F|$|P|Y|=!|!!|:!|"
    "=!|?!|?!|=!|A!|Z!|=!|&|#'|MF||+&|%|!~OF|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|O"
    "!|=!|P!|P!|K$|Q!|Z!|=!|&|#'|\"G||+&|%|!~$G|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A"
    "!|S!|=!|T!|T!|M$|U!|Z!|=!|&|#'|7G||+&|%|!~9G|&|P|Y|=!|!!|:!|=!|?!|?!|=!"
    "|A!|H!|=!|I!|I!|O$|J!|Z!|=!|&|#'|LG||+&|%|!~NG|&|P|Y|=!|!!|:!|=!|?!|?!|"
    "=!|A!|N!|=!|O!|O!|Q$|P!|Z!|=!|&|#'|!H||+&|%|!~#H|&|P|Y|=!|!!|:!|=!|?!|?"
    "!|=!|A!|M!|=!|N!|N!|S$|O!|Z!|=!|&|#'|6H||+&|%|!~8H|&|P|Y|=!|!!|:!|=!|?!"
    "|?!|=!|A!|R!|=!|S!|S!|U$|T!|Z!|=!|&|5?|KH|Y|+&|%|!~MH|$|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|Z!|=!|&|#'|ZH||+&|%|!~\\H|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Q!"
    "|=!|R!|R!|Y$|S!|Z!|=!|&|#'|/I||+&|%|!~1I|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|"
    "D!|=!|E!|E!|[$|F!|Z!|=!|&|#'|DI||+&|%|!~FI|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A"
    "!|B!|=!|C!|C!|]$|D!|Z!|=!|&|#'|YI||+&|%|!~[I|&|P|Y|=!|!!|:!|=!|?!|?!|=!"
    "|A!|D!|=!|E!|E!|_$|F!|Z!|=!|&|#'|.J||+&|%|!~0J|&|P|Y|=!|!!|:!|=!|?!|?!|"
    "=!|A!|C!|=!|D!|D!|!%|E!|Z!|=!|&|#'|CJ||+&|%|!~EJ|&|P|Y|=!|!!|:!|=!|?!|?"
    "!|=!|A!|D!|=!|E!|E!|#%|F!|Z!|=!|&|#'|XJ||+&|%|!~ZJ|&|P|Y|=!|!!|:!|=!|?!"
    "|?!|=!|A!|M!|=!|N!|N!|%%|O!|Z!|=!|&|#'|-K||+&|%|!~/K|&|P|Y|=!|!!|:!|=!|"
    "?!|?!|=!|A!|B!|=!|C!|C!|'%|D!|Z!|=!|&|#'|BK||+&|%|!~DK|&|P|Y|=!|!!|:!|="
    "!|?!|?!|=!|A!|D!|=!|E!|E!|)%|F!|Z!|=!|&|:Z|WK|@|+&|%|!~YK|$|P|Y|=!|!!|:"
    "!|=!|?!|?!|=!|A!|Z!|=!|&|#'|&L||+&|%|!~(L|(|P|Y|=!|!!|:!|=!|?!|?!|=!|A!"
    "|D!|=!|E!|E!|-%|F!|T!|=!|U!|U!|5%|V!|Z!|=!|&|#'|AL||+&|%|!~CL|&|P|Y|=!|"
    "!!|:!|=!|?!|?!|=!|A!|F!|=!|G!|G!|/%|H!|Z!|=!|&|#'|VL||+&|%|!~XL|&|P|Y|="
    "!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|1%|F!|Z!|=!|&|#'|+M||+&|%|!~-M|&|P|Y"
    "|=!|!!|:!|=!|?!|?!|=!|A!|W!|=!|X!|X!|3%|Y!|Z!|=!|&|ZR|@M|A|+&|%|!~BM|$|"
    "P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|OM||+&|%|!~QM|&|P|Y|=!|!!|:!|=!|"
    "?!|?!|=!|A!|K!|=!|L!|L!|7%|M!|Z!|=!|&|#'|$N||+&|%|!~&N|&|P|Y|=!|!!|:!|="
    "!|?!|?!|=!|A!|D!|=!|E!|E!|9%|F!|Z!|=!|&|#'|9N||+&|%|!~;N|&|P|Y|=!|!!|:!"
    "|=!|?!|?!|=!|A!|R!|=!|S!|S!|;%|T!|Z!|=!|&|5$!|NN|/|+&|%|!~PN|$|P|Y|=!|!"
    "!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|]N||+&|%|!~_N|*|P|Y|=!|!!|:!|=!|?!|?!|=!"
    "|A!|D!|=!|E!|E!|?%|F!|N!|=!|O!|O!|M%|P!|Q!|=!|R!|R!|#&|S!|Z!|=!|&|#'|>O"
    "||+&|%|!~@O|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|L!|=!|M!|M!|A%|N!|Z!|=!|&|#'|"
    "SO||+&|%|!~UO|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|O!|=!|P!|P!|C%|Q!|Z!|=!|&|#"
    "'|(P||+&|%|!~*P|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|K!|=!|L!|L!|E%|M!|Z!|=!|&"
    "|#'|=P||+&|%|!~?P|%|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|A!|G%|B!|Z!|=!|&|#'|OP|"
    "|+&|%|!~QP|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|I%|U!|Z!|=!|&|#'|$"
    "Q||+&|%|!~&Q|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|K%|F!|Z!|=!|&|1O"
    "|9Q|B|+&|%|!~;Q|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|HQ||+&|%|!~JQ|"
    "&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|J!|=!|K!|K!|O%|L!|Z!|=!|&|#'|]Q||+&|%|!~_"
    "Q|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|Q%|F!|Z!|=!|&|#'|2R||+&|%|!"
    "~4R|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|N!|S%|O!|Z!|=!|&|#'|GR||+&|%"
    "|!~IR|&|P|Y|=!|!!|:!|=!|?!|?!|U%|A!|R!|=!|S!|S!|!&|T!|Z!|=!|&|#'|\\R||+"
    "&|%|!~^R|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|B!|=!|C!|C!|W%|D!|Z!|=!|&|#'|1S|"
    "|+&|%|!~3S|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|N!|=!|O!|O!|Y%|P!|Z!|=!|&|#'|F"
    "S||+&|%|!~HS|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|T!|=!|U!|U!|[%|V!|Z!|=!|&|#'"
    "|[S||+&|%|!~]S|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|N!|]%|O!|Z!|=!|&|"
    "#'|0T||+&|%|!~2T|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|_%|U!|Z!|=!|"
    "&|5I\"|ET|F|+&|%|!~GT|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|5I|TT|J|+&|"
    "%|!~VT|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|#'|#U||+&|%|!~%U|&|P|Y|=!|"
    "!!|:!|=!|?!|?!|=!|A!|T!|=!|U!|U!|%&|V!|Z!|=!|&|#'|8U||+&|%|!~:U|&|P|Y|="
    "!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|'&|F!|Z!|=!|&|JQ\"|MU|U|+&|%|!~OU|$|"
    "P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|8-!|\\U|'|+&|%|!~^U||'|!~_U|9|!~_U|"
    "'|'|!|>|!~\"V|(|<,|\"V|!|>|<,|#V|(|)1|#V|\"|>|)1|$V|7|74|$V|&|!|'|L3|&V"
    "|9|L3|&V|'|'|!|>|L3|)V|7|15|)V|&||'|F4|+V|9|F4|+V|'|'|!|>|F4|.V|)|':|.V"
    "|\"|+|':|/V|#|+|Z:|0V|$|-|/;|1V||\"|#~0|1;|4V|||!~,|Z:|7V|\"|$|#|2|[:|:"
    "V|#|-|+;|;V||\"|#~3|*;|>V||+|W;|?V|$|-|+<|@V||\"|\"~0|-<|CV|||!~,|W;|FV"
    "|\"|$|#|2|X;|IV|$|-|'<|JV||\"|\"~3|&<|MV||+|S<|NV|$|-|&=|OV||\"|!~0|(=|"
    "RV|||!~,|S<|UV|\"|$|#|2|T<|XV|%|-|\"=|YV||\"|!~3|!=|\\V||,|':|]V|\"|#|#"
    "|2|(:| W|\"|-|5:|!W||\"|!~3|4:|$W||*|':|%W|#|>|K7|&W|)|$8|&W|$|+|$8|'W|"
    "#|,|$8|(W|$|#||2|$8|+W| \"|*|$8|,W||>|$8|-W|)|Q8|-W|\"|+|Q8|.W|#|,|Q8|/"
    "W|\"|#||2|Q8|2W| \"|*|Q8|3W||>|Q8|4W|)|)@|4W|$|+|)@|5W|#|-|:@|6W||$|!~0"
    "|<@|9W|||!~,|)@|<W|$|#|\"|2|*@|?W|#|*|)@|@W|\"|>|\\>|AW|)|5?|AW|\"|+|5?"
    "|BW|#|-|5?|CW||\"|\"~0|5?|FW|||!~-|5?|IW||\"|!~/|5?|LW||,|5?|MW|\"|#|\""
    "|2|5?|PW|_!|*|5?|QW|\"|>|5?|RW|)|5?|RW|$|+|5?|SW|#|,|5?|TW|$|#||2|5?|WW"
    "|_!|*|5?|XW||>|5?|YW|)|[A|YW|\"|+|[A|ZW|#|-|,B|[W||\"|!~/|+B|^W||,|[A|_"
    "W|\"|#|#|2|\\A|\"X|&|*|[A|#X|#|>|F@|$X|)|JC|$X|$|+|JC|%X|#|-|ZC|&X||$|!"
    "~/|YC|)X||,|JC|*X|$|#|#|2|KC|-X|(|*|JC|.X|#|>|6B|/X|)|>E|/X|\"|+|>E|0X|"
    "#|-|SE|1X||\"|!~/|RE|4X||,|>E|5X|\"|#|#|2|?E|8X|)|*|>E|9X|#|>|$D|:X|)|6"
    "G|:X|$|+|6G|;X|#|-|JG|<X||$|!~/|IG|?X||,|6G|@X|$|#|#|2|7G|CX|*|*|6G|DX|"
    "#|>|]E|EX|)|'J|EX|\"|+|'J|FX|#|-|7J|GX||\"|!~0|9J|JX|||!~,|'J|MX|\"|#|\""
    "|2|(J|PX|$|*|'J|QX|\"|>|\\H|RX|)|5I|RX|$|+|5I|SX|#|-|5I|TX||$|\"~0|5I|W"
    "X|||!~-|5I|ZX||$|!~/|5I|]X||,|5I|^X|$|#|\"|2|5I|!Y|_!|*|5I|\"Y|\"|>|5I|"
    "#Y|)|5I|#Y|\"|+|5I|$Y|#|,|5I|%Y|\"|#||2|5I|(Y|_!|*|5I|)Y||>|5I|*Y|)|IK|"
    "*Y|$|+|IK|+Y|#|-| L|,Y||$|!~/|_K|/Y||+|#L|0Y|\"|,|#L|1Y|$|\"|!|2|$L|4Y|"
    ",|,|IK|5Y|$|#|!|2|JK|8Y|+|*|IK|9Y|!|>|CJ|:Y|)|WM|:Y|\"|+|WM|;Y|#|-|.N|<"
    "Y||\"|#~/|-N|?Y||+|1N|@Y|$|-|KN|AY||\"|!~0|MN|DY|||!~,|1N|GY|\"|$|#|2|2"
    "N|JY|,|-|GN|KY||\"|!~3|FN|NY||,|WM|OY|\"|#|#|2|XM|RY|+|*|WM|SY|#|>|@L|T"
    "Y|)|(M|TY|$|+|(M|UY|#|-|(M|VY||$|\"~0|(M|YY|||!~-|(M|\\Y||$|!~/|(M|_Y||"
    ",|(M| Z|$|#|\"|2|(M|#Z|_!|*|(M|$Z|\"|>|(M|%Z|)|(M|%Z|\"|+|(M|&Z|#|,|(M|"
    "'Z|\"|#||2|(M|*Z|_!|*|(M|+Z||>|(M|,Z|)|-P|,Z|$|+|-P|-Z|#|-|AP|.Z||$|!~/"
    "|@P|1Z||,|-P|2Z|$|#|#|2|.P|5Z|-|*|-P|6Z|#|>|XN|7Z|)| R|7Z|\"|+| R|8Z|#|"
    "-|7R|9Z||\"|!~/|6R|<Z||,| R|=Z|\"|#|#|2|!R|@Z|.|*| R|AZ|#|>|KP|BZ|)|PS|"
    "BZ|$|+|PS|CZ|#|+|$T|DZ|\"|+|4T|EZ|%|,|4T|FZ|$|%|#|2|5T|IZ|!|4|>T|JZ|!~-"
    "|GT|KZ||$|!~/|FT|NZ||,|$T|OZ|$|\"|#|2|%T|RZ|0|,|PS|SZ|$|#|#|2|QS|VZ|/|*"
    "|PS|WZ|#|>|AR|XZ|)|!V|XZ|%|+|!V|YZ|\"|-|6V|ZZ||%|!~0|8V|]Z|||!~,|!V| [|"
    "%|\"|#|2|\"V|#[|/|*|!V|$[|#|>|RT|%[|)|3U|%[|#|+|3U|&[|$|-|3U|'[||#|\"~0"
    "|3U|*[|||!~-|3U|-[||#|!~/|3U|0[||,|3U|1[|#|$|\"|2|3U|4[|_!|*|3U|5[|\"|>"
    "|3U|6[|)|3U|6[|\"|+|3U|7[|%|-|3U|8[||\"|!~/|3U|;[||,|3U|<[|\"|%|!|2|3U|"
    "?[|_!|*|3U|@[|!|>|3U|A[|)|#X|A[|$|+|#X|B[|#|-|4X|C[||$|$~/|3X|F[||-|8X|"
    "G[||$|!~/|7X|J[||,|#X|K[|$|#|%|2|$X|N[|0|*|#X|O[|%|>|BV|P[|)|EY|P[|%|+|"
    "EY|Q[|\"|-|WY|R[||%|!~/|VY|U[||,|EY|V[|%|\"|!|2|FY|Y[|U|*|EY|Z[|!|>|BX|"
    "[[|)|6[|[[|#|+|6[|\\[|$|-|L[|][||#|!~/|K[| \\||,|6[|!\\|#|$|#|2|7[|$\\|"
    "1|*|6[|%\\|#|>|!Z|&\\|)|6]|&\\|\"|+|6]|'\\|%|-|H]|(\\||\"|\"~/|G]|+\\||"
    ",|6]|,\\|\"|%|%|2|7]|/\\|2|*|6]|0\\|%|>|V[|1\\|)|#_|1\\|$|+|#_|2\\|#|-|"
    "5_|3\\||$|!~/|4_|6\\||,|#_|7\\|$|#|#|2|$_|:\\|3|*|#_|;\\|#|>|R]|<\\|)|P"
    " !|<\\|%|+|P !|=\\|\"|-|\"!!|>\\||%|!~/|!!!|A\\||,|P !|B\\|%|\"|#|2|Q !"
    "|E\\|4|*|P !|F\\|#|>|?_|G\\|)|;\"!|G\\|#|+|;\"!|H\\|$|-|L\"!|I\\||#|!~/"
    "|K\"!|L\\||,|;\"!|M\\|#|$|#|2|<\"!|P\\|5|*|;\"!|Q\\|#|>|,!!|R\\|)|\"%!|"
    "R\\|\"|+|\"%!|S\\|%|-|1%!|T\\||\"|!~0|3%!|W\\|||!~,|\"%!|Z\\|\"|%|\"|2|"
    "#%!|]\\|%|*|\"%!|^\\|\"|>|\\#!|_\\|)|5$!|_\\|$|+|5$!| ]|#|,|5$!|!]|$|#|"
    "|2|5$!|$]| \"|*|5$!|%]||>|5$!|&]|)|\\#!|&]|%|+|\\#!|']|\"|-|\\#!|(]||%|"
    "\"~0|\\#!|+]|||!~-|\\#!|.]||%|!~/|\\#!|1]||,|\\#!|2]|%|\"|\"|2|\\#!|5]|"
    "_!|*|\\#!|6]|\"|>|\\#!|7]|)|\\#!|7]|#|+|\\#!|8]|$|-|\\#!|9]||#|!~/|\\#!"
    "|<]||,|\\#!|=]|#|$|!|2|\\#!|@]|_!|*|\\#!|A]|!|>|\\#!|B]|)|)'!|B]|\"|+|)"
    "'!|C]|%|-|8'!|D]||\"|$~/|7'!|G]||-|<'!|H]||\"|\"~/|;'!|K]||-|@'!|L]||\""
    "|!~.|@'!|O]|||/|?'!|Q]||-|F'!|R]||\"|!~.|F'!|U]||!|/|E'!|W]||,|)'!|X]|\""
    "|%|$|2|*'!|[]|6|-|4'!|\\]||\"|#~3|3'!|_]||*|)'!| ^|$|>|=%!|!^|)|3)!|!^|"
    "$|+|3)!|\"^|#|-|5)!|#^||$|#~/|4)!|&^||-|9)!|'^||$|!~/|8)!|*^||,|3)!|+^|"
    "$|#|$|2|3)!|.^|!\"|*|3)!|/^|$|>|R'!|0^|)|N*!|0^|%|+|N*!|1^|\"|-|P*!|2^|"
    "|%|!~/|O*!|5^||+|S*!|6^|#|,|S*!|7^|%|#|\"|2|T*!|:^|!|4|]*!|;^|!~,|N*!|<"
    "^|%|\"|\"|2|N*!|?^|!\"|*|N*!|@^|\"|>|C)!|A^|)|),!|A^|$|+|),!|B^|#|+|*,!"
    "|C^|\"|,|*,!|D^|$|\"||2|+,!|G^|!|4|4,!|H^|!~+|<,!|I^|%|,|<,!|J^|$|%||2|"
    "=,!|M^|!|4|F,!|N^|!~,|),!|O^|$|#||2|),!|R^|!\"|*|),!|S^||>|,+!|T^|)|*.!"
    "|T^|\"|+|*.!|U^|%|-|<.!|V^||\"|\"~/|;.!|Y^||-|@.!|Z^||\"|!~0|B.!|]^|||!"
    "~,|*.!| _|\"|%|\"|2|+.!|#_|7|*|*.!|$_|\"|>|U,!|%_|)|.-!|%_|#|+|.-!|&_|$"
    "|-|.-!|'_||#|\"~0|.-!|*_|||!~-|.-!|-_||#|!~/|.-!|0_||,|.-!|1_|#|$|\"|2|"
    ".-!|4_|_!|*|.-!|5_|\"|>|.-!|6_|)|F-!|6_|%|-|G-!|7_|!|%|!~/|F-!|:_|!|*|F"
    "-!|;_|\"|>|!~<_|)|.-!|<_|\"|+|.-!|=_|$|,|.-!|>_|\"|$||2|.-!|A_|_!|*|.-!"
    "|B_||>|.-!|C_|)|U/!|C_|#|+|U/!|D_|%|-|#0!|E_|!|#|!~0|%0!|H_|!||!~,|U/!|"
    "K_|#|%|!|2|V/!|N_|8|*|U/!|O_|!|>|L.!|P_|)|L.!|P_|$|+|L.!|Q_|\"|-|L.!|R_"
    "|!|$|\"~0|L.!|U_|!||!~-|L.!|X_|!|$|!~/|L.!|[_|!|,|L.!|\\_|$|\"|\"|2|L.!"
    "|__|_!|*|L.!|  !|\"|>|L.!|! !|)|L.!|! !|%|+|L.!|\" !|#|-|L.!|# !|!|%|!~"
    "/|L.!|& !|!|,|L.!|' !|%|#|!|2|L.!|* !|_!|*|L.!|+ !|!|>|L.!|, !|)|71!|, "
    "!|\"|+|71!|- !|$|-|M1!|. !|!|\"|\"~/|L1!|1 !|!|,|71!|2 !|\"|$|\"|2|81!|"
    "5 !|:|-|I1!|6 !|!|\"|!~3|H1!|9 !|!|*|71!|: !|\"|>|/0!|; !|)|_2!|; !|#|+"
    "|_2!|< !|%|-|43!|= !|!|#|\"~/|33!|@ !|!|,|_2!|A !|#|%|\"|2| 3!|D !|;|-|"
    "03!|E !|!|#|!~3|/3!|H !|!|*|_2!|I !|\"|>|W1!|J !|)|F4!|J !|$|+|F4!|K !|"
    "\"|-|Y4!|L !|!|$|\"~/|X4!|O !|!|,|F4!|P !|$|\"|\"|2|G4!|S !|9|-|U4!|T !"
    "|!|$|!~3|T4!|W !|!|*|F4!|X !|\"|>|>3!|Y !|)|?7!|Y !|%|+|?7!|Z !|#|,|?7!"
    "|[ !|%|#|!|2|@7!|^ !|F|*|?7!|_ !|!|>|@6!| !!|)|*9!| !!|\"|+|*9!|!!!|$|-"
    "|69!|\"!!|!|\"|#~/|59!|%!!|!|-|:9!|&!!|!|\"|\"~.|:9!|)!!|!||/|99!|+!!|!"
    "|-|@9!|,!!|!|\"|\"~.|@9!|/!!|!|!|/|?9!|1!!|!|,|*9!|2!!|\"|$|$|2|+9!|5!!"
    "|<|*|*9!|6!!|$|>|Q7!|7!!|)|>;!|7!!|#|+|>;!|8!!|%|-|W;!|9!!|!|#|$~/|V;!|"
    "<!!|!|-|[;!|=!!|!|#|\"~/|Z;!|@!!|!|+|^;!|A!!|$|-|;<!|B!!|!|#|!~0|=<!|E!"
    "!|!||!~,|^;!|H!!|#|$|$|2|_;!|K!!|>|-|7<!|L!!|!|#|!~3|6<!|O!!|!|,|>;!|P!"
    "!|#|%|$|2|?;!|S!!|=|-|S;!|T!!|!|#|#~3|R;!|W!!|!|*|>;!|X!!|$|>|L9!|Y!!|)"
    "|@:!|Y!!|\"|+|@:!|Z!!|$|-|@:!|[!!|!|\"|\"~0|@:!|^!!|!||!~-|@:!|!\"!|!|\""
    "|!~/|@:!|$\"!|!|,|@:!|%\"!|\"|$|\"|2|@:!|(\"!|_!|*|@:!|)\"!|\"|>|@:!|*\""
    "!|)|@:!|*\"!|%|+|@:!|+\"!|#|-|@:!|,\"!|!|%|!~/|@:!|/\"!|!|,|@:!|0\"!|%|"
    "#|!|2|@:!|3\"!|_!|*|@:!|4\"!|!|>|@:!|5\"!|)|O=!|5\"!|$|+|O=!|6\"!|\"|-|"
    "(>!|7\"!|!|$|\"~/|'>!|:\"!|!|+|+>!|;\"!|#|-|F>!|<\"!|!|$|!~0|H>!|?\"!|!"
    "||!~,|+>!|B\"!|$|#|\"|2|,>!|E\"!|B|-|B>!|F\"!|!|$|!~3|A>!|I\"!|!|,|O=!|"
    "J\"!|$|\"|\"|2|P=!|M\"!|?|*|O=!|N\"!|\"|>|H<!|O\"!|)|!=!|O\"!|%|+|!=!|P"
    "\"!|#|-|!=!|Q\"!|!|%|\"~0|!=!|T\"!|!||!~-|!=!|W\"!|!|%|!~/|!=!|Z\"!|!|,"
    "|!=!|[\"!|%|#|\"|2|!=!|^\"!|_!|*|!=!|_\"!|\"|>|!=!| #!|)|!=!| #!|\"|+|!"
    "=!|!#!|$|-|!=!|\"#!|!|\"|!~/|!=!|%#!|!|,|!=!|&#!|\"|$|!|2|!=!|)#!|_!|*|"
    "!=!|*#!|!|>|!=!|+#!|)|O?!|+#!|#|+|O?!|,#!|%|,|O?!|-#!|#|%|!|2|P?!|0#!|@"
    "|*|O?!|1#!|!|>|S>!|2#!|)|%A!|2#!|$|+|%A!|3#!|\"|,|%A!|4#!|$|\"|!|2|&A!|"
    "7#!|A|*|%A!|8#!|!|>|)@!|9#!|)|UB!|9#!|%|+|UB!|:#!|#|-|,C!|;#!|!|%|\"~/|"
    "+C!|>#!|!|-|0C!|?#!|!|%|!~.|0C!|B#!|!||/|/C!|D#!|!|-|6C!|E#!|!|%|!~.|6C"
    "!|H#!|!|!|/|5C!|J#!|!|,|UB!|K#!|%|#|\"|2|VB!|N#!|C|*|UB!|O#!|\"|>|@A!|P"
    "#!|)|/F!|P#!|\"|+|/F!|Q#!|$|-|GF!|R#!|!|\"|!~/|FF!|U#!|!|,|/F!|V#!|\"|$"
    "|!|2|0F!|Y#!|D|*|/F!|Z#!|!|>|)E!|[#!|)|ZG!|[#!|#|+|ZG!|\\#!|%|-|5H!|]#!"
    "|!|#|!~/|4H!| $!|!|,|ZG!|!$!|#|%|!|2|[G!|$$!|E|*|ZG!|%$!|!|>|QF!|&$!|)|"
    "!L!|&$!|$|+|!L!|'$!|\"|,|!L!|($!|$|\"||2|\"L!|+$!|!|*|!L!|,$!||>|$K!|-$"
    "!|)|\"N!|-$!|%|+|\"N!|.$!|#|-|2N!|/$!|!|%|\"~0|4N!|2$!|!||!~,|\"N!|5$!|"
    "%|#|#|2|#N!|8$!|G|*|\"N!|9$!|#|>|2L!|:$!|)|2M!|:$!|\"|+|2M!|;$!|$|-|4M!"
    "|<$!|!|\"|\"~/|3M!|?$!|!|-|8M!|@$!|!|\"|!~0|:M!|C$!|!||!~,|2M!|F$!|\"|$"
    "|\"|2|2M!|I$!|\"\"|*|2M!|J$!|\"|>|!~K$!|)|QL!|K$!|#|+|QL!|L$!|%|-|QL!|M"
    "$!|!|#|\"~0|QL!|P$!|!||!~-|QL!|S$!|!|#|!~/|QL!|V$!|!|,|QL!|W$!|#|%|\"|2"
    "|QL!|Z$!|_!|*|QL!|[$!|\"|>|QL!|\\$!|)|*M!|\\$!|$|-|+M!|]$!|\"|$|!~/|*M!"
    "| %!|\"|*|*M!|!%!|\"|>|!~\"%!|)|[L!|\"%!|\"|+|[L!|#%!|%|,|[L!|$%!|\"|%|"
    "|2|[L!|'%!| \"|*|[L!|(%!||>|[L!|)%!|)|QL!|)%!|#|+|QL!|*%!|$|,|QL!|+%!|#"
    "|$||2|QL!|.%!|_!|*|QL!|/%!||>|QL!|0%!|)|!~0%!|%|+|!~1%!|\"|,|!~2%!|%|\""
    "||2|!~5%!| \"|*|!~6%!||>|!~7%!|)|@P!|7%!|$|+|@P!|8%!|#|-|OP!|9%!|\"|$|#"
    "~/|NP!|<%!|\"|-|SP!|=%!|\"|$|!~/|RP!|@%!|\"|,|@P!|A%!|$|#|$|2|AP!|D%!|I"
    "|*|@P!|E%!|$|>|'O!|F%!|)|%R!|F%!|\"|+|%R!|G%!|%|-|4R!|H%!|\"|\"|!~/|3R!"
    "|K%!|\"|+|7R!|L%!|#|,|7R!|M%!|\"|#|\"|2|8R!|P%!|!|4|AR!|Q%!|!~,|%R!|R%!"
    "|\"|%|\"|2|&R!|U%!|I|*|%R!|V%!|\"|>|]P!|W%!|)|XS!|W%!|$|+|XS!|X%!|#|+|&"
    "T!|Y%!|%|,|&T!|Z%!|$|%|\"|2|'T!|]%!|!|4|0T!|^%!|!~-|9T!|_%!|\"|$|!~/|8T"
    "!|\"&!|\"|,|XS!|#&!|$|#|\"|2|YS!|&&!|I|*|XS!|'&!|\"|>|PR!|(&!|)|EU!|(&!"
    "|\"|+|EU!|)&!|%|,|EU!|*&!|\"|%|!|2|FU!|-&!|X|-|VU!|.&!|\"|\"|!~5|UU!|1&"
    "!|\"|*|EU!|2&!|!|>|CT!|3&!|)|(W!|3&!|#|+|(W!|4&!|$|-|6W!|5&!|\"|#|!~/|5"
    "W!|8&!|\"|,|(W!|9&!|#|$|\"|2|)W!|<&!|J|*|(W!|=&!|\"|>| V!|>&!|)|HX!|>&!"
    "|%|+|HX!|?&!|\"|-|ZX!|@&!|\"|%|!~/|YX!|C&!|\"|,|HX!|D&!|%|\"|\"|2|IX!|G"
    "&!|K|*|HX!|H&!|\"|>|@W!|I&!|)|+Z!|I&!|$|+|+Z!|J&!|#|-|CZ!|K&!|\"|$|!~/|"
    "BZ!|N&!|\"|,|+Z!|O&!|$|#|\"|2|,Z!|R&!|L|*|+Z!|S&!|\"|>|$Y!|T&!|)|U[!|T&"
    "!|\"|+|U[!|U&!|%|-|%\\!|V&!|\"|\"|!~/|$\\!|Y&!|\"|,|U[!|Z&!|\"|%|\"|2|V"
    "[!|]&!|M|*|U[!|^&!|\"|>|MZ!|_&!|)|6]!|_&!|#|+|6]!| '!|$|-|L]!|!'!|\"|#|"
    "!~/|K]!|$'!|\"|,|6]!|%'!|#|$|\"|2|7]!|('!|N|*|6]!|)'!|\"|>|/\\!|*'!|)|#"
    " \"|*'!|%|+|# \"|+'!|\"|-|0 \"|,'!|\"|%|\"~/|/ \"|/'!|\"|-|4 \"|0'!|\"|"
    "%|!~0|6 \"|3'!|\"||!~,|# \"|6'!|%|\"|\"|2|$ \"|9'!|P|*|# \"|:'!|\"|>|@^"
    "!|;'!|)|Y^!|;'!|$|+|Y^!|<'!|#|-|Y^!|='!|\"|$|\"~0|Y^!|@'!|\"||!~-|Y^!|C"
    "'!|\"|$|!~/|Y^!|F'!|\"|,|Y^!|G'!|$|#|\"|2|Y^!|J'!|_!|*|Y^!|K'!|\"|>|Y^!"
    "|L'!|)|?_!|L'!|\"|-|@_!|M'!|#|\"|!~/|?_!|P'!|#|*|?_!|Q'!|\"|>|!~R'!|)|Y"
    "^!|R'!|%|+|Y^!|S'!|#|,|Y^!|T'!|%|#||2|Y^!|W'!|_!|*|Y^!|X'!||>|Y^!|Y'!|)"
    "|\"\"\"|Y'!|$|+|\"\"\"|Z'!|\"|-|1\"\"|['!|#|$|#~/|0\"\"|^'!|#|-|5\"\"|_"
    "'!|#|$|!~/|4\"\"|\"(!|#|,|\"\"\"|#(!|$|\"|#|2|#\"\"|&(!|Q|*|\"\"\"|'(!|"
    "#|>|@ \"|((!|)|+$\"|((!|#|+|+$\"|)(!|%|,|+$\"|*(!|#|%||2|,$\"|-(!|Y|6|8"
    "$\"|.(!||*|+$\"|/(!||>|.#\"|0(!|)|1&\"|0(!|\"|+|1&\"|1(!|$|,|1&\"|2(!|\""
    "|$||2|2&\"|5(!|Z|6|F&\"|6(!||*|1&\"|7(!||>|4%\"|8(!|)|X'\"|8(!|%|+|X'\""
    "|9(!|#|,|X'\"|:(!|%|#|\"|2|Y'\"|=(!|Z|-|.(\"|>(!|#|%|!~5|-(\"|A(!|#|*|X"
    "'\"|B(!|\"|>|S&\"|C(!|)|9)\"|C(!|$|+|9)\"|D(!|\"|,|9)\"|E(!|$|\"|!|2|:)"
    "\"|H(!|Y|-|G)\"|I(!|#|$|!~5|F)\"|L(!|#|*|9)\"|M(!|!|>|8(\"|N(!|)|$+\"|N"
    "(!|#|-|%+\"|O(!|$|#|\"~/|$+\"|R(!|$|*|$+\"|S(!|#|>|Q)\"|T(!|)|@-\"|T(!|"
    "%|+|@-\"|U(!|\"|-|Z-\"|V(!|$|%|!~0|\\-\"|Y(!|$||!~,|@-\"|\\(!|%|\"|!|2|"
    "A-\"|_(!|M!|*|@-\"| )!|!|>|8,\"|!)!|)|8,\"|!)!|$|+|8,\"|\")!|#|-|8,\"|#"
    ")!|$|$|\"~0|8,\"|&)!|$||!~-|8,\"|))!|$|$|!~/|8,\"|,)!|$|,|8,\"|-)!|$|#|"
    "\"|2|8,\"|0)!|_!|*|8,\"|1)!|\"|>|8,\"|2)!|)|8,\"|2)!|\"|+|8,\"|3)!|%|,|"
    "8,\"|4)!|\"|%||2|8,\"|7)!|_!|*|8,\"|8)!||>|8,\"|9)!|)|B/\"|9)!|#|+|B/\""
    "|:)!|$|-|Y/\"|;)!|$|#|$~/|X/\"|>)!|$|-|]/\"|?)!|$|#|\"~/|\\/\"|B)!|$|,|"
    "B/\"|C)!|#|$|$|2|C/\"|F)!|N!|-|U/\"|G)!|$|#|#~3|T/\"|J)!|$|*|B/\"|K)!|$"
    "|>|&.\"|L)!|)|/1\"|L)!|%|+|/1\"|M)!|\"|,|/1\"|N)!|%|\"|\"|2|01\"|Q)!|]!"
    "|*|/1\"|R)!|\"|>|'0\"|S)!|)|C2\"|S)!|$|+|C2\"|T)!|#|-|R2\"|U)!|$|$|#~/|"
    "Q2\"|X)!|$|-|V2\"|Y)!|$|$|\"~/|U2\"|\\)!|$|,|C2\"|])!|$|#|#|2|D2\"| *!|"
    "[!|*|C2\"|!*!|#|>|!~\"*!|)|V3\"|\"*!|\"|+|V3\"|#*!|%|-|&4\"|$*!|$|\"|#~"
    "/|%4\"|'*!|$|-|*4\"|(*!|$|\"|\"~/|)4\"|+*!|$|,|V3\"|,*!|\"|%|#|2|W3\"|/"
    "*!|Z!|*|V3\"|0*!|#|>|!~1*!|)|*5\"|1*!|#|+|*5\"|2*!|$|-|<5\"|3*!|$|#|#~/"
    "|;5\"|6*!|$|-|@5\"|7*!|$|#|\"~/|?5\"|:*!|$|,|*5\"|;*!|#|$|#|2|+5\"|>*!|"
    "O!|*|*5\"|?*!|#|>|!~@*!|)|@6\"|@*!|%|+|@6\"|A*!|\"|-|U6\"|B*!|$|%|#~/|T"
    "6\"|E*!|$|-|Y6\"|F*!|$|%|\"~/|X6\"|I*!|$|,|@6\"|J*!|%|\"|#|2|A6\"|M*!|P"
    "!|*|@6\"|N*!|#|>|!~O*!|)|Y7\"|O*!|$|+|Y7\"|P*!|#|-|.8\"|Q*!|$|$|#~/|-8\""
    "|T*!|$|-|28\"|U*!|$|$|\"~/|18\"|X*!|$|,|Y7\"|Y*!|$|#|#|2|Z7\"|\\*!|Q!|*"
    "|Y7\"|]*!|#|>|!~^*!|)|29\"|^*!|\"|+|29\"|_*!|%|-|H9\"| +!|$|\"|#~/|G9\""
    "|#+!|$|-|L9\"|$+!|$|\"|\"~/|K9\"|'+!|$|,|29\"|(+!|\"|%|#|2|39\"|++!|R!|"
    "*|29\"|,+!|#|>|!~-+!|)|L:\"|-+!|#|+|L:\"|.+!|$|-|$;\"|/+!|$|#|#~/|#;\"|"
    "2+!|$|-|(;\"|3+!|$|#|\"~/|';\"|6+!|$|,|L:\"|7+!|#|$|#|2|M:\"|:+!|S!|*|L"
    ":\"|;+!|#|>|!~<+!|)|(<\"|<+!|%|+|(<\"|=+!|\"|-|A<\"|>+!|$|%|#~/|@<\"|A+"
    "!|$|-|E<\"|B+!|$|%|\"~/|D<\"|E+!|$|,|(<\"|F+!|%|\"|#|2|)<\"|I+!|T!|*|(<"
    "\"|J+!|#|>|!~K+!|)|E=\"|K+!|$|+|E=\"|L+!|#|-|U=\"|M+!|$|$|#~/|T=\"|P+!|"
    "$|-|Y=\"|Q+!|$|$|\"~/|X=\"|T+!|$|,|E=\"|U+!|$|#|#|2|F=\"|X+!|U!|*|E=\"|"
    "Y+!|#|>|!~Z+!|)|Y>\"|Z+!|\"|+|Y>\"|[+!|%|-|.?\"|\\+!|$|\"|#~/|-?\"|_+!|"
    "$|-|2?\"| ,!|$|\"|\"~/|1?\"|#,!|$|,|Y>\"|$,!|\"|%|#|2|Z>\"|',!|V!|*|Y>\""
    "|(,!|#|>|!~),!|)|2@\"|),!|#|+|2@\"|*,!|$|-|G@\"|+,!|$|#|#~/|F@\"|.,!|$|"
    "-|K@\"|/,!|$|#|\"~/|J@\"|2,!|$|,|2@\"|3,!|#|$|#|2|3@\"|6,!|W!|*|2@\"|7,"
    "!|#|>|!~8,!|)|KA\"|8,!|%|+|KA\"|9,!|\"|-|^A\"|:,!|$|%|#~/|]A\"|=,!|$|-|"
    "\"B\"|>,!|$|%|\"~/|!B\"|A,!|$|,|KA\"|B,!|%|\"|#|2|LA\"|E,!|X!|*|KA\"|F,"
    "!|#|>|!~G,!|)|6C\"|G,!|$|+|6C\"|H,!|#|-|MC\"|I,!|$|$|!~/|LC\"|L,!|$|,|6"
    "C\"|M,!|$|#|\"|2|7C\"|P,!|Y!|*|6C\"|Q,!|\"|>|,B\"|R,!|)|!E\"|R,!|\"|+|!"
    "E\"|S,!|%|-|1E\"|T,!|$|\"|!~/|0E\"|W,!|$|,|!E\"|X,!|\"|%|\"|2|\"E\"|[,!"
    "|\\!|*|!E\"|\\,!|\"|>|WC\"|],!|)|:G\"|],!|#|-|;G\"|^,!|%|#|\"~/|:G\"|!-"
    "!|%|*|:G\"|\"-!|#|>|*F\"|#-!|)|!J\"|#-!|$|+|!J\"|$-!|%|,|!J\"|%-!|$|%|!"
    "|2|\"J\"|(-!|^!|*|!J\"|)-!|!|>|\\H\"|*-!|)|DL\"|*-!|\"|+|DL\"|+-!|#|,|D"
    "L\"|,-!|\"|#|!|2|EL\"|/-!|[|-|QL\"|0-!|%|\"|!~5|PL\"|3-!|%|*|DL\"|4-!|!"
    "|>|DK\"|5-!|)|\"N\"|5-!|%|+|\"N\"|6-!|$|,|\"N\"|7-!|%|$|!|2|#N\"|:-!|[|"
    "-|/N\"|;-!|%|%|!~5|.N\"|>-!|%|*|\"N\"|?-!|!|>|[L\"|@-!|)|=O\"|@-!|#|+|="
    "O\"|A-!|\"|,|=O\"|B-!|#|\"|!|2|>O\"|E-!|X|-|NO\"|F-!|%|#|!~5|MO\"|I-!|%"
    "|*|=O\"|J-!|!|>|9N\"|K-!|)|YP\"|K-!|$|+|YP\"|L-!|%|,|YP\"|M-!|$|%|!|2|Z"
    "P\"|P-!|Y|-|'Q\"|Q-!|%|$|!~5|&Q\"|T-!|%|*|YP\"|U-!|!|>|XO\"|V-!|)|/R\"|"
    "V-!|\"|+|/R\"|W-!|#|,|/R\"|X-!|\"|#|!|2|0R\"|[-!|]|*|/R\"|\\-!|!|>|1Q\""
    "|]-!|)|?S\"|]-!|%|+|?S\"|^-!|$|,|?S\"|_-!|%|$|!|2|@S\"|\".!|^|*|?S\"|#."
    "!|!|>|@R\"|$.!|)|QT\"|$.!|#|+|QT\"|%.!|\"|,|QT\"|&.!|#|\"|!|2|RT\"|).!|"
    "[|-|^T\"|*.!|%|#|!~5|]T\"|-.!|%|*|QT\"|..!|!|>|QS\"|/.!|)|.V\"|/.!|$|+|"
    ".V\"|0.!|%|,|.V\"|1.!|$|%|!|2|/V\"|4.!|\\|-|AV\"|5.!|%|$|!~5|@V\"|8.!|%"
    "|*|.V\"|9.!|!|>|(U\"|:.!|)|OW\"|:.!|\"|+|OW\"|;.!|#|,|OW\"|<.!|\"|#|!|2"
    "|PW\"|?.!|X|-| X\"|@.!|%|\"|!~5|_W\"|C.!|%|*|OW\"|D.!|!|>|KV\"|E.!|}L|("
    "|}M||0|!|1||Temp$0||Temp$1||Temp$2||Temp$3||in_guard||token_count|}N|&|"
    "}O|#|}P|1|Missing closing quote on string literal|Missing closing quote"
    " on triple quoted string literal|}Q|}"
};

static const char* regex_str =
{
    "|!|}!|'$'| \"|'('|!\"|')'|\"\"|'*'|#\"|'+'|$\"|'.'|%\"|'?'|&\"|'\\\\$'|"
    "'\"|'\\\\('|(\"|'\\\\)'|)\"|'\\\\*'|*\"|'\\\\+'|+\"|'\\\\.'|,\"|'\\\\?'"
    "|-\"|'\\\\D'|.\"|'\\\\S'|/\"|'\\\\['|0\"|'\\\\\\\\'|1\"|'\\\\]'|2\"|'\\"
    "\\b'|3\"|'\\\\d'|4\"|'\\\\n'|5\"|'\\\\r'|6\"|'\\\\s'|7\"|'\\\\{'+'|8\"|"
    "'\\\\{','|9\"|'\\\\{'-'|:\"|'{','|;\"|*eof*|<\"|*epsilon*|=\"|*error*|>"
    "\"|<char>|?\"|<charset>|@\"|<macro>|A\"|<whitespace>|B\"|AstActionAdd|U"
    "!|AstActionAnd|Z!|AstActionAssign|N!|AstActionDivide|X!|AstActionDumpSt"
    "ack|]!|AstActionEqual|O!|AstActionGreaterEqual|T!|AstActionGreaterThan|"
    "S!|AstActionLessEqual|R!|AstActionLessThan|Q!|AstActionMultiply|W!|AstA"
    "ctionNot|\\!|AstActionNotEqual|P!|AstActionOr|[!|AstActionStatementList"
    "|M!|AstActionSubtract|V!|AstActionTokenCount|^!|AstActionUnaryMinus|Y!|"
    "AstAstChild|I|AstAstDot|P|AstAstFormer|G|AstAstItemList|H|AstAstKind|J|"
    "AstAstLexeme|M|AstAstLexemeString|N|AstAstLocation|K|AstAstLocationStri"
    "ng|L|AstAstLocator|O|AstAstSlice|Q|AstCaseSensitive|*|AstCharset|<!|Ast"
    "CharsetAltNewline|E!|AstCharsetCaret|H!|AstCharsetChar|?!|AstCharsetCr|"
    "G!|AstCharsetDash|I!|AstCharsetDigits|B!|AstCharsetDollar|J!|AstCharset"
    "Escape|D!|AstCharsetInvert|=!|AstCharsetLeftBracket|K!|AstCharsetNewlin"
    "e|F!|AstCharsetNotDigits|C!|AstCharsetNotWhitespace|A!|AstCharsetRange|"
    ">!|AstCharsetRightBracket|L!|AstCharsetString|V|AstCharsetWhitespace|@!"
    "|AstConflicts|(|AstEmpty|F|AstErrorRecovery|'|AstFalse|^|AstGrammar|\"|"
    "AstGroup|<|AstIdentifier|X|AstInteger|Y|AstKeepWhitespace|)|AstLookahea"
    "ds|&|AstMacroString|W|AstNegativeInteger|Z|AstNonterminalReference|E|As"
    "tNull|!|AstOneClosure|;|AstOptionList|#|AstOptional|9|AstOptions|S|AstR"
    "educeActions|T|AstRegex|_|AstRegexAltNewline|,!|AstRegexChar|%!|AstRege"
    "xCr|.!|AstRegexDigits|)!|AstRegexDollar|4!|AstRegexEscape|+!|AstRegexLe"
    "ftBrace|:!|AstRegexLeftBracket|8!|AstRegexLeftParen|6!|AstRegexList|!!|"
    "AstRegexNewline|-!|AstRegexNotDigits|*!|AstRegexNotWhitespace|(!|AstReg"
    "exOneClosure|$!|AstRegexOptional|\"!|AstRegexOr| !|AstRegexPeriod|3!|As"
    "tRegexPlus|1!|AstRegexQuestion|2!|AstRegexRightBrace|;!|AstRegexRightBr"
    "acket|9!|AstRegexRightParen|7!|AstRegexSpace|5!|AstRegexStar|0!|AstRege"
    "xString|U|AstRegexVBar|/!|AstRegexWhitespace|'!|AstRegexWildcard|&!|Ast"
    "RegexZeroClosure|#!|AstRule|6|AstRuleLeftAssoc|@|AstRuleList|%|AstRuleO"
    "peratorList|B|AstRuleOperatorSpec|C|AstRulePrecedence|=|AstRulePreceden"
    "ceList|>|AstRulePrecedenceSpec|?|AstRuleRhs|8|AstRuleRhsList|7|AstRuleR"
    "ightAssoc|A|AstString|[|AstTerminalReference|D|AstToken|R|AstTokenActio"
    "n|2|AstTokenDeclaration|+|AstTokenDescription|.|AstTokenError|5|AstToke"
    "nIgnore|4|AstTokenLexeme|3|AstTokenList|$|AstTokenOptionList|,|AstToken"
    "Precedence|1|AstTokenRegex|0|AstTokenRegexList|/|AstTokenTemplate|-|Ast"
    "TripleString|\\|AstTrue|]|AstUnknown||AstZeroClosure|:|Unknown|_!|}\"|/"
    "/{ -{ *//  Regular Expression Grammar{ -{ *//  ------------------------"
    "--{ -{ *//{ -{ *//  Grammar file for the scanner regular expressions.{ "
    "-{ *//{ -{ *{ -{ *options{ -{ *{ -{ *    lookaheads = 2{ -{ *    confli"
    "cts = 0{ -{ *    case_sensitive = true{ -{ *{ -{ *tokens{ -{ *{ -{ *   "
    " <whitespace>         : regex = ''' \\s+ '''{ -{ *                     "
    "      ignore = true{ -{ *{ -{ *    <macro>              : regex = ''' \\"
    "{'+ [^{'-]* \\{'- '''{ -{ *        { -{ *    <charset>            : reg"
    "ex = ''' \\[ ( [^\\]] {', \\\\ \\] )* \\] '''{ -{ *        { -{ *    <c"
    "har>               : regex = ''' [^?+*()\\[\\]{'+{'-{',.\\$\\s] '''{ -{"
    " *        { -{ *rules       { -{ *        { -{ *    Regex              "
    "  ::= RegexOr{ -{ *                         :   (AstRegex, $1){ -{ *{ -"
    "{ *    RegexOr              ::= RegexOrTerm ( '{',' RegexOrTerm : $2 )+"
    "{ -{ *                         :   (AstRegexOr, $1, $2._){ -{ *        "
    "{ -{ *    RegexOr              ::= RegexOrTerm{ -{ *        { -{ *    R"
    "egexOrTerm          ::= RegexUnopTerm+{ -{ *                         : "
    "  (AstRegexList, $1._){ -{ *        { -{ *    RegexUnopTerm        ::= "
    "RegexUnopTerm '*'{ -{ *                         :   (AstRegexZeroClosur"
    "e, $1){ -{ *        { -{ *    RegexUnopTerm        ::= RegexUnopTerm '+"
    "'{ -{ *                         :   (AstRegexOneClosure, $1){ -{ *     "
    "   { -{ *    RegexUnopTerm        ::= RegexUnopTerm '?'{ -{ *          "
    "               :   (AstRegexOptional, $1){ -{ *        { -{ *    RegexU"
    "nopTerm        ::= '(' RegexOr ')'{ -{ *                         :   $2"
    "{ -{ *        { -{ *    RegexUnopTerm        ::= <macro>{ -{ *         "
    "                :   (AstMacroString, &1){ -{ *        { -{ *    RegexUn"
    "opTerm        ::= Charset{ -{ *        { -{ *    RegexUnopTerm        :"
    ":= Char{ -{ *        { -{ *    Charset              ::= <charset>{ -{ *"
    "                         :   (AstCharsetString, &1) { -{ *{ -{ *    Cha"
    "rset              ::= '.' { -{ *                         :   (AstRegexW"
    "ildcard){ -{ *{ -{ *    Charset              ::= '\\\\s' { -{ *        "
    "                 :   (AstRegexWhitespace){ -{ *{ -{ *    Charset       "
    "       ::= '\\\\S' { -{ *                         :   (AstRegexNotWhite"
    "space){ -{ *{ -{ *    Charset              ::= '\\\\d' { -{ *          "
    "               :   (AstRegexDigits){ -{ *{ -{ *    Charset             "
    " ::= '\\\\D' { -{ *                         :   (AstRegexNotDigits){ -{"
    " *{ -{ *    Char                 ::= <char>{ -{ *                      "
    "   :   (AstRegexChar, &1){ -{ *{ -{ *    Char                 ::= '\\\\"
    "\\\\' { -{ *                         :   (AstRegexEscape){ -{ *{ -{ *  "
    "  Char                 ::= '$' { -{ *                         :   (AstR"
    "egexAltNewline){ -{ *{ -{ *    Char                 ::= '\\\\n' { -{ * "
    "                        :   (AstRegexNewline){ -{ *{ -{ *    Char      "
    "           ::= '\\\\r' { -{ *                         :   (AstRegexCr){"
    " -{ *{ -{ *    Char                 ::= '\\\\{',' { -{ *               "
    "          :   (AstRegexVBar){ -{ *{ -{ *    Char                 ::= '\\"
    "\\*' { -{ *                         :   (AstRegexStar){ -{ *{ -{ *    C"
    "har                 ::= '\\\\+' { -{ *                         :   (Ast"
    "RegexPlus){ -{ *{ -{ *    Char                 ::= '\\\\?' { -{ *      "
    "                   :   (AstRegexQuestion){ -{ *{ -{ *    Char          "
    "       ::= '\\\\.' { -{ *                         :   (AstRegexPeriod){"
    " -{ *{ -{ *    Char                 ::= '\\\\$' { -{ *                 "
    "        :   (AstRegexDollar){ -{ *{ -{ *    Char                 ::= '\\"
    "\\b' { -{ *                         :   (AstRegexSpace){ -{ *{ -{ *    "
    "Char                 ::= '\\\\(' { -{ *                         :   (As"
    "tRegexLeftParen){ -{ *{ -{ *    Char                 ::= '\\\\)' { -{ *"
    "                         :   (AstRegexRightParen){ -{ *{ -{ *    Char  "
    "               ::= '\\\\[' { -{ *                         :   (AstRegex"
    "LeftBracket){ -{ *{ -{ *    Char                 ::= '\\\\]' { -{ *    "
    "                     :   (AstRegexRightBracket){ -{ *{ -{ *    Char    "
    "             ::= '\\\\{'+' { -{ *                         :   (AstRegex"
    "LeftBrace){ -{ *{ -{ *    Char                 ::= '\\\\{'-' { -{ *    "
    "                     :   (AstRegexRightBrace){ -{ *{ -{ *|}#|\"|}$|!|}%"
    "|I|}&||}'|M|}(|*eof*|')'|'{','|'$'|'('|<charset>|<macro>|'.'|'\\\\$'|'\\"
    "\\('|'\\\\)'|'\\\\*'|'\\\\+'|'\\\\.'|'\\\\?'|'\\\\D'|'\\\\S'|'\\\\['|'\\"
    "\\]'|'\\\\b'|'\\\\d'|'\\\\n'|'\\\\\\\\'|'\\\\s'|'\\\\{'+'|'\\\\{','|'\\"
    "\\{'-'|'\\\\r'|<char>|'*'|'+'|'?'||||||||||*error*||*epsilon*|<whitespa"
    "ce>|})|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|"
    "@|X!|'!|||||| !|!|(!|!||}*|<\"|\"\"|;\"| \"|!\"|@\"|A\"|%\"|'\"|(\"|)\""
    "|*\"|+\"|,\"|-\"|.\"|/\"|0\"|2\"|3\"|4\"|5\"|1\"|7\"|8\"|9\"|:\"|6\"|?\""
    "|#\"|$\"|&\"|P$EA||||||P#EA|||>\"|08GB|=\"|B\"|}+||||||!|!|||||||||||||"
    "|||||||||!|||| \"|)!|]||||||!||||!|},|I|}-|!|!|\"|\"|\"|!|!|!|\"|!|\"|\""
    "|\"|#|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|}.|J|H|G|F|"
    "C|F|G|D|E|E|B|B|B|B|B|B|B|A|A|A|A|A|A|@|@|@|@|@|@|@|@|@|@|@|@|@|@|@|@|@"
    "|@|}/|*accept* ::= Regex|Regex ::= RegexOr|RegexOr ::= RegexOrTerm Rege"
    "xOr:1|RegexOr:1 ::= RegexOr:1 RegexOr:2|RegexOr:2 ::= '{',' RegexOrTerm"
    "|RegexOr:1 ::= RegexOr:2|RegexOr ::= RegexOrTerm|RegexOrTerm ::= RegexO"
    "rTerm:1|RegexOrTerm:1 ::= RegexOrTerm:1 RegexUnopTerm|RegexOrTerm:1 ::="
    " RegexUnopTerm|RegexUnopTerm ::= RegexUnopTerm '*'|RegexUnopTerm ::= Re"
    "gexUnopTerm '+'|RegexUnopTerm ::= RegexUnopTerm '?'|RegexUnopTerm ::= '"
    "(' RegexOr ')'|RegexUnopTerm ::= <macro>|RegexUnopTerm ::= Charset|Rege"
    "xUnopTerm ::= Char|Charset ::= <charset>|Charset ::= '.'|Charset ::= '\\"
    "\\s'|Charset ::= '\\\\S'|Charset ::= '\\\\d'|Charset ::= '\\\\D'|Char :"
    ":= <char>|Char ::= '\\\\\\\\'|Char ::= '$'|Char ::= '\\\\n'|Char ::= '\\"
    "\\r'|Char ::= '\\\\{','|Char ::= '\\\\*'|Char ::= '\\\\+'|Char ::= '\\\\"
    "?'|Char ::= '\\\\.'|Char ::= '\\\\$'|Char ::= '\\\\b'|Char ::= '\\\\('|"
    "Char ::= '\\\\)'|Char ::= '\\\\['|Char ::= '\\\\]'|Char ::= '\\\\{'+'|C"
    "har ::= '\\\\{'-'|}0|!~.!|6!|@!|J!|O!|!~W!|_!|)\"|1\"|9\"|A\"|I\"|N\"|!"
    "~!~V\"|^\"|$#|*#|0#|6#|<#|D#|J#|P#|V#|\\#|\"$|($|.$|4$|:$|@$|F$|L$|R$|X"
    "$|^$|$%|}1|\"|}2||}3|M|}4|R|}5|F|/!|O!|/\"|)!|,!|Q3|/$|R$|/%|W%|7&|O#|W"
    "&|7'|W'|7(|W(|7)|W)|7*|W*|7+|W+|7,|W,|7-|W-|7.|W.|7/|W/|70|W0|^3|L$|43|"
    "71|W1|72|W2|Q%|#!|!4|/#||$4|L%|Z3|O\"|}6|R4|}7| %|!!@%|\"!@$|#!0\"|$!(!"
    "|%!0|&!(|'!@!|(!0#|)!@#|*!H#|+!P\"|,!X\"|-!(#|.! #|/! \"|0!P!|1!P#|2!X#"
    "|3!8#|4!X!|5!8\"|6!(\"|7!H!|8! $|9!H\"|:!($|;!@\"|<!8|=!P$|>!X$|?! %|@#"
    "8!|A#0!|B#(&|C#X%|D# &|E#X|F#0$|G#P%|H#@|#!0\"|$!(!|%!0|&!(|'!@!|(!0#|)"
    "!@#|*!H#|+!P\"|,!X\"|-!(#|.! #|/! \"|0!P!|1!P#|2!X#|3!8#|4!X!|5!8\"|6!("
    "\"|7!H!|8! $|9!H\"|:!($|;!@\"|<!8| : H%|!: @%|\": @$|@#8!|A#0!|B# !| %|"
    "D#P|E#X| * H%|G#H|H#@| R!H%|!R!@%|\"R!@$|#R!0\"|$R!(!|%R!0|&R!(|'R!@!|("
    "R!0#|)R!@#|*R!H#|+R!P\"|,R!X\"|-R!(#|.R! #|/R! \"|0R!P!|1R!P#|2R!X#|3R!"
    "8#|4R!X!|5R!8\"|6R!(\"|7R!H!|8R! $|9R!H\"|:R!($|;R!@\"|<R!8|=R!P$|>R!X$"
    "|?R! %| *\"H%|!*\"@%|\"*\"@$|#*\"0\"|$*\"(!|%*\"0|&*\"(|'*\"@!|(*\"0#|)"
    "*\"@#|**\"H#|+*\"P\"|,*\"X\"|-*\"(#|.*\" #|/*\" \"|0*\"P!|1*\"P#|2*\"X#"
    "|3*\"8#|4*\"X!|5*\"8\"|6*\"(\"|7*\"H!|8*\" $|9*\"H\"|:*\"($|;*\"@\"|<*\""
    "8|=*\"P$|>*\"X$|?*\" %| Z\"H%|!Z\"@%|\"Z\"@$|#Z\"0\"|$Z\"(!|%Z\"0|&Z\"("
    "|'Z\"@!|(Z\"0#|)Z\"@#|*Z\"H#|+Z\"P\"|,Z\"X\"|-Z\"(#|.Z\" #|/Z\" \"|0Z\""
    "P!|1Z\"P#|2Z\"X#|3Z\"8#|4Z\"X!|5Z\"8\"|6Z\"(\"|7Z\"H!|8Z\" $|9Z\"H\"|:Z"
    "\"($|;Z\"@\"|<Z\"8|=Z\"P$|>Z\"X$|?Z\" %| $H%|!$@%|\"$@$|#$0\"|$$(!|%$0|"
    "&$(|'$@!|($0#|)$@#|*$H#|+$P\"|,$X\"|-$(#|.$ #|/$ \"|0$P!|1$P#|2$X#|3$8#"
    "|4$X!|5$8\"|6$(\"|7$H!|8$ $|9$H\"|:$($|;$@\"|<$8|=!P$|>!X$|?! %| J!H%|!"
    "J!@%|\"J!@$|#J!0\"|$J!(!|%J!0|&J!(|'J!@!|(J!0#|)J!@#|*J!H#|+J!P\"|,J!X\""
    "|-J!(#|.J! #|/J! \"|0J!P!|1J!P#|2J!X#|3J!8#|4J!X!|5J!8\"|6J!(\"|7J!H!|8"
    "J! $|9J!H\"|:J!($|;J!@\"|<J!8|=J!P$|>J!X$|?J! %| 2\"H%|!2\"@%|\"2\"@$|#"
    "2\"0\"|$2\"(!|%2\"0|&2\"(|'2\"@!|(2\"0#|)2\"@#|*2\"H#|+2\"P\"|,2\"X\"|-"
    "2\"(#|.2\" #|/2\" \"|02\"P!|12\"P#|22\"X#|32\"8#|42\"X!|52\"8\"|62\"(\""
    "|72\"H!|82\" $|92\"H\"|:2\"($|;2\"@\"|<2\"8|=2\"P$|>2\"X$|?2\" %| Z H%|"
    "!Z @%|\"Z @$|#!0\"|$!(!|%!0|&!(|'!@!|(!0#|)!@#|*!H#|+!P\"|,!X\"|-!(#|.!"
    " #|/! \"|0!P!|1!P#|2!X#|3!8#|4!X!|5!8\"|6!(\"|7!H!|8! $|9!H\"|:!($|;!@\""
    "|<!8| J H%|!J @%|\"J @$|@#8!|A#0!|B#H$| *!H%|!*!@%|\"*!@$|#*!0\"|$*!(!|"
    "%*!0|&*!(|'*!@!|(*!0#|)*!@#|**!H#|+*!P\"|,*!X\"|-*!(#|.*! #|/*! \"|0*!P"
    "!|1*!P#|2*!X#|3*!8#|4*!X!|5*!8\"|6*!(\"|7*!H!|8*! $|9*!H\"|:*!($|;*!@\""
    "|<*!8|=!P$|>!X$|?! %|#!0\"|$!(!|%!0|&!(|'!@!|(!0#|)!@#|*!H#|+!P\"|,!X\""
    "|-!(#|.! #|/! \"|0!P!|1!P#|2!X#|3!8#|4!X!|5!8\"|6!(\"|7!H!|8! $|9!H\"|:"
    "!($|;!@\"|<!8| $H%|!$@%|\"$@$|@#8!|A#0!|B# !|!!@%|D#P|E#X|!~G#(%| Z!H%|"
    "!Z!@%|\"Z!@$|#Z!0\"|$Z!(!|%Z!0|&Z!(|'Z!@!|(Z!0#|)Z!@#|*Z!H#|+Z!P\"|,Z!X"
    "\"|-Z!(#|.Z! #|/Z! \"|0Z!P!|1Z!P#|2Z!X#|3Z!8#|4Z!X!|5Z!8\"|6Z!(\"|7Z!H!"
    "|8Z! $|9Z!H\"|:Z!($|;Z!@\"|<Z!8|=Z!P$|>Z!X$|?Z! %| \"\"H%|!\"\"@%|\"\"\""
    "@$|#\"\"0\"|$\"\"(!|%\"\"0|&\"\"(|'\"\"@!|(\"\"0#|)\"\"@#|*\"\"H#|+\"\""
    "P\"|,\"\"X\"|-\"\"(#|.\"\" #|/\"\" \"|0\"\"P!|1\"\"P#|2\"\"X#|3\"\"8#|4"
    "\"\"X!|5\"\"8\"|6\"\"(\"|7\"\"H!|8\"\" $|9\"\"H\"|:\"\"($|;\"\"@\"|<\"\""
    "8|=\"\"P$|>\"\"X$|?\"\" %| :\"H%|!:\"@%|\":\"@$|#:\"0\"|$:\"(!|%:\"0|&:"
    "\"(|':\"@!|(:\"0#|):\"@#|*:\"H#|+:\"P\"|,:\"X\"|-:\"(#|.:\" #|/:\" \"|0"
    ":\"P!|1:\"P#|2:\"X#|3:\"8#|4:\"X!|5:\"8\"|6:\"(\"|7:\"H!|8:\" $|9:\"H\""
    "|::\"($|;:\"@\"|<:\"8|=:\"P$|>:\"X$|?:\" %| B\"H%|!B\"@%|\"B\"@$|#B\"0\""
    "|$B\"(!|%B\"0|&B\"(|'B\"@!|(B\"0#|)B\"@#|*B\"H#|+B\"P\"|,B\"X\"|-B\"(#|"
    ".B\" #|/B\" \"|0B\"P!|1B\"P#|2B\"X#|3B\"8#|4B\"X!|5B\"8\"|6B\"(\"|7B\"H"
    "!|8B\" $|9B\"H\"|:B\"($|;B\"@\"|<B\"8|=B\"P$|>B\"X$|?B\" %| J\"H%|!J\"@"
    "%|\"J\"@$|#J\"0\"|$J\"(!|%J\"0|&J\"(|'J\"@!|(J\"0#|)J\"@#|*J\"H#|+J\"P\""
    "|,J\"X\"|-J\"(#|.J\" #|/J\" \"|0J\"P!|1J\"P#|2J\"X#|3J\"8#|4J\"X!|5J\"8"
    "\"|6J\"(\"|7J\"H!|8J\" $|9J\"H\"|:J\"($|;J\"@\"|<J\"8|=J\"P$|>J\"X$|?J\""
    " %| R\"H%|!R\"@%|\"R\"@$|#R\"0\"|$R\"(!|%R\"0|&R\"(|'R\"@!|(R\"0#|)R\"@"
    "#|*R\"H#|+R\"P\"|,R\"X\"|-R\"(#|.R\" #|/R\" \"|0R\"P!|1R\"P#|2R\"X#|3R\""
    "8#|4R\"X!|5R\"8\"|6R\"(\"|7R\"H!|8R\" $|9R\"H\"|:R\"($|;R\"@\"|<R\"8|=R"
    "\"P$|>R\"X$|?R\" %| \"#H%|!\"#@%|\"\"#@$|#\"#0\"|$\"#(!|%\"#0|&\"#(|'\""
    "#@!|(\"#0#|)\"#@#|*\"#H#|+\"#P\"|,\"#X\"|-\"#(#|.\"# #|/\"# \"|0\"#P!|1"
    "\"#P#|2\"#X#|3\"#8#|4\"#X!|5\"#8\"|6\"#(\"|7\"#H!|8\"# $|9\"#H\"|:\"#($"
    "|;\"#@\"|<\"#8|=\"#P$|>\"#X$|?\"# %| *#H%|!*#@%|\"*#@$|#*#0\"|$*#(!|%*#"
    "0|&*#(|'*#@!|(*#0#|)*#@#|**#H#|+*#P\"|,*#X\"|-*#(#|.*# #|/*# \"|0*#P!|1"
    "*#P#|2*#X#|3*#8#|4*#X!|5*#8\"|6*#(\"|7*#H!|8*# $|9*#H\"|:*#($|;*#@\"|<*"
    "#8|=*#P$|>*#X$|?*# %| 2#H%|!2#@%|\"2#@$|#2#0\"|$2#(!|%2#0|&2#(|'2#@!|(2"
    "#0#|)2#@#|*2#H#|+2#P\"|,2#X\"|-2#(#|.2# #|/2# \"|02#P!|12#P#|22#X#|32#8"
    "#|42#X!|52#8\"|62#(\"|72#H!|82# $|92#H\"|:2#($|;2#@\"|<2#8|=2#P$|>2#X$|"
    "?2# %| :#H%|!:#@%|\":#@$|#:#0\"|$:#(!|%:#0|&:#(|':#@!|(:#0#|):#@#|*:#H#"
    "|+:#P\"|,:#X\"|-:#(#|.:# #|/:# \"|0:#P!|1:#P#|2:#X#|3:#8#|4:#X!|5:#8\"|"
    "6:#(\"|7:#H!|8:# $|9:#H\"|::#($|;:#@\"|<:#8|=:#P$|>:#X$|?:# %| B#H%|!B#"
    "@%|\"B#@$|#B#0\"|$B#(!|%B#0|&B#(|'B#@!|(B#0#|)B#@#|*B#H#|+B#P\"|,B#X\"|"
    "-B#(#|.B# #|/B# \"|0B#P!|1B#P#|2B#X#|3B#8#|4B#X!|5B#8\"|6B#(\"|7B#H!|8B"
    "# $|9B#H\"|:B#($|;B#@\"|<B#8|=B#P$|>B#X$|?B# %| J#H%|!J#@%|\"J#@$|#J#0\""
    "|$J#(!|%J#0|&J#(|'J#@!|(J#0#|)J#@#|*J#H#|+J#P\"|,J#X\"|-J#(#|.J# #|/J# "
    "\"|0J#P!|1J#P#|2J#X#|3J#8#|4J#X!|5J#8\"|6J#(\"|7J#H!|8J# $|9J#H\"|:J#($"
    "|;J#@\"|<J#8|=J#P$|>J#X$|?J# %| R#H%|!R#@%|\"R#@$|#R#0\"|$R#(!|%R#0|&R#"
    "(|'R#@!|(R#0#|)R#@#|*R#H#|+R#P\"|,R#X\"|-R#(#|.R# #|/R# \"|0R#P!|1R#P#|"
    "2R#X#|3R#8#|4R#X!|5R#8\"|6R#(\"|7R#H!|8R# $|9R#H\"|:R#($|;R#@\"|<R#8|=R"
    "#P$|>R#X$|?R# %| Z#H%|!Z#@%|\"Z#@$|#Z#0\"|$Z#(!|%Z#0|&Z#(|'Z#@!|(Z#0#|)"
    "Z#@#|*Z#H#|+Z#P\"|,Z#X\"|-Z#(#|.Z# #|/Z# \"|0Z#P!|1Z#P#|2Z#X#|3Z#8#|4Z#"
    "X!|5Z#8\"|6Z#(\"|7Z#H!|8Z# $|9Z#H\"|:Z#($|;Z#@\"|<Z#8|=Z#P$|>Z#X$|?Z# %"
    "| \"$H%|!\"$@%|\"\"$@$|#\"$0\"|$\"$(!|%\"$0|&\"$(|'\"$@!|(\"$0#|)\"$@#|"
    "*\"$H#|+\"$P\"|,\"$X\"|-\"$(#|.\"$ #|/\"$ \"|0\"$P!|1\"$P#|2\"$X#|3\"$8"
    "#|4\"$X!|5\"$8\"|6\"$(\"|7\"$H!|8\"$ $|9\"$H\"|:\"$($|;\"$@\"|<\"$8|=\""
    "$P$|>\"$X$|?\"$ %| *$H%|!*$@%|\"*$@$|#*$0\"|$*$(!|%*$0|&*$(|'*$@!|(*$0#"
    "|)*$@#|**$H#|+*$P\"|,*$X\"|-*$(#|.*$ #|/*$ \"|0*$P!|1*$P#|2*$X#|3*$8#|4"
    "*$X!|5*$8\"|6*$(\"|7*$H!|8*$ $|9*$H\"|:*$($|;*$@\"|<*$8|=*$P$|>*$X$|?*$"
    " %| 2$H%|!2$@%|\"2$@$|#2$0\"|$2$(!|%2$0|&2$(|'2$@!|(2$0#|)2$@#|*2$H#|+2"
    "$P\"|,2$X\"|-2$(#|.2$ #|/2$ \"|02$P!|12$P#|22$X#|32$8#|42$X!|52$8\"|62$"
    "(\"|72$H!|82$ $|92$H\"|:2$($|;2$@\"|<2$8|=2$P$|>2$X$|?2$ %| :$H%|!:$@%|"
    "\":$@$|#:$0\"|$:$(!|%:$0|&:$(|':$@!|(:$0#|):$@#|*:$H#|+:$P\"|,:$X\"|-:$"
    "(#|.:$ #|/:$ \"|0:$P!|1:$P#|2:$X#|3:$8#|4:$X!|5:$8\"|6:$(\"|7:$H!|8:$ $"
    "|9:$H\"|::$($|;:$@\"|<:$8|=:$P$|>:$X$|?:$ %| B$H%|!B$@%|\"B$@$|#B$0\"|$"
    "B$(!|%B$0|&B$(|'B$@!|(B$0#|)B$@#|*B$H#|+B$P\"|,B$X\"|-B$(#|.B$ #|/B$ \""
    "|0B$P!|1B$P#|2B$X#|3B$8#|4B$X!|5B$8\"|6B$(\"|7B$H!|8B$ $|9B$H\"|:B$($|;"
    "B$@\"|<B$8|=B$P$|>B$X$|?B$ %| J$H%|!J$@%|\"J$@$|#J$0\"|$J$(!|%J$0|&J$(|"
    "'J$@!|(J$0#|)J$@#|*J$H#|+J$P\"|,J$X\"|-J$(#|.J$ #|/J$ \"|0J$P!|1J$P#|2J"
    "$X#|3J$8#|4J$X!|5J$8\"|6J$(\"|7J$H!|8J$ $|9J$H\"|:J$($|;J$@\"|<J$8|=J$P"
    "$|>J$X$|?J$ %| R$H%|!R$@%|\"R$@$|#R$0\"|$R$(!|%R$0|&R$(|'R$@!|(R$0#|)R$"
    "@#|*R$H#|+R$P\"|,R$X\"|-R$(#|.R$ #|/R$ \"|0R$P!|1R$P#|2R$X#|3R$8#|4R$X!"
    "|5R$8\"|6R$(\"|7R$H!|8R$ $|9R$H\"|:R$($|;R$@\"|<R$8|=R$P$|>R$X$|?R$ %| "
    "Z$H%|!Z$@%|\"Z$@$|#Z$0\"|$Z$(!|%Z$0|&Z$(|'Z$@!|(Z$0#|)Z$@#|*Z$H#|+Z$P\""
    "|,Z$X\"|-Z$(#|.Z$ #|/Z$ \"|0Z$P!|1Z$P#|2Z$X#|3Z$8#|4Z$X!|5Z$8\"|6Z$(\"|"
    "7Z$H!|8Z$ $|9Z$H\"|:Z$($|;Z$@\"|<Z$8|=Z$P$|>Z$X$|?Z$ %| \"%H%|!\"%@%|\""
    "\"%@$|#\"%0\"|$\"%(!|%\"%0|&\"%(|'\"%@!|(\"%0#|)\"%@#|*\"%H#|+\"%P\"|,\""
    "%X\"|-\"%(#|.\"% #|/\"% \"|0\"%P!|1\"%P#|2\"%X#|3\"%8#|4\"%X!|5\"%8\"|6"
    "\"%(\"|7\"%H!|8\"% $|9\"%H\"|:\"%($|;\"%@\"|<\"%8|=\"%P$|>\"%X$|?\"% %|"
    " \"!H%|!\"!@%|\"\"!@$|#\"!0\"|$\"!(!|%\"!0|&\"!(|'\"!@!|(\"!0#|)\"!@#|*"
    "\"!H#|+\"!P\"|,\"!X\"|-\"!(#|.\"! #|/\"! \"|0\"!P!|1\"!P#|2\"!X#|3\"!8#"
    "|4\"!X!|5\"!8\"|6\"!(\"|7\"!H!|8\"! $|9\"!H\"|:\"!($|;\"!@\"|<\"!8|=!P$"
    "|>!X$|?! %| 2!H%|!2!@%|\"2!@$|#2!0\"|$2!(!|%2!0|&2!(|'2!@!|(2!0#|)2!@#|"
    "*2!H#|+2!P\"|,2!X\"|-2!(#|.2! #|/2! \"|02!P!|12!P#|22!X#|32!8#|42!X!|52"
    "!8\"|62!(\"|72!H!|82! $|92!H\"|:2!($|;2!@\"|<2!8|=2!P$|>2!X$|?2! %| :!H"
    "%|!:!@%|\":!@$|#:!0\"|$:!(!|%:!0|&:!(|':!@!|(:!0#|):!@#|*:!H#|+:!P\"|,:"
    "!X\"|-:!(#|.:! #|/:! \"|0:!P!|1:!P#|2:!X#|3:!8#|4:!X!|5:!8\"|6:!(\"|7:!"
    "H!|8:! $|9:!H\"|::!($|;:!@\"|<:!8|=:!P$|>:!X$|?:! %| B!H%|!B!@%|\"B!@$|"
    "#B!0\"|$B!(!|%B!0|&B!(|'B!@!|(B!0#|)B!@#|*B!H#|+B!P\"|,B!X\"|-B!(#|.B! "
    "#|/B! \"|0B!P!|1B!P#|2B!X#|3B!8#|4B!X!|5B!8\"|6B!(\"|7B!H!|8B! $|9B!H\""
    "|:B!($|;B!@\"|<B!8|=B!P$|>B!X$|?B! %|#!0\"|$!(!|%!0|&!(|'!@!|(!0#|)!@#|"
    "*!H#|+!P\"|,!X\"|-!(#|.! #|/! \"|0!P!|1!P#|2!X#|3!8#|4!X!|5!8\"|6!(\"|7"
    "!H!|8! $|9!H\"|:!($|;!@\"|<!8| R H%|!R @%|\"!@$|@#8!|A#0!|B# !|!~D#8%|E"
    "#X| $H%|!$@%|\"$@$|!~ 2 H%|!2 @%|\"!@$| B H%|!B @%|\"B @$| * H%|!!@%|!~"
    "!~!~!~!~!~!~!~!~!~!~!~!~!~C#8$|!~!~F#0$|!~!~!~!~!~C#8$|!~!~F#0$|C#0%|!~"
    "!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~}8|!|}9||}:||};|_|}<||}=|&|}>|'|}?||}@|)|"
    "}A|_|}B||}C|/|}D|_|}E||}F|5|}G|_|}H|Null||Halt|!|Label|\"|Call|#|ScanSt"
    "art|$|ScanChar|%|ScanAccept|&|ScanToken|'|ScanError|(|AstStart|)|AstFin"
    "ish|*|AstNew|+|AstForm|,|AstLoad|-|AstIndex|.|AstChild|/|AstChildSlice|"
    "0|AstKind|1|AstKindNum|2|AstLocation|3|AstLocationNum|4|AstLexeme|5|Ast"
    "LexemeString|6|Assign|7|DumpStack|8|Add|9|Subtract|:|Multiply|;|Divide|"
    "<|UnaryMinus|=|Return|>|Branch|?|BranchEqual|@|BranchNotEqual|A|BranchL"
    "essThan|B|BranchLessEqual|C|BranchGreaterThan|D|BranchGreaterEqual|E|}I"
    "|*%|}J|I)|}K|7|!~|$||>|!~\"|$|!~\"|%|!~\"|6||(|$|)|-|&|.|?|$|@|@|&|A|C|"
    "$|D|D|(|E|G|$|H|H|*|I|I|,|J|J|.|K|K|0|L|M|$|N|N|2|O|^|$|_|_|4| !|:!|$|;"
    "!|;!|6|<!|<!|<|>!|Z!|$|[!|[!|&!|\\!|\\!|)!|^!|_____#|$|&|/'|%!|<|+!|%|!"
    "~'!||&|B#|(!|L|\"|%|!~*!|\"|)|-|&|@|@|&|&|WC|1!|#|+!|%|!~3!||&|34|4!|$|"
    "+!|%|!~6!||&|?4|7!|!|+!|%|!~9!||&|//|:!|=|+!|%|!~<!||&|!1|=!|>|+!|%|!~?"
    "!||&|,:|@!|'|+!|%|!~B!||&|R2|C!|?|+!|%|!~E!||%|!~F!|$||;!|6|<!|<!|7|=!|"
    "=!|:|>!|_____#|6|%|!~S!|$||;!|6|<!|<!|7|=!|=!|8|>!|_____#|6|&| &| \"|%|"
    "+!|%|!~\"\"|$||;!|6|<!|<!|7|=!|=!|:|>!|_____#|6|&| &|/\"|%|+!|%|!~1\"||"
    "&|/'|2\"|<|+!|%|!~4\"|4|D|D|>|H|H|@|I|I|B|J|J|D|K|K|F|N|N|H|_|_|J|$!|$!"
    "|L|3!|3!|N|;!|;!|P|<!|<!|R|=!|=!|T|B!|B!|V|D!|D!|X|N!|N!|Z|R!|R!|\\|S!|"
    "S!|^|[!|[!| !|\\!|\\!|\"!|]!|]!|$!|&|BN|1#|(|+!|%|!~3#||&|-Q|4#|)|+!|%|"
    "!~6#||&|FR|7#|*|+!|%|!~9#||&|,I|:#|+|+!|%|!~<#||&|@J|=#|,|+!|%|!~?#||&|"
    ",M|@#|-|+!|%|!~B#||&|TK|C#|.|+!|%|!~E#||&|O?|F#|/|+!|%|!~H#||&|\\<|I#|0"
    "|+!|%|!~K#||&| T|L#|1|+!|%|!~N#||&|@B|O#|6|+!|%|!~Q#||&|;U|R#|2|+!|%|!~"
    "T#||&|XO|U#|3|+!|%|!~W#||&|9>|X#|4|+!|%|!~Z#||&|/E|[#|5|+!|%|!~]#||&|FF"
    "|^#|;|+!|%|!~ $||&|B;|!$|7|+!|%|!~#$||&|WV|$$|8|+!|%|!~&$||&|XG|'$|9|+!"
    "|%|!~)$||&|0X|*$|:|+!|%|!~,$||%|!~-$|#||\\!|&!|]!|]!|'!|^!|_____#|&!|&|"
    "^$|7$|&|+!|%|!~9$||&|N*|:$|\"|+!|%|!~<$||'|!~=$|9|!~=$|$|$|!|>|!~@$|)|Q"
    ")|@$|\"|+|Q)|A$|#|-|])|B$||\"|!~/|\\)|E$||,|Q)|F$|\"|#|!|2|R)|I$|_|*|Q)"
    "|J$|!|>|R(|K$|)|D+|K$|#|+|D+|L$|\"|-|R+|M$||#|\"~/|Q+|P$||-|V+|Q$||#|!~"
    "0|X+|T$|||!~,|D+|W$|#|\"|\"|2|E+|Z$| !|*|D+|[$|\"|>|'*|\\$|)|@*|\\$|\"|"
    "+|@*|]$|#|-|@*|^$||\"|\"~0|@*|!%|||!~-|@*|$%||\"|!~/|@*|'%||,|@*|(%|\"|"
    "#|\"|2|@*|+%|_!|*|@*|,%|\"|>|@*|-%|)| +|-%|#|-|!+|.%|!|#|!~/| +|1%|!|*|"
    " +|2%|\"|>|!~3%|)|@*|3%|\"|+|@*|4%|#|-|@*|5%|!|\"|!~/|@*|8%|!|,|@*|9%|\""
    "|#|!|2|@*|<%|_!|*|@*|=%|!|>|@*|>%|)|$.|>%|#|+|$.|?%|\"|-|4.|@%|!|#|!~0|"
    "6.|C%|!||!~,|$.|F%|#|\"|!|2|%.|I%|!!|*|$.|J%|!|>|^,|K%|)|^,|K%|\"|+|^,|"
    "L%|#|-|^,|M%|!|\"|\"~0|^,|P%|!||!~-|^,|S%|!|\"|!~/|^,|V%|!|,|^,|W%|\"|#"
    "|\"|2|^,|Z%|_!|*|^,|[%|\"|>|^,|\\%|)|^,|\\%|#|+|^,|]%|\"|-|^,|^%|!|#|!~"
    "/|^,|!&|!|,|^,|\"&|#|\"|!|2|^,|%&|_!|*|^,|&&|!|>|^,|'&|)|Q/|'&|\"|+|Q/|"
    "(&|#|-|(0|)&|!|\"|\"~/|'0|,&|!|,|Q/|-&|\"|#|\"|2|R/|0&|#!|*|Q/|1&|\"|>|"
    "H.|2&|)|C1|2&|#|+|C1|3&|\"|-|Y1|4&|!|#|\"~/|X1|7&|!|,|C1|8&|#|\"|\"|2|D"
    "1|;&|$!|*|C1|<&|\"|>|:0|=&|)|43|=&|\"|+|43|>&|#|-|H3|?&|!|\"|\"~/|G3|B&"
    "|!|,|43|C&|\"|#|\"|2|53|F&|\"!|*|43|G&|\"|>|+2|H&|)|!5|H&|#|-|\"5|I&|\""
    "|#|\"~/|!5|L&|\"|*|!5|M&|#|>|Z3|N&|)|26|N&|\"|+|26|O&|#|,|26|P&|\"|#|!|"
    "2|36|S&|W|-|D6|T&|\"|\"|!~5|C6|W&|\"|*|26|X&|!|>|35|Y&|)|49|Y&|#|+|49|Z"
    "&|\"|,|49|[&|#|\"|!|2|59|^&|V|-|H9|_&|\"|#|!~5|G9|\"'|\"|*|49|#'|!|>|38"
    "|$'|)|O:|$'|\"|+|O:|%'|#|,|O:|&'|\"|#|!|2|P:|)'|&!|*|O:|*'|!|>|S9|+'|)|"
    "'<|+'|#|+|'<|,'|\"|,|'<|-'|#|\"|!|2|(<|0'|'!|*|'<|1'|!|>|);|2'|)|A=|2'|"
    "\"|+|A=|3'|#|,|A=|4'|\"|#|!|2|B=|7'|(!|*|A=|8'|!|>|C<|9'|)|^>|9'|#|+|^>"
    "|:'|\"|,|^>|;'|#|\"|!|2|_>|>'|)!|*|^>|?'|!|>| >|@'|)|4@|@'|\"|+|4@|A'|#"
    "|,|4@|B'|\"|#|!|2|5@|E'|*!|*|4@|F'|!|>|6?|G'|)|MA|G'|#|+|MA|H'|\"|,|MA|"
    "I'|#|\"|!|2|NA|L'|%!|-|]A|M'|\"|#|!~5|\\A|P'|\"|*|MA|Q'|!|>|O@|R'|)|&C|"
    "R'|\"|+|&C|S'|#|,|&C|T'|\"|#|!|2|'C|W'|+!|*|&C|X'|!|>|'B|Y'|)|:D|Y'|#|+"
    "|:D|Z'|\"|,|:D|['|#|\"|!|2|;D|^'|,!|*|:D|_'|!|>|>C| (|)|TE| (|\"|+|TE|!"
    "(|#|,|TE|\"(|\"|#|!|2|UE|%(|-!|*|TE|&(|!|>|VD|'(|)|+G|'(|#|+|+G|((|\"|,"
    "|+G|)(|#|\"|!|2|,G|,(|.!|*|+G|-(|!|>|-F|.(|)|=H|.(|\"|+|=H|/(|#|,|=H|0("
    "|\"|#|!|2|>H|3(|/!|*|=H|4(|!|>|?G|5(|)|QI|5(|#|+|QI|6(|\"|,|QI|7(|#|\"|"
    "!|2|RI|:(|0!|*|QI|;(|!|>|SH|<(|)|%K|<(|\"|+|%K|=(|#|,|%K|>(|\"|#|!|2|&K"
    "|A(|1!|*|%K|B(|!|>|'J|C(|)|9L|C(|#|+|9L|D(|\"|,|9L|E(|#|\"|!|2|:L|H(|2!"
    "|*|9L|I(|!|>|;K|J(|)|QM|J(|\"|+|QM|K(|#|,|QM|L(|\"|#|!|2|RM|O(|3!|*|QM|"
    "P(|!|>|SL|Q(|)|'O|Q(|#|+|'O|R(|\"|,|'O|S(|#|\"|!|2|(O|V(|4!|*|'O|W(|!|>"
    "|)N|X(|)|=P|X(|\"|+|=P|Y(|#|,|=P|Z(|\"|#|!|2|>P|](|5!|*|=P|^(|!|>|?O|_("
    "|)|RQ|_(|#|+|RQ| )|\"|,|RQ|!)|#|\"|!|2|SQ|$)|6!|*|RQ|%)|!|>|TP|&)|)|+S|"
    "&)|\"|+|+S|')|#|,|+S|()|\"|#|!|2|,S|+)|7!|*|+S|,)|!|>|-R|-)|)|ET|-)|#|+"
    "|ET|.)|\"|,|ET|/)|#|\"|!|2|FT|2)|8!|*|ET|3)|!|>|GS|4)|)| V|4)|\"|+| V|5"
    ")|#|,| V|6)|\"|#|!|2|!V|9)|9!|*| V|:)|!|>|\"U|;)|)|<W|;)|#|+|<W|<)|\"|,"
    "|<W|=)|#|\"|!|2|=W|@)|:!|*|<W|A)|!|>|>V|B)|)|UX|B)|\"|+|UX|C)|#|,|UX|D)"
    "|\"|#|!|2|VX|G)|;!|*|UX|H)|!|>|WW|I)|}L|%|}M||0|!|1||Temp$0||Temp$1||to"
    "ken_count|}N|#|}O||}P|}Q|}"
};

static const char* charset_str =
{
    "|!|}!|'$'| \"|'-'|!\"|'\\\\$'|\"\"|'\\\\-'|#\"|'\\\\D'|$\"|'\\\\S'|%\"|"
    "'\\\\['|&\"|'\\\\\\\\'|'\"|'\\\\]'|(\"|'\\\\^'|)\"|'\\\\d'|*\"|'\\\\n'|"
    "+\"|'\\\\r'|,\"|'\\\\s'|-\"|'^'|.\"|*eof*|/\"|*epsilon*|0\"|*error*|1\""
    "|<char>|2\"|AstActionAdd|U!|AstActionAnd|Z!|AstActionAssign|N!|AstActio"
    "nDivide|X!|AstActionDumpStack|]!|AstActionEqual|O!|AstActionGreaterEqua"
    "l|T!|AstActionGreaterThan|S!|AstActionLessEqual|R!|AstActionLessThan|Q!"
    "|AstActionMultiply|W!|AstActionNot|\\!|AstActionNotEqual|P!|AstActionOr"
    "|[!|AstActionStatementList|M!|AstActionSubtract|V!|AstActionTokenCount|"
    "^!|AstActionUnaryMinus|Y!|AstAstChild|I|AstAstDot|P|AstAstFormer|G|AstA"
    "stItemList|H|AstAstKind|J|AstAstLexeme|M|AstAstLexemeString|N|AstAstLoc"
    "ation|K|AstAstLocationString|L|AstAstLocator|O|AstAstSlice|Q|AstCaseSen"
    "sitive|*|AstCharset|<!|AstCharsetAltNewline|E!|AstCharsetCaret|H!|AstCh"
    "arsetChar|?!|AstCharsetCr|G!|AstCharsetDash|I!|AstCharsetDigits|B!|AstC"
    "harsetDollar|J!|AstCharsetEscape|D!|AstCharsetInvert|=!|AstCharsetLeftB"
    "racket|K!|AstCharsetNewline|F!|AstCharsetNotDigits|C!|AstCharsetNotWhit"
    "espace|A!|AstCharsetRange|>!|AstCharsetRightBracket|L!|AstCharsetString"
    "|V|AstCharsetWhitespace|@!|AstConflicts|(|AstEmpty|F|AstErrorRecovery|'"
    "|AstFalse|^|AstGrammar|\"|AstGroup|<|AstIdentifier|X|AstInteger|Y|AstKe"
    "epWhitespace|)|AstLookaheads|&|AstMacroString|W|AstNegativeInteger|Z|As"
    "tNonterminalReference|E|AstNull|!|AstOneClosure|;|AstOptionList|#|AstOp"
    "tional|9|AstOptions|S|AstReduceActions|T|AstRegex|_|AstRegexAltNewline|"
    ",!|AstRegexChar|%!|AstRegexCr|.!|AstRegexDigits|)!|AstRegexDollar|4!|As"
    "tRegexEscape|+!|AstRegexLeftBrace|:!|AstRegexLeftBracket|8!|AstRegexLef"
    "tParen|6!|AstRegexList|!!|AstRegexNewline|-!|AstRegexNotDigits|*!|AstRe"
    "gexNotWhitespace|(!|AstRegexOneClosure|$!|AstRegexOptional|\"!|AstRegex"
    "Or| !|AstRegexPeriod|3!|AstRegexPlus|1!|AstRegexQuestion|2!|AstRegexRig"
    "htBrace|;!|AstRegexRightBracket|9!|AstRegexRightParen|7!|AstRegexSpace|"
    "5!|AstRegexStar|0!|AstRegexString|U|AstRegexVBar|/!|AstRegexWhitespace|"
    "'!|AstRegexWildcard|&!|AstRegexZeroClosure|#!|AstRule|6|AstRuleLeftAsso"
    "c|@|AstRuleList|%|AstRuleOperatorList|B|AstRuleOperatorSpec|C|AstRulePr"
    "ecedence|=|AstRulePrecedenceList|>|AstRulePrecedenceSpec|?|AstRuleRhs|8"
    "|AstRuleRhsList|7|AstRuleRightAssoc|A|AstString|[|AstTerminalReference|"
    "D|AstToken|R|AstTokenAction|2|AstTokenDeclaration|+|AstTokenDescription"
    "|.|AstTokenError|5|AstTokenIgnore|4|AstTokenLexeme|3|AstTokenList|$|Ast"
    "TokenOptionList|,|AstTokenPrecedence|1|AstTokenRegex|0|AstTokenRegexLis"
    "t|/|AstTokenTemplate|-|AstTripleString|\\|AstTrue|]|AstUnknown||AstZero"
    "Closure|:|Unknown|_!|}\"|//{ -{ *//  Character Set Grammar{ -{ *//  ---"
    "------------------{ -{ *//{ -{ *//  Grammar file for the scanner regula"
    "r expression character sets.{ -{ *//{ -{ *{ -{ *options{ -{ *{ -{ *    "
    "lookaheads = 2{ -{ *    conflicts = 0{ -{ *    keep_whitespace = true{ "
    "-{ *    case_sensitive = true{ -{ *{ -{ *tokens{ -{ *{ -{ *    <char>  "
    "             : regex = ''' [^\\^\\-\\[\\]\\$] '''{ -{ *{ -{ *rules     "
    "  { -{ *        { -{ *    CharsetRoot          ::= '^' CharsetList{ -{ "
    "*                         :   (AstCharsetInvert, $2._){ -{ *{ -{ *    C"
    "harsetRoot          ::= CharsetList{ -{ *{ -{ *    CharsetList         "
    " ::= CharsetItem+{ -{ *                         :   (AstCharset, $1._){"
    " -{ *{ -{ *    CharsetItem          ::= Char '-' Char{ -{ *            "
    "             :   (AstCharsetRange, $1, $3, @2){ -{ *{ -{ *    CharsetIt"
    "em          ::= Charset{ -{ *{ -{ *    CharsetItem          ::= Char{ -"
    "{ *                         :   (AstCharsetRange, $1){ -{ *{ -{ *    Ch"
    "arset              ::= '\\\\s' { -{ *                         :   (AstC"
    "harsetWhitespace){ -{ *{ -{ *    Charset              ::= '\\\\S' { -{ "
    "*                         :   (AstCharsetNotWhitespace){ -{ *{ -{ *    "
    "Charset              ::= '\\\\d' { -{ *                         :   (As"
    "tCharsetDigits){ -{ *{ -{ *    Charset              ::= '\\\\D' { -{ * "
    "                        :   (AstCharsetNotDigits){ -{ *{ -{ *    Char  "
    "               ::= '\\\\\\\\' { -{ *                         :   (AstCh"
    "arsetEscape){ -{ *{ -{ *    Char                 ::= '$' { -{ *        "
    "                 :   (AstCharsetAltNewline){ -{ *{ -{ *    Char        "
    "         ::= '\\\\n' { -{ *                         :   (AstCharsetNewl"
    "ine){ -{ *{ -{ *    Char                 ::= '\\\\r' { -{ *            "
    "             :   (AstCharsetCr){ -{ *{ -{ *    Char                 ::="
    " '\\\\^' { -{ *                         :   (AstCharsetCaret){ -{ *{ -{"
    " *    Char                 ::= '\\\\-' { -{ *                         :"
    "   (AstCharsetDash){ -{ *{ -{ *    Char                 ::= '\\\\$' { -"
    "{ *                         :   (AstCharsetDollar){ -{ *{ -{ *    Char "
    "                ::= '\\\\[' { -{ *                         :   (AstChar"
    "setLeftBracket){ -{ *{ -{ *    Char                 ::= '\\\\]' { -{ * "
    "                        :   (AstCharsetRightBracket){ -{ *{ -{ *    Cha"
    "r                 ::= <char>{ -{ *                         :   (AstChar"
    "setChar, &1){ -{ *{ -{ *{ -{ *|}#|\"|}$|!|}%|7|}&||}'|:|}(|*eof*|'$'|'\\"
    "\\$'|'\\\\-'|'\\\\['|'\\\\\\\\'|'\\\\]'|'\\\\^'|'\\\\n'|'\\\\r'|<char>|"
    "'\\\\D'|'\\\\S'|'\\\\d'|'\\\\s'|'-'|'^'|||||||*error*||*epsilon*|})|!|!"
    "|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|6#|=|||||!|!|!|}*|/\"| \"|\"\"|#\"|&\"|'"
    "\"|(\"|)\"|+\"|,\"|2\"|$\"|%\"|*\"|-\"|!\"|.\"||!~!~|||1\"|0S'1|0\"|}+|"
    "||||||||||!|||||||6#|=||||||!||},|7|}-|!|\"|!|!|\"|!|#|!|!|!|!|!|!|!|!|"
    "!|!|!|!|!|!|!|!|}.|8|6|6|4|5|5|3|3|3|2|2|2|2|1|1|1|1|1|1|1|1|1|1|}/|*ac"
    "cept* ::= CharsetRoot|CharsetRoot ::= '^' CharsetList|CharsetRoot ::= C"
    "harsetList|CharsetList ::= CharsetList:1|CharsetList:1 ::= CharsetList:"
    "1 CharsetItem|CharsetList:1 ::= CharsetItem|CharsetItem ::= Char '-' Ch"
    "ar|CharsetItem ::= Charset|CharsetItem ::= Char|Charset ::= '\\\\s'|Cha"
    "rset ::= '\\\\S'|Charset ::= '\\\\d'|Charset ::= '\\\\D'|Char ::= '\\\\"
    "\\\\'|Char ::= '$'|Char ::= '\\\\n'|Char ::= '\\\\r'|Char ::= '\\\\^'|C"
    "har ::= '\\\\-'|Char ::= '\\\\$'|Char ::= '\\\\['|Char ::= '\\\\]'|Char"
    " ::= <char>|}0|!~I|!~Q|Y|#!|+!|!~7!|?!|E!|K!|Q!|W!|]!|#\"|)\"|/\"|5\"|;"
    "\"|A\"|G\"|M\"|}1|\"|}2||}3|:|}4|>|}5|6|6!|E|L|[|\"!|6$|F!|E$|T$|#%|2%|"
    "A%|V!|&\"|6\"|F\"|V\"|&#|6#|F#|V#|\\|P%|<&|_%||&$|.&|1!|}6|W&|}7|@\"|A "
    "<|B F|C D|D H|E :|F J|G B|H >|I @|J \"|K 8|L 4|M 6|N 2|O P|P &|Q!V|R!0|"
    "S!X|T!Z|U!*|V!$|A <|B F|C D|D H|E :|F J|G B|H >|I @|J \"|K 8|L 4|M 6|N "
    "2|@\"|P &|Q!.|R!0|S!,|T!(|U!*|V!$|A <|B F|C D|D H|E :|F J|G B|H >|I @|J"
    " \"|K 8|L 4|M 6|N 2| ) :| % :|Q!.|R!0|S!,|T!L|U!*| - :|A <|B F|C D|D H|"
    "E :|F J|G B|H >|I @|J \"|K 8|L 4|M 6|N 2| \"T|!~Q!.|R!0|S!N| 9!:|!9!.|\""
    "9!3|#9!2|$9!4|%9!-|&9!5|'9!1|(9!/|)9!0|*9!!|+9!,|,9!*|-9!+|.9!)|/9!8| A"
    " :|!A .|\"A 3|#A 2|$A 4|%A -|&A 5|'A 1|(A /|)A 0|*A !|+A ,|,A *|-A +|.A"
    " )|O P| U :|!U .|\"U 3|#U 2|$U 4|%U -|&U 5|'U 1|(U /|)U 0|*U !|+U ,|,U "
    "*|-U +|.U )|/U 8| Y :|!Y .|\"Y 3|#Y 2|$Y 4|%Y -|&Y 5|'Y 1|(Y /|)Y 0|*Y "
    "!|+Y ,|,Y *|-Y +|.Y )|/Y 8| ] :|!] .|\"] 3|#] 2|$] 4|%] -|&] 5|'] 1|(] "
    "/|)] 0|*] !|+] ,|,] *|-] +|.] )|/] 8| !!:|!!!.|\"!!3|#!!2|$!!4|%!!-|&!!"
    "5|'!!1|(!!/|)!!0|*!!!|+!!,|,!!*|-!!+|.!!)|/!!8| %!:|!%!.|\"%!3|#%!2|$%!"
    "4|%%!-|&%!5|'%!1|(%!/|)%!0|*%!!|+%!,|,%!*|-%!+|.%!)|/%!8| )!:|!)!.|\")!"
    "3|#)!2|$)!4|%)!-|&)!5|')!1|()!/|))!0|*)!!|+)!,|,)!*|-)!+|.)!)|/)!8| -!:"
    "|!-!.|\"-!3|#-!2|$-!4|%-!-|&-!5|'-!1|(-!/|)-!0|*-!!|+-!,|,-!*|--!+|.-!)"
    "|/-!8| 1!:|!1!.|\"1!3|#1!2|$1!4|%1!-|&1!5|'1!1|(1!/|)1!0|*1!!|+1!,|,1!*"
    "|-1!+|.1!)|/1!8| 5!:|!5!.|\"5!3|#5!2|$5!4|%5!-|&5!5|'5!1|(5!/|)5!0|*5!!"
    "|+5!,|,5!*|-5!+|.5!)|/5!8| \"T|!\"<|\"\"F|#\"D|$\"H|%\":|&\"J|'\"B|(\">"
    "|)\"@|*\"\"|+\"8|,\"4|-\"6|.\"2|O P| 5 :|!5 .|\"5 3|#5 2|$5 4|%5 -|&5 5"
    "|'5 1|(5 /|)5 0|*5 !|+5 ,|,5 *|-5 +|.5 )| = :|!= .|\"= 3|#= 2|$= 4|%= -"
    "|&= 5|'= 1|(= /|)= 0|*= !|+= ,|,= *|-= +|.= )| E :|!E .|\"E 3|#E 2|$E 4"
    "|%E -|&E 5|'E 1|(E /|)E 0|*E !|+E ,|,E *|-E +|.E )| I :|!I .|\"I 3|#I 2"
    "|$I 4|%I -|&I 5|'I 1|(I /|)I 0|*I !|+I ,|,I *|-I +|.I )| M :|!M .|\"M 3"
    "|#M 2|$M 4|%M -|&M 5|'M 1|(M /|)M 0|*M !|+M ,|,M *|-M +|.M )| Q :|!Q .|"
    "\"Q 3|#Q 2|$Q 4|%Q -|&Q 5|'Q 1|(Q /|)Q 0|*Q !|+Q ,|,Q *|-Q +|.Q )| 1 :|"
    "!1 .|\"1 3|#1 2|$1 4|%1 -|&1 5|'1 1|(1 /|)1 0|*1 !|+1 ,|,1 *|-1 +|.1 )|"
    " 9 :|!9 .|\"9 3|#9 2|$9 4|%9 -|&9 5|'9 1|(9 /|)9 0|*9 !|+9 ,|,9 *|-9 +|"
    ".9 )| \"T|!\"<|\"\"F|#\"D|$\"H|%\":|&\"J|'\"B|(\">|)\"@|*\"\"|+\"8|,\"4"
    "|-\"6|.\"2|A <|B F|C D|D H|E :|F J|G B|H >|I @|J \"|!~!~!~!~!~!~Q!R|!~!"
    "~!~!~!~!~!~!~!~}8|!|}9||}:||};|?|}<||}=|%|}>|'|}?||}@|(|}A|?|}B||}C|-|}"
    "D|?|}E||}F|2|}G|?|}H|Null||Halt|!|Label|\"|Call|#|ScanStart|$|ScanChar|"
    "%|ScanAccept|&|ScanToken|'|ScanError|(|AstStart|)|AstFinish|*|AstNew|+|"
    "AstForm|,|AstLoad|-|AstIndex|.|AstChild|/|AstChildSlice|0|AstKind|1|Ast"
    "KindNum|2|AstLocation|3|AstLocationNum|4|AstLexeme|5|AstLexemeString|6|"
    "Assign|7|DumpStack|8|Add|9|Subtract|:|Multiply|;|Divide|<|UnaryMinus|=|"
    "Return|>|Branch|?|BranchEqual|@|BranchNotEqual|A|BranchLessThan|B|Branc"
    "hLessEqual|C|BranchGreaterThan|D|BranchGreaterEqual|E|}I|U\"|}J|O$|}K|7"
    "|!~|$||>|!~\"|$|!~\"|%|!~\"|(||C|$|D|D|&|E|L|$|M|M|(|N|:!|$|<!|<!|*|>!|"
    ">!|D|?!|_____#|$|&|\"$|;|*|F|%|!~=||&|24|>|!|F|%|!~@||&|E)|A|/|F|%|!~C|"
    "|&|\"$|D|*|F|%|!~F|,|D|D|,|M|M|.|$!|$!|0|3!|3!|2|;!|;!|4|<!|<!|6|=!|=!|"
    "8|>!|>!|:|D!|D!|<|N!|N!|>|R!|R!|@|S!|S!|B|&|&;|+!|\"|F|%|!~-!||&|P9|.!|"
    "#|F|%|!~0!||&|>1|1!|+|F|%|!~3!||&|G.|4!|,|F|%|!~6!||&|><|7!|$|F|%|!~9!|"
    "|&|Y2|:!|%|F|%|!~<!||&|[=|=!|&|F|%|!~?!||&|98|@!|'|F|%|!~B!||&|&0|C!|-|"
    "F|%|!~E!||&|L5|F!|(|F|%|!~H!||&|%7|I!|)|F|%|!~K!||&|+-|L!|.|F|%|!~N!||&"
    "|O%|O!|0|F|%|!~Q!||'|!~R!|9|!~R!|$|$|!|>|!~U!|)|=&|U!|\"|+|=&|V!|#|-|Q&"
    "|W!||\"|!~0|S&|Z!|||!~,|=&|]!|\"|#|\"|2|>&| \"|=!|*|=&|!\"|\"|>|6%|\"\""
    "|)|M(|\"\"|#|+|M(|#\"|\"|-|[(|$\"||#|!~0|](|'\"|||!~,|M(|*\"|#|\"|!|2|N"
    "(|-\"|<!|*|M(|.\"|!|>|I'|/\"|)|I'|/\"|\"|+|I'|0\"|#|-|I'|1\"||\"|\"~0|I"
    "'|4\"|||!~-|I'|7\"||\"|!~/|I'|:\"||,|I'|;\"|\"|#|\"|2|I'|>\"|_!|*|I'|?\""
    "|\"|>|I'|@\"|)|I'|@\"|#|+|I'|A\"|\"|-|I'|B\"||#|!~/|I'|E\"||,|I'|F\"|#|"
    "\"|!|2|I'|I\"|_!|*|I'|J\"|!|>|I'|K\"|)|,*|K\"|\"|+|,*|L\"|#|-|?*|M\"||\""
    "|#~/|>*|P\"||-|C*|Q\"||\"|!~/|B*|T\"||,|,*|U\"|\"|#|#|2|-*|X\"|>!|-|G*|"
    "Y\"||\"|\"~3|F*|\\\"||*|,*|]\"|#|>|')|^\"|)|5,|^\"|#|+|5,|_\"|\"|-|H,| "
    "#||#|!~/|G,|##||,|5,|$#|#|\"|!|2|6,|'#|>!|*|5,|(#|!|>|9+|)#|)|P-|)#|\"|"
    "+|P-|*#|#|,|P-|+#|\"|#|!|2|Q-|.#|@!|*|P-|/#|!|>|R,|0#|)|,/|0#|#|+|,/|1#"
    "|\"|,|,/|2#|#|\"|!|2|-/|5#|A!|*|,/|6#|!|>|..|7#|)|K0|7#|\"|+|K0|8#|#|,|"
    "K0|9#|\"|#|!|2|L0|<#|B!|*|K0|=#|!|>|M/|>#|)|#2|>#|#|+|#2|?#|\"|,|#2|@#|"
    "#|\"|!|2|$2|C#|C!|*|#2|D#|!|>|%1|E#|)|?3|E#|\"|+|?3|F#|#|,|?3|G#|\"|#|!"
    "|2|@3|J#|D!|*|?3|K#|!|>|@2|L#|)|U4|L#|#|+|U4|M#|\"|,|U4|N#|#|\"|!|2|V4|"
    "Q#|E!|*|U4|R#|!|>|Y3|S#|)|16|S#|\"|+|16|T#|#|,|16|U#|\"|#|!|2|26|X#|F!|"
    "*|16|Y#|!|>|35|Z#|)|J7|Z#|#|+|J7|[#|\"|,|J7|\\#|#|\"|!|2|K7|_#|G!|*|J7|"
    " $|!|>|L6|!$|)|^8|!$|\"|+|^8|\"$|#|,|^8|#$|\"|#|!|2|_8|&$|H!|*|^8|'$|!|"
    ">| 8|($|)|5:|($|#|+|5:|)$|\"|,|5:|*$|#|\"|!|2|6:|-$|I!|*|5:|.$|!|>|79|/"
    "$|)|K;|/$|\"|+|K;|0$|#|,|K;|1$|\"|#|!|2|L;|4$|J!|*|K;|5$|!|>|M:|6$|)|#="
    "|6$|#|+|#=|7$|\"|,|#=|8$|#|\"|!|2|$=|;$|K!|*|#=|<$|!|>|%<|=$|)|@>|=$|\""
    "|+|@>|>$|#|,|@>|?$|\"|#|!|2|A>|B$|L!|*|@>|C$|!|>|B=|D$|)|^?|D$|#|+|^?|E"
    "$|\"|,|^?|F$|#|\"|!|2|_?|I$|?!|-|0@|J$||#|!~5|/@|M$||*|^?|N$|!|>| ?|O$|"
    "}L|%|}M||0|!|1||Temp$0||Temp$1||token_count|}N|!|}O||}P|}Q|}"
};

//
//  Gramamr parsers                                                        
//  ---------------                                                        
//                                                                         
//  A grammar file is made up of a few mini-languages and we have separate 
//  parsers for some of them.                                              
//

ParserData* ParserImpl::grammar_parser_data = nullptr;
ParserData* ParserImpl::regex_parser_data = nullptr;
ParserData* ParserImpl::charset_parser_data = nullptr;

//
//  initialize                                                             
//  ----------                                                             
//                                                                         
//  On startup create our required parsers generated in a prior generation 
//  of this program.                                                       
//

void ParserImpl::initialize()
{

    map<string, int> kind_map;

    kind_map["AstUnknown"] = AstType::AstUnknown;
    kind_map["AstNull"] = AstType::AstNull;
    kind_map["AstGrammar"] = AstType::AstGrammar;
    kind_map["AstOptionList"] = AstType::AstOptionList;
    kind_map["AstTokenList"] = AstType::AstTokenList;
    kind_map["AstRuleList"] = AstType::AstRuleList;
    kind_map["AstLookaheads"] = AstType::AstLookaheads;
    kind_map["AstErrorRecovery"] = AstType::AstErrorRecovery;
    kind_map["AstConflicts"] = AstType::AstConflicts;
    kind_map["AstKeepWhitespace"] = AstType::AstKeepWhitespace;
    kind_map["AstCaseSensitive"] = AstType::AstCaseSensitive;
    kind_map["AstTokenDeclaration"] = AstType::AstTokenDeclaration;
    kind_map["AstTokenOptionList"] = AstType::AstTokenOptionList;
    kind_map["AstTokenTemplate"] = AstType::AstTokenTemplate;
    kind_map["AstTokenDescription"] = AstType::AstTokenDescription;
    kind_map["AstTokenRegexList"] = AstType::AstTokenRegexList;
    kind_map["AstTokenRegex"] = AstType::AstTokenRegex;
    kind_map["AstTokenPrecedence"] = AstType::AstTokenPrecedence;
    kind_map["AstTokenAction"] = AstType::AstTokenAction;
    kind_map["AstTokenLexeme"] = AstType::AstTokenLexeme;
    kind_map["AstTokenIgnore"] = AstType::AstTokenIgnore;
    kind_map["AstTokenError"] = AstType::AstTokenError;
    kind_map["AstRule"] = AstType::AstRule;
    kind_map["AstRuleRhsList"] = AstType::AstRuleRhsList;
    kind_map["AstRuleRhs"] = AstType::AstRuleRhs;
    kind_map["AstOptional"] = AstType::AstOptional;
    kind_map["AstZeroClosure"] = AstType::AstZeroClosure;
    kind_map["AstOneClosure"] = AstType::AstOneClosure;
    kind_map["AstGroup"] = AstType::AstGroup;
    kind_map["AstRulePrecedence"] = AstType::AstRulePrecedence;
    kind_map["AstRulePrecedenceList"] = AstType::AstRulePrecedenceList;
    kind_map["AstRulePrecedenceSpec"] = AstType::AstRulePrecedenceSpec;
    kind_map["AstRuleLeftAssoc"] = AstType::AstRuleLeftAssoc;
    kind_map["AstRuleRightAssoc"] = AstType::AstRuleRightAssoc;
    kind_map["AstRuleOperatorList"] = AstType::AstRuleOperatorList;
    kind_map["AstRuleOperatorSpec"] = AstType::AstRuleOperatorSpec;
    kind_map["AstTerminalReference"] = AstType::AstTerminalReference;
    kind_map["AstNonterminalReference"] = AstType::AstNonterminalReference;
    kind_map["AstEmpty"] = AstType::AstEmpty;
    kind_map["AstAstFormer"] = AstType::AstAstFormer;
    kind_map["AstAstItemList"] = AstType::AstAstItemList;
    kind_map["AstAstChild"] = AstType::AstAstChild;
    kind_map["AstAstKind"] = AstType::AstAstKind;
    kind_map["AstAstLocation"] = AstType::AstAstLocation;
    kind_map["AstAstLocationString"] = AstType::AstAstLocationString;
    kind_map["AstAstLexeme"] = AstType::AstAstLexeme;
    kind_map["AstAstLexemeString"] = AstType::AstAstLexemeString;
    kind_map["AstAstLocator"] = AstType::AstAstLocator;
    kind_map["AstAstDot"] = AstType::AstAstDot;
    kind_map["AstAstSlice"] = AstType::AstAstSlice;
    kind_map["AstToken"] = AstType::AstToken;
    kind_map["AstOptions"] = AstType::AstOptions;
    kind_map["AstReduceActions"] = AstType::AstReduceActions;
    kind_map["AstRegexString"] = AstType::AstRegexString;
    kind_map["AstCharsetString"] = AstType::AstCharsetString;
    kind_map["AstMacroString"] = AstType::AstMacroString;
    kind_map["AstIdentifier"] = AstType::AstIdentifier;
    kind_map["AstInteger"] = AstType::AstInteger;
    kind_map["AstNegativeInteger"] = AstType::AstNegativeInteger;
    kind_map["AstString"] = AstType::AstString;
    kind_map["AstTripleString"] = AstType::AstTripleString;
    kind_map["AstTrue"] = AstType::AstTrue;
    kind_map["AstFalse"] = AstType::AstFalse;
    kind_map["AstRegex"] = AstType::AstRegex;
    kind_map["AstRegexOr"] = AstType::AstRegexOr;
    kind_map["AstRegexList"] = AstType::AstRegexList;
    kind_map["AstRegexOptional"] = AstType::AstRegexOptional;
    kind_map["AstRegexZeroClosure"] = AstType::AstRegexZeroClosure;
    kind_map["AstRegexOneClosure"] = AstType::AstRegexOneClosure;
    kind_map["AstRegexChar"] = AstType::AstRegexChar;
    kind_map["AstRegexWildcard"] = AstType::AstRegexWildcard;
    kind_map["AstRegexWhitespace"] = AstType::AstRegexWhitespace;
    kind_map["AstRegexNotWhitespace"] = AstType::AstRegexNotWhitespace;
    kind_map["AstRegexDigits"] = AstType::AstRegexDigits;
    kind_map["AstRegexNotDigits"] = AstType::AstRegexNotDigits;
    kind_map["AstRegexEscape"] = AstType::AstRegexEscape;
    kind_map["AstRegexAltNewline"] = AstType::AstRegexAltNewline;
    kind_map["AstRegexNewline"] = AstType::AstRegexNewline;
    kind_map["AstRegexCr"] = AstType::AstRegexCr;
    kind_map["AstRegexVBar"] = AstType::AstRegexVBar;
    kind_map["AstRegexStar"] = AstType::AstRegexStar;
    kind_map["AstRegexPlus"] = AstType::AstRegexPlus;
    kind_map["AstRegexQuestion"] = AstType::AstRegexQuestion;
    kind_map["AstRegexPeriod"] = AstType::AstRegexPeriod;
    kind_map["AstRegexDollar"] = AstType::AstRegexDollar;
    kind_map["AstRegexSpace"] = AstType::AstRegexSpace;
    kind_map["AstRegexLeftParen"] = AstType::AstRegexLeftParen;
    kind_map["AstRegexRightParen"] = AstType::AstRegexRightParen;
    kind_map["AstRegexLeftBracket"] = AstType::AstRegexLeftBracket;
    kind_map["AstRegexRightBracket"] = AstType::AstRegexRightBracket;
    kind_map["AstRegexLeftBrace"] = AstType::AstRegexLeftBrace;
    kind_map["AstRegexRightBrace"] = AstType::AstRegexRightBrace;
    kind_map["AstCharset"] = AstType::AstCharset;
    kind_map["AstCharsetInvert"] = AstType::AstCharsetInvert;
    kind_map["AstCharsetRange"] = AstType::AstCharsetRange;
    kind_map["AstCharsetChar"] = AstType::AstCharsetChar;
    kind_map["AstCharsetWhitespace"] = AstType::AstCharsetWhitespace;
    kind_map["AstCharsetNotWhitespace"] = AstType::AstCharsetNotWhitespace;
    kind_map["AstCharsetDigits"] = AstType::AstCharsetDigits;
    kind_map["AstCharsetNotDigits"] = AstType::AstCharsetNotDigits;
    kind_map["AstCharsetEscape"] = AstType::AstCharsetEscape;
    kind_map["AstCharsetAltNewline"] = AstType::AstCharsetAltNewline;
    kind_map["AstCharsetNewline"] = AstType::AstCharsetNewline;
    kind_map["AstCharsetCr"] = AstType::AstCharsetCr;
    kind_map["AstCharsetCaret"] = AstType::AstCharsetCaret;
    kind_map["AstCharsetDash"] = AstType::AstCharsetDash;
    kind_map["AstCharsetDollar"] = AstType::AstCharsetDollar;
    kind_map["AstCharsetLeftBracket"] = AstType::AstCharsetLeftBracket;
    kind_map["AstCharsetRightBracket"] = AstType::AstCharsetRightBracket;
    kind_map["AstActionStatementList"] = AstType::AstActionStatementList;
    kind_map["AstActionAssign"] = AstType::AstActionAssign;
    kind_map["AstActionEqual"] = AstType::AstActionEqual;
    kind_map["AstActionNotEqual"] = AstType::AstActionNotEqual;
    kind_map["AstActionLessThan"] = AstType::AstActionLessThan;
    kind_map["AstActionLessEqual"] = AstType::AstActionLessEqual;
    kind_map["AstActionGreaterThan"] = AstType::AstActionGreaterThan;
    kind_map["AstActionGreaterEqual"] = AstType::AstActionGreaterEqual;
    kind_map["AstActionAdd"] = AstType::AstActionAdd;
    kind_map["AstActionSubtract"] = AstType::AstActionSubtract;
    kind_map["AstActionMultiply"] = AstType::AstActionMultiply;
    kind_map["AstActionDivide"] = AstType::AstActionDivide;
    kind_map["AstActionUnaryMinus"] = AstType::AstActionUnaryMinus;
    kind_map["AstActionAnd"] = AstType::AstActionAnd;
    kind_map["AstActionOr"] = AstType::AstActionOr;
    kind_map["AstActionNot"] = AstType::AstActionNot;
    kind_map["AstActionDumpStack"] = AstType::AstActionDumpStack;
    kind_map["AstActionTokenCount"] = AstType::AstActionTokenCount;

    grammar_parser_data = new ParserData();
    regex_parser_data = new ParserData();
    charset_parser_data = new ParserData();

    grammar_parser_data->decode(grammar_str, kind_map);
    regex_parser_data->decode(regex_str, kind_map);
    charset_parser_data->decode(charset_str, kind_map);

}

//
//  Copy control                                                         
//  ------------                                                         
//                                                                       
//  The ParserImpl object does support copying. But note that copying    
//  ParserData is a pointer copy. We use a reference count on ParserData 
//  to control that.                                                     
//

ParserImpl::~ParserImpl()
{
    ParserData::detach(prsd);
    delete errh;
    delete ast;
}

ParserImpl::ParserImpl(const ParserImpl& rhs)
{

    state = rhs.state;

    prsd = rhs.prsd;
    ParserData::attach(prsd);

    if (errh == nullptr)
    {
        errh = nullptr;
    }
    else
    {
        errh = new ErrorHandler(*rhs.errh);
    }

    if (ast == nullptr)
    {
        ast = nullptr;
    }
    else
    {
        ast = rhs.ast->clone();
    }

}

ParserImpl::ParserImpl(ParserImpl&& rhs) noexcept
{

    swap(state, rhs.state);
    swap(prsd, rhs.prsd);    
    swap(errh, rhs.errh);    
    swap(ast, rhs.ast);    

}

ParserImpl& ParserImpl::operator=(const ParserImpl& rhs)
{

    if (&rhs != this)
    {

        state = rhs.state;

        ParserData::detach(prsd);

        prsd = rhs.prsd;
        ParserData::attach(prsd);   

        if (errh == nullptr)
        {
            errh = nullptr;
        }
        else
        {
            delete errh;
            errh = new ErrorHandler(*rhs.errh);
        }

        if (ast == nullptr)
        {
            ast = nullptr;
        }
        else
        {
            delete ast;
            ast = rhs.ast->clone();
        }

    }

    return *this;

}

ParserImpl& ParserImpl::operator=(ParserImpl&& rhs) noexcept
{

    swap(state, rhs.state);
    swap(prsd, rhs.prsd);    
    swap(errh, rhs.errh);    
    swap(ast, rhs.ast);    

    return *this;

}

//
//  State Queries                                                        
//  -------------                                                        
//                                                                       
//  Some of our accessors throw exceptions if the object is in the wrong 
//  state. These predicates give the client the ability to check in      
//  advance.                                                             
//

bool ParserImpl::is_grammar_loaded()
{
    return state == ParserState::GrammarGood ||
           state == ParserState::SourceGood ||
           state == ParserState::SourceBad;
}

bool ParserImpl::is_grammar_failed()
{
    return state == ParserState::GrammarBad;
}

bool ParserImpl::is_source_loaded()
{
    return state == ParserState::SourceGood;
}

bool ParserImpl::is_source_failed()
{
    return state == ParserState::SourceBad;
}

//
//  set_kind_map                                                    
//  ------------                                                    
//                                                                   
//  As part of the bootstrap procedure *only* we need the ability to 
//  create a skeletal parser that only contains the Ast type map.    
//

void ParserImpl::set_kind_map(const std::map<std::string, int>& kind_map)
{

    //
    //  We're about to do a state transition. Check whether we are in an 
    //  appropriate state and clear unnecessary data from the existing   
    //  state.                                                           
    //

    switch (state)
    {

        case ParserState::Invalid:
        {
            break;
        }

        case ParserState::KindMapGood:
        {

            ParserData::detach(prsd);
            state = ParserState::Invalid;

            break;

        }

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        {

            ParserData::detach(prsd);

            delete errh;
            errh = nullptr;

            state = ParserState::Invalid;

            break;

        }

        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {

            ParserData::detach(prsd);

            delete ast;
            ast = nullptr;

            delete errh;
            errh = nullptr;

            state = ParserState::Invalid;

            break;

        }

        default:
        {
            throw logic_error("State error in Parser::set_ast_kind");
        }

    }

    try
    {

        //
        //  Create a new ParserData object and start an error list. 
        //

        prsd = new ParserData();
        ParserData::attach(prsd);

        prsd->set_kind_map(kind_map);

        state = ParserState::KindMapGood;

    }
    catch (...)
    {

        ParserData::detach(prsd);
        state = ParserState::Invalid;

        throw;

    }

}

//
//  generate                                                               
//  --------                                                               
//                                                                         
//  Generate a parser from an Ast. During bootstrapping we will get an Ast 
//  from a non-hoshi parser. We can use that Ast to generate a Hoshi       
//  parser.                                                                
//

void ParserImpl::generate(Ast* ast,
                          const Source& src,
                          const std::map<std::string, int>& kind_map,
                          const int64_t debug_flags)   
{

    //
    //  We're about to do a state transition. Check whether we are in an 
    //  appropriate state and clear unnecessary data from the existing   
    //  state.                                                           
    //

    switch (state)
    {

        case ParserState::Invalid:
        {
            break;
        }

        case ParserState::KindMapGood:
        {

            ParserData::detach(prsd);
            state = ParserState::Invalid;

            break;

        }

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        {

            ParserData::detach(prsd);

            delete errh;
            errh = nullptr;

            state = ParserState::Invalid;

            break;

        }

        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {

            ParserData::detach(prsd);

            delete ast;
            ast = nullptr;

            delete errh;
            errh = nullptr;

            state = ParserState::Invalid;

            break;

        }

        default:
        {
            throw logic_error("State error in Parser:;generate");
        }

    }

    //
    //  Allocate and free helper modules as we need them. 
    //

    prsd = nullptr;
    errh = nullptr;

    Grammar* gram = nullptr;
    ActionGenerator* actg = nullptr;
    CodeGenerator* code = nullptr;
    ScannerGenerator* scan = nullptr;
    ReduceGenerator* redg = nullptr;

    try
    {

        //
        //  Create a new ParserData object and start an error list. 
        //

        prsd = new ParserData();
        ParserData::attach(prsd);

        prsd->set_kind_map(kind_map);
        prsd->src = src;
        errh = new ErrorHandler(prsd->src);

        state = ParserState::GrammarGood;

        start_timer();

        //
        //  Load the ast into grammar tables. 
        //

        gram = new Grammar(*this, *errh, *prsd, ast, debug_flags);
        gram->extract();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  Perform a basic edit.
        //

        Editor(*this, *errh, *gram, debug_flags).generate();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  Construct the code generation utilities. 
        //

        code = new CodeGenerator(*this, *errh, *gram, *prsd, debug_flags);
        actg = new ActionGenerator(*this, *errh, *code, debug_flags);

        //
        //  Generate the LALR(k) parser and save the automaton in an action 
        //  table.                                                        
        //

        LalrGenerator(*this, *errh, *gram, *prsd, debug_flags).generate();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  Generate the scanner. 
        //

        scan = new ScannerGenerator(*this, *errh, *gram, *code, *actg, *prsd, debug_flags);
        scan->generate();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  There should be no more changes to the grammar so we can 
        //  generate the reduce actions.
        //

        redg = new ReduceGenerator(*this, *errh, *gram, *code, *actg, *prsd, debug_flags);
        redg->generate();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  We are done generating intermediate code. Optimize it and 
        //  convert to VM code.                                       
        //

        code->generate();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  Save whatever's left in the ParserData object. 
        //

        scan->save_parser_data();
        gram->save_parser_data();
        redg->save_parser_data();

        //
        //  Reclaim memory from remaining work objects. 
        //

        delete scan;
        scan = nullptr;

        delete gram;
        gram = nullptr;

        delete code;
        code = nullptr;

        delete actg;
        actg = nullptr;

        delete redg;
        redg = nullptr;

    }

    //
    //  If there are any errors then clean up the mess. 
    //

    catch (...)
    {

        ParserData::detach(prsd);
        state = ParserState::GrammarBad;

        delete gram;
        delete code;
        delete actg;
        delete redg;

        throw;

    }

}

//
//  generate_ast                                                          
//  ------------                                                          
//                                                                        
//  Generate an Ast from the bootstrapped Hoshi parser. This is only used 
//  to validate the bootstrap process.                                    
//

Ast* ParserImpl::generate_ast(const Source& src,
                              const int64_t debug_flags)   
{

    ErrorHandler temp_errh(src);
    ErrorHandler *save_errh = errh;

    try
    {

        errh = &temp_errh;
        Ast* ast;

        try
        {
            ParserEngine(*this, *errh, *grammar_parser_data, src, ast, debug_flags).parse();
        }
        catch (SourceError e)
        {
            throw GrammarError("Grammar errors");
        }

        bool any_changes = true;
        while (any_changes)
        {
            any_changes = false;
            expand_subtrees(ast, any_changes, debug_flags);
        }

        errh = save_errh;

        return ast;

    }
    catch (...)
    {

        if (errh == &temp_errh)
        {
            errh = save_errh;
        }

        throw;

    }

}

//
//  generate                               
//  --------                               
//                                         
//  Generate a parser from a grammar file. 
//

void ParserImpl::generate(const Source& src,
                          const std::map<std::string, int>& kind_map,
                          const int64_t debug_flags)   
{

    //
    //  We're about to do a state transition. Check whether we are in an 
    //  appropriate state and clear unnecessary data from the existing   
    //  state.                                                           
    //

    switch (state)
    {

        case ParserState::Invalid:
        {
            break;
        }

        case ParserState::KindMapGood:
        {

            ParserData::detach(prsd);
            state = ParserState::Invalid;

            break;

        }

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        {

            ParserData::detach(prsd);

            delete errh;
            errh = nullptr;

            state = ParserState::Invalid;

            break;

        }

        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {

            ParserData::detach(prsd);

            delete ast;
            ast = nullptr;

            delete errh;
            errh = nullptr;

            state = ParserState::Invalid;

            break;

        }

        default:
        {
            throw logic_error("State error in Parser::generate");
        }

    }

    //
    //  Allocate and free helper modules as we need them. 
    //

    prsd = nullptr;
    errh = nullptr;

    Grammar* gram = nullptr;
    ActionGenerator* actg = nullptr;
    CodeGenerator* code = nullptr;
    ScannerGenerator* scan = nullptr;
    ReduceGenerator* redg = nullptr;

    try
    {

        //
        //  Create a new ParserData object and start an error list. 
        //

        prsd = new ParserData();
        ParserData::attach(prsd);

        prsd->set_kind_map(kind_map);
        prsd->src = src;
        errh = new ErrorHandler(prsd->src);

        state = ParserState::GrammarGood;

        start_timer();

        //
        //  Parse the grammar string. 
        //

        try
        {
            ParserEngine(*this, *errh, *grammar_parser_data, src, ast, debug_flags).parse();
        }
        catch (SourceError e)
        {
            throw GrammarError("Grammar errors");
        }

        bool any_changes = true;
        while (any_changes)
        {
            any_changes = false;
            expand_subtrees(ast, any_changes, debug_flags);
        }

        //
        //  Load the ast into grammar tables. 
        //

        gram = new Grammar(*this, *errh, *prsd, ast, debug_flags);
        gram->extract();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  Perform a basic edit.
        //

        Editor(*this, *errh, *gram, debug_flags).generate();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  Construct the code generation utilities. 
        //

        code = new CodeGenerator(*this, *errh, *gram, *prsd, debug_flags);
        actg = new ActionGenerator(*this, *errh, *code, debug_flags);

        //
        //  Generate the LALR(k) parser and save the automaton in an action 
        //  table.                                                        
        //

        LalrGenerator(*this, *errh, *gram, *prsd, debug_flags).generate();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  Generate the scanner. 
        //

        scan = new ScannerGenerator(*this, *errh, *gram, *code, *actg, *prsd, debug_flags);
        scan->generate();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  There should be no more changes to the grammar so we can 
        //  generate the reduce actions.
        //

        redg = new ReduceGenerator(*this, *errh, *gram, *code, *actg, *prsd, debug_flags);
        redg->generate();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  We are done generating intermediate code. Optimize it and 
        //  convert to VM code.                                       
        //

        code->generate();

        if (errh->get_error_count() > 0)
        {
            throw GrammarError("Grammar errors");
        }

        //
        //  Save whatever's left in the ParserData object. 
        //

        scan->save_parser_data();
        gram->save_parser_data();
        redg->save_parser_data();

        //
        //  Reclaim memory from remaining work objects. 
        //

        delete scan;
        scan = nullptr;

        delete gram;
        gram = nullptr;

        delete code;
        code = nullptr;

        delete actg;
        actg = nullptr;

        delete redg;
        redg = nullptr;

    }

    //
    //  If there are any errors then clean up the mess. 
    //

    catch (...)
    {

        ParserData::detach(prsd);

        delete ast;
        ast = nullptr;

        state = ParserState::GrammarBad;

        delete gram;
        delete code;
        delete actg;
        delete redg;

        throw;

    }

}

//
//  parse                                              
//  -----                                              
//                                                     
//  Use the generated parser to parse a source string. 
//

void ParserImpl::parse(const Source& src, const int64_t debug_flags)
{

    //
    //  We're about to do a state transition. Check whether we are in an 
    //  appropriate state and clear unnecessary data from the existing   
    //  state.                                                           
    //

    switch (state)
    {

        case ParserState::GrammarGood:
        {
            break;
        }

        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {

            delete ast;
            ast = nullptr;

            delete errh;
            errh = nullptr;

            break;

        }

        default:
        {
            throw logic_error("State error in Parser::parse");
        }

    }

    try
    {

        errh = new ErrorHandler(src);
        ast = nullptr;
        ParserEngine(*this, *errh, *prsd, src, ast, debug_flags).parse();
        state = ParserState::SourceGood;

    }
    catch (...)
    {

        delete ast;
        ast = nullptr;

        state = ParserState::SourceBad;

        throw;

    }

}

//
//  get_kind_map                                                     
//  ------------                                                     
//                                                                       
//  Get the current kind_map. The client can use this to find out if 
//  he forgot to define any important kind strings.                      
//

map<string, int> ParserImpl::get_kind_map() const
{

    switch (state)
    {

        case ParserState::Invalid:
        case ParserState::GrammarBad:
        {
            throw logic_error("State error in Parser::get_kind_string");
        }

    }       

    return prsd->get_kind_map();

}

//
//  get_encoded_kind_map                                                     
//  --------------------                                                     
//                                                                       
//  Get the current kind_map. The client can use this to find out if 
//  he forgot to define any important kind strings.                      
//

string ParserImpl::get_encoded_kind_map() const
{

    switch (state)
    {

        case ParserState::Invalid:
        case ParserState::GrammarBad:
        {
            throw logic_error("State error in Parser::get_kind_string");
        }

    }       

    //
    //  Encode the kind map.
    //

    ostringstream ost;

    map<string, int> kind_map = prsd->get_kind_map();
    encode_long(ost, kind_map.size());

    for (auto mp: kind_map)
    {
        encode_string(ost, mp.first);
        encode_long(ost, mp.second);
    }

    return ost.str();

}

//
//  get_ast                                        
//  -------                                        
//                                                 
//  Return the result Ast from a successful parse. 
//

Ast* ParserImpl::get_ast() const
{

    //
    //  We need a valid parse to do this. 
    //

    switch (state)
    {

        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::get_ast");
        }

    }

    return ast;

}

//
//  get_encoded_ast                                        
//  ---------------                                        
//                                                 
//  Return the result Ast from a successful parse encoded as a string.
//

string ParserImpl::get_encoded_ast() const
{

    //
    //  We need a valid parse to do this. 
    //

    switch (state)
    {

        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::get_encoded_ast");
        }

    }

    //
    //  Encode the kind map first followed by the Ast. 
    //

    ostringstream ost;

    map<string, int> kind_map = prsd->get_kind_map();
    encode_long(ost, kind_map.size());

    for (auto mp: kind_map)
    {
        encode_string(ost, mp.first);
        encode_long(ost, mp.second);
    }

    encode_ast(ost, ast);

    return ost.str();

}

//
//  dump_ast                                              
//  --------                                              
//                                                    
//  For debugging we'll provide a nice dump function. 
//

void ParserImpl::dump_ast(Ast* root, std::ostream& os, int indent) const
{

    function<void(const Ast*, int)> dump = [&](const Ast* ast, int indent) -> void
    {

        if (indent > 0)
        {
            os << std::setw(indent) << "" << std::setw(0);
        }

        if (ast == nullptr)
        {
            os << "Nullptr" << std::endl;
            return;
        }

        os << get_kind_string(ast->get_kind())
           << "(" << ast->get_kind() << ")";

        if ((ast->get_lexeme()).length() > 0)
        {
            os << " [" << Source::to_ascii_chop(ast->get_lexeme()) << "]";
        }

        if (ast->get_location() >= 0)
        {
            os << " @ " << ast->get_location();
        }

        os << std::endl;

        for (int i = 0; i < ast->get_num_children(); i++)
        {
            Ast* child = ast->get_child(i);
            dump(child, indent + 4);
        }

    };

    dump(root, indent);

}

//
//  get_kind                                                          
//  --------                                                          
//                                                                    
//  Get the integer code for a given string.
//

int ParserImpl::get_kind(const string& kind_str) const
{

    switch (state)
    {

        case ParserState::Invalid:
        case ParserState::KindMapGood:
        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;  
        }

        default:
        {
            throw logic_error("State error in Parser::get_kind");
        }

    }       

    return prsd->get_kind(kind_str);

}

//
//  get_kind_force                                                          
//  --------------                                                          
//                                                                    
//  Get the integer code for a given string.
//

int ParserImpl::get_kind_force(const string& kind_str)
{

    switch (state)
    {

        case ParserState::Invalid:
        case ParserState::KindMapGood:
        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;  
        }

        default:
        {
            throw logic_error("State error in Parser::get_kind_force");
        }

    }       

    return prsd->get_kind_force(kind_str);

}

//
//  get_kind_string                       
//  ---------------                       
//                                        
//  Get the text name for a numeric code. 
//

string ParserImpl::get_kind_string(int kind) const
{

    switch (state)
    {

        case ParserState::Invalid:
        case ParserState::GrammarBad:
        {
            throw logic_error("State error in Parser::get_kind_string");
        }

    }       

    return prsd->get_kind_string(kind);

}

//
//  get_kind_string                       
//  ---------------                       
//                                        
//  Get the text name for an Ast
//

string ParserImpl::get_kind_string(const Ast* root) const
{

    switch (state)
    {

        case ParserState::Invalid:
        case ParserState::GrammarBad:
        {
            throw logic_error("State error in Parser::get_kind_string");
        }

    }       

    if (root == nullptr)
    {
        return "Unknown";
    }

    return prsd->get_kind_string(root->get_kind());

}

//
//  add_error                                    
//  ---------                                    
//                                               
//  Create a new message and add it to the list. 
//

void ParserImpl::add_error(ErrorType error_type,
                           int64_t location,
                           const string& short_message,
                           const string& long_message)
{

    //
    //  This is only valid if we are in a state that might have error 
    //  messages.                                                     
    //

    switch (state)
    {

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::add_error");
        }

    }

    //
    //  Pass on the request to the error handler. 
    //

    if (&long_message != &string_missing)
    {
        errh->add_error(error_type, location, short_message, long_message);
    }
    else
    {
        errh->add_error(error_type, location, short_message);
    }

}

//
//  get_error_count                                          
//  ---------------                                          
//                                                           
//  Return the number of messages over the error threshhold. 
//

int ParserImpl::get_error_count() const
{

    //
    //  This is only valid if we are in a state that might have error 
    //  messages.                                                     
    //

    switch (state)
    {

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::get_error_count");
        }

    }

    //
    //  Pass on the request to the error handler. 
    //

    return errh->get_error_count();

}

//
//  get_warning_count                                         
//  -----------------                                         
//                                                            
//  Return the number of messages below the error threshhold. 
//

int ParserImpl::get_warning_count() const
{

    //
    //  This is only valid if we are in a state that might have error 
    //  messages.                                                     
    //

    switch (state)
    {

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::get_warning_count");
        }

    }

    //
    //  Pass on the request to the error handler. 
    //

    return errh->get_warning_count();

}

//
//  get_error_messages                                   
//  ------------------                                   
//                                                       
//  Return the list of error messages in location order. 
//

vector<ErrorMessage> ParserImpl::get_error_messages()
{

    //
    //  This is only valid if we are in a state that might have error 
    //  messages.                                                     
    //

    switch (state)
    {

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::get_error_messages");
        }

    }

    //
    //  Pass on the request to the error handler. 
    //

    return errh->get_error_messages();

}

//
//  get_encoded_error_messages                                   
//  --------------------------                                   
//                                                       
//  Return the list of error messages in location order as a string.
//

string ParserImpl::get_encoded_error_messages()
{

    //
    //  This is only valid if we are in a state that might have error 
    //  messages.                                                     
    //

    switch (state)
    {

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::get_error_messages");
        }

    }

    //
    //  Get the error messages from the error handler and encode them. 
    //

    ostringstream ost;

    vector<ErrorMessage> message_list = errh->get_error_messages();

    encode_long(ost, message_list.size());

    for (ErrorMessage& message: message_list)
    {
        encode_long(ost, message.get_type());
        encode_string(ost, message.get_tag());
        encode_long(ost, message.get_severity());
        encode_long(ost, message.get_location());
        encode_long(ost, message.get_line_num());
        encode_long(ost, message.get_column_num());
        encode_string(ost, message.get_source_line());
        encode_string(ost, message.get_short_message());
        encode_string(ost, message.get_long_message());
        encode_string(ost, message.get_string());
    }

    return ost.str();

}

//
//  dump_source                         
//  -----------                         
//                                      
//  List the source and error messages. 
//

void ParserImpl::dump_source(const Source& src,
                             ostream& os,
                             int indent) const
{

    //
    //  This is only valid if we are in a state that might have error 
    //  messages.                                                     
    //

    switch (state)
    {

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::get_error_messages");
        }

    }

    //
    //  Pass on the request to the error handler. 
    //

    errh->dump_source(src, os, indent);

}

//
//  get_source_list
//  ---------------                         
//                                      
//  Return the source list as a string. 
//

string ParserImpl::get_source_list(const Source& src, int indent) const
{

    //
    //  This is only valid if we are in a state that might have error 
    //  messages.                                                     
    //

    switch (state)
    {

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::get_error_messages");
        }

    }

    //
    //  Pass on the request to the error handler. 
    //

    ostringstream ost;
    errh->dump_source(src, ost, indent);

    return ost.str();

}

//
//  export_cpp                                                            
//  ----------                                                            
//                                                                        
//  Create a serialized version of the parser data in a string and export 
//  as a C++ source file.                                                 
//

void ParserImpl::export_cpp(std::string file_name, std::string identifier) const
{

    //
    //  We need a valid grammar to do this. 
    //

    switch (state)
    {

        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::export_cpp");
        }

    }

    prsd->export_cpp(file_name, identifier);

}

//
//  encode                                                                
//  ------                                                                
//                                                                        
//  Encode the generated parser as a string so that it can be reloaded in 
//  a subsequent run. Not sure this will be useful by clients, but it's   
//  essential to bootstrapping.                                           
//

string ParserImpl::encode() const
{

    //
    //  We need a valid grammar to do this. 
    //

    switch (state)
    {

        case ParserState::GrammarGood:
        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {
            break;
        }

        default:
        {
            throw logic_error("State error in Parser::encode");
        }

    }

    return prsd->encode();

}

//
//  decode                                                   
//  ------                                                   
//                                                           
//  Decode the string version of a parser into a new parser. 
//

void ParserImpl::decode(const string& str,
                        const map<string, int>& kind_map)
{

    //
    //  We're about to do a state transition. Check whether we are in an 
    //  appropriate state and clear unnecessary data from the existing   
    //  state.                                                           
    //

    switch (state)
    {

        case ParserState::Invalid:
        {
            break;
        }

        case ParserState::KindMapGood:
        {

            ParserData::detach(prsd);
            state = ParserState::Invalid;

            break;

        }

        case ParserState::GrammarBad:
        case ParserState::GrammarGood:
        {

            ParserData::detach(prsd);

            delete errh;
            errh = nullptr;

            state = ParserState::Invalid;

            break;

        }

        case ParserState::SourceBad:
        case ParserState::SourceGood:
        {

            ParserData::detach(prsd);

            delete ast;
            ast = nullptr;

            delete errh;
            errh = nullptr;

            state = ParserState::Invalid;

            break;

        }

        default:
        {
            throw logic_error("State error in Parser::decode");
        }

    }

    try
    {

        prsd = new ParserData();
        ParserData::attach(prsd);

        errh = new ErrorHandler(prsd->src);
        prsd->decode(str, kind_map);

        state = ParserState::GrammarGood;

        start_timer();

    }
    catch (...)
    {

        ParserData::detach(prsd);
        state = ParserState::GrammarBad;
        throw;

    }

}

//
//  log_heading                                  
//  -----------                                  
//                                               
//  Dump an underlined heading for log messages. 
//

void ParserImpl::log_heading(const std::string& heading, ostream& os, int indent) const
{

    os << endl
       << setw(indent) << setfill(' ') << "" << setw(0) 
       << heading << endl
       << setw(heading.size()) << setfill('-') << "" << setw(0) << setfill(' ') << endl
       << endl;

}

//
//  start_timer                                                          
//  -----------                                                          
//                                                                       
//  We keep a timer to help us find spots that may be taking longer than 
//  expected.                                                            
//

void ParserImpl::start_timer()
{
    timer = high_resolution_clock::now();
}

//
//  elapsed_time_string                               
//  -------------------                               
//                                                    
//  Return as a string the elapsed time on our timer. 
//

string ParserImpl::elapsed_time_string()
{

    high_resolution_clock::time_point stop_timer = high_resolution_clock::now();
    int64_t elapsed = duration_cast<microseconds>(stop_timer - timer).count();

    ostringstream ost;

    ost << setw(2) << setfill('0') << elapsed / 1000000 / 60 / 60 % 24 << setw(0)
        << ":"
        << setw(2) << setfill('0') << elapsed / 1000000 / 60 % 60 << setw(0)
        << ":"
        << setw(2) << setfill('0') << elapsed / 1000000 % 60 << setw(0)
        << "."
        << setw(6) << setfill('0') << elapsed % 1000000 << setw(0);

    return ost.str();

}

//
//  get_grammar_kind_string                       
//  -----------------------                       
//                                        
//  Get the text name for a numeric code. 
//

string ParserImpl::get_grammar_kind_string(int kind) const
{
    return grammar_parser_data->get_kind_string(kind);
}

//
//  dump_grammar_ast                                              
//  ----------------                                              
//                                                    
//  For debugging we'll provide a nice dump function. 
//

void ParserImpl::dump_grammar_ast(Ast* root, std::ostream& os, int indent) const
{

    function<void(const Ast*, int)> dump = [&](const Ast* ast, int indent) -> void
    {

        if (indent > 0)
        {
            os << std::setw(indent) << "" << std::setw(0);
        }

        if (ast == nullptr)
        {
            os << "Nullptr" << std::endl;
            return;
        }

        os << grammar_parser_data->get_kind_string(ast->get_kind())
           << "(" << ast->get_kind() << ")";

        if ((ast->get_lexeme()).length() > 0)
        {
            os << " [" << Source::to_ascii_chop(ast->get_lexeme()) << ']';
        }

        if (ast->get_location() >= 0)
        {
            os << " @ " << ast->get_location();
        }

        os << std::endl;

        for (int i = 0; i < ast->get_num_children(); i++)
        {
            Ast* child = ast->get_child(i);
            dump(child, indent + 4);
        }

    };

    dump(root, indent);

}

//
//  adjust_location                                                        
//  ---------------                                                        
//                                                                         
//  Apply a location adjustment to all nodes in a subtree. We use this for 
//  languages that tunnel through other languages, such as regular         
//  expressions.                                                           
//

void ParserImpl::adjust_location(Ast* ast, int64_t adjustment)
{

    if (adjustment < 0)
    {
        ast->set_location(-1);
    }
    else if (ast->get_location() >= 0)
    {
        ast->set_location(ast->get_location() + adjustment);
    }

    for (int i = 0; i < ast->get_num_children(); i++)
    {
        adjust_location(ast->get_child(i), adjustment);
    }

}

//
//  get_source_string                                                    
//  -----------------                                                    
//                                                                       
//  We have several wrappers around strings that we need to parse with a 
//  sub-parsers. This routine extracts that source.                      
//

void ParserImpl::get_source_string(Ast* ast, string& source, int64_t& adjustment)
{

    adjustment = ast->get_location();

    switch (ast->get_kind())
    {

        case AstType::AstString:
        case AstType::AstCharsetString:
        case AstType::AstMacroString:
        {

            source = ast->get_lexeme().substr(1, ast->get_lexeme().length() - 2);
            adjustment++;
            break;

        }

        case AstType::AstTripleString:
        {

            source = ast->get_lexeme().substr(3, ast->get_lexeme().length() - 6);
            adjustment += 3;
            break;

        }

        default:
        {
            dump_ast(ast);
            cout << "Invalid ast type! String expected." << endl;
            exit(1);
        }

    }

}

//
//  expand_subtrees                                                     
//  ---------------                                                     
//                                                                      
//  Expand expressions in sub languages. These are regular expressions, 
//  character sets in regular expressions and regex macros.                   
//

void ParserImpl::expand_subtrees(Ast* ast, bool& any_changes, int64_t debug_flags)
{

    function<void(Ast*, bool&, int64_t)> expand_subtrees_r =
        [&](Ast* ast, bool& any_changes, int64_t debug_flags) -> void
    {

        for (int i = 0; i < ast->get_num_children(); i++)
        {

            switch (ast->get_child(i)->get_kind())
            {

                //
                //  Regular expressions. 
                //

                case AstType::AstRegexString:
                {

                    any_changes = true;

                    string source;
                    int64_t adjustment = 0;
                    get_source_string(ast->get_child(i)->get_child(0), source, adjustment);
                     
                    ErrorHandler child_errh(source);
                    Ast* child_ast = nullptr;
        
                    try {

                        ParserEngine(*this,
                                     child_errh,
                                     *regex_parser_data,
                                     source,
                                     child_ast,
                                     debug_flags)
                            .parse();

                    }
                    catch (SourceError e)
                    {

                       for (auto msg: child_errh.get_error_messages())
                       {

                            errh->add_error(msg.get_type(),
                                            (msg.get_location() < 0 || adjustment < 0) ?
                                                msg.get_location() : msg.get_location() + adjustment,
                                            msg.get_short_message(),
                                            msg.get_long_message());

                        }

                        child_ast = new Ast(0);
                        child_ast->set_kind(-1);

                    }
                                        
                    adjust_location(child_ast, adjustment);
                    delete ast->get_child(i);
                    ast->set_child(i, child_ast);

                    break;

                }

                //
                //  Character sets within regular expressions.
                //

                case AstType::AstCharsetString:
                {

                    any_changes = true;

                    string source;
                    int64_t adjustment = 0;
                    get_source_string(ast->get_child(i), source, adjustment);
                     
                    ErrorHandler child_errh(source);
                    Ast* child_ast = nullptr;
        
                    try {

                        ParserEngine(*this,
                                     child_errh,
                                     *charset_parser_data,
                                     source,
                                     child_ast,
                                     debug_flags)
                            .parse();

                    }
                    catch (SourceError e)
                    {

                       for (auto msg: child_errh.get_error_messages())
                       {

                            errh->add_error(msg.get_type(),
                                            (msg.get_location() < 0 || adjustment < 0) ?
                                                msg.get_location() : msg.get_location() + adjustment,
                                            msg.get_short_message(),
                                            msg.get_long_message());

                        }

                        child_ast = new Ast(0);
                        child_ast->set_kind(-1);

                    }
                                        
                    adjust_location(child_ast, adjustment);
                    delete ast->get_child(i);
                    ast->set_child(i, child_ast);

                    break;

                }

                //
                //  Macros in regular expressions. 
                //

                case AstType::AstMacroString:
                {

                    any_changes = true;

                    string key;
                    int64_t adjustment = 0;
                    get_source_string(ast->get_child(i), key, adjustment);

                    LibraryToken* token = LibraryToken::get_library_token(key);
                    if (token == nullptr || token->regex_string.length() == 0)
                    {

                        ostringstream ost;
                        ost << "Unknown regex macro: " << key << ".";

                        errh->add_error(ErrorType::ErrorUnknownMacro,
                                        ast->get_child(i)->get_location(),
                                        ost.str());

                        throw GrammarError("Grammar errors");
                        
                    }

                    ErrorHandler child_errh(token->regex_string);
                    Ast* child_ast = nullptr;
        
                    try {

                        ParserEngine(*this,
                                     child_errh,
                                     *regex_parser_data,
                                     token->regex_string,
                                     child_ast,
                                     debug_flags)
                            .parse();

                    }
                    catch (SourceError e)
                    {

                       for (auto msg: child_errh.get_error_messages())
                       {

                            errh->add_error(msg.get_type(),
                                            (msg.get_location() < 0 || adjustment < 0) ?
                                                msg.get_location() : msg.get_location() + adjustment,
                                            msg.get_short_message(),
                                            msg.get_long_message());

                        }

                        child_ast = new Ast(0);
                        child_ast->set_kind(-1);

                    }
                                        
                    adjust_location(child_ast, adjustment);
                    delete ast->get_child(i);
                    ast->set_child(i, child_ast);

                    break;

                }

                //
                //  If we don't need to expand the subtree, keep examining 
                //  nodes.                                                 
                //

                default:
                {
                    expand_subtrees_r(ast->get_child(i), any_changes, debug_flags);
                    break;
                }

            }

        }

    };

    expand_subtrees_r(ast, any_changes, debug_flags);

    if (errh->get_error_count() > 0)
    {
        throw GrammarError("Grammar errors");
    }

    return;

}

//
//  parse_library_regex                                                   
//  -------------------                                                   
//                                                                        
//  We need this small utility to handle library tokens. We want to enter 
//  the regexs as strings and this turns them into Asts.                  
//

Ast* ParserImpl::parse_library_regex(const Source& src,
                                     const int64_t debug_flags)   
{

    ErrorHandler temp_errh(src);
    ErrorHandler *save_errh = errh;

    try
    {

        errh = &temp_errh;
        Ast* ast;

        ParserEngine(*this, *errh, *regex_parser_data, src, ast, debug_flags).parse();

        bool any_changes = true;
        while (any_changes)
        {
            any_changes = false;
            expand_subtrees(ast, any_changes, debug_flags);
        }

        errh = save_errh;

        return ast;

    }
    catch (...)
    {
        cerr << "Failed to parse library regex: " << endl;
        exit(1);
    }

}

//
//  Primitive String Encoders and Decoders                            
//  --------------------------------------                            
//                                                                    
//  Encode or decode small types as strings. We use these to marshall 
//  aggregates to pass back and forth to client languages.            
//

void ParserImpl::encode_long(ostream& os, int64_t value)
{
    os << value << "|";    
}

int64_t ParserImpl::decode_long(istream& is)
{

    ostringstream ost;

    for (;;) 
    {

        char c;
        is >> c;
 
        if (c == '`')
        {
            is >> c;
        }
        else if (c == '|')
        {
            break;
        }

        ost << c;

    }
        
    return stoll(ost.str());

}

void ParserImpl::encode_string(ostream& os, const string& value)
{

    for (char c: value)
    {

        if (c == '`' || c == '|')
        {
            os << '`';
        }
     
        os << c;

    }

    os << "|";    

}

string ParserImpl::decode_string(istream& is)
{

    ostringstream ost;

    for (;;) 
    {

        char c;
        is >> c;
 
        if (c == '`')
        {
            is >> c;
        }
        else if (c == '|')
        {
            break;
        }

        ost << c;

    }
        
    return ost.str();

}

void ParserImpl::encode_ast(ostream& os, const Ast* ast)
{

    if (ast == nullptr)
    {
        encode_long(os, -1);
        return;
    }

    encode_long(os, ast->get_num_children());
    encode_long(os, ast->get_kind());
    encode_long(os, ast->get_location());
    encode_string(os, ast->get_lexeme());

    for (int i = 0; i < ast->get_num_children(); i++)
    {
        encode_ast(os, ast->get_child(i));
    }

}

} // namespace hoshi

