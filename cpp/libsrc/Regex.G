//
//  Regular Expression Grammar
//  --------------------------
//
//  Grammar file for the scanner regular expressions.
//

options

    lookaheads = 2
    conflicts = 0
    case_sensitive = true

tokens

    <whitespace>         : regex = ''' \s+ '''
                           ignore = true

    <macro>              : regex = ''' \{ [^}]* \} '''
        
    <charset>            : regex = ''' \[ ( [^\]] | \\ \] )* \] '''
        
    <char>               : regex = ''' [^?+*()\[\]{}|.\$\s] '''
        
rules       
        
    Regex                ::= RegexOr
                         :   (AstRegex, $1)

    RegexOr              ::= RegexOrTerm ( '|' RegexOrTerm : $2 )+
                         :   (AstRegexOr, $1, $2._)
        
    RegexOr              ::= RegexOrTerm
        
    RegexOrTerm          ::= RegexUnopTerm+
                         :   (AstRegexList, $1._)
        
    RegexUnopTerm        ::= RegexUnopTerm '*'
                         :   (AstRegexZeroClosure, $1)
        
    RegexUnopTerm        ::= RegexUnopTerm '+'
                         :   (AstRegexOneClosure, $1)
        
    RegexUnopTerm        ::= RegexUnopTerm '?'
                         :   (AstRegexOptional, $1)
        
    RegexUnopTerm        ::= '(' RegexOr ')'
                         :   $2
        
    RegexUnopTerm        ::= <macro>
                         :   (AstMacroString, &1)
        
    RegexUnopTerm        ::= Charset
        
    RegexUnopTerm        ::= Char
        
    Charset              ::= <charset>
                         :   (AstCharsetString, &1) 

    Charset              ::= '.' 
                         :   (AstRegexWildcard)

    Charset              ::= '\\s' 
                         :   (AstRegexWhitespace)

    Charset              ::= '\\S' 
                         :   (AstRegexNotWhitespace)

    Charset              ::= '\\d' 
                         :   (AstRegexDigits)

    Charset              ::= '\\D' 
                         :   (AstRegexNotDigits)

    Char                 ::= <char>
                         :   (AstRegexChar, &1)

    Char                 ::= '\\\\' 
                         :   (AstRegexEscape)

    Char                 ::= '$' 
                         :   (AstRegexAltNewline)

    Char                 ::= '\\n' 
                         :   (AstRegexNewline)

    Char                 ::= '\\r' 
                         :   (AstRegexCr)

    Char                 ::= '\\|' 
                         :   (AstRegexVBar)

    Char                 ::= '\\*' 
                         :   (AstRegexStar)

    Char                 ::= '\\+' 
                         :   (AstRegexPlus)

    Char                 ::= '\\?' 
                         :   (AstRegexQuestion)

    Char                 ::= '\\.' 
                         :   (AstRegexPeriod)

    Char                 ::= '\\$' 
                         :   (AstRegexDollar)

    Char                 ::= '\\b' 
                         :   (AstRegexSpace)

    Char                 ::= '\\(' 
                         :   (AstRegexLeftParen)

    Char                 ::= '\\)' 
                         :   (AstRegexRightParen)

    Char                 ::= '\\[' 
                         :   (AstRegexLeftBracket)

    Char                 ::= '\\]' 
                         :   (AstRegexRightBracket)

    Char                 ::= '\\{' 
                         :   (AstRegexLeftBrace)

    Char                 ::= '\\}' 
                         :   (AstRegexRightBrace)

