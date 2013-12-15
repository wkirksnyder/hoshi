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
    "y'|G\"|'error'|H\"|'error_recovery'|I\"|'false'|J\"|'ignore'|K\"|'keep_"
    "whitespace'|L\"|'lexeme'|M\"|'lookaheads'|N\"|'options'|O\"|'precedence"
    "'|P\"|'regex'|Q\"|'rules'|R\"|'template'|S\"|'token_count'|T\"|'tokens'"
    "|U\"|'true'|V\"|'{','|W\"|*eof*|X\"|*epsilon*|Y\"|*error*|Z\"|<brackets"
    "tring>|[\"|<comment>|\\\"|<identifier>|]\"|<integer>|^\"|<string>|_\"|<"
    "stringerror>| #|<triplestring>|!#|<triplestringerror>|\"#|<whitespace>|"
    "##|AstActionAdd|U!|AstActionAnd|Z!|AstActionAssign|N!|AstActionDivide|X"
    "!|AstActionDumpStack|]!|AstActionEqual|O!|AstActionGreaterEqual|T!|AstA"
    "ctionGreaterThan|S!|AstActionLessEqual|R!|AstActionLessThan|Q!|AstActio"
    "nMultiply|W!|AstActionNot|\\!|AstActionNotEqual|P!|AstActionOr|[!|AstAc"
    "tionStatementList|M!|AstActionSubtract|V!|AstActionTokenCount|^!|AstAct"
    "ionUnaryMinus|Y!|AstAstChild|I|AstAstDot|P|AstAstFormer|G|AstAstItemLis"
    "t|H|AstAstKind|J|AstAstLexeme|M|AstAstLexemeString|N|AstAstLocation|K|A"
    "stAstLocationString|L|AstAstLocator|O|AstAstSlice|Q|AstCaseSensitive|*|"
    "AstCharset|<!|AstCharsetAltNewline|E!|AstCharsetCaret|H!|AstCharsetChar"
    "|?!|AstCharsetCr|G!|AstCharsetDash|I!|AstCharsetDigits|B!|AstCharsetDol"
    "lar|J!|AstCharsetEscape|D!|AstCharsetInvert|=!|AstCharsetLeftBracket|K!"
    "|AstCharsetNewline|F!|AstCharsetNotDigits|C!|AstCharsetNotWhitespace|A!"
    "|AstCharsetRange|>!|AstCharsetRightBracket|L!|AstCharsetString|V|AstCha"
    "rsetWhitespace|@!|AstConflicts|(|AstEmpty|F|AstErrorRecovery|'|AstFalse"
    "|^|AstFormer:2|\"\"|AstGrammar|\"|AstGroup|<|AstIdentifier|X|AstInteger"
    "|Y|AstKeepWhitespace|)|AstLookaheads|&|AstMacroString|W|AstNegativeInte"
    "ger|Z|AstNonterminalReference|E|AstNull|!|AstOneClosure|;|AstOptionList"
    "|#|AstOptional|9|AstOptions|S|AstReduceActions|T|AstRegex|_|AstRegexAlt"
    "Newline|,!|AstRegexChar|%!|AstRegexCr|.!|AstRegexDigits|)!|AstRegexDoll"
    "ar|4!|AstRegexEscape|+!|AstRegexLeftBrace|:!|AstRegexLeftBracket|8!|Ast"
    "RegexLeftParen|6!|AstRegexList|!!|AstRegexNewline|-!|AstRegexNotDigits|"
    "*!|AstRegexNotWhitespace|(!|AstRegexOneClosure|$!|AstRegexOptional|\"!|"
    "AstRegexOr| !|AstRegexPeriod|3!|AstRegexPlus|1!|AstRegexQuestion|2!|Ast"
    "RegexRightBrace|;!|AstRegexRightBracket|9!|AstRegexRightParen|7!|AstReg"
    "exSpace|5!|AstRegexStar|0!|AstRegexString|U|AstRegexVBar|/!|AstRegexWhi"
    "tespace|'!|AstRegexWildcard|&!|AstRegexZeroClosure|#!|AstRule|6|AstRule"
    "LeftAssoc|@|AstRuleList|%|AstRuleOperatorList|B|AstRuleOperatorSpec|C|A"
    "stRulePrecedence|=|AstRulePrecedenceList|>|AstRulePrecedenceSpec|?|AstR"
    "uleRhs|8|AstRuleRhsList|7|AstRuleRightAssoc|A|AstString|[|AstTerminalRe"
    "ference|D|AstToken|R|AstTokenAction|2|AstTokenDeclaration|+|AstTokenDes"
    "cription|.|AstTokenError|5|AstTokenIgnore|4|AstTokenLexeme|3|AstTokenLi"
    "st|$|AstTokenOptionList|,|AstTokenPrecedence|1|AstTokenRegex|0|AstToken"
    "RegexList|/|AstTokenTemplate|-|AstTripleString|\\|AstTrue|]|AstUnknown|"
    "|AstZeroClosure|:|Null| \"|ReduceActions|!\"|Unknown|_!|}\"|//{ -{ *// "
    " Main Grammar{ -{ *//  ------------{ -{ *//{ -{ *//  Grammar file for t"
    "he main part of the grammar.{ -{ *//{ -{ *{ -{ *options{ -{ *{ -{ *    "
    "lookaheads = 4{ -{ *    error_recovery = true{ -{ *    conflicts = 0{ -"
    "{ *    case_sensitive = true{ -{ *{ -{ *tokens{ -{ *{ -{ *    <comment>"
    "            : regex = ''' {'+cpp_comment{'- '''{ -{ *                  "
    "         ignore = true{ -{ *{ -{ *    <whitespace>         : regex = ''"
    "' {'+whitespace{'- '''{ -{ *                           ignore = true{ -"
    "{ *{ -{ *    <integer>            : regex = ''' [0-9]+ '''{ -{ *{ -{ * "
    "   <identifier>         : regex = ''' [A-Za-z][a-zA-Z0-9_]* '''{ -{ *  "
    "                         precedence = 50{ -{ *{ -{ *    <bracketstring>"
    "      : regex = [ !in_guard ] => ''' <[a-zA-Z][a-zA-Z0-9_]*> '''{ -{ *{"
    " -{ *    <string>             : regex = ''' ' ( \\\\ [^\\n] {', [^'\\\\"
    "\\n] )* ' {',{ -{ *                                       \" ( \\\\ [^\\"
    "n] {', [^\"\\\\\\n] )* \" '''{ -{ *{ -{ *    <stringerror>        : reg"
    "ex = ''' ' ( \\\\ [^\\n] {', [^'\\\\\\n] )* \\n {',{ -{ *              "
    "                         \" ( \\\\ [^\\n] {', [^\"\\\\\\n] )* \\n '''{ "
    "-{ *                           error = \"Missing closing quote on strin"
    "g literal\"{ -{ *{ -{ *    <triplestring>       : regex = \"''' ( [^'] "
    "{', '[^'] {', ''[^'] )* '''\"{ -{ *{ -{ *    <triplestringerror>  : reg"
    "ex = \"''' ( [^'] {', '[^'] {', ''[^'] )*\"{ -{ *                      "
    "     error = \"Missing closing quote on triple quoted string literal\"{"
    " -{ *{ -{ *    '['                  : action = [ in_guard := 1; ]{ -{ *"
    "{ -{ *    ']'                  : action = [ in_guard := 0; ]{ -{ *{ -{ "
    "*    '<'                  : regex = [ in_guard ] => '<'{ -{ *{ -{ *rule"
    "s{ -{ *{ -{ *    //{ -{ *    //  Overall Structure{ -{ *    //  -------"
    "----------{ -{ *    //{ -{ *{ -{ *    Grammar              ::= OptionSe"
    "ction?{ -{ *                             TokenSection?{ -{ *           "
    "                  RuleSection{ -{ *                         :   (AstGra"
    "mmar, @3,{ -{ *                                 (AstOptionList, @1, $1."
    "_),{ -{ *                                 (AstTokenList, @2, $2._),{ -{"
    " *                                 (AstRuleList, @3, $3._){ -{ *       "
    "                      ){ -{ *{ -{ *    //{ -{ *    //  Option Sublangua"
    "ge{ -{ *    //  ------------------{ -{ *    //{ -{ *{ -{ *    OptionSec"
    "tion        ::= 'options' OptionSpec*{ -{ *                         :  "
    " (AstOptionList, $2._){ -{ *{ -{ *    OptionSpec           ::= 'lookahe"
    "ads' '=' IntegerValue{ -{ *                         :   (AstLookaheads,"
    " $3){ -{ *{ -{ *    OptionSpec           ::= 'conflicts' '=' IntegerVal"
    "ue{ -{ *                         :   (AstConflicts, $3){ -{ *{ -{ *    "
    "OptionSpec           ::= 'error_recovery' '=' BooleanValue{ -{ *       "
    "                  :   (AstErrorRecovery, $3){ -{ *{ -{ *    OptionSpec "
    "          ::= 'keep_whitespace' '=' BooleanValue{ -{ *                 "
    "        :   (AstKeepWhitespace, $3){ -{ *{ -{ *    OptionSpec          "
    " ::= 'case_sensitive' '=' BooleanValue{ -{ *                         : "
    "  (AstCaseSensitive, $3){ -{ *{ -{ *    //{ -{ *    //  Token Sublangua"
    "ge{ -{ *    //  -----------------{ -{ *    //{ -{ *{ -{ *    TokenSecti"
    "on         ::= 'tokens' TokenSpec*{ -{ *                         :   (A"
    "stTokenList, $2._){ -{ *{ -{ *    TokenSpec            ::= TerminalSymb"
    "ol{ -{ *                         :   (AstTokenDeclaration, $1, (AstToke"
    "nOptionList)){ -{ *{ -{ *    TokenSpec            ::= TerminalSymbol ':"
    "' TokenOption*{ -{ *                         :   (AstTokenDeclaration, "
    "$1, (AstTokenOptionList, @3, $3._)){ -{ *{ -{ *    TokenOption         "
    " ::= 'template' '=' TerminalSymbol{ -{ *                         :   (A"
    "stTokenTemplate, $3){ -{ *{ -{ *    TokenOption          ::= 'descripti"
    "on' '=' StringValue{ -{ *                         :   (AstTokenDescript"
    "ion, $3){ -{ *{ -{ *    TokenOption          ::= 'regex' '=' RegexStrin"
    "g{ -{ *                         :   (AstTokenRegexList, (AstTokenRegex,"
    " (AstNull, @\"-1\"), $3)){ -{ *{ -{ *    TokenOption          ::= 'rege"
    "x' '=' TokenRegex+{ -{ *                         :   (AstTokenRegexList"
    ", $3._){ -{ *{ -{ *    TokenRegex           ::= '[' ActionExpression ']"
    "' '=>' RegexString{ -{ *                         :   (AstTokenRegex, $2"
    ", $5){ -{ *{ -{ *    RegexString          ::= StringValue{ -{ *        "
    "                 :   (AstRegexString, $1){ -{ *{ -{ *    TokenOption   "
    "       ::= 'precedence' '=' IntegerValue{ -{ *                         "
    ":   (AstTokenPrecedence, $3){ -{ *{ -{ *    TokenOption          ::= 'a"
    "ction' '=' '[' ActionStatementList ']'{ -{ *                         : "
    "  (AstTokenAction, $4){ -{ *{ -{ *    TokenOption          ::= 'lexeme'"
    " '=' BooleanValue{ -{ *                         :   (AstTokenLexeme, $3"
    "){ -{ *{ -{ *    TokenOption          ::= 'ignore' '=' BooleanValue{ -{"
    " *                         :   (AstTokenIgnore, $3){ -{ *{ -{ *    Toke"
    "nOption          ::= 'error' '=' StringValue{ -{ *                     "
    "    :   (AstTokenError, $3){ -{ *{ -{ *    //{ -{ *    //  Rule Sublang"
    "uage{ -{ *    //  ----------------{ -{ *    //{ -{ *{ -{ *    RuleSecti"
    "on          ::= 'rules'? Rule+{ -{ *                         :   (AstRu"
    "leList, $2._){ -{ *{ -{ *    Rule                 ::= NonterminalRefere"
    "nce '::=' RuleRhsList ReduceActions{ -{ *                         :   ("
    "AstRule, @2, $1, $3, $4.1, $4.2){ -{ *{ -{ *    ReduceActions        ::"
    "= ':' ReduceAstAction ':' ReduceGuardAction{ -{ *                      "
    "   :   ($2, $4){ -{ *{ -{ *    ReduceActions        ::= ':' ReduceAstAc"
    "tion{ -{ *                         :   ($2, (AstNull, @\"-1\")){ -{ *{ "
    "-{ *    ReduceActions        ::= empty{ -{ *                         : "
    "  ((AstNull, @\"-1\"), (AstNull, @\"-1\")){ -{ *{ -{ *    RuleRhsList  "
    "        ::= RuleRhs ( '{',' RuleRhs : $2 )*{ -{ *                      "
    "   :   (AstRuleRhsList, $1, $2._){ -{ *{ -{ *    RuleRhs              :"
    ":= RuleSequenceTerm+{ -{ *                         :   (AstRuleRhs, $1."
    "_){ -{ *{ -{ *    RuleSequenceTerm     ::= RuleUnopTerm '*'{ -{ *      "
    "                   :   (AstZeroClosure, @2, $1){ -{ *{ -{ *    RuleSequ"
    "enceTerm     ::= RuleUnopTerm '+'{ -{ *                         :   (As"
    "tOneClosure, @2, $1){ -{ *{ -{ *    RuleSequenceTerm     ::= RuleUnopTe"
    "rm '?'{ -{ *                         :   (AstOptional, @2, $1){ -{ *{ -"
    "{ *    RuleSequenceTerm     ::= RuleUnopTerm{ -{ *{ -{ *    RuleUnopTer"
    "m         ::= SymbolReference{ -{ *{ -{ *    RuleUnopTerm         ::= '"
    "empty'{ -{ *                         :   (AstEmpty){ -{ *{ -{ *    Rule"
    "UnopTerm         ::= '(' RuleRhsList ReduceActions ')'{ -{ *           "
    "              :   (AstGroup, $2, $3.1, $3.2){ -{ *{ -{ *    Rule       "
    "          ::= NonterminalReference '::^' SymbolReference PrecedenceSpec"
    "+{ -{ *                         :   (AstRulePrecedence, @2, $1, $3, (As"
    "tRulePrecedenceList, @4, $4._)){ -{ *{ -{ *    PrecedenceSpec       ::="
    " Assoc Operator+{ -{ *                         :   (AstRulePrecedenceSp"
    "ec, $1, (AstRuleOperatorList, @2, $2._)){ -{ *{ -{ *    Assoc          "
    "      ::= '<<'{ -{ *                         :   (AstRuleLeftAssoc){ -{"
    " *{ -{ *    Assoc                ::= '>>'{ -{ *                        "
    " :   (AstRuleRightAssoc){ -{ *{ -{ *    Operator             ::= Symbol"
    "Reference ReduceActions{ -{ *                         :   (AstRuleOpera"
    "torSpec, $1, $2.1, $2.2){ -{ *{ -{ *    SymbolReference      ::= Termin"
    "alReference{ -{ *{ -{ *    SymbolReference      ::= NonterminalReferenc"
    "e{ -{ *{ -{ *    TerminalReference    ::= TerminalSymbol{ -{ *         "
    "                :   (AstTerminalReference, $1){ -{ *{ -{ *    Nontermin"
    "alReference ::= NonterminalSymbol{ -{ *                         :   (As"
    "tNonterminalReference, $1){ -{ *{ -{ *    //{ -{ *    //  Ast Former Su"
    "blanguage{ -{ *    //  ----------------------{ -{ *    //{ -{ *{ -{ *  "
    "  ReduceAstAction      ::= AstFormer{ -{ *{ -{ *    ReduceAstAction    "
    "  ::= AstChild{ -{ *{ -{ *    ReduceAstAction      ::= empty{ -{ *     "
    "                    :   (AstNull){ -{ *{ -{ *    AstFormer            :"
    ":= '(' ( AstItem ( ','? AstItem : $2 )* : ($1, $2._) )? ')'{ -{ *      "
    "                   :   (AstAstFormer, $2._){ -{ *{ -{ *    AstItem     "
    "         ::= AstChild{ -{ *{ -{ *    AstChild             ::= '$' AstCh"
    "ildSpec '.' AstSliceSpec{ -{ *                         :   (AstAstChild"
    ", $2, $4){ -{ *{ -{ *    AstChild             ::= '$' AstChildSpec{ -{ "
    "*                         :   (AstAstChild, $2, (AstNull, @\"-1\")){ -{"
    " *{ -{ *    AstChild             ::= '$' AstSliceSpec{ -{ *            "
    "             :   (AstAstChild, (AstNull, @\"-1\"), $2){ -{ *{ -{ *    A"
    "stItem              ::= Identifier{ -{ *                         :   (A"
    "stIdentifier, &1){ -{ *{ -{ *    AstItem              ::= '%' AstChildS"
    "pec{ -{ *                         :   (AstAstKind, $2){ -{ *{ -{ *    A"
    "stItem              ::= '@' AstChildSpec{ -{ *                         "
    ":   (AstAstLocation, $2){ -{ *{ -{ *    AstItem              ::= '@' St"
    "ringValue{ -{ *                         :   (AstAstLocationString, $2){"
    " -{ *{ -{ *    AstItem              ::= '&' AstChildSpec{ -{ *         "
    "                :   (AstAstLexeme, $2){ -{ *{ -{ *    AstItem          "
    "    ::= '&' StringValue{ -{ *                         :   (AstAstLexeme"
    "String, $2){ -{ *{ -{ *    AstItem              ::= AstFormer{ -{ *{ -{"
    " *    AstChildSpec         ::= AstChildNumber ( '.' AstChildNumber : $2"
    " )*{ -{ *                         :   (AstAstDot, $1, $2._){ -{ *{ -{ *"
    "    AstSliceSpec         ::= AstFirstChildNumber '_' AstLastChildNumber"
    "{ -{ *                         :   (AstAstSlice, $1, $3){ -{ *{ -{ *   "
    " AstFirstChildNumber  ::= AstChildNumber{ -{ *{ -{ *    AstFirstChildNu"
    "mber  ::= empty{ -{ *                         :   (AstInteger, &\"1\"){"
    " -{ *{ -{ *    AstLastChildNumber   ::= AstChildNumber{ -{ *{ -{ *    A"
    "stLastChildNumber   ::= empty{ -{ *                         :   (AstNeg"
    "ativeInteger, &\"1\"){ -{ *{ -{ *    AstChildNumber       ::= '-' <inte"
    "ger>{ -{ *                         :   (AstNegativeInteger, &2){ -{ *{ "
    "-{ *    AstChildNumber       ::= <integer>{ -{ *                       "
    "  :   (AstInteger, &1){ -{ *{ -{ *    ReduceGuardAction    ::= '[' Acti"
    "onStatementList ']'{ -{ *                         :   $2{ -{ *{ -{ *   "
    " //{ -{ *    //  Action Sublanguage{ -{ *    //  ------------------{ -{"
    " *    //{ -{ *{ -{ *    ActionStatementList  ::= ActionStatement*{ -{ *"
    "                         :   (AstActionStatementList, $1._){ -{ *{ -{ *"
    "    ActionStatement      ::= Identifier ':=' ActionExpression ';'{ -{ *"
    "                         :   (AstActionAssign, @2, $1, $3){ -{ *{ -{ * "
    "   ActionStatement      ::= 'dump_stack' ';'{ -{ *                     "
    "    :   (AstActionDumpStack){ -{ *{ -{ *    ActionExpression     ::^ Ac"
    "tionUnopExpression << '{','  : (AstActionOr, $1, $2){ -{ *             "
    "                                     << '&'  : (AstActionAnd, $1, $2){ "
    "-{ *                                                  << '='  : (AstAct"
    "ionEqual, $1, $2){ -{ *                                                "
    "     '/=' : (AstActionNotEqual, $1, $2){ -{ *                          "
    "                           '<'  : (AstActionLessThan, $1, $2){ -{ *    "
    "                                                 '<=' : (AstActionLessE"
    "qual, $1, $2){ -{ *                                                    "
    " '>'  : (AstActionGreaterThan, $1, $2){ -{ *                           "
    "                          '>=' : (AstActionGreaterEqual, $1, $2){ -{ * "
    "                                                 << '+'  : (AstActionAd"
    "d, $1, $2){ -{ *                                                     '-"
    "'  : (AstActionSubtract, $1, $2){ -{ *                                 "
    "                 << '*'  : (AstActionMultiply, $1, $2){ -{ *           "
    "                                          '/'  : (AstActionDivide, $1, "
    "$2){ -{ *{ -{ *    ActionUnopExpression ::= '-' ActionUnopTerm{ -{ *   "
    "                      :   (AstActionUnaryMinus, $2){ -{ *{ -{ *    Acti"
    "onUnopExpression ::= '!' ActionUnopTerm{ -{ *                         :"
    "   (AstActionNot, $2){ -{ *{ -{ *    ActionUnopExpression ::= ActionUno"
    "pTerm{ -{ *{ -{ *    ActionUnopTerm       ::= '(' ActionExpression ')'{"
    " -{ *                         :   $2{ -{ *{ -{ *    ActionUnopTerm     "
    "  ::= IntegerValue{ -{ *{ -{ *    ActionUnopTerm       ::= Identifier{ "
    "-{ *{ -{ *    ActionUnopTerm       ::= 'token_count'{ -{ *             "
    "            :   (AstActionTokenCount){ -{ *{ -{ *    //{ -{ *    //  Li"
    "teral Wrappers{ -{ *    //  ----------------{ -{ *    //{ -{ *{ -{ *   "
    " TerminalSymbol       ::= <string>{ -{ *                         :   (A"
    "stString, &1){ -{ *{ -{ *    TerminalSymbol       ::= <bracketstring>{ "
    "-{ *                         :   (AstString, &1){ -{ *{ -{ *    Nonterm"
    "inalSymbol    ::= <identifier>{ -{ *                         :   (AstId"
    "entifier, &1){ -{ *{ -{ *    IntegerValue         ::= <integer>{ -{ *  "
    "                       :   (AstInteger, &1){ -{ *{ -{ *    BooleanValue"
    "         ::= 'true'{ -{ *                         :   (AstTrue){ -{ *{ "
    "-{ *    BooleanValue         ::= 'false'{ -{ *                         "
    ":   (AstFalse){ -{ *{ -{ *    StringValue          ::= <string>{ -{ *  "
    "                       :   (AstString, &1){ -{ *{ -{ *    StringValue  "
    "        ::= <triplestring>{ -{ *                         :   (AstTriple"
    "String, &1){ -{ *{ -{ *    Identifier           ::= <identifier>{ -{ * "
    "                        :   (AstIdentifier, &1){ -{ *|}#|$|}$|!|}%| \"|"
    "}&|%|}'|%\"|}(|<identifier>|')'|<string>|<bracketstring>|'('|*eof*|'&'|"
    "'{','|':'|'-'|'>>'|'<<'|'$'|']'|'rules'|'%'|'@'|'+'|'='|','|'*'|';'|<in"
    "teger>|'<'|'>'|'/='|'>='|'<='|'template'|'action'|'regex'|'error'|'desc"
    "ription'|'precedence'|'lexeme'|'ignore'|'empty'|'/'|'token_count'|'!'|'"
    "tokens'|'?'|'['|'case_sensitive'|'conflicts'|'error_recovery'|'_'|'keep"
    "_whitespace'|'lookaheads'|'.'|'dump_stack'|<triplestring>|'::^'|'::='|'"
    "true'|'false'|':='|'=>'|'options'||||||||||||||||||||||||||||||||||||||"
    "||||||||||||||||||||||||||||||<stringerror>|<triplestringerror>|*error*"
    "||*epsilon*|<comment>|<whitespace>|})|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!"
    "|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|"
    "!|!|!|!|!|!|!||||||?!||||||||P!|?\"|O!||||||A!||||||||A!||||||||@|3!|:!"
    "||||||B!||||||||Z!||||||||P!|?\"|O!||||||!||!|||}*|]\"|(\"|_\"|[\"|'\"|"
    "X\"|&\"|W\"|0\"|,\"|<\"|6\"|$\"|@\"|R\"|%\"|>\"|*\"|8\"|+\"|)\"|4\"|^\""
    "|5\"|:\"|/\"|;\"|7\"|S\"|B\"|Q\"|H\"|E\"|P\"|M\"|K\"|G\"|.\"|T\"|#\"|U\""
    "|=\"|?\"|C\"|D\"|I\"|A\"|L\"|N\"|-\"|F\"|!#|2\"|1\"|V\"|J\"|3\"|9\"|O\""
    "||A!||H!||P]Y;||I!||I!||PP\\;||J!||Z!||P]Y;||- 0 ,|1%T (|(||P||Y||P]Y;|"
    "|!!||:!||P]Y;||?!||?!||P]Y;||A!||D!||P]Y;||E!||E!||PR\\;||F!||T!||P]Y;|"
    "|U!||U!||P^\\;||V!|| #|\"#|Z\"||Y\"|\\\"|##|}+|!||!|!||||||||||||||||||"
    "|!|!||||||||||||||||||||||||||||!|||||||||||||?!||||||||P!|?\"|O!||||||"
    "A!||||||||A!||||||||P|]#|O!||||||B!||||||||Z!||||||||P!|?\"|O!||||!|!||"
    "||!|!|},|/\"|}-|!|#|!||!||\"|\"||#|#|#|#|#|\"|\"||!|#|\"||#|#|#|#|\"|!|"
    "%|!|#|%|#|#|#|\"|!||\"|!|$|$|\"||\"|\"|\"||!|\"|!|\"|\"|\"|!|!|!|$|$|\""
    "|!|\"|\"|!|!|!|\"|!|!|!|!|!|!||#|!|\"|\"|\"|!||||!|$|\"|\"|!|\"|\"|\"|\""
    "|\"|!|\"|\"|\"||#|!||!||\"|!|#|!|\"||$|\"|#|!|#|!|#|#|#|#|#|#|!|#|#|!|#"
    "|#|!|\"|\"|!|#|!|!|!|!|!|!|!|!|!|!|!|!|}.|!\"|P!|U!|U!|V!|V!|K!|Y!|Y!|Z"
    "!|Z!|Z!|Z!|Z!|[!|\\!|\\!|]!|]!|?!|?!|T!|T!|T!|T!|I!|I!|C!|9!|T!|T!|T!|T"
    "!|T!|Q!|R!|R!|S!|S!|D!|2!|2!|2!|3!|G!|X!|G!|/!|0!|0!|+!|+!|+!|+!|,!|,!|"
    ",!|D!|F!|F!|@!|J!|J!|A!|A!|H!|%!|%!|&!|\"!|7!|7!|7!|*!|;!|<!|E!|M!|N!|N"
    "!|E!|;!|5!|)!|)!|)!|5!|5!|5!|5!|5!|5!|5!|4!|B!|L!|B!|>!|:!|:!|O!|O!|(!|"
    "(!|=!|6!|8!|8!|W!|W!|.!|.!|'!|'!|$!|$!|$!|$!|$!|$!|$!| !| !| !|_|_|_|^|"
    "^|^|]|]|]|]|!!|!!|#!|\\|1!|1!|-!|-!|[|}/|*accept* ::= Grammar|Grammar :"
    ":= Grammar:1 Grammar:2 RuleSection|Grammar:1 ::= OptionSection|Grammar:"
    "1 ::= *epsilon*|Grammar:2 ::= TokenSection|Grammar:2 ::= *epsilon*|Opti"
    "onSection ::= 'options' OptionSection:1|OptionSection:1 ::= OptionSecti"
    "on:1 OptionSpec|OptionSection:1 ::= *epsilon*|OptionSpec ::= 'lookahead"
    "s' '=' IntegerValue|OptionSpec ::= 'conflicts' '=' IntegerValue|OptionS"
    "pec ::= 'error_recovery' '=' BooleanValue|OptionSpec ::= 'keep_whitespa"
    "ce' '=' BooleanValue|OptionSpec ::= 'case_sensitive' '=' BooleanValue|T"
    "okenSection ::= 'tokens' TokenSection:1|TokenSection:1 ::= TokenSection"
    ":1 TokenSpec|TokenSection:1 ::= *epsilon*|TokenSpec ::= TerminalSymbol|"
    "TokenSpec ::= TerminalSymbol ':' TokenSpec:1|TokenSpec:1 ::= TokenSpec:"
    "1 TokenOption|TokenSpec:1 ::= *epsilon*|TokenOption ::= 'template' '=' "
    "TerminalSymbol|TokenOption ::= 'description' '=' StringValue|TokenOptio"
    "n ::= 'regex' '=' RegexString|TokenOption ::= 'regex' '=' TokenOption:1"
    "|TokenOption:1 ::= TokenOption:1 TokenRegex|TokenOption:1 ::= TokenRege"
    "x|TokenRegex ::= '[' ActionExpression ']' '=>' RegexString|RegexString "
    "::= StringValue|TokenOption ::= 'precedence' '=' IntegerValue|TokenOpti"
    "on ::= 'action' '=' '[' ActionStatementList ']'|TokenOption ::= 'lexeme"
    "' '=' BooleanValue|TokenOption ::= 'ignore' '=' BooleanValue|TokenOptio"
    "n ::= 'error' '=' StringValue|RuleSection ::= RuleSection:1 RuleSection"
    ":2|RuleSection:1 ::= 'rules'|RuleSection:1 ::= *epsilon*|RuleSection:2 "
    "::= RuleSection:2 Rule|RuleSection:2 ::= Rule|Rule ::= NonterminalRefer"
    "ence '::=' RuleRhsList ReduceActions|ReduceActions ::= ':' ReduceAstAct"
    "ion ':' ReduceGuardAction|ReduceActions ::= ':' ReduceAstAction|ReduceA"
    "ctions ::= *epsilon*|RuleRhsList ::= RuleRhs RuleRhsList:1|RuleRhsList:"
    "1 ::= RuleRhsList:1 RuleRhsList:2|RuleRhsList:2 ::= '{',' RuleRhs|RuleR"
    "hsList:1 ::= *epsilon*|RuleRhs ::= RuleRhs:1|RuleRhs:1 ::= RuleRhs:1 Ru"
    "leSequenceTerm|RuleRhs:1 ::= RuleSequenceTerm|RuleSequenceTerm ::= Rule"
    "UnopTerm '*'|RuleSequenceTerm ::= RuleUnopTerm '+'|RuleSequenceTerm ::="
    " RuleUnopTerm '?'|RuleSequenceTerm ::= RuleUnopTerm|RuleUnopTerm ::= Sy"
    "mbolReference|RuleUnopTerm ::= 'empty'|RuleUnopTerm ::= '(' RuleRhsList"
    " ReduceActions ')'|Rule ::= NonterminalReference '::^' SymbolReference "
    "Rule:1|Rule:1 ::= Rule:1 PrecedenceSpec|Rule:1 ::= PrecedenceSpec|Prece"
    "denceSpec ::= Assoc PrecedenceSpec:1|PrecedenceSpec:1 ::= PrecedenceSpe"
    "c:1 Operator|PrecedenceSpec:1 ::= Operator|Assoc ::= '<<'|Assoc ::= '>>"
    "'|Operator ::= SymbolReference ReduceActions|SymbolReference ::= Termin"
    "alReference|SymbolReference ::= NonterminalReference|TerminalReference "
    "::= TerminalSymbol|NonterminalReference ::= NonterminalSymbol|ReduceAst"
    "Action ::= AstFormer|ReduceAstAction ::= AstChild|ReduceAstAction ::= *"
    "epsilon*|AstFormer ::= '(' AstFormer:1 ')'|AstFormer:1 ::= AstFormer:2|"
    "AstFormer:2 ::= AstItem AstFormer:3|AstFormer:3 ::= AstFormer:3 AstForm"
    "er:4|AstFormer:4 ::= AstFormer:5 AstItem|AstFormer:5 ::= ','|AstFormer:"
    "5 ::= *epsilon*|AstFormer:3 ::= *epsilon*|AstFormer:1 ::= *epsilon*|Ast"
    "Item ::= AstChild|AstChild ::= '$' AstChildSpec '.' AstSliceSpec|AstChi"
    "ld ::= '$' AstChildSpec|AstChild ::= '$' AstSliceSpec|AstItem ::= Ident"
    "ifier|AstItem ::= '%' AstChildSpec|AstItem ::= '@' AstChildSpec|AstItem"
    " ::= '@' StringValue|AstItem ::= '&' AstChildSpec|AstItem ::= '&' Strin"
    "gValue|AstItem ::= AstFormer|AstChildSpec ::= AstChildNumber AstChildSp"
    "ec:1|AstChildSpec:1 ::= AstChildSpec:1 AstChildSpec:2|AstChildSpec:2 ::"
    "= '.' AstChildNumber|AstChildSpec:1 ::= *epsilon*|AstSliceSpec ::= AstF"
    "irstChildNumber '_' AstLastChildNumber|AstFirstChildNumber ::= AstChild"
    "Number|AstFirstChildNumber ::= *epsilon*|AstLastChildNumber ::= AstChil"
    "dNumber|AstLastChildNumber ::= *epsilon*|AstChildNumber ::= '-' <intege"
    "r>|AstChildNumber ::= <integer>|ReduceGuardAction ::= '[' ActionStateme"
    "ntList ']'|ActionStatementList ::= ActionStatementList:1|ActionStatemen"
    "tList:1 ::= ActionStatementList:1 ActionStatement|ActionStatementList:1"
    " ::= *epsilon*|ActionStatement ::= Identifier ':=' ActionExpression ';'"
    "|ActionStatement ::= 'dump_stack' ';'|ActionExpression ::= ActionExpres"
    "sion '{',' ActionExpression:1|ActionExpression ::= ActionExpression:1|A"
    "ctionExpression:1 ::= ActionExpression:1 '&' ActionExpression:2|ActionE"
    "xpression:1 ::= ActionExpression:2|ActionExpression:2 ::= ActionExpress"
    "ion:2 '=' ActionExpression:3|ActionExpression:2 ::= ActionExpression:2 "
    "'/=' ActionExpression:3|ActionExpression:2 ::= ActionExpression:2 '<' A"
    "ctionExpression:3|ActionExpression:2 ::= ActionExpression:2 '<=' Action"
    "Expression:3|ActionExpression:2 ::= ActionExpression:2 '>' ActionExpres"
    "sion:3|ActionExpression:2 ::= ActionExpression:2 '>=' ActionExpression:"
    "3|ActionExpression:2 ::= ActionExpression:3|ActionExpression:3 ::= Acti"
    "onExpression:3 '+' ActionExpression:4|ActionExpression:3 ::= ActionExpr"
    "ession:3 '-' ActionExpression:4|ActionExpression:3 ::= ActionExpression"
    ":4|ActionExpression:4 ::= ActionExpression:4 '*' ActionUnopExpression|A"
    "ctionExpression:4 ::= ActionExpression:4 '/' ActionUnopExpression|Actio"
    "nExpression:4 ::= ActionUnopExpression|ActionUnopExpression ::= '-' Act"
    "ionUnopTerm|ActionUnopExpression ::= '!' ActionUnopTerm|ActionUnopExpre"
    "ssion ::= ActionUnopTerm|ActionUnopTerm ::= '(' ActionExpression ')'|Ac"
    "tionUnopTerm ::= IntegerValue|ActionUnopTerm ::= Identifier|ActionUnopT"
    "erm ::= 'token_count'|TerminalSymbol ::= <string>|TerminalSymbol ::= <b"
    "racketstring>|NonterminalSymbol ::= <identifier>|IntegerValue ::= <inte"
    "ger>|BooleanValue ::= 'true'|BooleanValue ::= 'false'|StringValue ::= <"
    "string>|StringValue ::= <triplestring>|Identifier ::= <identifier>|}0|!"
    "~L&|!~)'|!~/'|5'|='|G'|M'|U'|]'|%(|-(|5(|=(|G(|M(|X(|')|1)|7)|?)|G)|V)|"
    "^)|(*|0*|:*|B*|J*|R*|Z*|\"+|*+|!~2+|8+|B+|J+|\\+|&,|2,|@,|J,|T,|Y,|_,|'"
    "-|1-|9-|C-|M-|!~!~W-|]-|+.|>.|H.|P.|_.|)/|1/|7/|=/|!~!~K/|S/|!~!~[/|!0|"
    "!~)0|30|=0|!~B0|H0|N0|!~T0|^0|*1|61|>1|F1|N1|V1|^1|!~&2|02|:2|?2|E2|!~O"
    "2|!~V2|]2|%3|-3|23|:3|D3|J3|V3|\\3|!~&4|!~04|:4|D4|N4|X4|\"5|!~,5|65|!~"
    "@5|J5|!~T5|\\5|!~$6|!~!~)6|/6|76|?6|G6|O6|U6|[6|#7|+7|}1|\"|}2||}3|?#|}"
    "4|>$|}5|U+|['|%7|L'|-*|](|#+|P.|8Z|>'|9#|P)|:/|[_|9'|F#|W#|Y#|:$|Y=|8*|"
    "/&|_&|M<|+.|U.|_*|S%|.&|>)|Q%|;%|K#|8)|ZF|28|)>|K%|; !|+ !|)!!|N7|76|(^"
    "|.^|+K|?J|$5|N$|6;|^]|JY|?X|)X|HG|UJ| %|#R|HR|R$|4C|U]|L]|FS|ZA|@%|G-|\\"
    "2|PF|4#|B\"|8[|;)|:)|*)|,*|))|#)|2(|*(|\\V|(S|;U|U_|H,|O3|[M|%'|A5|'^|="
    "4|L/|]$|&,|P;|O(|J!|D+|J)|X%|]!|%0|0_|A_|V%|!)|&]|Z!!|X4|T5|& !|[;|'C|,"
    "O|R9|5&|9&|<-|<V|94|B?|XU|O2|*2|TV|-?|UT|8W|?L|+U|EW|P'|FY|TX|\\X|M1|OT"
    "| L|I'|=!!|$?|,!!|4^|2 !|_^|!5|Y:|B3|A;|Q@|_#|.A|4A|RA|7C|8$|1L|$S|3F|N"
    "F|QI|+G|?T|;\"|L@|Y\"|:=|':|4.|U7|2&|E'|4I|>M|BK|B&|SK|G(|]C|]R|V<|EX|9"
    "W|VY|1Z|7Z|#\\|9[|G\\|M\\|6@|LV|5$|W'|0%|$Y|! !|HI|S)|<6|XH|#:|[Q|4Q|YP"
    "|>P|#P|<O|XE|=E| E|*H|AC|7+|>O|5%|B0|RM|/-|X$|P>|R#|D:||V+|W)|45|-'|L4|"
    "F1|F$|4$|&(|?8|\"-|G[|H*|L1|X,|U4|L8|G+|P\"|P(|I1|TC|!T|PU|0B|,&|KB|I?|"
    ")N|>9|PD|*M|2@|HN|M6|G0|<&|#=|?Z|%/|%U|6Q|Y$|;2|#1|8?|E1|>7|?/|9R|[1|^!"
    "|3*|\\3|0>|)1|C(|B#|YI|33|)T|^'|}6| $!|}7| 0 &'|!$ 0'|\"$ F'|#$ P|$$ .'"
    "|%4|&$ $'|'$ B'|($ 8'|)$ 6'|*$ ,\"|+$ *\"|,$ H\"|-$ @'|.$ 6|/$ 8#|0$ :#"
    "|1$ 4'|2$ <'|3$ 0%|4$ 2'|5$ :'|6$ D'|7$ H%|8$ P%|9$ L%|:$ R%|;$ N%|<$ 0"
    "\"|=$ 8\"|>$ 4\"|?$ >\"|@$ 2\"|A$ 6\"|B$ :\"|C$ <\"|D$ J!|E$ Z%|F$ \"%|"
    "G$  %|H$ .|I$  \"|J$ >'|K$ D|L$ >|M$ @|N$ F$|O$ B|P$ <|Q0 *'|R$ $&|S$ V"
    "#|T$ &!|U$ $!|V$ 2!|W$ 4!|X$ H&|Y$ ,&|Z$ (|[, &(|\\, ((|], V'|^, T'|_, "
    "P'| - N'|!- @(|\"- *(|#- N|$- L'|%- >(|&- N!|'- J'|(- Z'|)- X'|*- ^'|+-"
    " :(|,- F!|-- <(|.- H'|/- 6(|0- B!|1- $(|2- 0(|3- 8(|4- \\'|5-  (|6- R'|"
    "7- @\"|8- (%|9- 2(|:- F#|;- 0#|<- 2#|=- 0$|>- \"(|?- R!|@- .(|A- (\"|B-"
    " D$|C- B(|D- 4(|E- 4$|F- $\"|G- X!|H- ,(|I-  $|J- T\"|K- &|L- 8%|M- ,%|"
    "N- .%|O- <%|P- \"|Q- 2|R- 4|S- H|T- .\"|U- $|V- *|W- \"&|X- J\"|Y- 0|Z-"
    " :|[- ,|\\- 8|]- T| 0 &'|!0 0'|\"0 R|#0 P|$HS 04|%HS PW|&0 $'|'H$! L|(H"
    "S  9|)H$!PN|*HS @1|+HS 01|,HS  5|-H$!PK|.H$!P\"|/HS  ;|0HS 0;|1H$!@N|2H"
    "$!0M|3HS  J|4H$! O|5H$! V|J$ &$|7H$! M|8H$! N|9H$!@M|:H$!0N|;H$!PM|<H$!"
    " 2|=H$! 3|>H$!@2|?H$!P3|@H$!02|AH$!P2|BH$!03|CH$!@3| (@ P$|EH$!0O|\"(@ "
    "0&|#(@  &|HH$!P!|J$ .$|-$ ^%|KH$!@$|LH$!P#|MH$! $|NHS PD|OH$!0$|PH$!@#|"
    "QHS 0Y| 0 &'|!0 0'|\"0 R|#0 P|$0 B\"|%0 ^&|&0 $'|'H$! L|(0 (#|)H$!PN|*0"
    " ,\"|+0 *\"|,0 H\"|-H$!PK|.H$!P\"|/0 8#|00 :#|1H$!@N|2H$!0M|30 0%|4H$! "
    "O|5H$! V|-$ &&|7H$! M|8H$! N|9H$!@M|:H$!0N|;H$!PM|<H$! 2|=H$! 3|>H$!@2|"
    "?H$!P3|@H$!02|AH$!P2|BH$!03|CH$!@3| H? P$|EH$!0O|\"H? 0&|#H?  &|HH$!P!|"
    " $ F|=- 0$|KH$!@$|LH$!P#|MH$! $|N0 F$|OH$!0$|PH$!@#|Q0 *'| 0 &'|!0 0'|\""
    "0 R|#0 P|$(S 04|%(S PW|&0 $'|'H$! L|((S  9|)H$!PN|*(S @1|+(S 01|,(S  5|"
    "-H$!PK|.H$!P\"|/(S  ;|0(S 0;|1H$!@N|2H$!0M|3(S  J|4H$! O|5H$! V|2$ Z|7H"
    "$! M|8H$! N|9H$!@M|:H$!0N|;H$!PM|<H$! 2|=H$! 3|>H$!@2|?H$!P3|@H$!02|AH$"
    "!P2|BH$!03|CH$!@3|'$ @%|EH$!0O|6  \\&|2$ \\|HH$!P!|2$ ^|-$ >%|KH$!@$|LH"
    "$!P#|MH$! $|N(S PD|OH$!0$|PH$!@#|Q(S 0Y| $ &'|!HH 0B|\"$ R|#$ P|$$ .'|!"
    "HW  R|&$ <#|\"- L|#- N|)$ R$|&$ F%|'HW  L|,$ H\"|V$ 2!|W$ 4!|/$ 8#|0$ :"
    "#|-HW PK| 0 F|!(T @=|\"0 R|#0 P|6$ *!|%(T PW|2$  !|5HW  V| (7 P$|!(7 @="
    "|*(T @1|+(T 01| H3 P$|%(7 PW|.(/ P\"|'(7 @5|((7 @.|%H3 PW|D$ J!| 0 F|F$"
    " \"%|G$  %|1- 8!|D- J|%0 ^&|\"$ R|#$ P|)  Z&|<(/  2|=(/  3|>(/ @2|?(/ P"
    "3|@(/ 02|A(/ P2|B(/ 03|C(/ @3|*$ ,\"|+$ *\"|S- H|X$ H&|6  X&|[, N(|\\, "
    "J$|], ^$|^, T$|_, \\$| - Z$|!- :!|\"- <!|#- N|$- X$|%- H!|&- N!|'- V$|'"
    "$ @%|)- .#|*- ,#|+- D!|,- F!|!(5 @=|.- B%|/- @!|0- B!|5$ J&|NHN 0D|3- \""
    "\"|($ T!|5- 4#|5$ P&| H$!P$|!H$! R|\"H$!0&|#H$! &|;- 0#|<- 2#|&H$!PL|'H"
    "$! L| H( P$|)H$!PN|\"H( 0&|#H(  &|!$ 2$|-H$!PK|.H$!P\"|\"$ T#|($ (!|1H$"
    "!@N|2H$!0M|!- R#|4H$! O|5H$! V|.H( P\"|7H$! M|8H$! N|9H$!@M|:H$!0N|;H$!"
    "PM|<H$! 2|=H$! 3|>H$!@2|?H$!P3|@H$!02|AH$!P2|BH$!03|CH$!@3| (3 P$|EH$!0"
    "O|V$ 2!|W$ 4!|HH$!P!|%(3 PW|G- X!|KH$!@$|LH$!P#|MH$! $|-$ ,'|OH$!0$|PH$"
    "!@#| 0 F|!H!! R|\"H. 0&|#H.  &|@- &\"|A- (\"|&H!!PL|'H!! L|6$ *!|)H!!PN"
    "|F- $\"|V$ 2!|W$ 4!|-H!!PK|.0 6|S$ V#|1- 6!|1H!!@N|2H!!0M|6$ H$|4H!! O|"
    "5H!! V|2- N\"|7H!! M|8H!! N|9H!!@M|:H!!0N|;H!!PM|<H.  2|=H.  3|>H. @2|?"
    "H. P3|@H. 02|AH. P2|BH. 03|CH. @3|N(Q PD|EH!!0O|1- 0!|N$ F$|H0 .|-- ,$|"
    "!$ L#|K0 D|L0 >|M0 @|\\, ,!|O0 B|P0 <| 0 &'|!0 0'|\"($!0&|#($! &|$0 H(|"
    "%($!PW|&('!@X|'0 B'|(($!@.|)('!PN|*($!@1|+($!01|,('! 5|-('!PK|Y$ ,&|/('"
    "! ;|0('!0;|10 4'|2('!0M|3('! J|40 2'|5('! V|%H  PW|7('! M|8('! N|9('!@M"
    "|:('!0N|;('!PM| HG 09|!HE 0B|2$ X| (! P$|$HG 04|)$ H#|&HG @;| HU 09|D($"
    "!0-|E('!0O|T$ &!|U$ $!|,HG  5|I($! 0| (U 09|/HG  ;|0HG 0;|.(! P\"|6$ >#"
    "|3$ 0%|-HU @Y| H3 P$|!$ L#|%4|T($!P(|U($!@(|%H3 PW|-(U @Y|X('! U| 0 &'|"
    "!0 0'|\"0 R|#0 P|$0 H(|%0 ^&|&0 $'|'0 B'|(0 J(|)(!!PN|*HD @1|+HD 01|,HD"
    "  5|-(!!PK|H(! P!|/HD  ;|0HD 0;|10 4'|2(!!0M|3HD  J|40 2'|5(!! V|2$ &#|"
    "7(!! M|8(!! N|9(!!@M|:(!!0N|;(!!PM|RHU @P| $ &'|2$ $#|\"$ R|#$ P|$$ P(|"
    "!$ 0&|R(U @P|D(< 0-|E(!!0O|)$ R$|(- *&|'$ @%|I(<  0| 0 &'|!0 ('|\"0 R|#"
    "0 P|$0 H(|%(#!PW|&(&!@;|'(#!@5|((#! X|6$ *!|*(#!@1|+(#!01|,(&! 5| (2 P$"
    "|.0 6|/(&! ;|0(&!0;|1(#!P/|!(E 0B|3(&! J|4(#!@/|6$ *!|6- &%|D$ J!|8- (%"
    "|F$ \"%|G$  %|.$ 6|<0 0\"|=0 8\"|>0 4\"|?0 >\"|@0 2\"|A0 6\"|B0 :\"|C0 "
    "<\"|D(#!0-|2$ \"#|M- ,%|N- .%| H2 P$|I(#! 0|J(&!0?|2$  #|2$ \\\"|%H2 PW"
    "|[, P$|\\, J$|], ^$|^, T$|_, \\$| - Z$|!- :!|\"- <!|#- N|$- X$|%- H!|&-"
    " N!|'- V$|\\, $$|2$ Z\"|2$ X\"|+- D!|,- F!| H1 P$|.- B%|/- @!|0- B!|6$ "
    "*!|\"$ T#|3- \"\"| HD PX|!0 0'|\"HD 0&|#HD  &|$HD 04|%HD PW|&0 $'|'(!! "
    "L|(HD  9|)(!!PN|*HD @1|+HD 01|,HD  5|-(!!PK|!~/HD  ;|0HD 0;|1(!!@N|2(!!"
    "0M|3HD  J|4(!! O|5(!! V| ($ P$|7(!! M|8(!! N|9(!!@M|:(!!0N|;(!!PM| $ &'"
    "|!HH 0B|\"$ R|#$ P|$$ H(| $ F|&$ <#|\\, .!|.($ P\"|E(!!0O|%(1 PW|2$ ^\""
    "|,$ H\"|V$ 2!|W$ 4!|/$ 8#|0$ :#|!~!~S$ V#|!~ 0 F|!(D @=|\"0 R|#0 P|$$ B"
    "\"|%(D PW|Q- 2|R- 4|((D  9|!~*(D @1|+(D 01|,$ H\"|H($ P!|.(* P\"|D$ J!|"
    "K($ @$|L($ P#|M($  $|1- *$|O($ 0$|P($ @#|!~ H' P$|-- \\#|\"H' 0&|#H'  &"
    "| (\" P$|<(*  2|=(*  3|>(* @2|?(* P3|@(* 02|A(* P2|B(* 03|C(* @3|9- L&|"
    ".H' P\"|[, 6#|!~!~.(\" P\"|J$ .$|!~!- :!|\"- <!|#- N| HV 09|%- H!|&- N!"
    "|\"- L|#- N|)- .#|*- ,#|+- D!|,- F!|!~!~/- @!|0- B!|-HV @Y|!~3- \"\"| $"
    " &'|5- 4#|\"$ R|#$ P|$$ P(|!~!~;- 0#|<- 2#|)$ R$|)- F\"|*- D\"|!~!~ H! "
    "P$| 0 F|!(D @=|\"0 R|#0 P|$$ B\"|%(D PW|D- \"!|6$ *!|((D  9|7- @\"|*(D "
    "@1|+(D 01|,$ H\"|.H! P\"|.(* P\"|=- 0$|Y- 0|?- R!|\"$ T#|RHV @P|!~D$ J!"
    "|!~F$ \"%|G$  %|!~!~!~<(*  2|=(*  3|>(* @2|?(* P3|@(* 02|A(* P2|B(* 03|"
    "C(* @3|V$ 2!|W$ 4!|!~HH! P!|!~!~!~!~[, P$|\\, J$|], ^$|^, T$|_, \\$| - "
    "Z$|!- :!|\"- <!|#- N|$- X$|%- H!|&- N!|'- .&|Z$ (|!~)$ H#|+- D!|,- F!|!"
    "~1- ($|/- J#|0- B!| $ *#|S$ V#|\"$ F'|#$ P|$$ N$|!~6$ >#|)- F\"|*- D\"|"
    ")$ R$| 0 &'|!H9 @=|\"H9 0&|#H9  &|$0 P(|%H9 PW|!~'H9 @5|(H9 @.|)$ R$|!~"
    "7- @\"|6$ *!| $ F|!~\"$ F|#$ F|-- X#|%$ F|?- R!|NHQ PD|($ F|6$ *!|*$ F|"
    "+$ F|!~ H6 P$|!H6 @=|F$ \"%|G$  %|K- &|%H6 PW|J$ V(|'H6 @5|(H6 @.|P- \""
    "|DH9 0-| H< P$|F$ \"%|G$  %|U- $|S$ V#|%H< PW|!~V$ 2!|W$ 4!|(- D#|*$ ,\""
    "|+$ *\"|[, P$|\\, ((|], ^$|^, T$|_, \\$| - 6&|!- R#|!~!~4- @#|[, P$|\\,"
    " J$|], ^$|^, T$|_, @&|:- F#|T(> P$|U(> P$|-- X(|>- B#|!~!~1- $(|!~ H#!P"
    "$|!H#!@=|\"H#!0&|#H#! &|$H#!@-|%H#!PW|9- ^#|'H#!@5|(H#! X| (G 09|*H#!@1"
    "|+H#!01|!~$(G 04|.H#!P\"|&(G @;|C- \"$|1H#!P/|!~!~4H#!@/|,(G  5|I-  $|!"
    "~/(G  ;|0(G 0;|!~!~<H#! 2|=H#! 3|>H#!@2|?H#!P3|@H#!02|AH#!P2|BH#!03|CH#"
    "!@3|DH#!0-| (( P$|!~\"(( 0&|#((  &|IH#! 0| (#!P$|!(#!@=|\"(#!0&|#(#! &|"
    "$(#!@-|%(#!PW|!~'(#!@5|((#! X|.(( P\"|*(#!@1|+(#!01|!~!~.(#!P\"|!~ 0 F|"
    "1(#!P/|@- P\"|A- (\"|4(#!@/|%0 ^&|!~!~!~!~*0 ,\"|+0 *\"|<(#! 2|=(#! 3|>"
    "(#!@2|?(#!P3|@(#!02|A(#!P2|B(#!03|C(#!@3|D(#!0-| (' P$|!~\"$ R|#$ P|I(#"
    "! 0| 0 F|!(B @=|\"0 R|#0 P|$(B @-|%(B PW|!~'(B @5|(0  '|.(' P\"|*(B @1|"
    "+(B 01|!~ (? P$|.0 6|\"(? 0&|#(?  &|1(B P/|%(? PW|!~4(B @/|!~!~*(? @1|+"
    "(? 01|!~!~!~<H*  2|=H*  3|>H* @2|?H* P3|@H* 02|AH* P2|BH* 03|CH* @3|D(B"
    " 0-|!~ ('!09|!('!0X|!~I(B  0|$('!04|!~&('!@X|'('! L|!~)('!PN|!~!~,('! 5"
    "|-('!PK|!~/('! ;|0('!0;|1('!@N|2('!0M|3('! J|4('! O|5('! V|!- V|7('! M|"
    "8('! N|9('!@M|:('!0N|;('!PM|!~ (V 09|!~!~!~!~ (K 09|!0 \"'|!~E('!0O|$(K"
    " 04|\\- 8|&0 $'|'(\"! L|-(V @Y|)(\"!PN|!~!~,(K  5|-(\"!PK|!~/(K  ;|0(K "
    "0;|1(\"!@N|2(\"!0M|3(K  J|4(\"! O|5(\"! V|X('! U|7(\"! M|8(\"! N|9(\"!@"
    "M|:(\"!0N|;(\"!PM| (5 P$|!(5 @=|!~!~!~%(5 PW| (K 09|!0 \"'|($ T!|E(\"!0"
    "O|$(K 04|!~&0 $'|'(\"! L|!~)(\"!PN|!~R(V @P|,(K  5|-(\"!PK|]- T|/(K  ;|"
    "0(K 0;|1(\"!@N|2(\"!0M|3(K  J|4(\"! O|5(\"! V|X$ H&|7(\"! M|8(\"! N|9(\""
    "!@M|:(\"!0N|;(\"!PM| 0 &'|!0 2$|\"0 R|#0 P|$0 B\"|!$ 0&|&0 <#| 0 *#| HU"
    " 09|E(\"!0O|)$ H#|'$ @%|,0 H\"|!~.0 6|/0 8#|00 :#|-$ >%|!~30 0%|-0 ,'|-"
    "HU @Y| $ *#|6$ >#|!~5$ P&|$$ N$|!~<0 0\"|=0 8\"|>0 4\"|?0 >\"|@0 2\"|A0"
    " 6\"|B0 :\"|C0 <\"|!~ H&!PX|!H&!0B|\"H&!0&|#H&! &|$H&!04|J(. 0?|&H&!@;|"
    "6$ R(|!~!~NHQ PD|2- Z(|,H&! 5|!~.H&!P\"|/H&! ;|0H&!0;| 0 F|!0 L#|3H&! J"
    "|R0 $&|RHU @P|%0 ^&|F$ \"%|'0 L\"|(0 T!|!~!~<H&! 2|=H&! 3|>H&!@2|?H&!P3"
    "|@H&!02|AH&!P2|BH&!03|CH&!@3|(- L(| (&!PX|!(&!0B|\"(&!0&|#(&! &|$(&!04|"
    "JH&!0?|&(&!@;|[, P$|\\, J$|], D%|!~!~,(&! 5| H= P$|.(&!P\"|/(&! ;|0(&!0"
    ";|:- F#|%H= PW|3(&! J|6- *%|>- 2%|8- (%|*H= @1|+H= 01|!~!~!~<(&! 2|=(&!"
    " 3|>(&!@2|?(&!P3|@(&!02|A(&!P2|B(&!03|C(&!@3| 0 *#|!~!~!~$$ N$|!~J(&!0?"
    "|!~!~)$ R$|!~!~!~-HU @Y|!~ HR PX|!HR  Y|\"HR 0&|#HR  &|$HR 04|%HR PW|&H"
    "R @;|6$ *!|(HR  9|)$ H#|*HR @1|+HR 01|,HR  5| (6 P$|!(6 @=|/HR  ;|0HR 0"
    ";|!~%(6 PW|3HR  J|'(6 @5|((6 @.|6$ >#|F$ \"%|G$  %|!~ (D P$|!(D @=|\"(D"
    " 0&|#(D  &|$$ B\"|%(D PW|G- X!|!~((D  9|RHU @P|*(D @1|+(D 01|,$ H\"|!~!"
    "~!~!~!~[, P$|\\, J$|], ^$|^, T$|_, \\$| - Z$|!~!~!~$- X$|!~ H@ P$|'- V$"
    "|\"H@ 0&|#H@  &|   T&|%H@ PW|\"$ R|#$ P|.- L$|%(> PW|*H@ @1|+H@ 01|!~J$"
    " .$|*(> @1|+(> 01|6- &%|(- :%|8- (%| 0 &'|!0 ('|\"0 R|#0 P|$0 H(|%0 ^&|"
    "&HD @;|'(< @5|(0 J(| 0 *#|*HD @1|+HD 01|,HD  5|$$ N$|!~/HD  ;|0HD 0;|1("
    "< P/|)$ R$|3HD  J|4(< @/|)$ H#|-HU @Y|!~ (5 P$|)- F\"|*- D\"|!~!~%(5 PW"
    "|)$ H#|6$ *!|($ T!|!~6$ >#|!~D(< 0-|O- <%|!~7- @\"| $ *#|I(<  0|\"$ T#|"
    "6$ >#|$$ N$|=- 0$|!~F$ \"%|G$  %|)$ D(|!- :!|\"- <!|#- N| (= P$|%- R\"|"
    "&- N!|!~!~%(= PW|RHU @P|!~!~6$ F(|*(= @1|+(= 01|!~!~NHQ PD|[, P$|\\, J$"
    "|], ^$|^, T$|_, \\$| - Z$|\"$ T#|!~!~$- X$|F$ \"%|G$  %|'- V$|)$ H#|!~!"
    "~(- 8$|!~!~.- L$|!~H- P#|!~S$ V#|!~(- 4%|6$ >#|6- R'|4- 6$|8- (%|!~[, P"
    "$|\\, J$|], ^$|^, T$|_, \\$| - Z$|!~2- V!| H%!P$|$- 2&|\"H%!0&|#H%! &|:"
    "- F#|(- 8$|!(W  R|!~>- 2%|!~-- >$|&$ F%|'(W  L|!~.H%!P\"|!~S$ V#|4- @$|"
    "-(W PK|!~!~!~ 0 F|!~\"0 R|#0 P|5(W  V|!~<H%! 2|=H%! 3|>H%!@2|?H%!P3|@H%"
    "!02|AH%!P2|BH%!03|CH%!@3|.0 6|(- 8$|!~!~HH%!P!|!~-- :$|KH%!@$|LH%!P#|MH"
    "%! $| H\" P$|OH%!0$|PH%!@#|4- <$|<0 0\"|=0 8\"|>0 4\"|?0 >\"|@0 2\"|A0 "
    "6\"|B0 :\"|C0 <\"|!~!~.H\" P\"|!~H0 .|!~!~K0 D|L0 >|M0 @|!~O0 B|P0 <| 0"
    " F|!(; @=|\"0 R|#0 P|$(; @-|%0 ^&|!~'(; @5|(0 T!|!~*0 ,\"|+0 *\"|!~!~!~"
    "H$ .| (%!P$|1(; P/|\"(%!0&|#(%! &|4(; @/|!~!~ HI PX|!HI  Y|\"HI 0&|#HI "
    " &|$HI 04|%HI PW|&HI @;|.(%!P\"|(HI  9|!~*HI @1|+HI 01|,HI  5|D(; 0-|!~"
    "/HI  ;|0HI 0;|!~I(;  0|3HI  J|!~<(%! 2|=(%! 3|>(%!@2|?(%!P3|@(%!02|A(%!"
    "P2|B(%!03|C(%!@3| $ F|!~\"$ R|#$ P|H(%!P!|!~!~K(%!@$|L(%!P#|M(%! $|!~O("
    "%!0$|P(%!@#| 0 &'|!(9 @=|\"(9 0&|#(9  &|$0 P(|%(9 PW|!~'(9 @5|((9 @.|)$"
    " R$|!~!~!~ 0 F|!(T @=|\"0 R|#0 P|2- N#|%(T PW|!~!~!~6$ *!|*(T @1|+(T 01"
    "|!~!~.(/ P\"|V- *|!~!~@- &\"|A- (\"|[- ,|!~!~D(9 0-|F- $\"|F$ \"%|G$  %"
    "|!~<(/  2|=(/  3|>(/ @2|?(/ P3|@(/ 02|A(/ P2|B(/ 03|C(/ @3|!~!~!~!- :!|"
    "\"- <!|#- N|!~%- P!|&- N!|!~[, P$|\\, J$|], ^$|^, D&| 0 &'|!0 ('|\"0 R|"
    "#0 P|$0 B\"|%0 ^&|&0 <#|Y$ ,&|(0 (#|!~*0 ,\"|+0 *\"|,0 H\"|!~!~/0 8#|00"
    " :#|!~!~30 0%| (P PX|!(P  Y|\"(P 0&|#(P  &|$(P 04|%(P PW|&(P @;|!~((P  "
    "9|!~*(P @1|+(P 01|,(P  5|!~!~/(P  ;|0(P 0;|!~!(X  R|3(P  J|!~ $ *#|!~&("
    "X PL|'(X  L|$$ B\"|N(Q PD|&$ <#|!~Q0 *'|-(X PK|!~!~,$ H\"|!~2$ J%|/$ 8#"
    "|0$ :#|5(X  V|!~7$ H%|8$ P%|9$ L%|:$ R%|;$ N%|!~N(Q PD|!~!~Q(P 0Y| $ 6%"
    "|!$ 6%|\"$ 6%|#$ 6%|$$ 6%|%$ 6%|&$ 6%|!~($ 6%|!~*$ 6%|+$ 6%|,$ 6%|!~!~/"
    "$ 6%|0$ 6%|!~!~3$ 6%|!~ HN PX|!HN  Y|\"HN 0&|#HN  &|$HN 04|%HN PW|&HN @"
    ";|B- D$|(HN  9|[, 6#|*HN @1|+HN 01|,HN  5|!~!~/HN  ;|0HN 0;|!~!~3HN  J|"
    "!~!~!~)- .#|*- ,#|NHN 0D|!~B- D$|Q$ 6%|   R&|!H7 @=|\"$ R|#$ P|$$ L!|%H"
    "7 PW|5- (&|'H7 @5|(H7 @.|!~!~ (S PX|!(S  Y|\"(S 0&|#(S  &|$(S 04|%(S PW"
    "|&(S @;|!~((S  9|Q  V&|*(S @1|+(S 01|,(S  5|!~!~/(S  ;|0(S 0;|\"$ T#|!~"
    "3(S  J|!~!~!~!~!~D$ J!| HS PX|!HS  Y|\"HS 0&|#HS  &|$HS 04|%HS PW|&HS @"
    ";|!~(HS  9|!~*HS @1|+HS 01|,HS  5|!~!~/HS  ;|0HS 0;|!~!~3HS  J|N(S PD|!"
    "~!~Q(S 0Y|!~!~!~!~!- :!|\"- <!|#- N|J$ Z#|%- H!|&- N!|!~!~!~!~+- Z!|,- "
    "F!|S$ V#|!~L- 8%|!~!~!~NHS PD|!~!~QHS 0Y| HB P$|!HB @=|\"HB 0&|#HB  &|$"
    "HB @-|%HB PW|!~'HB @5|(HB @.| $ *#|*HB @1|+HB 01|!~$$ N$|!~!~-- \\#|1HB"
    " P/|)$ R$|!~4HB @/|!~ HA P$|!HA @=|\"HA 0&|#HA  &|$HA @-|%HA PW|9- ^#|'"
    "HA @5|(HA @.|6$ *!|*HA @1|+HA 01|!~!~DHB 0-|!~C- \"$|1HA P/|!~IHB  0|4H"
    "A @/|!~I-  $| (F 09|!(F 0B|F$ \"%|G$  %|$(F 04|!~&(F @;|THB P(|UHB @(|!"
    "~!~!~,(F  5|DHA 0-|!~/(F  ;|0(F 0;|!~IHA  0|3(F  J|!~!~!~[, P$|\\, J$|]"
    ", ^$|^, T$|_, \\$| - 4&|T$ &!|U$ $!| ($!P$|!($!@=|\"($!0&|#($! &|$($!@-"
    "|%($!PW|!~'($!@5|(($!@.|!~*($!@1|+($!01|!~!~!~!~ (* P$|1($!P/|\"(* 0&|#"
    "(*  &|4($!@/|!~!~ HO PX|!HO  Y|\"HO 0&|#HO  &|$HO 04|%HO PW|&HO @;|.(* "
    "P\"|(HO  9|!~*HO @1|+HO 01|,HO  5|D($!0-|!~/HO  ;|0HO 0;|!~I($! 0|3HO  "
    "J|!~<(*  2|=(*  3|>(* @2|?(* P3|@(* 02|A(* P2|B(* 03|C(* @3|T($!P(|U($!"
    "@(|!~ $ 6%|!$ 6%|\"$ 6%|#$ 6%|$$ 6%|%$ 6%|&$ 6%|!~($ 6%|!~*$ 6%|+$ 6%|,"
    "$ 6%|!~N(Q PD|/$ 6%|0$ 6%|QHO 0Y|!~3$ 6%| (P PX|!(P  Y|\"(P 0&|#(P  &|$"
    "(P 04|%(P PW|&(P @;|!~((P  9| $ *#|*(P @1|+(P 01|,(P  5|$$ N$|!~/(P  ;|"
    "0(P 0;|!~)$ R$|3(P  J| 0 F|!0 L#|\"0 R|#0 P|$0 L!|%0 ^&|NHN 0D|'0 L\"|("
    "0 T!|Q$ 6%| H> P$|6$ *!|\"H> 0&|#H>  &|!~%H> PW|?- R!|!~!0 0&|!~*H> @1|"
    "+H> 01|!~&0 F%|'0 @%|!~)0 V%|F$ \"%|G$  %|Q(P 0Y|-0 >%|!~!~!~10 T%|20 J"
    "%|D0 J!|40 X%|50 P&|!~70 H%|80 P%|90 L%|:0 R%|;0 N%|!~!~!~[, P$|\\, J$|"
    "], ^$|^, T$|_, \\$| - Z$|E0 Z%|!~!~$- X$| 0 *#|!0 2$|'- V$|!~$0 B\"|!( "
    "! R|&0 <#|!~!~.- L$|&( !PL|'( ! L|,0 H\"|)( !PN|!~/0 8#|00 :#|-( !PK|!~"
    "30 0%|B- D$|1( !@N|2( !0M|!~4( ! O|5( ! V| $ *#|7( ! M|8( ! N|9( !@M|:("
    " !0N|;( !PM|!H!! R|!~!~!~!~&H!!PL|'H!! L|-HT @Y|)H!!PN|E( !0O|!~!~-H!!P"
    "K|!~!~!~1H!!@N|2H!!0M|!~4H!! O|5H!! V|!~7H!! M|8H!! N|9H!!@M|:H!!0N|;H!"
    "!PM|!~ $ *#|!~!~!~$$ N$|!~!~!(\"! R|EH!!0O|)$ R$|!~!~&(\"!PL|'(\"! L|!~"
    ")(\"!PN|R$ $&|!~!~-(\"!PK|!~!~6$ *!|1(\"!@N|2(\"!0M|[,  &|4(\"! O|5(\"!"
    " V|!~7(\"! M|8(\"! N|9(\"!@M|:(\"!0N|;(\"!PM|!~!~ $ *#|!~F$ \"%|G$  %|$"
    "$ N$|E- 4$|!~E(\"!0O| (: P$|!(: @=|\"(: 0&|#(:  &|$(: @-|%(: PW|!~'(: @"
    "5|((: @.|!~!~!~!~!~6$ *!|[, P$|\\, J$|], ^$|^, T$|_, \\$| - Z$|!~!~!0 0"
    "&|$- X$|!~!~'- V$|&0 F%|'0 @%|F$ \"%|)0 V%|!~!~.- B%|-0 >%|D(: 0-|!~!~1"
    "0 T%|20 J%|W- \"&|4$ X%|50 P&|!~70 H%|80 P%|90 L%|:0 R%|;0 N%|!0 0&|[, "
    "P$|\\, J$|], D%|!~&0 F%|'0 @%|!~)0 V%|E$ Z%|!~!~-0 >%|!~!~!~10 T%|20 J%"
    "|!~40 X%|50 P&|!~70 H%|80 P%|90 L%|:0 R%|;0 N%| (J PX|!(J  Y|\"(J 0&|#("
    "J  &|$(J 04|%(J PW|&(J @;|!~((J  9|E0 Z%|*(J @1|+(J 01|,(J  5| H5 P$|!H"
    "5 @=|/(J  ;|0(J 0;|!(_  R|%H5 PW|3(J  J|'$ L\"|(H5 @.|&(_ PL|'(_  L|!~)"
    "(_ PN| $ *#|!~!~-(_ PK|$$ N$|!~!~1(_ @N|2(_ 0M|)$ R$|4(_  O|5(_  V|!~7("
    "_  M|8(_  N|9(_ @M|:(_ 0N|;(_ PM|!~!~!0 0&|!~6$ *!|Q$ B$|!~&$ F%|'0 @%|"
    "E(_ 0O|!~!H_  R|!~!~-0 >%|!~&H_ PL|'H_  L|!~)H_ PN|F$ \"%|G$  %|50 P&|-"
    "H_ PK|!~!~!~1H_ @N|2H_ 0M|!~4H_  O|5H_  V|!~7H_  M|8H_  N|9H_ @M|:H_ 0N"
    "|;H_ PM|!~!~!~[, P$|\\, J$|], ^$|^, T$|_, \\$| - Z$|EH_ 0O|!~!~$- X$|!~"
    "!~'- V$|!~!~!~!~!~!~.- N&| 0 &'|!0 ('|\"(J 0&|#(J  &|$0 B\"|%(J PW|&0 <"
    "#|!~((J  9|!~*(J @1|+(J 01|,0 H\"|!~!~/0 8#|00 :#|!(^  R|!~30 0%|!~!~&("
    "^ PL|'(^  L|!~)(^ PN|!~!~X- J\"|-(^ PK|!~!~!~1(^ @N|2(^ 0M|!~4(^  O|5(^"
    "  V|!~7(^  M|8(^  N|9(^ @M|:(^ 0N|;(^ PM|!~!~!(]  R|!~!~Q$ B$|!~&(] PL|"
    "'(]  L|E(^ 0O|)(] PN|!~!~!~-(] PK|!~!~!~1(] @N|2(] 0M|!~4$ X%|5(]  V|!~"
    "7(]  M|8(]  N|9(] @M|:(] 0N|;(] PM|!H\\  R|!~!~!~!~&H\\ PL|'H\\  L|!~)H"
    "\\ PN|E$ Z%|!~!~-H\\ PK|!~!~!~1H\\ @N|2H\\ 0M|!~4$ X%|5H\\  V|!~7H\\  M"
    "|8H\\  N|9H\\ @M|:H\\ 0N|;H\\ PM|!H]  R|!~!~!~!~&H] PL|'H]  L|!~)H] PN|"
    "E$ Z%|!~!~-H] PK|!~!~!~1H] @N|2H] 0M|!~4$ X%|5H]  V|!~7H]  M|8H]  N|9H]"
    " @M|:H] 0N|;H] PM|!H ! R| $ F|!~\"$ R|#$ P|&H !PL|'H ! L|!~)H !PN|E$ Z%"
    "|!~ $ F|-H !PK|\"$ R|#$ P|$$ L!|1H !@N|2H !0M|!~4H ! O|5H ! V|!~7H ! M|"
    "8H ! N|9H !@M|:H !0N|;H !PM|!~!~!H\"! R|!~!~!~!~&H\"!PL|'H\"! L|EH !0O|"
    ")H\"!PN|!~!~!~-H\"!PK|!~!~!~1H\"!@N|2H\"!0M|D$ J!|4H\"! O|5H\"! V|!~7H\""
    "! M|8H\"! N|9H\"!@M|:H\"!0N|;H\"!PM|!~ $ F|!~\"$ R|#$ P|$$ L!|!~!~!~EH\""
    "!0O|!- :!|\"- <!|#- N|!~%- R\"|&- N!|!~!~!~!~!- :!|\"- <!|#- N|!~%- H!|"
    "&- N!|!~!~!~!~+- D!|,- F!|!~!~/- @!|0- B!|!H^  R|D$ J!|3- >!|!~!~&H^ PL"
    "|'H^  L|!~)H^ PN|!~!~!~-H^ PK|H- V\"|!~J- T\"|1H^ @N|2H^ 0M|!~4H^  O|5H"
    "^  V|!~7H^  M|8H^  N|9H^ @M|:H^ 0N|;H^ PM|!~!~!~!- :!|\"- <!|#- N|!~%- "
    "H!|&- N!|EH^ 0O|!~!~!~+- D!|,- F!|!~!~/- @!|0- B!|!(!! R|!~3- \"\"|!~!~"
    "&(!!PL|'(!! L|!~)(!!PN|!~!~!~-(!!PK|!~!~!~1(!!@N|2(!!0M|!~4(!! O|5(!! V"
    "|!~7(!! M|8(!! N|9(!!@M|:(!!0N|;(!!PM| (O PX|!(O  Y|\"(O 0&|#(O  &|$(O "
    "04|%(O PW|&(O @;|!~((O  9|E(!!0O|*(O @1|+(O 01|,(O  5|!~!~/(O  ;|0(O 0;"
    "|!~!~3(O  J| HO PX|!HO  Y|\"HO 0&|#HO  &|$HO 04|%HO PW|&HO @;|!~(HO  9|"
    " $ *#|*HO @1|+HO 01|,HO  5|$$ N$|!~/HO  ;|0HO 0;|!~!0 0&|3HO  J|!~!~!~&"
    "0 F%|'0 @%|!~)0 V%|!~!~Q(O 0Y|-0 >%|6$ *!|!~!~10 T%|20 J%|!~40 X%|50 P&"
    "|!~70 H%|80 P%|90 L%|:0 R%|;0 N%|!~!~F$ \"%|!~QHO 0Y|!~!~!~!~E0 Z%| HA "
    "P$|!HA @=|\"HA 0&|#HA  &|$HA @-|%HA PW|!~'HA @5|(HA @.|!~*HA @1|+HA 01|"
    "!~[, P$|\\, J$|], \\%|!~1HA P/|!~!~4HA @/|!~ (A P$|!(A @=|\"(A 0&|#(A  "
    "&|$(A @-|%(A PW|!~'(A @5|((A @.|!~*(A @1|+(A 01|!~!~DHA 0-|!~!~1(A P/|!"
    "~IHA  0|4(A @/|!~ (B P$|!(B @=|\"(B 0&|#(B  &|$(B @-|%(B PW|!~'(B @5|(("
    "B @.|!~*(B @1|+(B 01|!~!~D(A 0-|!~!~1(B P/|!~I(A  0|4(B @/|!~!~ HP PX|!"
    "HP  Y|\"HP 0&|#HP  &|$HP 04|%HP PW|&HP @;|!~(HP  9|!~*HP @1|+HP 01|,HP "
    " 5|D(B 0-|!~/HP  ;|0HP 0;| $ *#|I(B  0|3HP  J|!~$$ N$|!~!~!~!~)$ R$|!~!"
    "~!~ HD PX|!HD  Y|\"HD 0&|#HD  &|$HD 04|%HD PW|&HD @;|!~(HD  9|6$ *!|*HD"
    " @1|+HD 01|,HD  5|!~!~/HD  ;|0HD 0;|!~!HX  R|3HD  J|!~!~!~&HX PL|'HX  L"
    "|F$ \"%|G$  %|!~!~!~-HX PK| (, P$|!~\"(, 0&|#(,  &|2$ J%|!~!~5HX  V|!~7"
    "$ H%|8$ P%|9$ L%|:$ R%|;$ N%|.(, P\"|[, P$|\\, J$|], ^$|^, T$|_, \\$| -"
    " Z$|!~!~!~$- X$|!~!~'- .&|<(,  2|=(,  3|>(, @2|?(, P3|@(, 02|A(, P2|B(,"
    " 03|C(, @3|!~!~!~!~!~!~J$ Z#| 0 &'|!0 ('|\"(C 0&|#(C  &|$(N 04|%(C PW|&"
    "(N @;|!~((C  9|!~*(C @1|+(C 01|,(N  5|!~!~/(N  ;|0(N 0;|!~!~3(N  J| (R "
    "PX|!(R  Y|\"(R 0&|#(R  &|$(R 04|%(R PW|&(R @;|!~((R  9|!~*(R @1|+(R 01|"
    ",(R  5|!~!~/(R  ;|0(R 0;|!~!~3(R  J| $ F|!$ F|\"$ F|#$ F|$$ F|%$ F|!~'$"
    " F|($ F| $ F|!~\"$ R|#$ P|$$ L!|!~!~C- $%|1$ F|!~!~4$ F|!~!~ 0 &'|!0 ('"
    "|\"HC 0&|#HC  &|$(I 04|%HC PW|&(I @;|!~(HC  9|!~*HC @1|+HC 01|,(I  5|D$"
    " F|!~/(I  ;|0(I 0;|!~I$ F|3(I  J|!~!~D$ J!|!~!~!~!~!~!~TH7 P$|UH7 P$| 0"
    " &'|!0 ('|\"0 R|#0 P|$0 B\"|%0 ^&|&0 <#|!~(0 (#|!~*0 ,\"|+0 *\"|,0 H\"|"
    "!~!~/0 8#|00 :#|!~!~30 0%|!- :!|\"- <!|#- N|!~%- H!|&- N!|!~!~!~!~+- D!"
    "|,- F!|!~!~/- J#|0- B!| HJ PX|!HJ  Y|\"HJ 0&|#HJ  &|$HJ 04|%HJ PW|&HJ @"
    ";|!~(HJ  9|!~*HJ @1|+HJ 01|,HJ  5|!~!~/HJ  ;|0HJ 0;|!H[  R| H- P$|3HJ  "
    "J|\"H- 0&|#H-  &|&H[ PL|'H[  L|!~)$ V%|!~!~!~-H[ PK|!~!~.H- P\"|1$ T%|2"
    "H[ 0M|!~!~5H[  V|!~7H[  M|8H[  N|9H[ @M|:H[ 0N|;H[ PM|!~!~<H-  2|=H-  3"
    "|>H- @2|?H- P3|@H- 02|AH- P2|BH- 03|CH- @3|!~!~!([  R|!~!~!~JH- 0?|&([ "
    "PL|'([  L|!~)$ V%|!~!~!~-([ PK|!~!~!~1$ T%|2([ 0M|!~!~5([  V|!~7([  M|8"
    "([  N|9([ @M|:([ 0N|;([ PM|!HZ  R|!~!~!~!~&HZ PL|'HZ  L|!~)$ V%|!~!~!~-"
    "HZ PK|!~!~!~1$ T%|2HZ 0M|!~!~5HZ  V|!~7HZ  M|8HZ  N|9HZ @M|:HZ 0N|;HZ P"
    "M|!HY  R|!~!~!~!~&HY PL|'HY  L|!~)$ V%|!~!~!~-HY PK|!~!~!~1$ T%|2HY 0M|"
    "!~!~5HY  V|!~7HY  M|8HY  N|9HY @M|:HY 0N|;HY PM|!(Y  R| 0 F|!~\"0 R|#0 "
    "P|&(Y PL|'(Y  L|!~)$ V%|!~!~!~-(Y PK|!~!~.0 6|1$ T%|2(Y 0M|!~!~5(Y  V|!"
    "~7(Y  M|8(Y  N|9(Y @M|:(Y 0N|;(Y PM|!~!~<0 0\"|=0 8\"|>0 4\"|?0 >\"|@0 "
    "2\"|A0 6\"|B0 :\"|C0 <\"|!~!~!(Z  R|!~!~!~JH- 0?|&(Z PL|'(Z  L| () P$|)"
    "$ V%|\"() 0&|#()  &|!~-(Z PK|!~!~!~1$ T%|2(Z 0M|!~!~5(Z  V|.() P\"|7(Z "
    " M|8(Z  N|9(Z @M|:(Z 0N|;(Z PM|!~!~ 0 F|!~\"0 R|#0 P|!~!~<$ 0\"|=$ 8\"|"
    ">$ 4\"|?$ >\"|@$ 2\"|A$ 6\"|B$ :\"|C$ <\"|.0 6| (D P$|!(D @=|\"(D 0&|#("
    "D  &|$$ B\"|%(D PW|!~!~((D  9|!~*(D @1|+(D 01|,$ H\"|<0 0\"|=0 8\"|>0 4"
    "\"|?0 >\"|@0 2\"|A0 6\"|B0 :\"|C0 <\"| $ *#|!~!~!~$$ N$|!~J0 Z#|!~!(\\ "
    " R|)$ R$|!~ $ *#|!HH 0B|&(\\ PL|'(\\  L|$$ B\"|)$ V%|&$ <#|!~!~-(\\ PK|"
    "!~6$ *!|,$ H\"|1$ T%|2(\\ 0M|/$ 8#|0$ :#|5(\\  V|!~7(\\  M|8(\\  N|9(\\"
    " @M|:(\\ 0N|;(\\ PM|!~!~!~F$ \"%|G$  %|!~ H9 P$|!H9 @=|\"H9 0&|#H9  &|$"
    "H9 @-|%H9 PW|!~'H9 @5|(H9 @.|!~!~)- F\"|*- D\"|!~!~!~!~T- .\"|[, P$|\\,"
    " J$|], ^$|^, T$|_, \\$| - Z$|!~7- @\"|!~$- 2&|!0 0&|[, 6#|!~!~!~&0 F%|'"
    "0 @%| 0 F|DH9 0-|\"0 R|#0 P|!~-0 >%|!~!~)- .#|*- ,#|2$ J%|!~!~50 P&|.0 "
    "6|7$ H%|8$ P%|9$ L%|:$ R%|;$ N%|5- 4#|!~ H, P$|!~\"H, 0&|#H,  &|;- 0#|<"
    "- 2#|<0 0\"|=0 8\"|>0 4\"|?0 >\"|@0 2\"|A0 6\"|B0 :\"|C0 <\"|.H, P\"|!~"
    " (4 P$|!(4 @=|\"(4 0&|#(4  &|J(. 0?|%(4 PW| (. P$|!~\"(. 0&|#(.  &|*(4 "
    "@1|+(4 01|<H,  2|=H,  3|>H, @2|?H, P3|@H, 02|AH, P2|BH, 03|CH, @3|.(. P"
    "\"|!~ 0 F|!$ L#|\"H@ 0&|#H@  &|JH, 0?|%0 ^&| (- P$|!~\"(- 0&|#(-  &|*H@"
    " @1|+H@ 01|<(.  2|=(.  3|>(. @2|?(. P3|@(. 02|A(. P2|B(. 03|C(. @3|.(- "
    "P\"|!~ (C P$|!(C @=|\"(C 0&|#(C  &|J(. 0?|%(C PW|!~!~((C  9|!~*(C @1|+("
    "C 01|<(-  2|=(-  3|>(- @2|?(- P3|@(- 02|A(- P2|B(- 03|C(- @3|!~!~!0 0&|"
    "!~!~!~J(- 0?|&0 F%|'0 @%| H* P$|)$ V%|\"H* 0&|#H*  &|!~-0 >%|!~!~!~1$ T"
    "%|20 J%|!~!~50 P&|.H* P\"|70 H%|80 P%|90 L%|:0 R%|;0 N%|!~!~!~!~!~!~!~!"
    "~<H*  2|=H*  3|>H* @2|?H* P3|@H* 02|AH* P2|BH* 03|CH* @3| (< P$|!(< @=|"
    "\"(< 0&|#(<  &|$(< @-|%(< PW|!~'(< @5|((< @.|!~!~!~!~!~!~!~ (/ P$|1(< P"
    "/|\"(/ 0&|#(/  &|4(< @/|!~!~!~ (+ P$|!~\"(+ 0&|#(+  &|!~!~.(/ P\"|!~ H4"
    " P$|!H4 @=|\"H4 0&|#H4  &|D(< 0-|%H4 PW|.(+ P\"|!~($ (#|I(<  0|*H4 @1|+"
    "H4 01|<(/  2|=(/  3|>(/ @2|?(/ P3|@(/ 02|A(/ P2|B(/ 03|C(/ @3|<(+  2|=("
    "+  3|>(+ @2|?(+ P3|@(+ 02|A(+ P2|B(+ 03|C(+ @3| H+ P$| $ *#|\"H+ 0&|#H+"
    "  &|!~$$ N$|!~!~!~!~)$ R$|!~!~ H. P$|.H+ P\"|\"H. 0&|#H.  &|!~!~!~!~!~!"
    "~6$ *!|!~!~!~.H. P\"|<H+  2|=H+  3|>H+ @2|?H+ P3|@H+ 02|AH+ P2|BH+ 03|C"
    "H+ @3|!~!~!~F$ \"%|G$  %|<H.  2|=H.  3|>H. @2|?H. P3|@H. 02|AH. P2|BH. "
    "03|CH. @3| H; P$|!H; @=|\"H; 0&|#H;  &|$H; @-|%H; PW|!~'H; @5|(H; @.|!~"
    "!~[, P$|\\, J$|], ^$|^, T$|_, \\$| - 8&|1H; P/|!~!~4H; @/|!~ (; P$|!(; "
    "@=|\"(; 0&|#(;  &|$(; @-|%(; PW| $ *#|'(; @5|((; @.|!~$$ N$|!~!~!~DH; 0"
    "-|)$ R$|!~1(; P/|!~IH;  0|4(; @/| (0 P$|!~\"(0 0&|#(0  &|!~!~!~6$ *!| H"
    "0 P$|!~\"H0 0&|#H0  &|!~!~.(0 P\"|D(; 0-| (T P$|!(T @=|\"(T 0&|#(T  &|I"
    "(;  0|%(T PW|.H0 P\"|F$ \"%|G$  %|!~*(T @1|+(T 01|<(0  2|=(0  3|>(0 @2|"
    "?(0 P3|@(0 02|A(0 P2|B(0 03|C(0 @3|<H0  2|=H0  3|>H0 @2|?H0 P3|@H0 02|A"
    "H0 P2|BH0 03|CH0 @3|[, P$|\\, J$|], ^$|^, T$|_, \\$| - 6&| H/ P$|!~\"H/"
    " 0&|#H/  &| H: P$|!H: @=|\"H: 0&|#H:  &|$H: @-|%H: PW|!~'H: @5|(H: @.|!"
    "~.H/ P\"|!~ $ *#|!~!~!~$$ N$|1$ ^!|!~!~4$ \\!|)$ R$|!~!~<H/  2|=H/  3|>"
    "H/ @2|?H/ P3|@H/ 02|AH/ P2|BH/ 03|CH/ @3|!~!~6$ *!|!~DH: 0-|!~!~ $ *#|!"
    "~I$  \"|!~$$ N$|!~ $ *#| (# P$|!~)$ R$|$$ N$|F$ \"%|G$  %|!~ 0 F|)$ R$|"
    "\"0 R|#0 P|!~%0 ^&|!~.(# P\"|6$ *!|!~*0 ,\"|+0 *\"|!~!~6$ *!|!~!~!~[, P"
    "$|\\, J$|], ^$|^, T$|_, \\$| - :&|F$ \"%|G$  %|!~!~!~!~F$ \"%|G$  %|!~H"
    "(# P!|!~!~K$ D|L$ >|M$ @|!~O$ B|P$ <|!~!~!~[, P$|\\, J$|], ^$|^, T$|_, "
    "\\$| - <&|[, P$|\\, J$|], ^$|^, T$|_, \\$| - >&| H) P$| $ *#|\"H) 0&|#H"
    ")  &|!~$$ N$|!~!~!~!~)$ R$|!~!~!~.H) P\"| $ *#|!~!~!~$$ N$|!~!~!~6$ *!|"
    ")$ R$|!~!~!~<H)  2|=H)  3|>H) @2|?H) P3|@H) 02|AH) P2|BH) 03|CH) @3|!~6"
    "$ R(|!~F$ \"%|G$  %|!~!~ $ *#|!~!~!~$$ N$|!~!~!~!~)$ R$|F$ \"%|G$  %|!~"
    "!~!~Z- :|!~[, P$|\\, J$|], ^$|^, T$|_, B&|6$ *!|!~!~!~!~!~!~!~!~[, P$|\\"
    ", J$|], T(|^, T$|_, B&| $ *#|!~F$ \"%|G$  %|$$ N$|!~ $ *#|!~!~)$ R$|$$ "
    "N$|!~!~!~!~)$ R$|!~!~!~!~!~!~6$ *!|[, P$|\\, J$|], ^$|^, T$|_, @&|6$ *!"
    "|!~!~ (H 09|!(H 0B|!~!~$(H 04|!~&(H @;|F$ \"%|G$  %|!~!~!~,(H  5|F$ \"%"
    "|G$  %|/(H  ;|0(H 0;|!~!~3(H  J|!~!~!~!~!~!~!~!~[, P$|\\, J$|], ^$|^, D"
    "&|!~!~[, P$|\\, J$|], ^$|^, F&| (9 P$|!(9 @=|\"(9 0&|#(9  &|$(9 @-|%(9 "
    "PW|!~'(9 @5|((9 @.| (8 P$|!(8 @=|\"(8 0&|#(8  &|$(8 @-|%(8 PW|!~'(8 @5|"
    "((8 @.| H8 P$|!H8 @=|\"H8 0&|#H8  &|$H8 @-|%H8 PW|!~'H8 @5|(H8 @.| (5 P"
    "$| (& P$|\"(5 0&|#(5  &|!~%(5 PW|!~ H& P$|($ T!|D(9 0-|*(5 @1|+(5 01|!~"
    " (L 09|!(L 0B|.(& P\"|!~$(L 04|D(8 0-|&(L @;|!~.H& P\"|!~!~!~,(L  5|!~D"
    "H8 0-|/(L  ;|0(L 0;|!~!~3(L  J|!~!~!~E- 4$|!~!~!~!~H(& P!|!~!~K(& @$|L("
    "& P#|M(&  $|HH& P!|O(& 0$|P(& @#|KH& @$|LH& P#|MH&  $|!~OH& 0$|PH& @#| "
    "(M 09|!(M 0B|!~!~$(M 04|!~&(M @;|!~!~!~!~!~,(M  5|!~!~/(M  ;|0(M 0;| (N"
    " 09|!(N 0B|3(M  J|!~$(N 04|!~&(N @;|!~!~2- N#|!~!~,(N  5|!~!~/(N  ;|0(N"
    " 0;| (I 09|!(I 0B|3(N  J|!~$(I 04|!~&(I @;|!~!~!~!~!~,(I  5|!~!~/(I  ;|"
    "0(I 0;|!~!~3(I  J| HC P$|!HC @=|\"HC 0&|#HC  &|!~%HC PW| H# P$|!~(HC  9"
    "|!~*HC @1|+HC 01| HF 09|!HF 0B|!~!~$HF 04|!~&HF @;|\"$ T#|.H# P\"|!~ (%"
    " P$|!~,HF  5|!~)$ H#|/HF  ;|0HF 0;| HM 09|!HM 0B|3HF  J|!~$HM 04|!~&HM "
    "@;|.(% P\"|!~ H$ P$|6$ >#|!~,HM  5|!~!~/HM  ;|0HM 0;|HH# P!|!~3HM  J|KH"
    "# @$|LH# P#|MH#  $|.H$ P\"|OH# 0$|PH# @#|!~!~!~!~!~!~!~H(% P!|!~!~K(% @"
    "$|L(% P#|M(%  $|S$ V#|O(% 0$|P(% @#|!~!~!~!~!~!~!~HH$ P!|!~!~KH$ @$|LH$"
    " P#|MH$  $| H% P$|OH$ 0$|PH$ @#| HL 09|!HL 0B|(- 8$|!~$HL 04|!~&HL @;|-"
    "- >$|!~!~!~.H% P\"|,HL  5|!~4- @$|/HL  ;|0HL 0;| HK 09|!HK 0B|3HL  J|!~"
    "$HK 04|!~&HK @;|!~!~!~!~!~,HK  5|!~!~/HK  ;|0HK 0;|!~!~3HK  J|HH% P!|!~"
    "!~KH% @$|LH% P#|MH%  $|!~OH% 0$|PH% @#| (K 09|!(K 0B|!~!~$(K 04|!~&(K @"
    ";|!~!~!~!~!~,(K  5|!~!~/(K  ;|0(K 0;|!~!~3(K  J|!~!~!~!~!~!~!~!~!~!~!~!"
    "~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~"
    "!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!"
    "~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~}8|!|}9|"
    "|}:||};|_#|}<||}=|(|}>|'|}?||}@|+|}A|_#|}B||}C|3|}D|_'|}E||}F|<|}G|_'|}"
    "H|Null||Halt|!|Label|\"|Call|#|ScanStart|$|ScanChar|%|ScanAccept|&|Scan"
    "Token|'|ScanError|(|AstStart|)|AstFinish|*|AstNew|+|AstForm|,|AstLoad|-"
    "|AstIndex|.|AstChild|/|AstChildSlice|0|AstKind|1|AstKindNum|2|AstLocati"
    "on|3|AstLocationNum|4|AstLexeme|5|AstLexemeString|6|Assign|7|DumpStack|"
    "8|Add|9|Subtract|:|Multiply|;|Divide|<|UnaryMinus|=|Return|>|Branch|?|B"
    "ranchEqual|@|BranchNotEqual|A|BranchLessThan|B|BranchLessEqual|C|Branch"
    "GreaterThan|D|BranchGreaterEqual|E|}I|37|}J|G1!|}K|7|!~|'||>|!~\"|$|!~\""
    "|%|!~\"|P|)|-|$|@|@|$|A|A|&|B|B|(|D|D|.|E|E|0|F|F|2|G|G|4|H|H|?|I|I|A|J"
    "|J|C|K|K|E|L|L|G|M|M|I|N|N|K|O|O|M|P|Y|W|Z|Z|Y|[|[|\"!|\\|\\|$!|]|]|/!|"
    "^|^|3!|_|_|9!| !| !|;!|!!|:!|=!|;!|;!|?!|=!|=!|A!|?!|?!|C!|A!|A!|E!|B!|"
    "B!|=!|C!|C!|Q!|D!|D!|=\"|E!|E!|%#|F!|F!|I#|G!|H!|=!|I!|I!|S#|J!|J!|=!|K"
    "!|K!|_#|L!|L!|=$|M!|N!|=!|O!|O!|[$|P!|P!|)%|Q!|Q!|=!|R!|R!|=%|S!|S!|=!|"
    "T!|T!|O%|U!|Z!|=!|\\!|\\!|;&|&|$%|3\"|$\"|\"|%|!~5\"|\"|)|-|$|@|@|$|&|B"
    "F\"|<\"|G|=&|%|!~>\"||%|!~?\"|'||)|(|*|*|)|+|A|(|B|B|+|C|;!|(|<!|<!|-|="
    "!|_____#|(|&|W,|U\"|^!|@&|%|!~W\"||&|E*|X\"|\"|=&|%|!~Z\"||%|!~[\"|\"||"
    ")|(|+|_____#|(|&|RQ!|\"#|,|=&|%|!~$#||&|KX!|%#|/|=&|%|!~'#||&|8]!|(#|&|"
    "=&|%|!~*#||%|!~+#|'||)|5|*|*|)|+|F|5|G|G|7|H|;!|5|<!|<!|6|=!|_____#|5|%"
    "|!~A#|'||)|5|*|*|)|+|F|5|G|G|+|H|;!|5|<!|<!|6|=!|_____#|5|%|!~W#|\"||)|"
    "5|+|_____#|5|&|E*|^#|\"|=&|%|!~ $|!|G|G|9|&|D1|$$|_!|B&|%|!~&$|#||F|9|G"
    "|G|;|H|_____#|9|%|!~0$|#||F|9|G|G|<|H|_____#|9|%|!~:$|#||F|9|G|G|=|H|__"
    "___#|9|&|90|D$|S|=&|%|!~F$||&|<:!|G$|$|=&|%|!~I$||&|Z:!|J$|!|=&|%|!~L$|"
    "|&|'3!|M$|4|=&|%|!~O$||&|O4!|P$|1|=&|%|!~R$||&|-O!|S$|3|=&|%|!~U$||&|>)"
    "\"|V$|)|=&|%|!~X$||&|#R!|Y$|Q|=&|%|!~[$||&|VC\"|\\$|E|=&|%|!~^$|#|J|J|O"
    "|O|O|S|]|]|U|%|!~(%|#||I|O|J|J|P|K|_____#|O|%|!~2%|%||I|O|J|J|P|K|N|O|O"
    "|O|Q|P|_____#|O|&|>#|B%|#\"|\"|%|!~D%||&|>#|E%|#\"|\"|%|!~G%|\"||)|S|+|"
    "_____#|S|&|K8\"|N%|9|=&|%|!~P%||&|I&|Q%|6|=&|%|!~S%|!|P|Y|W|&|:O|W%|(|="
    "&|%|!~Y%|\"|Z|Z|[|]|]| !|%|!~ &|\"|]|]|\\|>!|>!|^|&|=(!|'&|U|=&|%|!~)&|"
    "|&|L<!|*&|T|=&|%|!~,&||&|\\0\"|-&|X|=&|%|!~/&||&|21\"|0&|5|=&|%|!~2&||@"
    "|<6|3&|&!|&||&|[5|6&|7|=&|%|!~8&|$|\\|\\|'!|]|]|)!|!!|:!|+!|A!|Z!|+!|&|"
    ">A!|E&|+|=&|%|!~G&||&|=;\"|H&|;|=&|%|!~J&||%|!~K&|%|P|Y|+!|^|^|,!|!!|:!"
    "|+!|?!|?!|+!|A!|Z!|+!|A|P)|[&|.!|&||&|.)|^&|#|=&|%|!~ '||&|GA|!'|2|=&|%"
    "|!~#'|!|^|^|1!|&|FY|''|Y|=&|%|!~)'||&|W<\"|*'|8|=&|%|!~,'|\"|]|]|5!|^|^"
    "|7!|&|3>\"|3'|:|=&|%|!~5'||&|TB!|6'|*|=&|%|!~8'||&|66!|9'|I|=&|%|!~;'||"
    "&|+Z!|<'|0|=&|%|!~>'||&|>'|?'||=&|%|!~A'|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|"
    "Z!|=!|&|'4|N'|J|D&|%|!~P'||&|!5|Q'|-|H&|%|!~S'||&|?#\"|T'|N|=&|%|!~V'||"
    "&|>'|W'||=&|%|!~Y'|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|B!|=!|C!|C!|G!|D!|Z!|="
    "!|&|>'|,(||=&|%|!~.(|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|I!|U!|Z!"
    "|=!|&|>'|A(||=&|%|!~C(|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|H!|=!|I!|I!|K!|J!|"
    "Z!|=!|&|>'|V(||=&|%|!~X(|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|N!|=!|O!|O!|M!|P"
    "!|Z!|=!|&|>'|+)||=&|%|!~-)|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|N!|O!"
    "|O!|Z!|=!|&|A^|@)|=|=&|%|!~B)|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|>'|"
    "O)||=&|%|!~Q)|'|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|A!|S!|B!|N!|=!|O!|O!|-\"|P!"
    "|Z!|=!|&|>'|'*||=&|%|!~)*|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R!|=!|S!|S!|U!|"
    "T!|Z!|=!|&|>'|<*||=&|%|!~>*|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|W"
    "!|F!|Z!|=!|&|>'|Q*||=&|%|!~S*|$|P|Y|=!|!!|:!|=!|?!|?!|Y!|A!|Z!|=!|&|>'|"
    " +||=&|%|!~\"+|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R!|=!|S!|S!|[!|T!|Z!|=!|&|"
    ">'|5+||=&|%|!~7+|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|]!|F!|Z!|=!|"
    "&|>'|J+||=&|%|!~L+|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|N!|_!|O!|Z!|="
    "!|&|>'|_+||=&|%|!~!,|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R!|=!|S!|S!|!\"|T!|Z"
    "!|=!|&|>'|4,||=&|%|!~6,|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|H!|=!|I!|I!|#\"|J"
    "!|Z!|=!|&|>'|I,||=&|%|!~K,|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|%\""
    "|U!|Z!|=!|&|>'|^,||=&|%|!~ -|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|H!|=!|I!|I!|"
    "'\"|J!|Z!|=!|&|>'|3-||=&|%|!~5-|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|U!|=!|V!|"
    "V!|)\"|W!|Z!|=!|&|>'|H-||=&|%|!~J-|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|"
    "E!|E!|+\"|F!|Z!|=!|&|HH|]-|K|=&|%|!~_-|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!"
    "|=!|&|>'|,.||=&|%|!~..|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|N!|/\"|O!"
    "|Z!|=!|&|>'|A.||=&|%|!~C.|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|E!|=!|F!|F!|1\""
    "|G!|Z!|=!|&|>'|V.||=&|%|!~X.|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|K!|=!|L!|L!|"
    "3\"|M!|Z!|=!|&|>'|+/||=&|%|!~-/|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|H!|=!|I!|"
    "I!|5\"|J!|Z!|=!|&|>'|@/||=&|%|!~B/|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|B!|=!|"
    "C!|C!|7\"|D!|Z!|=!|&|>'|U/||=&|%|!~W/|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|"
    "=!|T!|T!|9\"|U!|Z!|=!|&|>'|*0||=&|%|!~,0|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|"
    "R!|=!|S!|S!|;\"|T!|Z!|=!|&|*C|?0|L|=&|%|!~A0|$|P|Y|=!|!!|:!|=!|?!|?!|=!"
    "|A!|Z!|=!|&|>'|N0||=&|%|!~P0|(|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|"
    "?\"|F!|T!|=!|U!|U!|S\"|V!|Z!|=!|&|>'|)1||=&|%|!~+1|&|P|Y|=!|!!|:!|=!|?!"
    "|?!|=!|A!|R!|=!|S!|S!|A\"|T!|Z!|=!|&|>'|>1||=&|%|!~@1|&|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|B!|=!|C!|C!|C\"|D!|Z!|=!|&|>'|S1||=&|%|!~U1|&|P|Y|=!|!!|:!"
    "|=!|?!|?!|=!|A!|Q!|=!|R!|R!|E\"|S!|Z!|=!|&|>'|(2||=&|%|!~*2|&|P|Y|=!|!!"
    "|:!|=!|?!|?!|=!|A!|H!|=!|I!|I!|G\"|J!|Z!|=!|&|>'|=2||=&|%|!~?2|&|P|Y|=!"
    "|!!|:!|=!|?!|?!|=!|A!|O!|=!|P!|P!|I\"|Q!|Z!|=!|&|>'|R2||=&|%|!~T2|&|P|Y"
    "|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|K\"|U!|Z!|=!|&|>'|'3||=&|%|!~)3|&|"
    "P|Y|=!|!!|:!|=!|?!|?!|=!|A!|H!|=!|I!|I!|M\"|J!|Z!|=!|&|>'|<3||=&|%|!~>3"
    "|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|N!|=!|O!|O!|O\"|P!|Z!|=!|&|>'|Q3||=&|%|!"
    "~S3|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|N!|Q\"|O!|Z!|=!|&|6S|&4|@|=&"
    "|%|!~(4|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|>'|54||=&|%|!~74|&|P|Y|=!"
    "|!!|:!|=!|?!|?!|=!|A!|L!|=!|M!|M!|U\"|N!|Z!|=!|&|>'|J4||=&|%|!~L4|&|P|Y"
    "|=!|!!|:!|=!|?!|?!|=!|A!|O!|=!|P!|P!|W\"|Q!|Z!|=!|&|>'|_4||=&|%|!~!5|$|"
    "P|Y|=!|!!|:!|=!|?!|?!|Y\"|A!|Z!|=!|&|>'|.5||=&|%|!~05|&|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|R!|=!|S!|S!|[\"|T!|Z!|=!|&|>'|C5||=&|%|!~E5|&|P|Y|=!|!!|:!"
    "|=!|?!|?!|=!|A!|S!|=!|T!|T!|]\"|U!|Z!|=!|&|>'|X5||=&|%|!~Z5|%|P|Y|=!|!!"
    "|:!|=!|?!|?!|=!|A!|A!|_\"|B!|Z!|=!|&|>'|*6||=&|%|!~,6|&|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|B!|=!|C!|C!|!#|D!|Z!|=!|&|>'|?6||=&|%|!~A6|&|P|Y|=!|!!|:!|"
    "=!|?!|?!|=!|A!|J!|=!|K!|K!|##|L!|Z!|=!|&|R2\"|T6|R|=&|%|!~V6|$|P|Y|=!|!"
    "!|:!|=!|?!|?!|=!|A!|Z!|=!|&|>'|#7||=&|%|!~%7|(|P|Y|=!|!!|:!|=!|?!|?!|=!"
    "|A!|L!|=!|M!|M!|'#|N!|Q!|=!|R!|R!|/#|S!|Z!|=!|&|>'|>7||=&|%|!~@7|&|P|Y|"
    "=!|!!|:!|=!|?!|?!|=!|A!|O!|=!|P!|P!|)#|Q!|Z!|=!|&|>'|S7||=&|%|!~U7|&|P|"
    "Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|+#|U!|Z!|=!|&|>'|(8||=&|%|!~*8|&|"
    "P|Y|=!|!!|:!|=!|?!|?!|=!|A!|X!|=!|Y!|Y!|-#|Z!|Z!|=!|&|+9!|=8|D|=&|%|!~?"
    "8|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|>'|L8||=&|%|!~N8|&|P|Y|=!|!!|:!"
    "|=!|?!|?!|=!|A!|Q!|=!|R!|R!|1#|S!|Z!|=!|&|>'|!9||=&|%|!~#9|&|P|Y|=!|!!|"
    ":!|=!|?!|?!|=!|A!|N!|=!|O!|O!|3#|P!|Z!|=!|&|>'|69||=&|%|!~89|&|P|Y|=!|!"
    "!|:!|=!|?!|?!|=!|A!|Q!|=!|R!|R!|5#|S!|Z!|=!|&|W#!|K9|?|=&|%|!~M9|$|P|Y|"
    "=!|!!|:!|=!|?!|?!|7#|A!|Z!|=!|&|>'|Z9||=&|%|!~\\9|&|P|Y|=!|!!|:!|=!|?!|"
    "?!|=!|A!|Q!|=!|R!|R!|9#|S!|Z!|=!|&|>'|/:||=&|%|!~1:|&|P|Y|=!|!!|:!|=!|?"
    "!|?!|=!|A!|D!|=!|E!|E!|;#|F!|Z!|=!|&|>'|D:||=&|%|!~F:|&|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|B!|=!|C!|C!|=#|D!|Z!|=!|&|>'|Y:||=&|%|!~[:|&|P|Y|=!|!!|:!|"
    "=!|?!|?!|=!|A!|N!|=!|O!|O!|?#|P!|Z!|=!|&|>'|.;||=&|%|!~0;|&|P|Y|=!|!!|:"
    "!|=!|?!|?!|=!|A!|U!|=!|V!|V!|A#|W!|Z!|=!|&|>'|C;||=&|%|!~E;|&|P|Y|=!|!!"
    "|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|C#|F!|Z!|=!|&|>'|X;||=&|%|!~Z;|&|P|Y|=!|"
    "!!|:!|=!|?!|?!|=!|A!|Q!|=!|R!|R!|E#|S!|Z!|=!|&|>'|-<||=&|%|!~/<|&|P|Y|="
    "!|!!|:!|=!|?!|?!|=!|A!|X!|=!|Y!|Y!|G#|Z!|Z!|=!|&|XD|B<|M|=&|%|!~D<|$|P|"
    "Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|>'|Q<||=&|%|!~S<|%|P|Y|=!|!!|:!|=!|?!"
    "|?!|=!|A!|A!|K#|B!|Z!|=!|&|>'|#=||=&|%|!~%=|&|P|Y|=!|!!|:!|=!|?!|?!|=!|"
    "A!|K!|=!|L!|L!|M#|M!|Z!|=!|&|>'|8=||=&|%|!~:=|&|P|Y|=!|!!|:!|=!|?!|?!|="
    "!|A!|R!|=!|S!|S!|O#|T!|Z!|=!|&|>'|M=||=&|%|!~O=|&|P|Y|=!|!!|:!|=!|?!|?!"
    "|=!|A!|D!|=!|E!|E!|Q#|F!|Z!|=!|&|+U\"|\">|W|=&|%|!~$>|$|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|Z!|=!|&|>'|1>||=&|%|!~3>|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|F!|"
    "=!|G!|G!|U#|H!|Z!|=!|&|>'|F>||=&|%|!~H>|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|M"
    "!|=!|N!|N!|W#|O!|Z!|=!|&|>'|[>||=&|%|!~]>|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!"
    "|N!|=!|O!|O!|Y#|P!|Z!|=!|&|>'|0?||=&|%|!~2?|&|P|Y|=!|!!|:!|=!|?!|?!|=!|"
    "A!|Q!|=!|R!|R!|[#|S!|Z!|=!|&|>'|E?||=&|%|!~G?|&|P|Y|=!|!!|:!|=!|?!|?!|="
    "!|A!|D!|=!|E!|E!|]#|F!|Z!|=!|&|*\"!|Z?|C|=&|%|!~\\?|$|P|Y|=!|!!|:!|=!|?"
    "!|?!|=!|A!|Z!|=!|&|>'|)@||=&|%|!~+@|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!"
    "|E!|E!|!$|F!|Z!|=!|&|>'|>@||=&|%|!~@@|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|"
    "=!|E!|E!|#$|F!|Z!|=!|&|>'|S@||=&|%|!~U@|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|O"
    "!|=!|P!|P!|%$|Q!|Z!|=!|&|>'|(A||=&|%|!~*A|$|P|Y|=!|!!|:!|=!|?!|?!|'$|A!"
    "|Z!|=!|&|>'|7A||=&|%|!~9A|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|V!|=!|W!|W!|)$|"
    "X!|Z!|=!|&|>'|LA||=&|%|!~NA|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|G!|=!|H!|H!|+"
    "$|I!|Z!|=!|&|>'|!B||=&|%|!~#B|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|H!|=!|I!|I!"
    "|-$|J!|Z!|=!|&|>'|6B||=&|%|!~8B|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|"
    "T!|/$|U!|Z!|=!|&|>'|KB||=&|%|!~MB|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E"
    "!|E!|1$|F!|Z!|=!|&|>'| C||=&|%|!~\"C|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R!|="
    "!|S!|S!|3$|T!|Z!|=!|&|>'|5C||=&|%|!~7C|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|O!"
    "|=!|P!|P!|5$|Q!|Z!|=!|&|>'|JC||=&|%|!~LC|%|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|"
    "A!|7$|B!|Z!|=!|&|>'|\\C||=&|%|!~^C|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|B!|=!|"
    "C!|C!|9$|D!|Z!|=!|&|>'|1D||=&|%|!~3D|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|="
    "!|E!|E!|;$|F!|Z!|=!|&|OF|FD|O|=&|%|!~HD|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z"
    "!|=!|&|>'|UD||=&|%|!~WD|(|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|?$|F!"
    "|N!|=!|O!|O!|I$|P!|Z!|=!|&|>'|0E||=&|%|!~2E|&|P|Y|=!|!!|:!|=!|?!|?!|=!|"
    "A!|W!|=!|X!|X!|A$|Y!|Z!|=!|&|>'|EE||=&|%|!~GE|&|P|Y|=!|!!|:!|=!|?!|?!|="
    "!|A!|D!|=!|E!|E!|C$|F!|Z!|=!|&|>'|ZE||=&|%|!~\\E|&|P|Y|=!|!!|:!|=!|?!|?"
    "!|=!|A!|L!|=!|M!|M!|E$|N!|Z!|=!|&|>'|/F||=&|%|!~1F|&|P|Y|=!|!!|:!|=!|?!"
    "|?!|=!|A!|D!|=!|E!|E!|G$|F!|Z!|=!|&|= !|DF|B|=&|%|!~FF|$|P|Y|=!|!!|:!|="
    "!|?!|?!|=!|A!|Z!|=!|&|>'|SF||=&|%|!~UF|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|N!"
    "|=!|O!|O!|K$|P!|Z!|=!|&|>'|(G||=&|%|!~*G|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|"
    "J!|=!|K!|K!|M$|L!|Z!|=!|&|>'|=G||=&|%|!~?G|%|P|Y|=!|!!|:!|=!|?!|?!|=!|A"
    "!|A!|O$|B!|Z!|=!|&|>'|OG||=&|%|!~QG|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|G!|=!"
    "|H!|H!|Q$|I!|Z!|=!|&|>'|$H||=&|%|!~&H|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|"
    "=!|E!|E!|S$|F!|Z!|=!|&|>'|9H||=&|%|!~;H|%|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|A"
    "!|U$|B!|Z!|=!|&|>'|KH||=&|%|!~MH|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|C!|=!|D!"
    "|D!|W$|E!|Z!|=!|&|>'| I||=&|%|!~\"I|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R!|=!"
    "|S!|S!|Y$|T!|Z!|=!|&|:A|5I|P|=&|%|!~7I|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!"
    "|=!|&|>'|DI||=&|%|!~FI|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|O!|=!|P!|P!|]$|Q!|"
    "Z!|=!|&|>'|YI||=&|%|!~[I|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|_$|U"
    "!|Z!|=!|&|>'|.J||=&|%|!~0J|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|H!|=!|I!|I!|!%"
    "|J!|Z!|=!|&|>'|CJ||=&|%|!~EJ|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|N!|=!|O!|O!|"
    "#%|P!|Z!|=!|&|>'|XJ||=&|%|!~ZJ|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|N"
    "!|%%|O!|Z!|=!|&|>'|-K||=&|%|!~/K|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R!|=!|S!"
    "|S!|'%|T!|Z!|=!|&|P?|BK|Z|=&|%|!~DK|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!"
    "|&|>'|QK||=&|%|!~SK|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Q!|=!|R!|R!|+%|S!|Z!|"
    "=!|&|>'|&L||=&|%|!~(L|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|-%|F!|Z"
    "!|=!|&|>'|;L||=&|%|!~=L|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|B!|=!|C!|C!|/%|D!"
    "|Z!|=!|&|>'|PL||=&|%|!~RL|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|1%|"
    "F!|Z!|=!|&|>'|%M||=&|%|!~'M|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|C!|=!|D!|D!|3"
    "%|E!|Z!|=!|&|>'|:M||=&|%|!~<M|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!"
    "|5%|F!|Z!|=!|&|>'|OM||=&|%|!~QM|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|"
    "N!|7%|O!|Z!|=!|&|>'|$N||=&|%|!~&N|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|B!|=!|C"
    "!|C!|9%|D!|Z!|=!|&|>'|9N||=&|%|!~;N|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!"
    "|E!|E!|;%|F!|Z!|=!|&|L\\|NN|A|=&|%|!~PN|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z"
    "!|=!|&|>'|]N||=&|%|!~_N|(|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|?%|F!"
    "|T!|=!|U!|U!|G%|V!|Z!|=!|&|>'|8O||=&|%|!~:O|&|P|Y|=!|!!|:!|=!|?!|?!|=!|"
    "A!|F!|=!|G!|G!|A%|H!|Z!|=!|&|>'|MO||=&|%|!~OO|&|P|Y|=!|!!|:!|=!|?!|?!|="
    "!|A!|D!|=!|E!|E!|C%|F!|Z!|=!|&|>'|\"P||=&|%|!~$P|&|P|Y|=!|!!|:!|=!|?!|?"
    "!|=!|A!|W!|=!|X!|X!|E%|Y!|Z!|=!|&|,U|7P|>|=&|%|!~9P|$|P|Y|=!|!!|:!|=!|?"
    "!|?!|=!|A!|Z!|=!|&|>'|FP||=&|%|!~HP|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|K!|=!"
    "|L!|L!|I%|M!|Z!|=!|&|>'|[P||=&|%|!~]P|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|"
    "=!|E!|E!|K%|F!|Z!|=!|&|>'|0Q||=&|%|!~2Q|&|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|R"
    "!|=!|S!|S!|M%|T!|Z!|=!|&|G&!|EQ|.|=&|%|!~GQ|$|P|Y|=!|!!|:!|=!|?!|?!|=!|"
    "A!|Z!|=!|&|>'|TQ||=&|%|!~VQ|*|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|Q"
    "%|F!|N!|=!|O!|O!|_%|P!|Q!|=!|R!|R!|5&|S!|Z!|=!|&|>'|5R||=&|%|!~7R|&|P|Y"
    "|=!|!!|:!|=!|?!|?!|=!|A!|L!|=!|M!|M!|S%|N!|Z!|=!|&|>'|JR||=&|%|!~LR|&|P"
    "|Y|=!|!!|:!|=!|?!|?!|=!|A!|O!|=!|P!|P!|U%|Q!|Z!|=!|&|>'|_R||=&|%|!~!S|&"
    "|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|K!|=!|L!|L!|W%|M!|Z!|=!|&|>'|4S||=&|%|!~6S"
    "|%|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|A!|Y%|B!|Z!|=!|&|>'|FS||=&|%|!~HS|&|P|Y|"
    "=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|[%|U!|Z!|=!|&|>'|[S||=&|%|!~]S|&|P|"
    "Y|=!|!!|:!|=!|?!|?!|=!|A!|D!|=!|E!|E!|]%|F!|Z!|=!|&|CQ|0T|<|=&|%|!~2T|$"
    "|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|>'|?T||=&|%|!~AT|&|P|Y|=!|!!|:!|=!"
    "|?!|?!|=!|A!|J!|=!|K!|K!|!&|L!|Z!|=!|&|>'|TT||=&|%|!~VT|&|P|Y|=!|!!|:!|"
    "=!|?!|?!|=!|A!|D!|=!|E!|E!|#&|F!|Z!|=!|&|>'|)U||=&|%|!~+U|&|P|Y|=!|!!|:"
    "!|=!|?!|?!|=!|A!|M!|=!|N!|N!|%&|O!|Z!|=!|&|>'|>U||=&|%|!~@U|&|P|Y|=!|!!"
    "|:!|=!|?!|?!|'&|A!|R!|=!|S!|S!|3&|T!|Z!|=!|&|>'|SU||=&|%|!~UU|&|P|Y|=!|"
    "!!|:!|=!|?!|?!|=!|A!|B!|=!|C!|C!|)&|D!|Z!|=!|&|>'|(V||=&|%|!~*V|&|P|Y|="
    "!|!!|:!|=!|?!|?!|=!|A!|N!|=!|O!|O!|+&|P!|Z!|=!|&|>'|=V||=&|%|!~?V|&|P|Y"
    "|=!|!!|:!|=!|?!|?!|=!|A!|T!|=!|U!|U!|-&|V!|Z!|=!|&|>'|RV||=&|%|!~TV|&|P"
    "|Y|=!|!!|:!|=!|?!|?!|=!|A!|M!|=!|N!|N!|/&|O!|Z!|=!|&|>'|'W||=&|%|!~)W|&"
    "|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|S!|=!|T!|T!|1&|U!|Z!|=!|&|GK\"|<W|F|=&|%|!"
    "~>W|$|P|Y|=!|!!|:!|=!|?!|?!|=!|A!|Z!|=!|&|GK|KW|H|=&|%|!~MW|$|P|Y|=!|!!"
    "|:!|=!|?!|?!|=!|A!|Z!|=!|&|>'|ZW||=&|%|!~\\W|&|P|Y|=!|!!|:!|=!|?!|?!|=!"
    "|A!|T!|=!|U!|U!|7&|V!|Z!|=!|&|>'|/X||=&|%|!~1X|&|P|Y|=!|!!|:!|=!|?!|?!|"
    "=!|A!|D!|=!|E!|E!|9&|F!|Z!|=!|&|\\S\"|DX|V|=&|%|!~FX|$|P|Y|=!|!!|:!|=!|"
    "?!|?!|=!|A!|Z!|=!|&|J/!|SX|'|=&|%|!~UX||'|!~VX|9|!~VX|'|'|!|>|!~YX|(|W,"
    "|YX|!|>|W,|ZX|(|D1|ZX|\"|>|D1|[X|7|R4|[X|&|!|'|'4|]X|9|'4|]X|'|'|!|>|'4"
    "| Y|7|L5| Y|&||'|!5|\"Y|9|!5|\"Y|'|'|!|>|!5|%Y|)|B:|%Y|\"|+|B:|&Y|#|+|5"
    ";|'Y|$|-|J;|(Y||\"|#~0|L;|+Y|||!~,|5;|.Y|\"|$|#|2|6;|1Y|#|-|F;|2Y||\"|#"
    "~3|E;|5Y||+|2<|6Y|$|-|F<|7Y||\"|\"~0|H<|:Y|||!~,|2<|=Y|\"|$|#|2|3<|@Y|$"
    "|-|B<|AY||\"|\"~3|A<|DY||+|.=|EY|$|-|A=|FY||\"|!~0|C=|IY|||!~,|.=|LY|\""
    "|$|#|2|/=|OY|%|-|==|PY||\"|!~3|<=|SY||,|B:|TY|\"|#|#|2|C:|WY|\"|-|P:|XY"
    "||\"|!~3|O:|[Y||*|B:|\\Y|#|>|&8|]Y|)|?8|]Y|$|+|?8|^Y|#|,|?8|_Y|$|#||2|?"
    "8|\"Z| \"|*|?8|#Z||>|?8|$Z|)|,9|$Z|\"|+|,9|%Z|#|,|,9|&Z|\"|#||2|,9|)Z| "
    "\"|*|,9|*Z||>|,9|+Z|)|D@|+Z|$|+|D@|,Z|#|-|U@|-Z||$|!~0|W@|0Z|||!~,|D@|3"
    "Z|$|#|\"|2|E@|6Z|#|*|D@|7Z|\"|>|7?|8Z|)|P?|8Z|\"|+|P?|9Z|#|-|P?|:Z||\"|"
    "\"~0|P?|=Z|||!~-|P?|@Z||\"|!~/|P?|CZ||,|P?|DZ|\"|#|\"|2|P?|GZ|_!|*|P?|H"
    "Z|\"|>|P?|IZ|)|P?|IZ|$|+|P?|JZ|#|,|P?|KZ|$|#||2|P?|NZ|_!|*|P?|OZ||>|P?|"
    "PZ|)|6B|PZ|\"|+|6B|QZ|#|-|GB|RZ||\"|!~/|FB|UZ||,|6B|VZ|\"|#|#|2|7B|YZ|&"
    "|*|6B|ZZ|#|>|!A|[Z|)|%D|[Z|$|+|%D|\\Z|#|-|5D|]Z||$|!~/|4D| [||,|%D|![|$"
    "|#|#|2|&D|$[|(|*|%D|%[|#|>|QB|&[|)|XE|&[|\"|+|XE|'[|#|-|,F|([||\"|!~/|+"
    "F|+[||,|XE|,[|\"|#|#|2|YE|/[|'|*|XE|0[|#|>|?D|1[|)|PG|1[|$|+|PG|2[|#|-|"
    "%H|3[||$|!~/|$H|6[||,|PG|7[|$|#|#|2|QG|:[|)|*|PG|;[|#|>|6F|<[|)|HI|<[|\""
    "|+|HI|=[|#|-|\\I|>[||\"|!~/|[I|A[||,|HI|B[|\"|#|#|2|II|E[|*|*|HI|F[|#|>"
    "|/H|G[|)|9L|G[|$|+|9L|H[|#|-|IL|I[||$|!~0|KL|L[|||!~,|9L|O[|$|#|\"|2|:L"
    "|R[|$|*|9L|S[|\"|>|.K|T[|)|GK|T[|\"|+|GK|U[|#|-|GK|V[||\"|\"~0|GK|Y[|||"
    "!~-|GK|\\[||\"|!~/|GK|_[||,|GK| \\|\"|#|\"|2|GK|#\\|_!|*|GK|$\\|\"|>|GK"
    "|%\\|)|GK|%\\|$|+|GK|&\\|#|,|GK|'\\|$|#||2|GK|*\\|_!|*|GK|+\\||>|GK|,\\"
    "|)|[M|,\\|\"|+|[M|-\\|#|-|2N|.\\||\"|!~/|1N|1\\||+|5N|2\\|$|,|5N|3\\|\""
    "|$|!|2|6N|6\\|,|,|[M|7\\|\"|#|!|2|\\M|:\\|+|*|[M|;\\|!|>|UL|<\\|)|)P|<\\"
    "|$|+|)P|=\\|#|-|@P|>\\||$|#~/|?P|A\\||+|CP|B\\|\"|-|]P|C\\||$|!~0|_P|F\\"
    "|||!~,|CP|I\\|$|\"|#|2|DP|L\\|,|-|YP|M\\||$|!~3|XP|P\\||,|)P|Q\\|$|#|#|"
    "2|*P|T\\|+|*|)P|U\\|#|>|RN|V\\|)|:O|V\\|\"|+|:O|W\\|#|-|:O|X\\||\"|\"~0"
    "|:O|[\\|||!~-|:O|^\\||\"|!~/|:O|!]||,|:O|\"]|\"|#|\"|2|:O|%]|_!|*|:O|&]"
    "|\"|>|:O|']|)|:O|']|$|+|:O|(]|#|,|:O|)]|$|#||2|:O|,]|_!|*|:O|-]||>|:O|."
    "]|)|?R|.]|\"|+|?R|/]|#|-|SR|0]||\"|!~/|RR|3]||,|?R|4]|\"|#|#|2|@R|7]|-|"
    "*|?R|8]|#|>|*Q|9]|)|2T|9]|$|+|2T|:]|#|-|IT|;]||$|!~/|HT|>]||,|2T|?]|$|#"
    "|#|2|3T|B]|.|*|2T|C]|#|>|]R|D]|)|\"V|D]|\"|+|\"V|E]|#|+|6V|F]|$|+|FV|G]"
    "|%|,|FV|H]|\"|%|#|2|GV|K]|!|4|PV|L]|!~-|YV|M]||\"|!~/|XV|P]||,|6V|Q]|\""
    "|$|#|2|7V|T]|0|,|\"V|U]|\"|#|#|2|#V|X]|/|*|\"V|Y]|#|>|ST|Z]|)|3X|Z]|%|+"
    "|3X|[]|$|-|HX|\\]||%|!~0|JX|_]|||!~,|3X|\"^|%|$|#|2|4X|%^|/|*|3X|&^|#|>"
    "|$W|'^|)|EW|'^|#|+|EW|(^|\"|-|EW|)^||#|\"~0|EW|,^|||!~-|EW|/^||#|!~/|EW"
    "|2^||,|EW|3^|#|\"|\"|2|EW|6^|_!|*|EW|7^|\"|>|EW|8^|)|EW|8^|$|+|EW|9^|%|"
    "-|EW|:^||$|!~/|EW|=^||,|EW|>^|$|%|!|2|EW|A^|_!|*|EW|B^|!|>|EW|C^|)|5Z|C"
    "^|\"|+|5Z|D^|#|-|FZ|E^||\"|$~/|EZ|H^||-|JZ|I^||\"|!~/|IZ|L^||,|5Z|M^|\""
    "|#|%|2|6Z|P^|0|*|5Z|Q^|%|>|TX|R^|)|W[|R^|%|+|W[|S^|$|-|)\\|T^||%|!~/|(\\"
    "|W^||,|W[|X^|%|$|!|2|X[|[^|U|*|W[|\\^|!|>|TZ|]^|)|H]|]^|#|+|H]|^^|\"|-|"
    "^]|_^||#|!~/|]]|\"_||,|H]|#_|#|\"|#|2|I]|&_|1|*|H]|'_|#|>|3\\|(_|)|H_|("
    "_|$|+|H_|)_|%|-|Z_|*_||$|\"~/|Y_|-_||,|H_|._|$|%|%|2|I_|1_|2|*|H_|2_|%|"
    ">|(^|3_|)|5!!|3_|\"|+|5!!|4_|#|-|G!!|5_||\"|!~/|F!!|8_||,|5!!|9_|\"|#|#"
    "|2|6!!|<_|3|*|5!!|=_|#|>|$ !|>_|)|\"#!|>_|%|+|\"#!|?_|$|-|4#!|@_||%|!~/"
    "|3#!|C_||,|\"#!|D_|%|$|#|2|##!|G_|4|*|\"#!|H_|#|>|Q!!|I_|)|M$!|I_|#|+|M"
    "$!|J_|\"|-|^$!|K_||#|!~/|]$!|N_||,|M$!|O_|#|\"|#|2|N$!|R_|5|*|M$!|S_|#|"
    ">|>#!|T_|)|4'!|T_|$|+|4'!|U_|%|-|C'!|V_||$|!~0|E'!|Y_|||!~,|4'!|\\_|$|%"
    "|\"|2|5'!|__|%|*|4'!|  !|\"|>|.&!|! !|)|G&!|! !|\"|+|G&!|\" !|#|,|G&!|#"
    " !|\"|#||2|G&!|& !| \"|*|G&!|' !||>|G&!|( !|)|.&!|( !|%|+|.&!|) !|$|-|."
    "&!|* !||%|\"~0|.&!|- !|||!~-|.&!|0 !||%|!~/|.&!|3 !||,|.&!|4 !|%|$|\"|2"
    "|.&!|7 !|_!|*|.&!|8 !|\"|>|.&!|9 !|)|.&!|9 !|#|+|.&!|: !|\"|-|.&!|; !||"
    "#|!~/|.&!|> !||,|.&!|? !|#|\"|!|2|.&!|B !|_!|*|.&!|C !|!|>|.&!|D !|)|;)"
    "!|D !|$|+|;)!|E !|%|-|J)!|F !||$|$~/|I)!|I !||-|N)!|J !||$|\"~/|M)!|M !"
    "||-|R)!|N !||$|!~.|R)!|Q !|||/|Q)!|S !||-|X)!|T !||$|!~.|X)!|W !||!|/|W"
    ")!|Y !||,|;)!|Z !|$|%|$|2|<)!|] !|6|-|F)!|^ !||$|#~3|E)!|!!!||*|;)!|\"!"
    "!|$|>|O'!|#!!|)|E+!|#!!|\"|+|E+!|$!!|#|-|G+!|%!!||\"|#~/|F+!|(!!||-|K+!"
    "|)!!||\"|!~/|J+!|,!!||,|E+!|-!!|\"|#|$|2|E+!|0!!|!\"|*|E+!|1!!|$|>|$*!|"
    "2!!|)| -!|2!!|%|+| -!|3!!|$|-|\"-!|4!!||%|!~/|!-!|7!!||+|%-!|8!!|#|,|%-"
    "!|9!!|%|#|\"|2|&-!|<!!|!|4|/-!|=!!|!~,| -!|>!!|%|$|\"|2| -!|A!!|!\"|*| "
    "-!|B!!|\"|>|U+!|C!!|)|;.!|C!!|\"|+|;.!|D!!|#|+|<.!|E!!|$|,|<.!|F!!|\"|$"
    "||2|=.!|I!!|!|4|F.!|J!!|!~+|N.!|K!!|%|,|N.!|L!!|\"|%||2|O.!|O!!|!|4|X.!"
    "|P!!|!~,|;.!|Q!!|\"|#||2|;.!|T!!|!\"|*|;.!|U!!||>|>-!|V!!|)|<0!|V!!|$|+"
    "|<0!|W!!|%|-|N0!|X!!||$|\"~/|M0!|[!!||-|R0!|\\!!||$|!~0|T0!|_!!|||!~,|<"
    "0!|\"\"!|$|%|\"|2|=0!|%\"!|7|*|<0!|&\"!|\"|>|'/!|'\"!|)|@/!|'\"!|#|+|@/"
    "!|(\"!|\"|-|@/!|)\"!||#|\"~0|@/!|,\"!|||!~-|@/!|/\"!||#|!~/|@/!|2\"!||,"
    "|@/!|3\"!|#|\"|\"|2|@/!|6\"!|_!|*|@/!|7\"!|\"|>|@/!|8\"!|)|X/!|8\"!|%|-"
    "|Y/!|9\"!|!|%|!~/|X/!|<\"!|!|*|X/!|=\"!|\"|>|!~>\"!|)|@/!|>\"!|$|+|@/!|"
    "?\"!|\"|,|@/!|@\"!|$|\"||2|@/!|C\"!|_!|*|@/!|D\"!||>|@/!|E\"!|)|'2!|E\""
    "!|#|+|'2!|F\"!|%|-|52!|G\"!|!|#|!~0|72!|J\"!|!||!~,|'2!|M\"!|#|%|!|2|(2"
    "!|P\"!|8|*|'2!|Q\"!|!|>|^0!|R\"!|)|^0!|R\"!|\"|+|^0!|S\"!|$|-|^0!|T\"!|"
    "!|\"|\"~0|^0!|W\"!|!||!~-|^0!|Z\"!|!|\"|!~/|^0!|]\"!|!|,|^0!|^\"!|\"|$|"
    "\"|2|^0!|!#!|_!|*|^0!|\"#!|\"|>|^0!|##!|)|^0!|##!|%|+|^0!|$#!|#|-|^0!|%"
    "#!|!|%|!~/|^0!|(#!|!|,|^0!|)#!|%|#|!|2|^0!|,#!|_!|*|^0!|-#!|!|>|^0!|.#!"
    "|)|I3!|.#!|$|+|I3!|/#!|\"|-|_3!|0#!|!|$|\"~/|^3!|3#!|!|,|I3!|4#!|$|\"|\""
    "|2|J3!|7#!|:|-|[3!|8#!|!|$|!~3|Z3!|;#!|!|*|I3!|<#!|\"|>|A2!|=#!|)|15!|="
    "#!|#|+|15!|>#!|%|-|F5!|?#!|!|#|\"~/|E5!|B#!|!|,|15!|C#!|#|%|\"|2|25!|F#"
    "!|;|-|B5!|G#!|!|#|!~3|A5!|J#!|!|*|15!|K#!|\"|>|)4!|L#!|)|X6!|L#!|\"|+|X"
    "6!|M#!|$|-|+7!|N#!|!|\"|\"~/|*7!|Q#!|!|,|X6!|R#!|\"|$|\"|2|Y6!|U#!|9|-|"
    "'7!|V#!|!|\"|!~3|&7!|Y#!|!|*|X6!|Z#!|\"|>|P5!|[#!|)|Q9!|[#!|%|+|Q9!|\\#"
    "!|#|,|Q9!|]#!|%|#|!|2|R9!| $!|F|*|Q9!|!$!|!|>|R8!|\"$!|)|<;!|\"$!|$|+|<"
    ";!|#$!|\"|-|H;!|$$!|!|$|#~/|G;!|'$!|!|-|L;!|($!|!|$|\"~.|L;!|+$!|!||/|K"
    ";!|-$!|!|-|R;!|.$!|!|$|\"~.|R;!|1$!|!|!|/|Q;!|3$!|!|,|<;!|4$!|$|\"|$|2|"
    "=;!|7$!|<|*|<;!|8$!|$|>|#:!|9$!|)|P=!|9$!|#|+|P=!|:$!|%|-|)>!|;$!|!|#|$"
    "~/|(>!|>$!|!|-|->!|?$!|!|#|\"~/|,>!|B$!|!|+|0>!|C$!|\"|-|M>!|D$!|!|#|!~"
    "0|O>!|G$!|!||!~,|0>!|J$!|#|\"|$|2|1>!|M$!|>|-|I>!|N$!|!|#|!~3|H>!|Q$!|!"
    "|,|P=!|R$!|#|%|$|2|Q=!|U$!|=|-|%>!|V$!|!|#|#~3|$>!|Y$!|!|*|P=!|Z$!|$|>|"
    "^;!|[$!|)|R<!|[$!|$|+|R<!|\\$!|\"|-|R<!|]$!|!|$|\"~0|R<!| %!|!||!~-|R<!"
    "|#%!|!|$|!~/|R<!|&%!|!|,|R<!|'%!|$|\"|\"|2|R<!|*%!|_!|*|R<!|+%!|\"|>|R<"
    "!|,%!|)|R<!|,%!|%|+|R<!|-%!|#|-|R<!|.%!|!|%|!~/|R<!|1%!|!|,|R<!|2%!|%|#"
    "|!|2|R<!|5%!|_!|*|R<!|6%!|!|>|R<!|7%!|)|!@!|7%!|\"|+|!@!|8%!|$|-|:@!|9%"
    "!|!|\"|\"~/|9@!|<%!|!|+|=@!|=%!|#|-|X@!|>%!|!|\"|!~0|Z@!|A%!|!||!~,|=@!"
    "|D%!|\"|#|\"|2|>@!|G%!|B|-|T@!|H%!|!|\"|!~3|S@!|K%!|!|,|!@!|L%!|\"|$|\""
    "|2|\"@!|O%!|?|*|!@!|P%!|\"|>|Z>!|Q%!|)|3?!|Q%!|%|+|3?!|R%!|#|-|3?!|S%!|"
    "!|%|\"~0|3?!|V%!|!||!~-|3?!|Y%!|!|%|!~/|3?!|\\%!|!|,|3?!|]%!|%|#|\"|2|3"
    "?!| &!|_!|*|3?!|!&!|\"|>|3?!|\"&!|)|3?!|\"&!|$|+|3?!|#&!|\"|-|3?!|$&!|!"
    "|$|!~/|3?!|'&!|!|,|3?!|(&!|$|\"|!|2|3?!|+&!|_!|*|3?!|,&!|!|>|3?!|-&!|)|"
    "!B!|-&!|#|+|!B!|.&!|%|,|!B!|/&!|#|%|!|2|\"B!|2&!|@|*|!B!|3&!|!|>|%A!|4&"
    "!|)|7C!|4&!|\"|+|7C!|5&!|$|,|7C!|6&!|\"|$|!|2|8C!|9&!|A|*|7C!|:&!|!|>|;"
    "B!|;&!|)|'E!|;&!|%|+|'E!|<&!|#|-|>E!|=&!|!|%|\"~/|=E!|@&!|!|-|BE!|A&!|!"
    "|%|!~.|BE!|D&!|!||/|AE!|F&!|!|-|HE!|G&!|!|%|!~.|HE!|J&!|!|!|/|GE!|L&!|!"
    "|,|'E!|M&!|%|#|\"|2|(E!|P&!|C|*|'E!|Q&!|\"|>|RC!|R&!|)|AH!|R&!|$|+|AH!|"
    "S&!|\"|-|YH!|T&!|!|$|!~/|XH!|W&!|!|,|AH!|X&!|$|\"|!|2|BH!|[&!|D|*|AH!|\\"
    "&!|!|>|;G!|]&!|)|,J!|]&!|#|+|,J!|^&!|%|-|GJ!|_&!|!|#|!~/|FJ!|\"'!|!|,|,"
    "J!|#'!|#|%|!|2|-J!|&'!|E|*|,J!|''!|!|>|#I!|('!|)|3N!|('!|\"|+|3N!|)'!|$"
    "|,|3N!|*'!|\"|$||2|4N!|-'!|!|*|3N!|.'!||>|6M!|/'!|)|4P!|/'!|%|+|4P!|0'!"
    "|#|-|DP!|1'!|!|%|\"~0|FP!|4'!|!||!~,|4P!|7'!|%|#|#|2|5P!|:'!|G|*|4P!|;'"
    "!|#|>|DN!|<'!|)|DO!|<'!|$|+|DO!|='!|\"|-|FO!|>'!|!|$|\"~/|EO!|A'!|!|-|J"
    "O!|B'!|!|$|!~0|LO!|E'!|!||!~,|DO!|H'!|$|\"|\"|2|DO!|K'!|\"\"|*|DO!|L'!|"
    "\"|>|!~M'!|)|#O!|M'!|#|+|#O!|N'!|%|-|#O!|O'!|!|#|\"~0|#O!|R'!|!||!~-|#O"
    "!|U'!|!|#|!~/|#O!|X'!|!|,|#O!|Y'!|#|%|\"|2|#O!|\\'!|_!|*|#O!|]'!|\"|>|#"
    "O!|^'!|)|<O!|^'!|\"|-|=O!|_'!|\"|\"|!~/|<O!|\"(!|\"|*|<O!|#(!|\"|>|!~$("
    "!|)|-O!|$(!|$|+|-O!|%(!|%|,|-O!|&(!|$|%||2|-O!|)(!| \"|*|-O!|*(!||>|-O!"
    "|+(!|)|#O!|+(!|#|+|#O!|,(!|\"|,|#O!|-(!|#|\"||2|#O!|0(!|_!|*|#O!|1(!||>"
    "|#O!|2(!|)|!~2(!|%|+|!~3(!|$|,|!~4(!|%|$||2|!~7(!| \"|*|!~8(!||>|!~9(!|"
    ")|RR!|9(!|\"|+|RR!|:(!|#|-|!S!|;(!|\"|\"|#~/| S!|>(!|\"|-|%S!|?(!|\"|\""
    "|!~/|$S!|B(!|\"|,|RR!|C(!|\"|#|$|2|SR!|F(!|I|*|RR!|G(!|$|>|9Q!|H(!|)|7T"
    "!|H(!|$|+|7T!|I(!|%|-|FT!|J(!|\"|$|!~/|ET!|M(!|\"|+|IT!|N(!|#|,|IT!|O(!"
    "|$|#|\"|2|JT!|R(!|!|4|ST!|S(!|!~,|7T!|T(!|$|%|\"|2|8T!|W(!|I|*|7T!|X(!|"
    "\"|>|/S!|Y(!|)|*V!|Y(!|\"|+|*V!|Z(!|#|+|8V!|[(!|%|,|8V!|\\(!|\"|%|\"|2|"
    "9V!|_(!|!|4|BV!| )!|!~-|KV!|!)!|\"|\"|!~/|JV!|$)!|\"|,|*V!|%)!|\"|#|\"|"
    "2|+V!|()!|I|*|*V!|))!|\"|>|\"U!|*)!|)|WW!|*)!|$|+|WW!|+)!|%|,|WW!|,)!|$"
    "|%|!|2|XW!|/)!|X|-|(X!|0)!|\"|$|!~5|'X!|3)!|\"|*|WW!|4)!|!|>|UV!|5)!|)|"
    ":Y!|5)!|#|+|:Y!|6)!|\"|-|HY!|7)!|\"|#|!~/|GY!|:)!|\"|,|:Y!|;)!|#|\"|\"|"
    "2|;Y!|>)!|J|*|:Y!|?)!|\"|>|2X!|@)!|)|ZZ!|@)!|%|+|ZZ!|A)!|$|-|,[!|B)!|\""
    "|%|!~/|+[!|E)!|\"|,|ZZ!|F)!|%|$|\"|2|[Z!|I)!|K|*|ZZ!|J)!|\"|>|RY!|K)!|)"
    "|=\\!|K)!|\"|+|=\\!|L)!|#|-|U\\!|M)!|\"|\"|!~/|T\\!|P)!|\"|,|=\\!|Q)!|\""
    "|#|\"|2|>\\!|T)!|L|*|=\\!|U)!|\"|>|6[!|V)!|)|'^!|V)!|$|+|'^!|W)!|%|-|7^"
    "!|X)!|\"|$|!~/|6^!|[)!|\"|,|'^!|\\)!|$|%|\"|2|(^!|_)!|M|*|'^!| *!|\"|>|"
    "_\\!|!*!|)|H_!|!*!|#|+|H_!|\"*!|\"|-|^_!|#*!|\"|#|!~/|]_!|&*!|\"|,|H_!|"
    "'*!|#|\"|\"|2|I_!|**!|N|*|H_!|+*!|\"|>|A^!|,*!|)|5\"\"|,*!|%|+|5\"\"|-*"
    "!|$|-|B\"\"|.*!|\"|%|\"~/|A\"\"|1*!|\"|-|F\"\"|2*!|\"|%|!~0|H\"\"|5*!|\""
    "||!~,|5\"\"|8*!|%|$|\"|2|6\"\"|;*!|P|*|5\"\"|<*!|\"|>|R \"|=*!|)|+!\"|="
    "*!|\"|+|+!\"|>*!|#|-|+!\"|?*!|\"|\"|\"~0|+!\"|B*!|\"||!~-|+!\"|E*!|\"|\""
    "|!~/|+!\"|H*!|\"|,|+!\"|I*!|\"|#|\"|2|+!\"|L*!|_!|*|+!\"|M*!|\"|>|+!\"|"
    "N*!|)|Q!\"|N*!|$|-|R!\"|O*!|#|$|!~/|Q!\"|R*!|#|*|Q!\"|S*!|\"|>|!~T*!|)|"
    "+!\"|T*!|%|+|+!\"|U*!|#|,|+!\"|V*!|%|#||2|+!\"|Y*!|_!|*|+!\"|Z*!||>|+!\""
    "|[*!|)|4$\"|[*!|\"|+|4$\"|\\*!|$|-|C$\"|]*!|#|\"|#~/|B$\"| +!|#|-|G$\"|"
    "!+!|#|\"|!~/|F$\"|$+!|#|,|4$\"|%+!|\"|$|#|2|5$\"|(+!|Q|*|4$\"|)+!|#|>|R"
    "\"\"|*+!|)|=&\"|*+!|#|+|=&\"|++!|%|,|=&\"|,+!|#|%||2|>&\"|/+!|Y|6|J&\"|"
    "0+!||*|=&\"|1+!||>|@%\"|2+!|)|C(\"|2+!|$|+|C(\"|3+!|\"|,|C(\"|4+!|$|\"|"
    "|2|D(\"|7+!|Z|6|X(\"|8+!||*|C(\"|9+!||>|F'\"|:+!|)|**\"|:+!|%|+|**\"|;+"
    "!|#|,|**\"|<+!|%|#|\"|2|+*\"|?+!|Z|-|@*\"|@+!|#|%|!~5|?*\"|C+!|#|*|**\""
    "|D+!|\"|>|%)\"|E+!|)|K+\"|E+!|\"|+|K+\"|F+!|$|,|K+\"|G+!|\"|$|!|2|L+\"|"
    "J+!|Y|-|Y+\"|K+!|#|\"|!~5|X+\"|N+!|#|*|K+\"|O+!|!|>|J*\"|P+!|)|6-\"|P+!"
    "|#|-|7-\"|Q+!|$|#|\"~/|6-\"|T+!|$|*|6-\"|U+!|#|>|#,\"|V+!|)|R/\"|V+!|%|"
    "+|R/\"|W+!|$|-|,0\"|X+!|$|%|!~0|.0\"|[+!|$||!~,|R/\"|^+!|%|$|!|2|S/\"|!"
    ",!|M!|*|R/\"|\",!|!|>|J.\"|#,!|)|J.\"|#,!|\"|+|J.\"|$,!|#|-|J.\"|%,!|$|"
    "\"|\"~0|J.\"|(,!|$||!~-|J.\"|+,!|$|\"|!~/|J.\"|.,!|$|,|J.\"|/,!|\"|#|\""
    "|2|J.\"|2,!|_!|*|J.\"|3,!|\"|>|J.\"|4,!|)|J.\"|4,!|$|+|J.\"|5,!|%|,|J.\""
    "|6,!|$|%||2|J.\"|9,!|_!|*|J.\"|:,!||>|J.\"|;,!|)|T1\"|;,!|#|+|T1\"|<,!|"
    "\"|-|+2\"|=,!|$|#|$~/|*2\"|@,!|$|-|/2\"|A,!|$|#|\"~/|.2\"|D,!|$|,|T1\"|"
    "E,!|#|\"|$|2|U1\"|H,!|N!|-|'2\"|I,!|$|#|#~3|&2\"|L,!|$|*|T1\"|M,!|$|>|8"
    "0\"|N,!|)|A3\"|N,!|%|+|A3\"|O,!|$|,|A3\"|P,!|%|$|\"|2|B3\"|S,!|]!|*|A3\""
    "|T,!|\"|>|92\"|U,!|)|U4\"|U,!|\"|+|U4\"|V,!|#|-|$5\"|W,!|$|\"|#~/|#5\"|"
    "Z,!|$|-|(5\"|[,!|$|\"|\"~/|'5\"|^,!|$|,|U4\"|_,!|\"|#|#|2|V4\"|\"-!|[!|"
    "*|U4\"|#-!|#|>|!~$-!|)|(6\"|$-!|$|+|(6\"|%-!|%|-|86\"|&-!|$|$|#~/|76\"|"
    ")-!|$|-|<6\"|*-!|$|$|\"~/|;6\"|--!|$|,|(6\"|.-!|$|%|#|2|)6\"|1-!|Z!|*|("
    "6\"|2-!|#|>|!~3-!|)|<7\"|3-!|#|+|<7\"|4-!|\"|-|N7\"|5-!|$|#|#~/|M7\"|8-"
    "!|$|-|R7\"|9-!|$|#|\"~/|Q7\"|<-!|$|,|<7\"|=-!|#|\"|#|2|=7\"|@-!|O!|*|<7"
    "\"|A-!|#|>|!~B-!|)|R8\"|B-!|%|+|R8\"|C-!|$|-|'9\"|D-!|$|%|#~/|&9\"|G-!|"
    "$|-|+9\"|H-!|$|%|\"~/|*9\"|K-!|$|,|R8\"|L-!|%|$|#|2|S8\"|O-!|P!|*|R8\"|"
    "P-!|#|>|!~Q-!|)|+:\"|Q-!|\"|+|+:\"|R-!|#|-|@:\"|S-!|$|\"|#~/|?:\"|V-!|$"
    "|-|D:\"|W-!|$|\"|\"~/|C:\"|Z-!|$|,|+:\"|[-!|\"|#|#|2|,:\"|^-!|Q!|*|+:\""
    "|_-!|#|>|!~ .!|)|D;\"| .!|$|+|D;\"|!.!|%|-|Z;\"|\".!|$|$|#~/|Y;\"|%.!|$"
    "|-|^;\"|&.!|$|$|\"~/|];\"|).!|$|,|D;\"|*.!|$|%|#|2|E;\"|-.!|R!|*|D;\"|."
    ".!|#|>|!~/.!|)|^<\"|/.!|#|+|^<\"|0.!|\"|-|6=\"|1.!|$|#|#~/|5=\"|4.!|$|-"
    "|:=\"|5.!|$|#|\"~/|9=\"|8.!|$|,|^<\"|9.!|#|\"|#|2|_<\"|<.!|S!|*|^<\"|=."
    "!|#|>|!~>.!|)|:>\"|>.!|%|+|:>\"|?.!|$|-|S>\"|@.!|$|%|#~/|R>\"|C.!|$|-|W"
    ">\"|D.!|$|%|\"~/|V>\"|G.!|$|,|:>\"|H.!|%|$|#|2|;>\"|K.!|T!|*|:>\"|L.!|#"
    "|>|!~M.!|)|W?\"|M.!|\"|+|W?\"|N.!|#|-|'@\"|O.!|$|\"|#~/|&@\"|R.!|$|-|+@"
    "\"|S.!|$|\"|\"~/|*@\"|V.!|$|,|W?\"|W.!|\"|#|#|2|X?\"|Z.!|U!|*|W?\"|[.!|"
    "#|>|!~\\.!|)|+A\"|\\.!|$|+|+A\"|].!|%|-|@A\"|^.!|$|$|#~/|?A\"|!/!|$|-|D"
    "A\"|\"/!|$|$|\"~/|CA\"|%/!|$|,|+A\"|&/!|$|%|#|2|,A\"|)/!|V!|*|+A\"|*/!|"
    "#|>|!~+/!|)|DB\"|+/!|#|+|DB\"|,/!|\"|-|YB\"|-/!|$|#|#~/|XB\"|0/!|$|-|]B"
    "\"|1/!|$|#|\"~/|\\B\"|4/!|$|,|DB\"|5/!|#|\"|#|2|EB\"|8/!|W!|*|DB\"|9/!|"
    "#|>|!~:/!|)|]C\"|:/!|%|+|]C\"|;/!|$|-|0D\"|</!|$|%|#~/|/D\"|?/!|$|-|4D\""
    "|@/!|$|%|\"~/|3D\"|C/!|$|,|]C\"|D/!|%|$|#|2|^C\"|G/!|X!|*|]C\"|H/!|#|>|"
    "!~I/!|)|HE\"|I/!|\"|+|HE\"|J/!|#|-|_E\"|K/!|$|\"|!~/|^E\"|N/!|$|,|HE\"|"
    "O/!|\"|#|\"|2|IE\"|R/!|Y!|*|HE\"|S/!|\"|>|>D\"|T/!|)|3G\"|T/!|$|+|3G\"|"
    "U/!|%|-|CG\"|V/!|$|$|!~/|BG\"|Y/!|$|,|3G\"|Z/!|$|%|\"|2|4G\"|]/!|\\!|*|"
    "3G\"|^/!|\"|>|)F\"|_/!|)|LI\"|_/!|#|-|MI\"| 0!|%|#|\"~/|LI\"|#0!|%|*|LI"
    "\"|$0!|#|>|<H\"|%0!|)|3L\"|%0!|\"|+|3L\"|&0!|%|,|3L\"|'0!|\"|%|!|2|4L\""
    "|*0!|^!|*|3L\"|+0!|!|>|.K\"|,0!|)|VN\"|,0!|$|+|VN\"|-0!|#|,|VN\"|.0!|$|"
    "#|!|2|WN\"|10!|[|-|#O\"|20!|%|$|!~5|\"O\"|50!|%|*|VN\"|60!|!|>|VM\"|70!"
    "|)|4P\"|70!|%|+|4P\"|80!|\"|,|4P\"|90!|%|\"|!|2|5P\"|<0!|[|-|AP\"|=0!|%"
    "|%|!~5|@P\"|@0!|%|*|4P\"|A0!|!|>|-O\"|B0!|)|OQ\"|B0!|#|+|OQ\"|C0!|$|,|O"
    "Q\"|D0!|#|$|!|2|PQ\"|G0!|X|-| R\"|H0!|%|#|!~5|_Q\"|K0!|%|*|OQ\"|L0!|!|>"
    "|KP\"|M0!|)|+S\"|M0!|\"|+|+S\"|N0!|%|,|+S\"|O0!|\"|%|!|2|,S\"|R0!|Y|-|9"
    "S\"|S0!|%|\"|!~5|8S\"|V0!|%|*|+S\"|W0!|!|>|*R\"|X0!|)|AT\"|X0!|$|+|AT\""
    "|Y0!|#|,|AT\"|Z0!|$|#|!|2|BT\"|]0!|]|*|AT\"|^0!|!|>|CS\"|_0!|)|QU\"|_0!"
    "|%|+|QU\"| 1!|\"|,|QU\"|!1!|%|\"|!|2|RU\"|$1!|^|*|QU\"|%1!|!|>|RT\"|&1!"
    "|)|#W\"|&1!|#|+|#W\"|'1!|$|,|#W\"|(1!|#|$|!|2|$W\"|+1!|[|-|0W\"|,1!|%|#"
    "|!~5|/W\"|/1!|%|*|#W\"|01!|!|>|#V\"|11!|)|@X\"|11!|\"|+|@X\"|21!|%|,|@X"
    "\"|31!|\"|%|!|2|AX\"|61!|\\|-|SX\"|71!|%|\"|!~5|RX\"|:1!|%|*|@X\"|;1!|!"
    "|>|:W\"|<1!|)|!Z\"|<1!|$|+|!Z\"|=1!|#|,|!Z\"|>1!|$|#|!|2|\"Z\"|A1!|X|-|"
    "2Z\"|B1!|%|$|!~5|1Z\"|E1!|%|*|!Z\"|F1!|!|>|]X\"|G1!|}L|(|}M||0|!|1||Tem"
    "p$0||Temp$1||Temp$2||Temp$3||in_guard||token_count|}N|&|}O|#|}P|1|Missi"
    "ng closing quote on string literal|Missing closing quote on triple quot"
    "ed string literal|}Q|}"
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
    "-{ *//{ -{ *{ -{ *options{ -{ *{ -{ *    lookaheads = 2{ -{ *    error_"
    "recovery = true{ -{ *    conflicts = 0{ -{ *    case_sensitive = true{ "
    "-{ *{ -{ *tokens{ -{ *{ -{ *    <whitespace>         : regex = ''' \\s+"
    " '''{ -{ *                           ignore = true{ -{ *{ -{ *    <macr"
    "o>              : regex = ''' \\{'+ [^{'-]* \\{'- '''{ -{ *        { -{"
    " *    <charset>            : regex = ''' \\[ ( [^\\]] {', \\\\ \\] )* \\"
    "] '''{ -{ *        { -{ *    <char>               : regex = ''' [^?+*()"
    "\\[\\]{'+{'-{',.\\$\\s] '''{ -{ *        { -{ *rules       { -{ *      "
    "  { -{ *    Regex                ::= RegexOr{ -{ *                     "
    "    :   (AstRegex, $1){ -{ *{ -{ *    RegexOr              ::= RegexOrT"
    "erm ( '{',' RegexOrTerm : $2 )+{ -{ *                         :   (AstR"
    "egexOr, $1, $2._){ -{ *        { -{ *    RegexOr              ::= Regex"
    "OrTerm{ -{ *        { -{ *    RegexOrTerm          ::= RegexUnopTerm+{ "
    "-{ *                         :   (AstRegexList, $1._){ -{ *        { -{"
    " *    RegexUnopTerm        ::= RegexUnopTerm '*'{ -{ *                 "
    "        :   (AstRegexZeroClosure, $1){ -{ *        { -{ *    RegexUnopT"
    "erm        ::= RegexUnopTerm '+'{ -{ *                         :   (Ast"
    "RegexOneClosure, $1){ -{ *        { -{ *    RegexUnopTerm        ::= Re"
    "gexUnopTerm '?'{ -{ *                         :   (AstRegexOptional, $1"
    "){ -{ *        { -{ *    RegexUnopTerm        ::= '(' RegexOr ')'{ -{ *"
    "                         :   $2{ -{ *        { -{ *    RegexUnopTerm   "
    "     ::= <macro>{ -{ *                         :   (AstMacroString, &1)"
    "{ -{ *        { -{ *    RegexUnopTerm        ::= Charset{ -{ *        {"
    " -{ *    RegexUnopTerm        ::= Char{ -{ *        { -{ *    Charset  "
    "            ::= <charset>{ -{ *                         :   (AstCharset"
    "String, &1) { -{ *{ -{ *    Charset              ::= '.' { -{ *        "
    "                 :   (AstRegexWildcard){ -{ *{ -{ *    Charset         "
    "     ::= '\\\\s' { -{ *                         :   (AstRegexWhitespace"
    "){ -{ *{ -{ *    Charset              ::= '\\\\S' { -{ *               "
    "          :   (AstRegexNotWhitespace){ -{ *{ -{ *    Charset           "
    "   ::= '\\\\d' { -{ *                         :   (AstRegexDigits){ -{ "
    "*{ -{ *    Charset              ::= '\\\\D' { -{ *                     "
    "    :   (AstRegexNotDigits){ -{ *{ -{ *    Char                 ::= <ch"
    "ar>{ -{ *                         :   (AstRegexChar, &1){ -{ *{ -{ *   "
    " Char                 ::= '\\\\\\\\' { -{ *                         :  "
    " (AstRegexEscape){ -{ *{ -{ *    Char                 ::= '$' { -{ *   "
    "                      :   (AstRegexAltNewline){ -{ *{ -{ *    Char     "
    "            ::= '\\\\n' { -{ *                         :   (AstRegexNew"
    "line){ -{ *{ -{ *    Char                 ::= '\\\\r' { -{ *           "
    "              :   (AstRegexCr){ -{ *{ -{ *    Char                 ::= "
    "'\\\\{',' { -{ *                         :   (AstRegexVBar){ -{ *{ -{ *"
    "    Char                 ::= '\\\\*' { -{ *                         :  "
    " (AstRegexStar){ -{ *{ -{ *    Char                 ::= '\\\\+' { -{ * "
    "                        :   (AstRegexPlus){ -{ *{ -{ *    Char         "
    "        ::= '\\\\?' { -{ *                         :   (AstRegexQuestio"
    "n){ -{ *{ -{ *    Char                 ::= '\\\\.' { -{ *              "
    "           :   (AstRegexPeriod){ -{ *{ -{ *    Char                 ::="
    " '\\\\$' { -{ *                         :   (AstRegexDollar){ -{ *{ -{ "
    "*    Char                 ::= '\\\\b' { -{ *                         : "
    "  (AstRegexSpace){ -{ *{ -{ *    Char                 ::= '\\\\(' { -{ "
    "*                         :   (AstRegexLeftParen){ -{ *{ -{ *    Char  "
    "               ::= '\\\\)' { -{ *                         :   (AstRegex"
    "RightParen){ -{ *{ -{ *    Char                 ::= '\\\\[' { -{ *     "
    "                    :   (AstRegexLeftBracket){ -{ *{ -{ *    Char      "
    "           ::= '\\\\]' { -{ *                         :   (AstRegexRigh"
    "tBracket){ -{ *{ -{ *    Char                 ::= '\\\\{'+' { -{ *     "
    "                    :   (AstRegexLeftBrace){ -{ *{ -{ *    Char        "
    "         ::= '\\\\{'-' { -{ *                         :   (AstRegexRigh"
    "tBrace){ -{ *{ -{ *|}#|\"|}$|!|}%|I|}&||}'|M|}(|*eof*|')'|'{','|'$'|'('"
    "|<charset>|<macro>|'.'|'\\\\$'|'\\\\('|'\\\\)'|'\\\\*'|'\\\\+'|'\\\\.'|"
    "'\\\\?'|'\\\\D'|'\\\\S'|'\\\\['|'\\\\]'|'\\\\b'|'\\\\d'|'\\\\n'|'\\\\\\"
    "\\'|'\\\\s'|'\\\\{'+'|'\\\\{','|'\\\\{'-'|'\\\\r'|<char>|'*'|'+'|'?'|||"
    "|||||||*error*||*epsilon*|<whitespace>|})|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!"
    "|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|\"#||5!||||||!\"|!| \"|!||}*|<\"|\"\""
    "|;\"| \"|!\"|@\"|A\"|%\"|'\"|(\"|)\"|*\"|+\"|,\"|-\"|.\"|/\"|0\"|2\"|3\""
    "|4\"|5\"|1\"|7\"|8\"|9\"|:\"|6\"|?\"|#\"|$\"|&\"|P\\<?||||P3^>||||P4^>|"
    ">\"||=\"|B\"|}+||||||!|!||||||||||||||||||||||!||||@#|>\"|X!||||||||||!"
    "|},|I|}-|!|!|\"|\"|\"|!|!|!|\"|!|\"|\"|\"|#|!|!|!|!|!|!|!|!|!|!|!|!|!|!"
    "|!|!|!|!|!|!|!|!|!|!|!|!|!|}.|J|H|G|F|C|F|G|D|E|E|B|B|B|B|B|B|B|A|A|A|A"
    "|A|A|@|@|@|@|@|@|@|@|@|@|@|@|@|@|@|@|@|@|}/|*accept* ::= Regex|Regex ::"
    "= RegexOr|RegexOr ::= RegexOrTerm RegexOr:1|RegexOr:1 ::= RegexOr:1 Reg"
    "exOr:2|RegexOr:2 ::= '{',' RegexOrTerm|RegexOr:1 ::= RegexOr:2|RegexOr "
    "::= RegexOrTerm|RegexOrTerm ::= RegexOrTerm:1|RegexOrTerm:1 ::= RegexOr"
    "Term:1 RegexUnopTerm|RegexOrTerm:1 ::= RegexUnopTerm|RegexUnopTerm ::= "
    "RegexUnopTerm '*'|RegexUnopTerm ::= RegexUnopTerm '+'|RegexUnopTerm ::="
    " RegexUnopTerm '?'|RegexUnopTerm ::= '(' RegexOr ')'|RegexUnopTerm ::= "
    "<macro>|RegexUnopTerm ::= Charset|RegexUnopTerm ::= Char|Charset ::= <c"
    "harset>|Charset ::= '.'|Charset ::= '\\\\s'|Charset ::= '\\\\S'|Charset"
    " ::= '\\\\d'|Charset ::= '\\\\D'|Char ::= <char>|Char ::= '\\\\\\\\'|Ch"
    "ar ::= '$'|Char ::= '\\\\n'|Char ::= '\\\\r'|Char ::= '\\\\{','|Char ::"
    "= '\\\\*'|Char ::= '\\\\+'|Char ::= '\\\\?'|Char ::= '\\\\.'|Char ::= '"
    "\\\\$'|Char ::= '\\\\b'|Char ::= '\\\\('|Char ::= '\\\\)'|Char ::= '\\\\"
    "['|Char ::= '\\\\]'|Char ::= '\\\\{'+'|Char ::= '\\\\{'-'|}0|!~.!|6!|@!"
    "|J!|O!|!~W!|_!|)\"|1\"|9\"|A\"|I\"|N\"|!~!~V\"|^\"|$#|*#|0#|6#|<#|D#|J#"
    "|P#|V#|\\#|\"$|($|.$|4$|:$|@$|F$|L$|R$|X$|^$|$%|}1|\"|}2||}3|M|}4|R|}5|"
    "F|/!|O!|/\"|)!|,!|Q3|/$|R$|/%|W%|7&|O#|W&|7'|W'|7(|W(|7)|W)|7*|W*|7+|W+"
    "|7,|W,|7-|W-|7.|W.|7/|W/|70|W0|^3|L$|43|71|W1|72|W2|Q%|#!|!4|/#||$4|L%|"
    "Z3|O\"|}6|R4|}7| %|!!@%|\"!@$|#!0\"|$!(!|%!0|&!(|'!@!|(!0#|)!@#|*!H#|+!"
    "P\"|,!X\"|-!(#|.! #|/! \"|0!P!|1!P#|2!X#|3!8#|4!X!|5!8\"|6!(\"|7!H!|8! "
    "$|9!H\"|:!($|;!@\"|<!8|=!P$|>!X$|?! %|@#8!|A#0!|B#(&|C#X%|D# &|E#X|F#0$"
    "|G#P%|H#@|#!0\"|$!(!|%!0|&!(|'!@!|(!0#|)!@#|*!H#|+!P\"|,!X\"|-!(#|.! #|"
    "/! \"|0!P!|1!P#|2!X#|3!8#|4!X!|5!8\"|6!(\"|7!H!|8! $|9!H\"|:!($|;!@\"|<"
    "!8| : H%|!: @%|\": @$|@#8!|A#0!|B# !| %|D#P|E#X| * H%|G#H|H#@| R!H%|!R!"
    "@%|\"R!@$|#R!0\"|$R!(!|%R!0|&R!(|'R!@!|(R!0#|)R!@#|*R!H#|+R!P\"|,R!X\"|"
    "-R!(#|.R! #|/R! \"|0R!P!|1R!P#|2R!X#|3R!8#|4R!X!|5R!8\"|6R!(\"|7R!H!|8R"
    "! $|9R!H\"|:R!($|;R!@\"|<R!8|=R!P$|>R!X$|?R! %| *\"H%|!*\"@%|\"*\"@$|#*"
    "\"0\"|$*\"(!|%*\"0|&*\"(|'*\"@!|(*\"0#|)*\"@#|**\"H#|+*\"P\"|,*\"X\"|-*"
    "\"(#|.*\" #|/*\" \"|0*\"P!|1*\"P#|2*\"X#|3*\"8#|4*\"X!|5*\"8\"|6*\"(\"|"
    "7*\"H!|8*\" $|9*\"H\"|:*\"($|;*\"@\"|<*\"8|=*\"P$|>*\"X$|?*\" %| Z\"H%|"
    "!Z\"@%|\"Z\"@$|#Z\"0\"|$Z\"(!|%Z\"0|&Z\"(|'Z\"@!|(Z\"0#|)Z\"@#|*Z\"H#|+"
    "Z\"P\"|,Z\"X\"|-Z\"(#|.Z\" #|/Z\" \"|0Z\"P!|1Z\"P#|2Z\"X#|3Z\"8#|4Z\"X!"
    "|5Z\"8\"|6Z\"(\"|7Z\"H!|8Z\" $|9Z\"H\"|:Z\"($|;Z\"@\"|<Z\"8|=Z\"P$|>Z\""
    "X$|?Z\" %| $H%|!$@%|\"$@$|#$0\"|$$(!|%$0|&$(|'$@!|($0#|)$@#|*$H#|+$P\"|"
    ",$X\"|-$(#|.$ #|/$ \"|0$P!|1$P#|2$X#|3$8#|4$X!|5$8\"|6$(\"|7$H!|8$ $|9$"
    "H\"|:$($|;$@\"|<$8|=!P$|>!X$|?! %| J!H%|!J!@%|\"J!@$|#J!0\"|$J!(!|%J!0|"
    "&J!(|'J!@!|(J!0#|)J!@#|*J!H#|+J!P\"|,J!X\"|-J!(#|.J! #|/J! \"|0J!P!|1J!"
    "P#|2J!X#|3J!8#|4J!X!|5J!8\"|6J!(\"|7J!H!|8J! $|9J!H\"|:J!($|;J!@\"|<J!8"
    "|=J!P$|>J!X$|?J! %| 2\"H%|!2\"@%|\"2\"@$|#2\"0\"|$2\"(!|%2\"0|&2\"(|'2\""
    "@!|(2\"0#|)2\"@#|*2\"H#|+2\"P\"|,2\"X\"|-2\"(#|.2\" #|/2\" \"|02\"P!|12"
    "\"P#|22\"X#|32\"8#|42\"X!|52\"8\"|62\"(\"|72\"H!|82\" $|92\"H\"|:2\"($|"
    ";2\"@\"|<2\"8|=2\"P$|>2\"X$|?2\" %| Z H%|!Z @%|\"Z @$|#!0\"|$!(!|%!0|&!"
    "(|'!@!|(!0#|)!@#|*!H#|+!P\"|,!X\"|-!(#|.! #|/! \"|0!P!|1!P#|2!X#|3!8#|4"
    "!X!|5!8\"|6!(\"|7!H!|8! $|9!H\"|:!($|;!@\"|<!8| J H%|!J @%|\"J @$|@#8!|"
    "A#0!|B#H$| *!H%|!*!@%|\"*!@$|#*!0\"|$*!(!|%*!0|&*!(|'*!@!|(*!0#|)*!@#|*"
    "*!H#|+*!P\"|,*!X\"|-*!(#|.*! #|/*! \"|0*!P!|1*!P#|2*!X#|3*!8#|4*!X!|5*!"
    "8\"|6*!(\"|7*!H!|8*! $|9*!H\"|:*!($|;*!@\"|<*!8|=!P$|>!X$|?! %|#!0\"|$!"
    "(!|%!0|&!(|'!@!|(!0#|)!@#|*!H#|+!P\"|,!X\"|-!(#|.! #|/! \"|0!P!|1!P#|2!"
    "X#|3!8#|4!X!|5!8\"|6!(\"|7!H!|8! $|9!H\"|:!($|;!@\"|<!8| $H%|!$@%|\"$@$"
    "|@#8!|A#0!|B# !|!!@%|D#P|E#X|!~G#(%| Z!H%|!Z!@%|\"Z!@$|#Z!0\"|$Z!(!|%Z!"
    "0|&Z!(|'Z!@!|(Z!0#|)Z!@#|*Z!H#|+Z!P\"|,Z!X\"|-Z!(#|.Z! #|/Z! \"|0Z!P!|1"
    "Z!P#|2Z!X#|3Z!8#|4Z!X!|5Z!8\"|6Z!(\"|7Z!H!|8Z! $|9Z!H\"|:Z!($|;Z!@\"|<Z"
    "!8|=Z!P$|>Z!X$|?Z! %| \"\"H%|!\"\"@%|\"\"\"@$|#\"\"0\"|$\"\"(!|%\"\"0|&"
    "\"\"(|'\"\"@!|(\"\"0#|)\"\"@#|*\"\"H#|+\"\"P\"|,\"\"X\"|-\"\"(#|.\"\" #"
    "|/\"\" \"|0\"\"P!|1\"\"P#|2\"\"X#|3\"\"8#|4\"\"X!|5\"\"8\"|6\"\"(\"|7\""
    "\"H!|8\"\" $|9\"\"H\"|:\"\"($|;\"\"@\"|<\"\"8|=\"\"P$|>\"\"X$|?\"\" %| "
    ":\"H%|!:\"@%|\":\"@$|#:\"0\"|$:\"(!|%:\"0|&:\"(|':\"@!|(:\"0#|):\"@#|*:"
    "\"H#|+:\"P\"|,:\"X\"|-:\"(#|.:\" #|/:\" \"|0:\"P!|1:\"P#|2:\"X#|3:\"8#|"
    "4:\"X!|5:\"8\"|6:\"(\"|7:\"H!|8:\" $|9:\"H\"|::\"($|;:\"@\"|<:\"8|=:\"P"
    "$|>:\"X$|?:\" %| B\"H%|!B\"@%|\"B\"@$|#B\"0\"|$B\"(!|%B\"0|&B\"(|'B\"@!"
    "|(B\"0#|)B\"@#|*B\"H#|+B\"P\"|,B\"X\"|-B\"(#|.B\" #|/B\" \"|0B\"P!|1B\""
    "P#|2B\"X#|3B\"8#|4B\"X!|5B\"8\"|6B\"(\"|7B\"H!|8B\" $|9B\"H\"|:B\"($|;B"
    "\"@\"|<B\"8|=B\"P$|>B\"X$|?B\" %| J\"H%|!J\"@%|\"J\"@$|#J\"0\"|$J\"(!|%"
    "J\"0|&J\"(|'J\"@!|(J\"0#|)J\"@#|*J\"H#|+J\"P\"|,J\"X\"|-J\"(#|.J\" #|/J"
    "\" \"|0J\"P!|1J\"P#|2J\"X#|3J\"8#|4J\"X!|5J\"8\"|6J\"(\"|7J\"H!|8J\" $|"
    "9J\"H\"|:J\"($|;J\"@\"|<J\"8|=J\"P$|>J\"X$|?J\" %| R\"H%|!R\"@%|\"R\"@$"
    "|#R\"0\"|$R\"(!|%R\"0|&R\"(|'R\"@!|(R\"0#|)R\"@#|*R\"H#|+R\"P\"|,R\"X\""
    "|-R\"(#|.R\" #|/R\" \"|0R\"P!|1R\"P#|2R\"X#|3R\"8#|4R\"X!|5R\"8\"|6R\"("
    "\"|7R\"H!|8R\" $|9R\"H\"|:R\"($|;R\"@\"|<R\"8|=R\"P$|>R\"X$|?R\" %| \"#"
    "H%|!\"#@%|\"\"#@$|#\"#0\"|$\"#(!|%\"#0|&\"#(|'\"#@!|(\"#0#|)\"#@#|*\"#H"
    "#|+\"#P\"|,\"#X\"|-\"#(#|.\"# #|/\"# \"|0\"#P!|1\"#P#|2\"#X#|3\"#8#|4\""
    "#X!|5\"#8\"|6\"#(\"|7\"#H!|8\"# $|9\"#H\"|:\"#($|;\"#@\"|<\"#8|=\"#P$|>"
    "\"#X$|?\"# %| *#H%|!*#@%|\"*#@$|#*#0\"|$*#(!|%*#0|&*#(|'*#@!|(*#0#|)*#@"
    "#|**#H#|+*#P\"|,*#X\"|-*#(#|.*# #|/*# \"|0*#P!|1*#P#|2*#X#|3*#8#|4*#X!|"
    "5*#8\"|6*#(\"|7*#H!|8*# $|9*#H\"|:*#($|;*#@\"|<*#8|=*#P$|>*#X$|?*# %| 2"
    "#H%|!2#@%|\"2#@$|#2#0\"|$2#(!|%2#0|&2#(|'2#@!|(2#0#|)2#@#|*2#H#|+2#P\"|"
    ",2#X\"|-2#(#|.2# #|/2# \"|02#P!|12#P#|22#X#|32#8#|42#X!|52#8\"|62#(\"|7"
    "2#H!|82# $|92#H\"|:2#($|;2#@\"|<2#8|=2#P$|>2#X$|?2# %| :#H%|!:#@%|\":#@"
    "$|#:#0\"|$:#(!|%:#0|&:#(|':#@!|(:#0#|):#@#|*:#H#|+:#P\"|,:#X\"|-:#(#|.:"
    "# #|/:# \"|0:#P!|1:#P#|2:#X#|3:#8#|4:#X!|5:#8\"|6:#(\"|7:#H!|8:# $|9:#H"
    "\"|::#($|;:#@\"|<:#8|=:#P$|>:#X$|?:# %| B#H%|!B#@%|\"B#@$|#B#0\"|$B#(!|"
    "%B#0|&B#(|'B#@!|(B#0#|)B#@#|*B#H#|+B#P\"|,B#X\"|-B#(#|.B# #|/B# \"|0B#P"
    "!|1B#P#|2B#X#|3B#8#|4B#X!|5B#8\"|6B#(\"|7B#H!|8B# $|9B#H\"|:B#($|;B#@\""
    "|<B#8|=B#P$|>B#X$|?B# %| J#H%|!J#@%|\"J#@$|#J#0\"|$J#(!|%J#0|&J#(|'J#@!"
    "|(J#0#|)J#@#|*J#H#|+J#P\"|,J#X\"|-J#(#|.J# #|/J# \"|0J#P!|1J#P#|2J#X#|3"
    "J#8#|4J#X!|5J#8\"|6J#(\"|7J#H!|8J# $|9J#H\"|:J#($|;J#@\"|<J#8|=J#P$|>J#"
    "X$|?J# %| R#H%|!R#@%|\"R#@$|#R#0\"|$R#(!|%R#0|&R#(|'R#@!|(R#0#|)R#@#|*R"
    "#H#|+R#P\"|,R#X\"|-R#(#|.R# #|/R# \"|0R#P!|1R#P#|2R#X#|3R#8#|4R#X!|5R#8"
    "\"|6R#(\"|7R#H!|8R# $|9R#H\"|:R#($|;R#@\"|<R#8|=R#P$|>R#X$|?R# %| Z#H%|"
    "!Z#@%|\"Z#@$|#Z#0\"|$Z#(!|%Z#0|&Z#(|'Z#@!|(Z#0#|)Z#@#|*Z#H#|+Z#P\"|,Z#X"
    "\"|-Z#(#|.Z# #|/Z# \"|0Z#P!|1Z#P#|2Z#X#|3Z#8#|4Z#X!|5Z#8\"|6Z#(\"|7Z#H!"
    "|8Z# $|9Z#H\"|:Z#($|;Z#@\"|<Z#8|=Z#P$|>Z#X$|?Z# %| \"$H%|!\"$@%|\"\"$@$"
    "|#\"$0\"|$\"$(!|%\"$0|&\"$(|'\"$@!|(\"$0#|)\"$@#|*\"$H#|+\"$P\"|,\"$X\""
    "|-\"$(#|.\"$ #|/\"$ \"|0\"$P!|1\"$P#|2\"$X#|3\"$8#|4\"$X!|5\"$8\"|6\"$("
    "\"|7\"$H!|8\"$ $|9\"$H\"|:\"$($|;\"$@\"|<\"$8|=\"$P$|>\"$X$|?\"$ %| *$H"
    "%|!*$@%|\"*$@$|#*$0\"|$*$(!|%*$0|&*$(|'*$@!|(*$0#|)*$@#|**$H#|+*$P\"|,*"
    "$X\"|-*$(#|.*$ #|/*$ \"|0*$P!|1*$P#|2*$X#|3*$8#|4*$X!|5*$8\"|6*$(\"|7*$"
    "H!|8*$ $|9*$H\"|:*$($|;*$@\"|<*$8|=*$P$|>*$X$|?*$ %| 2$H%|!2$@%|\"2$@$|"
    "#2$0\"|$2$(!|%2$0|&2$(|'2$@!|(2$0#|)2$@#|*2$H#|+2$P\"|,2$X\"|-2$(#|.2$ "
    "#|/2$ \"|02$P!|12$P#|22$X#|32$8#|42$X!|52$8\"|62$(\"|72$H!|82$ $|92$H\""
    "|:2$($|;2$@\"|<2$8|=2$P$|>2$X$|?2$ %| :$H%|!:$@%|\":$@$|#:$0\"|$:$(!|%:"
    "$0|&:$(|':$@!|(:$0#|):$@#|*:$H#|+:$P\"|,:$X\"|-:$(#|.:$ #|/:$ \"|0:$P!|"
    "1:$P#|2:$X#|3:$8#|4:$X!|5:$8\"|6:$(\"|7:$H!|8:$ $|9:$H\"|::$($|;:$@\"|<"
    ":$8|=:$P$|>:$X$|?:$ %| B$H%|!B$@%|\"B$@$|#B$0\"|$B$(!|%B$0|&B$(|'B$@!|("
    "B$0#|)B$@#|*B$H#|+B$P\"|,B$X\"|-B$(#|.B$ #|/B$ \"|0B$P!|1B$P#|2B$X#|3B$"
    "8#|4B$X!|5B$8\"|6B$(\"|7B$H!|8B$ $|9B$H\"|:B$($|;B$@\"|<B$8|=B$P$|>B$X$"
    "|?B$ %| J$H%|!J$@%|\"J$@$|#J$0\"|$J$(!|%J$0|&J$(|'J$@!|(J$0#|)J$@#|*J$H"
    "#|+J$P\"|,J$X\"|-J$(#|.J$ #|/J$ \"|0J$P!|1J$P#|2J$X#|3J$8#|4J$X!|5J$8\""
    "|6J$(\"|7J$H!|8J$ $|9J$H\"|:J$($|;J$@\"|<J$8|=J$P$|>J$X$|?J$ %| R$H%|!R"
    "$@%|\"R$@$|#R$0\"|$R$(!|%R$0|&R$(|'R$@!|(R$0#|)R$@#|*R$H#|+R$P\"|,R$X\""
    "|-R$(#|.R$ #|/R$ \"|0R$P!|1R$P#|2R$X#|3R$8#|4R$X!|5R$8\"|6R$(\"|7R$H!|8"
    "R$ $|9R$H\"|:R$($|;R$@\"|<R$8|=R$P$|>R$X$|?R$ %| Z$H%|!Z$@%|\"Z$@$|#Z$0"
    "\"|$Z$(!|%Z$0|&Z$(|'Z$@!|(Z$0#|)Z$@#|*Z$H#|+Z$P\"|,Z$X\"|-Z$(#|.Z$ #|/Z"
    "$ \"|0Z$P!|1Z$P#|2Z$X#|3Z$8#|4Z$X!|5Z$8\"|6Z$(\"|7Z$H!|8Z$ $|9Z$H\"|:Z$"
    "($|;Z$@\"|<Z$8|=Z$P$|>Z$X$|?Z$ %| \"%H%|!\"%@%|\"\"%@$|#\"%0\"|$\"%(!|%"
    "\"%0|&\"%(|'\"%@!|(\"%0#|)\"%@#|*\"%H#|+\"%P\"|,\"%X\"|-\"%(#|.\"% #|/\""
    "% \"|0\"%P!|1\"%P#|2\"%X#|3\"%8#|4\"%X!|5\"%8\"|6\"%(\"|7\"%H!|8\"% $|9"
    "\"%H\"|:\"%($|;\"%@\"|<\"%8|=\"%P$|>\"%X$|?\"% %| \"!H%|!\"!@%|\"\"!@$|"
    "#\"!0\"|$\"!(!|%\"!0|&\"!(|'\"!@!|(\"!0#|)\"!@#|*\"!H#|+\"!P\"|,\"!X\"|"
    "-\"!(#|.\"! #|/\"! \"|0\"!P!|1\"!P#|2\"!X#|3\"!8#|4\"!X!|5\"!8\"|6\"!(\""
    "|7\"!H!|8\"! $|9\"!H\"|:\"!($|;\"!@\"|<\"!8|=!P$|>!X$|?! %| 2!H%|!2!@%|"
    "\"2!@$|#2!0\"|$2!(!|%2!0|&2!(|'2!@!|(2!0#|)2!@#|*2!H#|+2!P\"|,2!X\"|-2!"
    "(#|.2! #|/2! \"|02!P!|12!P#|22!X#|32!8#|42!X!|52!8\"|62!(\"|72!H!|82! $"
    "|92!H\"|:2!($|;2!@\"|<2!8|=2!P$|>2!X$|?2! %| :!H%|!:!@%|\":!@$|#:!0\"|$"
    ":!(!|%:!0|&:!(|':!@!|(:!0#|):!@#|*:!H#|+:!P\"|,:!X\"|-:!(#|.:! #|/:! \""
    "|0:!P!|1:!P#|2:!X#|3:!8#|4:!X!|5:!8\"|6:!(\"|7:!H!|8:! $|9:!H\"|::!($|;"
    ":!@\"|<:!8|=:!P$|>:!X$|?:! %| B!H%|!B!@%|\"B!@$|#B!0\"|$B!(!|%B!0|&B!(|"
    "'B!@!|(B!0#|)B!@#|*B!H#|+B!P\"|,B!X\"|-B!(#|.B! #|/B! \"|0B!P!|1B!P#|2B"
    "!X#|3B!8#|4B!X!|5B!8\"|6B!(\"|7B!H!|8B! $|9B!H\"|:B!($|;B!@\"|<B!8|=B!P"
    "$|>B!X$|?B! %|#!0\"|$!(!|%!0|&!(|'!@!|(!0#|)!@#|*!H#|+!P\"|,!X\"|-!(#|."
    "! #|/! \"|0!P!|1!P#|2!X#|3!8#|4!X!|5!8\"|6!(\"|7!H!|8! $|9!H\"|:!($|;!@"
    "\"|<!8| R H%|!R @%|\"!@$|@#8!|A#0!|B# !|!~D#8%|E#X| $H%|!$@%|\"$@$|!~ 2"
    " H%|!2 @%|\"!@$| B H%|!B @%|\"B @$| * H%|!!@%|!~!~!~!~!~!~!~!~!~!~!~!~!"
    "~!~C#8$|!~!~F#0$|!~!~!~!~!~C#8$|!~!~F#0$|C#0%|!~!~!~!~!~!~!~!~!~!~!~!~!"
    "~!~!~!~}8|!|}9||}:||};|_|}<||}=|&|}>|'|}?||}@|)|}A|_|}B||}C|/|}D|_|}E||"
    "}F|5|}G|_|}H|Null||Halt|!|Label|\"|Call|#|ScanStart|$|ScanChar|%|ScanAc"
    "cept|&|ScanToken|'|ScanError|(|AstStart|)|AstFinish|*|AstNew|+|AstForm|"
    ",|AstLoad|-|AstIndex|.|AstChild|/|AstChildSlice|0|AstKind|1|AstKindNum|"
    "2|AstLocation|3|AstLocationNum|4|AstLexeme|5|AstLexemeString|6|Assign|7"
    "|DumpStack|8|Add|9|Subtract|:|Multiply|;|Divide|<|UnaryMinus|=|Return|>"
    "|Branch|?|BranchEqual|@|BranchNotEqual|A|BranchLessThan|B|BranchLessEqu"
    "al|C|BranchGreaterThan|D|BranchGreaterEqual|E|}I|*%|}J|I)|}K|7|!~|$||>|"
    "!~\"|$|!~\"|%|!~\"|6||(|$|)|-|&|.|?|$|@|@|&|A|C|$|D|D|(|E|G|$|H|H|*|I|I"
    "|,|J|J|.|K|K|0|L|M|$|N|N|2|O|^|$|_|_|4| !|:!|$|;!|;!|6|<!|<!|<|>!|Z!|$|"
    "[!|[!|&!|\\!|\\!|)!|^!|_____#|$|&|J'|%!|<|+!|%|!~'!||&|]#|(!|L|\"|%|!~*"
    "!|\"|)|-|&|@|@|&|&|2D|1!|#|+!|%|!~3!||&|N4|4!|$|+!|%|!~6!||&|Z4|7!|!|+!"
    "|%|!~9!||&|J/|:!|=|+!|%|!~<!||&|<1|=!|>|+!|%|!~?!||&|G:|@!|'|+!|%|!~B!|"
    "|&|-3|C!|?|+!|%|!~E!||%|!~F!|$||;!|6|<!|<!|7|=!|=!|:|>!|_____#|6|%|!~S!"
    "|$||;!|6|<!|<!|7|=!|=!|8|>!|_____#|6|&|;&| \"|%|+!|%|!~\"\"|$||;!|6|<!|"
    "<!|7|=!|=!|:|>!|_____#|6|&|;&|/\"|%|+!|%|!~1\"||&|J'|2\"|<|+!|%|!~4\"|4"
    "|D|D|>|H|H|@|I|I|B|J|J|D|K|K|F|N|N|H|_|_|J|$!|$!|L|3!|3!|N|;!|;!|P|<!|<"
    "!|R|=!|=!|T|B!|B!|V|D!|D!|X|N!|N!|Z|R!|R!|\\|S!|S!|^|[!|[!| !|\\!|\\!|\""
    "!|]!|]!|$!|&|]N|1#|(|+!|%|!~3#||&|HQ|4#|)|+!|%|!~6#||&|!S|7#|*|+!|%|!~9"
    "#||&|GI|:#|+|+!|%|!~<#||&|[J|=#|,|+!|%|!~?#||&|GM|@#|-|+!|%|!~B#||&|/L|"
    "C#|.|+!|%|!~E#||&|*@|F#|/|+!|%|!~H#||&|7=|I#|0|+!|%|!~K#||&|;T|L#|1|+!|"
    "%|!~N#||&|[B|O#|6|+!|%|!~Q#||&|VU|R#|2|+!|%|!~T#||&|3P|U#|3|+!|%|!~W#||"
    "&|T>|X#|4|+!|%|!~Z#||&|JE|[#|5|+!|%|!~]#||&|!G|^#|;|+!|%|!~ $||&|];|!$|"
    "7|+!|%|!~#$||&|2W|$$|8|+!|%|!~&$||&|3H|'$|9|+!|%|!~)$||&|KX|*$|:|+!|%|!"
    "~,$||%|!~-$|#||\\!|&!|]!|]!|'!|^!|_____#|&!|&|9%|7$|&|+!|%|!~9$||&|)+|:"
    "$|\"|+!|%|!~<$||'|!~=$|9|!~=$|$|$|!|>|!~@$|)|,*|@$|\"|+|,*|A$|#|-|8*|B$"
    "||\"|!~/|7*|E$||,|,*|F$|\"|#|!|2|-*|I$|_|*|,*|J$|!|>|-)|K$|)|_+|K$|#|+|"
    "_+|L$|\"|-|-,|M$||#|\"~/|,,|P$||-|1,|Q$||#|!~0|3,|T$|||!~,|_+|W$|#|\"|\""
    "|2| ,|Z$| !|*|_+|[$|\"|>|B*|\\$|)|[*|\\$|\"|+|[*|]$|#|-|[*|^$||\"|\"~0|"
    "[*|!%|||!~-|[*|$%||\"|!~/|[*|'%||,|[*|(%|\"|#|\"|2|[*|+%|_!|*|[*|,%|\"|"
    ">|[*|-%|)|;+|-%|#|-|<+|.%|!|#|!~/|;+|1%|!|*|;+|2%|\"|>|!~3%|)|[*|3%|\"|"
    "+|[*|4%|#|-|[*|5%|!|\"|!~/|[*|8%|!|,|[*|9%|\"|#|!|2|[*|<%|_!|*|[*|=%|!|"
    ">|[*|>%|)|?.|>%|#|+|?.|?%|\"|-|O.|@%|!|#|!~0|Q.|C%|!||!~,|?.|F%|#|\"|!|"
    "2|@.|I%|!!|*|?.|J%|!|>|9-|K%|)|9-|K%|\"|+|9-|L%|#|-|9-|M%|!|\"|\"~0|9-|"
    "P%|!||!~-|9-|S%|!|\"|!~/|9-|V%|!|,|9-|W%|\"|#|\"|2|9-|Z%|_!|*|9-|[%|\"|"
    ">|9-|\\%|)|9-|\\%|#|+|9-|]%|\"|-|9-|^%|!|#|!~/|9-|!&|!|,|9-|\"&|#|\"|!|"
    "2|9-|%&|_!|*|9-|&&|!|>|9-|'&|)|,0|'&|\"|+|,0|(&|#|-|C0|)&|!|\"|\"~/|B0|"
    ",&|!|,|,0|-&|\"|#|\"|2|-0|0&|#!|*|,0|1&|\"|>|#/|2&|)|^1|2&|#|+|^1|3&|\""
    "|-|42|4&|!|#|\"~/|32|7&|!|,|^1|8&|#|\"|\"|2|_1|;&|$!|*|^1|<&|\"|>|U0|=&"
    "|)|O3|=&|\"|+|O3|>&|#|-|#4|?&|!|\"|\"~/|\"4|B&|!|,|O3|C&|\"|#|\"|2|P3|F"
    "&|\"!|*|O3|G&|\"|>|F2|H&|)|<5|H&|#|-|=5|I&|\"|#|\"~/|<5|L&|\"|*|<5|M&|#"
    "|>|54|N&|)|M6|N&|\"|+|M6|O&|#|,|M6|P&|\"|#|!|2|N6|S&|W|-|_6|T&|\"|\"|!~"
    "5|^6|W&|\"|*|M6|X&|!|>|N5|Y&|)|O9|Y&|#|+|O9|Z&|\"|,|O9|[&|#|\"|!|2|P9|^"
    "&|V|-|#:|_&|\"|#|!~5|\":|\"'|\"|*|O9|#'|!|>|N8|$'|)|*;|$'|\"|+|*;|%'|#|"
    ",|*;|&'|\"|#|!|2|+;|)'|&!|*|*;|*'|!|>|.:|+'|)|B<|+'|#|+|B<|,'|\"|,|B<|-"
    "'|#|\"|!|2|C<|0'|'!|*|B<|1'|!|>|D;|2'|)|\\=|2'|\"|+|\\=|3'|#|,|\\=|4'|\""
    "|#|!|2|]=|7'|(!|*|\\=|8'|!|>|^<|9'|)|9?|9'|#|+|9?|:'|\"|,|9?|;'|#|\"|!|"
    "2|:?|>'|)!|*|9?|?'|!|>|;>|@'|)|O@|@'|\"|+|O@|A'|#|,|O@|B'|\"|#|!|2|P@|E"
    "'|*!|*|O@|F'|!|>|Q?|G'|)|(B|G'|#|+|(B|H'|\"|,|(B|I'|#|\"|!|2|)B|L'|%!|-"
    "|8B|M'|\"|#|!~5|7B|P'|\"|*|(B|Q'|!|>|*A|R'|)|AC|R'|\"|+|AC|S'|#|,|AC|T'"
    "|\"|#|!|2|BC|W'|+!|*|AC|X'|!|>|BB|Y'|)|UD|Y'|#|+|UD|Z'|\"|,|UD|['|#|\"|"
    "!|2|VD|^'|,!|*|UD|_'|!|>|YC| (|)|/F| (|\"|+|/F|!(|#|,|/F|\"(|\"|#|!|2|0"
    "F|%(|-!|*|/F|&(|!|>|1E|'(|)|FG|'(|#|+|FG|((|\"|,|FG|)(|#|\"|!|2|GG|,(|."
    "!|*|FG|-(|!|>|HF|.(|)|XH|.(|\"|+|XH|/(|#|,|XH|0(|\"|#|!|2|YH|3(|/!|*|XH"
    "|4(|!|>|ZG|5(|)|,J|5(|#|+|,J|6(|\"|,|,J|7(|#|\"|!|2|-J|:(|0!|*|,J|;(|!|"
    ">|.I|<(|)|@K|<(|\"|+|@K|=(|#|,|@K|>(|\"|#|!|2|AK|A(|1!|*|@K|B(|!|>|BJ|C"
    "(|)|TL|C(|#|+|TL|D(|\"|,|TL|E(|#|\"|!|2|UL|H(|2!|*|TL|I(|!|>|VK|J(|)|,N"
    "|J(|\"|+|,N|K(|#|,|,N|L(|\"|#|!|2|-N|O(|3!|*|,N|P(|!|>|.M|Q(|)|BO|Q(|#|"
    "+|BO|R(|\"|,|BO|S(|#|\"|!|2|CO|V(|4!|*|BO|W(|!|>|DN|X(|)|XP|X(|\"|+|XP|"
    "Y(|#|,|XP|Z(|\"|#|!|2|YP|](|5!|*|XP|^(|!|>|ZO|_(|)|-R|_(|#|+|-R| )|\"|,"
    "|-R|!)|#|\"|!|2|.R|$)|6!|*|-R|%)|!|>|/Q|&)|)|FS|&)|\"|+|FS|')|#|,|FS|()"
    "|\"|#|!|2|GS|+)|7!|*|FS|,)|!|>|HR|-)|)| U|-)|#|+| U|.)|\"|,| U|/)|#|\"|"
    "!|2|!U|2)|8!|*| U|3)|!|>|\"T|4)|)|;V|4)|\"|+|;V|5)|#|,|;V|6)|\"|#|!|2|<"
    "V|9)|9!|*|;V|:)|!|>|=U|;)|)|WW|;)|#|+|WW|<)|\"|,|WW|=)|#|\"|!|2|XW|@)|:"
    "!|*|WW|A)|!|>|YV|B)|)|0Y|B)|\"|+|0Y|C)|#|,|0Y|D)|\"|#|!|2|1Y|G)|;!|*|0Y"
    "|H)|!|>|2X|I)|}L|%|}M||0|!|1||Temp$0||Temp$1||token_count|}N|#|}O||}P|}"
    "Q|}"
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
    "lookaheads = 2{ -{ *    error_recovery = true{ -{ *    conflicts = 0{ -"
    "{ *    keep_whitespace = true{ -{ *    case_sensitive = true{ -{ *{ -{ "
    "*tokens{ -{ *{ -{ *    <char>               : regex = ''' [^\\^\\-\\[\\"
    "]\\$] '''{ -{ *{ -{ *rules       { -{ *        { -{ *    CharsetRoot   "
    "       ::= '^' CharsetList{ -{ *                         :   (AstCharse"
    "tInvert, $2._){ -{ *{ -{ *    CharsetRoot          ::= CharsetList{ -{ "
    "*{ -{ *    CharsetList          ::= CharsetItem+{ -{ *                 "
    "        :   (AstCharset, $1._){ -{ *{ -{ *    CharsetItem          ::= "
    "Char '-' Char{ -{ *                         :   (AstCharsetRange, $1, $"
    "3, @2){ -{ *{ -{ *    CharsetItem          ::= Charset{ -{ *{ -{ *    C"
    "harsetItem          ::= Char{ -{ *                         :   (AstChar"
    "setRange, $1){ -{ *{ -{ *    Charset              ::= '\\\\s' { -{ *   "
    "                      :   (AstCharsetWhitespace){ -{ *{ -{ *    Charset"
    "              ::= '\\\\S' { -{ *                         :   (AstCharse"
    "tNotWhitespace){ -{ *{ -{ *    Charset              ::= '\\\\d' { -{ * "
    "                        :   (AstCharsetDigits){ -{ *{ -{ *    Charset  "
    "            ::= '\\\\D' { -{ *                         :   (AstCharsetN"
    "otDigits){ -{ *{ -{ *    Char                 ::= '\\\\\\\\' { -{ *    "
    "                     :   (AstCharsetEscape){ -{ *{ -{ *    Char        "
    "         ::= '$' { -{ *                         :   (AstCharsetAltNewli"
    "ne){ -{ *{ -{ *    Char                 ::= '\\\\n' { -{ *             "
    "            :   (AstCharsetNewline){ -{ *{ -{ *    Char                "
    " ::= '\\\\r' { -{ *                         :   (AstCharsetCr){ -{ *{ -"
    "{ *    Char                 ::= '\\\\^' { -{ *                         "
    ":   (AstCharsetCaret){ -{ *{ -{ *    Char                 ::= '\\\\-' {"
    " -{ *                         :   (AstCharsetDash){ -{ *{ -{ *    Char "
    "                ::= '\\\\$' { -{ *                         :   (AstChar"
    "setDollar){ -{ *{ -{ *    Char                 ::= '\\\\[' { -{ *      "
    "                   :   (AstCharsetLeftBracket){ -{ *{ -{ *    Char     "
    "            ::= '\\\\]' { -{ *                         :   (AstCharsetR"
    "ightBracket){ -{ *{ -{ *    Char                 ::= <char>{ -{ *      "
    "                   :   (AstCharsetChar, &1){ -{ *{ -{ *{ -{ *|}#|\"|}$|"
    "!|}%|9|}&||}'|:|}(|*eof*|'$'|'\\\\$'|'\\\\-'|'\\\\['|'\\\\\\\\'|'\\\\]'"
    "|'\\\\^'|'\\\\n'|'\\\\r'|<char>|'\\\\D'|'\\\\S'|'\\\\d'|'\\\\s'|'-'|'^'"
    "||||||||*epsilon*|*error*|})|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|6#|B||||"
    "||!|!|}*|/\"| \"|\"\"|#\"|&\"|'\"|(\"|)\"|+\"|,\"|2\"|$\"|%\"|*\"|-\"|!"
    "\"|.\"||!~!~||||0\"|1\"|}+|||||||||||!|||||||6#|B||||||||},|7|}-|!|\"|!"
    "|!|\"|!|#|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|!|}.|7|6|6|4|5|5|3|3|3|2|2|2|2|"
    "1|1|1|1|1|1|1|1|1|1|}/|*accept* ::= CharsetRoot|CharsetRoot ::= '^' Cha"
    "rsetList|CharsetRoot ::= CharsetList|CharsetList ::= CharsetList:1|Char"
    "setList:1 ::= CharsetList:1 CharsetItem|CharsetList:1 ::= CharsetItem|C"
    "harsetItem ::= Char '-' Char|CharsetItem ::= Charset|CharsetItem ::= Ch"
    "ar|Charset ::= '\\\\s'|Charset ::= '\\\\S'|Charset ::= '\\\\d'|Charset "
    "::= '\\\\D'|Char ::= '\\\\\\\\'|Char ::= '$'|Char ::= '\\\\n'|Char ::= "
    "'\\\\r'|Char ::= '\\\\^'|Char ::= '\\\\-'|Char ::= '\\\\$'|Char ::= '\\"
    "\\['|Char ::= '\\\\]'|Char ::= <char>|}0|!~I|!~Q|Y|#!|+!|!~7!|?!|E!|K!|"
    "Q!|W!|]!|#\"|)\"|/\"|5\"|;\"|A\"|G\"|M\"|}1|\"|}2||}3|:|}4|>|}5|6|6$|E$"
    "|T$|#%|2%|6!|A%|L|E|F!|V!|[|\"!|&\"|6\"|F\"|V\"|&#|6#|F#|V#|<&|\\|P%|_%"
    "||&$|.&|1!|}6|W&|}7|@\"|A H|B J|C @|D B|E 6|F >|G D|H <|I F|J 4|K &|L ("
    "|M \"|N $|O L|P 0|Q!V|R!.|S!X|T!Z|U!:|V!2|A H|B J|C @|D B|E 6|F >|G D|H"
    " <|I F|J 4|K &|L (|M \"|N $|@\"|P 0|Q!,|R!.|S!*|T!8|U!:|V!2|A H|B J|C @"
    "|D B|E 6|F >|G D|H <|I F|J 4|K &|L (|M \"|N $| ) :| % :|Q!,|R!.|S!*|T!N"
    "|U!:| - :|A H|B J|C @|D B|E 6|F >|G D|H <|I F|J 4|K &|L (|M \"|N $| \"T"
    "|!~Q!,|R!.|S!P| A :|!A 4|\"A 5|#A 0|$A 1|%A +|&A /|'A 2|(A .|)A 3|*A *|"
    "+A #|,A $|-A !|.A \"|O L| 9!:|!9!4|\"9!5|#9!0|$9!1|%9!+|&9!/|'9!2|(9!.|"
    ")9!3|*9!*|+9!#|,9!$|-9!!|.9!\"|/9!6| U :|!U 4|\"U 5|#U 0|$U 1|%U +|&U /"
    "|'U 2|(U .|)U 3|*U *|+U #|,U $|-U !|.U \"|/U 6| ] :|!] 4|\"] 5|#] 0|$] "
    "1|%] +|&] /|'] 2|(] .|)] 3|*] *|+] #|,] $|-] !|.] \"|/] 6| 5!:|!5!4|\"5"
    "!5|#5!0|$5!1|%5!+|&5!/|'5!2|(5!.|)5!3|*5!*|+5!#|,5!$|-5!!|.5!\"|/5!6| )"
    "!:|!)!4|\")!5|#)!0|$)!1|%)!+|&)!/|')!2|()!.|))!3|*)!*|+)!#|,)!$|-)!!|.)"
    "!\"|/)!6| 1!:|!1!4|\"1!5|#1!0|$1!1|%1!+|&1!/|'1!2|(1!.|)1!3|*1!*|+1!#|,"
    "1!$|-1!!|.1!\"|/1!6| %!:|!%!4|\"%!5|#%!0|$%!1|%%!+|&%!/|'%!2|(%!.|)%!3|"
    "*%!*|+%!#|,%!$|-%!!|.%!\"|/%!6| !!:|!!!4|\"!!5|#!!0|$!!1|%!!+|&!!/|'!!2"
    "|(!!.|)!!3|*!!*|+!!#|,!!$|-!!!|.!!\"|/!!6| Y :|!Y 4|\"Y 5|#Y 0|$Y 1|%Y "
    "+|&Y /|'Y 2|(Y .|)Y 3|*Y *|+Y #|,Y $|-Y !|.Y \"|/Y 6| -!:|!-!4|\"-!5|#-"
    "!0|$-!1|%-!+|&-!/|'-!2|(-!.|)-!3|*-!*|+-!#|,-!$|--!!|.-!\"|/-!6| \"T|!\""
    "H|\"\"J|#\"@|$\"B|%\"6|&\">|'\"D|(\"<|)\"F|*\"4|+\"&|,\"(|-\"\"|.\"$|O "
    "L| M :|!M 4|\"M 5|#M 0|$M 1|%M +|&M /|'M 2|(M .|)M 3|*M *|+M #|,M $|-M "
    "!|.M \"| E :|!E 4|\"E 5|#E 0|$E 1|%E +|&E /|'E 2|(E .|)E 3|*E *|+E #|,E"
    " $|-E !|.E \"| Q :|!Q 4|\"Q 5|#Q 0|$Q 1|%Q +|&Q /|'Q 2|(Q .|)Q 3|*Q *|+"
    "Q #|,Q $|-Q !|.Q \"| I :|!I 4|\"I 5|#I 0|$I 1|%I +|&I /|'I 2|(I .|)I 3|"
    "*I *|+I #|,I $|-I !|.I \"| 5 :|!5 4|\"5 5|#5 0|$5 1|%5 +|&5 /|'5 2|(5 ."
    "|)5 3|*5 *|+5 #|,5 $|-5 !|.5 \"| = :|!= 4|\"= 5|#= 0|$= 1|%= +|&= /|'= "
    "2|(= .|)= 3|*= *|+= #|,= $|-= !|.= \"| 1 :|!1 4|\"1 5|#1 0|$1 1|%1 +|&1"
    " /|'1 2|(1 .|)1 3|*1 *|+1 #|,1 $|-1 !|.1 \"| 9 :|!9 4|\"9 5|#9 0|$9 1|%"
    "9 +|&9 /|'9 2|(9 .|)9 3|*9 *|+9 #|,9 $|-9 !|.9 \"| \"T|!\"H|\"\"J|#\"@|"
    "$\"B|%\"6|&\">|'\"D|(\"<|)\"F|*\"4|+\"&|,\"(|-\"\"|.\"$|A H|B J|C @|D B"
    "|E 6|F >|G D|H <|I F|J 4|!~!~!~!~!~!~Q!R|!~!~!~!~!~!~!~!~!~}8|!|}9||}:|"
    "|};|?|}<||}=|%|}>|'|}?||}@|(|}A|?|}B||}C|-|}D|?|}E||}F|2|}G|?|}H|Null||"
    "Halt|!|Label|\"|Call|#|ScanStart|$|ScanChar|%|ScanAccept|&|ScanToken|'|"
    "ScanError|(|AstStart|)|AstFinish|*|AstNew|+|AstForm|,|AstLoad|-|AstInde"
    "x|.|AstChild|/|AstChildSlice|0|AstKind|1|AstKindNum|2|AstLocation|3|Ast"
    "LocationNum|4|AstLexeme|5|AstLexemeString|6|Assign|7|DumpStack|8|Add|9|"
    "Subtract|:|Multiply|;|Divide|<|UnaryMinus|=|Return|>|Branch|?|BranchEqu"
    "al|@|BranchNotEqual|A|BranchLessThan|B|BranchLessEqual|C|BranchGreaterT"
    "han|D|BranchGreaterEqual|E|}I|U\"|}J|O$|}K|7|!~|$||>|!~\"|$|!~\"|%|!~\""
    "|(||C|$|D|D|&|E|L|$|M|M|(|N|:!|$|<!|<!|*|>!|>!|D|?!|_____#|$|&|=$|;|*|F"
    "|%|!~=||&|M4|>|!|F|%|!~@||&| *|A|/|F|%|!~C||&|=$|D|*|F|%|!~F|,|D|D|,|M|"
    "M|.|$!|$!|0|3!|3!|2|;!|;!|4|<!|<!|6|=!|=!|8|>!|>!|:|D!|D!|<|N!|N!|>|R!|"
    "R!|@|S!|S!|B|&|A;|+!|\"|F|%|!~-!||&|+:|.!|#|F|%|!~0!||&|Y1|1!|+|F|%|!~3"
    "!||&|\"/|4!|,|F|%|!~6!||&|Y<|7!|$|F|%|!~9!||&|43|:!|%|F|%|!~<!||&|6>|=!"
    "|&|F|%|!~?!||&|T8|@!|'|F|%|!~B!||&|A0|C!|-|F|%|!~E!||&|'6|F!|(|F|%|!~H!"
    "||&|@7|I!|)|F|%|!~K!||&|F-|L!|.|F|%|!~N!||&|*&|O!|0|F|%|!~Q!||'|!~R!|9|"
    "!~R!|$|$|!|>|!~U!|)|X&|U!|\"|+|X&|V!|#|-|,'|W!||\"|!~0|.'|Z!|||!~,|X&|]"
    "!|\"|#|\"|2|Y&| \"|=!|*|X&|!\"|\"|>|Q%|\"\"|)|()|\"\"|#|+|()|#\"|\"|-|6"
    ")|$\"||#|!~0|8)|'\"|||!~,|()|*\"|#|\"|!|2|))|-\"|<!|*|()|.\"|!|>|$(|/\""
    "|)|$(|/\"|\"|+|$(|0\"|#|-|$(|1\"||\"|\"~0|$(|4\"|||!~-|$(|7\"||\"|!~/|$"
    "(|:\"||,|$(|;\"|\"|#|\"|2|$(|>\"|_!|*|$(|?\"|\"|>|$(|@\"|)|$(|@\"|#|+|$"
    "(|A\"|\"|-|$(|B\"||#|!~/|$(|E\"||,|$(|F\"|#|\"|!|2|$(|I\"|_!|*|$(|J\"|!"
    "|>|$(|K\"|)|G*|K\"|\"|+|G*|L\"|#|-|Z*|M\"||\"|#~/|Y*|P\"||-|^*|Q\"||\"|"
    "!~/|]*|T\"||,|G*|U\"|\"|#|#|2|H*|X\"|>!|-|\"+|Y\"||\"|\"~3|!+|\\\"||*|G"
    "*|]\"|#|>|B)|^\"|)|P,|^\"|#|+|P,|_\"|\"|-|#-| #||#|!~/|\"-|##||,|P,|$#|"
    "#|\"|!|2|Q,|'#|>!|*|P,|(#|!|>|T+|)#|)|+.|)#|\"|+|+.|*#|#|,|+.|+#|\"|#|!"
    "|2|,.|.#|@!|*|+.|/#|!|>|--|0#|)|G/|0#|#|+|G/|1#|\"|,|G/|2#|#|\"|!|2|H/|"
    "5#|A!|*|G/|6#|!|>|I.|7#|)|&1|7#|\"|+|&1|8#|#|,|&1|9#|\"|#|!|2|'1|<#|B!|"
    "*|&1|=#|!|>|(0|>#|)|>2|>#|#|+|>2|?#|\"|,|>2|@#|#|\"|!|2|?2|C#|C!|*|>2|D"
    "#|!|>|@1|E#|)|Z3|E#|\"|+|Z3|F#|#|,|Z3|G#|\"|#|!|2|[3|J#|D!|*|Z3|K#|!|>|"
    "[2|L#|)|05|L#|#|+|05|M#|\"|,|05|N#|#|\"|!|2|15|Q#|E!|*|05|R#|!|>|44|S#|"
    ")|L6|S#|\"|+|L6|T#|#|,|L6|U#|\"|#|!|2|M6|X#|F!|*|L6|Y#|!|>|N5|Z#|)|%8|Z"
    "#|#|+|%8|[#|\"|,|%8|\\#|#|\"|!|2|&8|_#|G!|*|%8| $|!|>|'7|!$|)|99|!$|\"|"
    "+|99|\"$|#|,|99|#$|\"|#|!|2|:9|&$|H!|*|99|'$|!|>|;8|($|)|P:|($|#|+|P:|)"
    "$|\"|,|P:|*$|#|\"|!|2|Q:|-$|I!|*|P:|.$|!|>|R9|/$|)|&<|/$|\"|+|&<|0$|#|,"
    "|&<|1$|\"|#|!|2|'<|4$|J!|*|&<|5$|!|>|(;|6$|)|>=|6$|#|+|>=|7$|\"|,|>=|8$"
    "|#|\"|!|2|?=|;$|K!|*|>=|<$|!|>|@<|=$|)|[>|=$|\"|+|[>|>$|#|,|[>|?$|\"|#|"
    "!|2|\\>|B$|L!|*|[>|C$|!|>|]=|D$|)|9@|D$|#|+|9@|E$|\"|,|9@|F$|#|\"|!|2|:"
    "@|I$|?!|-|K@|J$||#|!~5|J@|M$||*|9@|N$|!|>|;?|O$|}L|%|}M||0|!|1||Temp$0|"
    "|Temp$1||token_count|}N|!|}O||}P|}Q|}"
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
//  set_ast_kind_map                                                    
//  ----------------                                                    
//                                                                   
//  As part of the bootstrap procedure *only* we need the ability to 
//  create a skeletal parser that only contains the Ast type map.    
//

