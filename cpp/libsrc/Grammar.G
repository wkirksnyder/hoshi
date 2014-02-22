//
//  Main Grammar
//  ------------
//
//  Grammar file for the main part of the grammar.
//

options

    lookaheads = 4
    conflicts = 0
    case_sensitive = true

tokens

    <comment>            : regex = ''' {cpp_comment} '''
                           ignore = true

    <whitespace>         : regex = ''' {whitespace} '''
                           ignore = true

    <integer>            : regex = ''' [0-9]+ '''

    <identifier>         : regex = ''' [A-Za-z][a-zA-Z0-9_]* '''
                           precedence = 50

    <bracketstring>      : regex = [ !in_guard ] => ''' <[a-zA-Z][a-zA-Z0-9_]*> '''

    <string>             : regex = ''' ' ( \\ [^\n] | [^'\\\n] )* ' |
                                       " ( \\ [^\n] | [^"\\\n] )* " '''

    <stringerror>        : regex = ''' ' ( \\ [^\n] | [^'\\\n] )* \n |
                                       " ( \\ [^\n] | [^"\\\n] )* \n '''
                           error = "Missing closing quote on string literal"

    <triplestring>       : regex = "''' ( [^'] | '[^'] | ''[^'] )* '''"

    <triplestringerror>  : regex = "''' ( [^'] | '[^'] | ''[^'] )*"
                           error = "Missing closing quote on triple quoted string literal"

    '['                  : action = [ in_guard := 1; ]

    ']'                  : action = [ in_guard := 0; ]

    '<'                  : regex = [ in_guard ] => '<'

rules

    //
    //  Overall Structure
    //  -----------------
    //

    Grammar              ::= OptionSection?
                             TokenSection?
                             RuleSection
                         :   (AstGrammar, @3,
                                 (AstOptionList, @1, $1._),
                                 (AstTokenList, @2, $2._),
                                 (AstRuleList, @3, $3._)
                             )

    //
    //  Option Sublanguage
    //  ------------------
    //

    OptionSection        ::= 'options' OptionSpec*
                         :   (AstOptionList, $2._)

    OptionSpec           ::= 'lookaheads' '=' IntegerValue
                         :   (AstLookaheads, $3)

    OptionSpec           ::= 'conflicts' '=' IntegerValue
                         :   (AstConflicts, $3)

    OptionSpec           ::= 'keep_whitespace' '=' BooleanValue
                         :   (AstKeepWhitespace, $3)

    OptionSpec           ::= 'case_sensitive' '=' BooleanValue
                         :   (AstCaseSensitive, $3)

    //
    //  Token Sublanguage
    //  -----------------
    //

    TokenSection         ::= 'tokens' TokenSpec*
                         :   (AstTokenList, $2._)

    TokenSpec            ::= TerminalSymbol
                         :   (AstTokenDeclaration, $1, (AstTokenOptionList))

    TokenSpec            ::= TerminalSymbol ':' TokenOption*
                         :   (AstTokenDeclaration, $1, (AstTokenOptionList, @3, $3._))

    TokenOption          ::= 'template' '=' TerminalSymbol
                         :   (AstTokenTemplate, $3)

    TokenOption          ::= 'description' '=' StringValue
                         :   (AstTokenDescription, $3)

    TokenOption          ::= 'regex' '=' RegexString
                         :   (AstTokenRegexList, (AstTokenRegex, (AstNull, @"-1"), $3))

    TokenOption          ::= 'regex' '=' TokenRegex+
                         :   (AstTokenRegexList, $3._)

    TokenRegex           ::= '[' ActionExpression ']' '=>' RegexString
                         :   (AstTokenRegex, $2, $5)

    RegexString          ::= StringValue
                         :   (AstRegexString, $1)

    TokenOption          ::= 'precedence' '=' IntegerValue
                         :   (AstTokenPrecedence, $3)

    TokenOption          ::= 'action' '=' '[' ActionStatementList ']'
                         :   (AstTokenAction, $4)

    TokenOption          ::= 'lexeme' '=' BooleanValue
                         :   (AstTokenLexeme, $3)

    TokenOption          ::= 'ignore' '=' BooleanValue
                         :   (AstTokenIgnore, $3)

    TokenOption          ::= 'error' '=' StringValue
                         :   (AstTokenError, $3)

    //
    //  Rule Sublanguage
    //  ----------------
    //

    RuleSection          ::= 'rules'? Rule+
                         :   (AstRuleList, $2._)

    Rule                 ::= NonterminalReference '::=' RuleRhsList ReduceActions
                         :   (AstRule, @2, $1, $3, $4.1, $4.2)

    ReduceActions        ::= ':' ReduceAstAction ':' ReduceGuardAction
                         :   ($2, $4)

    ReduceActions        ::= ':' ReduceAstAction
                         :   ($2, (AstNull, @"-1"))

    ReduceActions        ::= empty
                         :   ((AstNull, @"-1"), (AstNull, @"-1"))

    RuleRhsList          ::= RuleRhs ( '|' RuleRhs : $2 )*
                         :   (AstRuleRhsList, $1, $2._)

    RuleRhs              ::= RuleSequenceTerm+
                         :   (AstRuleRhs, $1._)

    RuleSequenceTerm     ::= RuleUnopTerm '*'
                         :   (AstZeroClosure, @2, $1)

    RuleSequenceTerm     ::= RuleUnopTerm '+'
                         :   (AstOneClosure, @2, $1)

    RuleSequenceTerm     ::= RuleUnopTerm '?'
                         :   (AstOptional, @2, $1)

    RuleSequenceTerm     ::= RuleUnopTerm

    RuleUnopTerm         ::= SymbolReference

    RuleUnopTerm         ::= 'empty'
                         :   (AstEmpty)

    RuleUnopTerm         ::= '(' RuleRhsList ReduceActions ')'
                         :   (AstGroup, $2, $3.1, $3.2)

    Rule                 ::= NonterminalReference '::^' SymbolReference PrecedenceSpec+
                         :   (AstRulePrecedence, @2, $1, $3, (AstRulePrecedenceList, @4, $4._))

    PrecedenceSpec       ::= Assoc Operator+
                         :   (AstRulePrecedenceSpec, $1, (AstRuleOperatorList, @2, $2._))

    Assoc                ::= '<<'
                         :   (AstRuleLeftAssoc)

    Assoc                ::= '>>'
                         :   (AstRuleRightAssoc)

    Operator             ::= SymbolReference ReduceActions
                         :   (AstRuleOperatorSpec, $1, $2.1, $2.2)

    SymbolReference      ::= TerminalReference

    SymbolReference      ::= NonterminalReference

    TerminalReference    ::= TerminalSymbol
                         :   (AstTerminalReference, $1)

    NonterminalReference ::= NonterminalSymbol
                         :   (AstNonterminalReference, $1)

    //
    //  Ast Former Sublanguage
    //  ----------------------
    //

    ReduceAstAction      ::= AstFormer

    ReduceAstAction      ::= AstChild

    ReduceAstAction      ::= empty
                         :   (AstNull)

    AstFormer            ::= '(' ( AstItem ( ','? AstItem : $2 )* : ($1, $2._) )? ')'
                         :   (AstAstFormer, $2._)

    AstItem              ::= AstChild

    AstChild             ::= '$' AstChildSpec '.' AstSliceSpec
                         :   (AstAstChild, $2, $4)

    AstChild             ::= '$' AstChildSpec
                         :   (AstAstChild, $2, (AstNull, @"-1"))

    AstChild             ::= '$' AstSliceSpec
                         :   (AstAstChild, (AstNull, @"-1"), $2)

    AstItem              ::= Identifier
                         :   (AstIdentifier, &1)

    AstItem              ::= '%' AstChildSpec
                         :   (AstAstKind, $2)

    AstItem              ::= '@' AstChildSpec
                         :   (AstAstLocation, $2)

    AstItem              ::= '@' StringValue
                         :   (AstAstLocationString, $2)

    AstItem              ::= '&' AstChildSpec
                         :   (AstAstLexeme, $2)

    AstItem              ::= '&' StringValue
                         :   (AstAstLexemeString, $2)

    AstItem              ::= AstFormer

    AstChildSpec         ::= AstChildNumber ( '.' AstChildNumber : $2 )*
                         :   (AstAstDot, $1, $2._)

    AstSliceSpec         ::= AstFirstChildNumber '_' AstLastChildNumber
                         :   (AstAstSlice, $1, $3)

    AstFirstChildNumber  ::= AstChildNumber

    AstFirstChildNumber  ::= empty
                         :   (AstInteger, &"1")

    AstLastChildNumber   ::= AstChildNumber

    AstLastChildNumber   ::= empty
                         :   (AstNegativeInteger, &"1")

    AstChildNumber       ::= '-' <integer>
                         :   (AstNegativeInteger, &2)

    AstChildNumber       ::= <integer>
                         :   (AstInteger, &1)

    ReduceGuardAction    ::= '[' ActionStatementList ']'
                         :   $2

    //
    //  Action Sublanguage
    //  ------------------
    //

    ActionStatementList  ::= ActionStatement*
                         :   (AstActionStatementList, $1._)

    ActionStatement      ::= Identifier ':=' ActionExpression ';'
                         :   (AstActionAssign, @2, $1, $3)

    ActionStatement      ::= 'dump_stack' ';'
                         :   (AstActionDumpStack)

    ActionExpression     ::^ ActionUnopExpression << '|'  : (AstActionOr, $1, $2)
                                                  << '&'  : (AstActionAnd, $1, $2)
                                                  << '='  : (AstActionEqual, $1, $2)
                                                     '/=' : (AstActionNotEqual, $1, $2)
                                                     '<'  : (AstActionLessThan, $1, $2)
                                                     '<=' : (AstActionLessEqual, $1, $2)
                                                     '>'  : (AstActionGreaterThan, $1, $2)
                                                     '>=' : (AstActionGreaterEqual, $1, $2)
                                                  << '+'  : (AstActionAdd, $1, $2)
                                                     '-'  : (AstActionSubtract, $1, $2)
                                                  << '*'  : (AstActionMultiply, $1, $2)
                                                     '/'  : (AstActionDivide, $1, $2)

    ActionUnopExpression ::= '-' ActionUnopTerm
                         :   (AstActionUnaryMinus, $2)

    ActionUnopExpression ::= '!' ActionUnopTerm
                         :   (AstActionNot, $2)

    ActionUnopExpression ::= ActionUnopTerm

    ActionUnopTerm       ::= '(' ActionExpression ')'
                         :   $2

    ActionUnopTerm       ::= IntegerValue

    ActionUnopTerm       ::= Identifier

    ActionUnopTerm       ::= 'token_count'
                         :   (AstActionTokenCount)

    //
    //  Literal Wrappers
    //  ----------------
    //

    TerminalSymbol       ::= <string>
                         :   (AstString, &1)

    TerminalSymbol       ::= <bracketstring>
                         :   (AstString, &1)

    NonterminalSymbol    ::= <identifier>
                         :   (AstIdentifier, &1)

    IntegerValue         ::= <integer>
                         :   (AstInteger, &1)

    BooleanValue         ::= 'true'
                         :   (AstTrue)

    BooleanValue         ::= 'false'
                         :   (AstFalse)

    StringValue          ::= <string>
                         :   (AstString, &1)

    StringValue          ::= <triplestring>
                         :   (AstTripleString, &1)

    Identifier           ::= <identifier>
                         :   (AstIdentifier, &1)