void ParserImpl::set_ast_kind_map(const std::map<std::string, int>& ast_kind_map)
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

        prsd->set_kind_map(ast_kind_map);

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
                          const std::map<std::string, int>& ast_kind_map,
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

        prsd->set_kind_map(ast_kind_map);
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
                          const std::map<std::string, int>& ast_kind_map,
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

        prsd->set_kind_map(ast_kind_map);
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
//  get_ast_kind_map                                                     
//  ----------------                                                     
//                                                                       
//  Get the current ast_kind_map. The client can use this to find out if 
//  he forgot to define any important kind strings.                      
//

map<string, int> ParserImpl::get_ast_kind_map() const
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
    encode_int64(kind_map.size(), ost);

    for (auto mp: kind_map)
    {
        encode_string(mp.first, ost);
        encode_int64(mp.second, ost);
    }

    encode_ast(ast, ost);

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

    encode_int64(message_list.size(), ost);

    for (ErrorMessage& message: message_list)
    {
        encode_int64(message.get_type(), ost);
        encode_string(message.get_tag(), ost);
        encode_int64(message.get_severity(), ost);
        encode_int64(message.get_location(), ost);
        encode_int64(message.get_line_num(), ost);
        encode_int64(message.get_column_num(), ost);
        encode_string(message.get_source_line(), ost);
        encode_string(message.get_short_message(), ost);
        encode_string(message.get_long_message(), ost);
        encode_string(message.get_string(), ost);
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
                        const map<string, int>& ast_kind_map)
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
        prsd->decode(str, ast_kind_map);

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
//  Primitive String Encoders                                             
//  -------------------------                                             
//                                                                        
//  Encode small types as strings. we use these to marshall structures to 
//  send back to client languages.                                        
//

void ParserImpl::encode_int64(const int64_t i, ostream& os) const
{
    os << i << "|";
}

void ParserImpl::encode_string(const string& s, ostream& os) const
{
    encode_int64(s.length(), os);
    os << s << "|";
}

void ParserImpl::encode_ast(const Ast* a, ostream& os) const
{

    encode_int64(a->get_kind(), os);
    encode_string(a->get_lexeme(), os);
    encode_int64(a->get_location(), os);
    encode_int64(a->get_num_children(), os);

    for (int i = 0; i < a->get_num_children(); i++)
    {
        encode_ast(a->get_child(i), os);
    }

}

} // namespace hoshi

