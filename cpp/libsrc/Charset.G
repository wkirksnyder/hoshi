//
//  Character Set Grammar
//  ---------------------
//
//  Grammar file for the scanner regular expression character sets.
//

options

    lookaheads = 2
    conflicts = 0
    keep_whitespace = true
    case_sensitive = true

tokens

    <char>               : regex = ''' [^\^\-\[\]\$] '''

rules       
        
    CharsetRoot          ::= '^' CharsetList
                         :   (AstCharsetInvert, $2._)

    CharsetRoot          ::= CharsetList

    CharsetList          ::= CharsetItem+
                         :   (AstCharset, $1._)

    CharsetItem          ::= Char '-' Char
                         :   (AstCharsetRange, $1, $3, @2)

    CharsetItem          ::= Charset

    CharsetItem          ::= Char
                         :   (AstCharsetRange, $1)

    Charset              ::= '\\s' 
                         :   (AstCharsetWhitespace)

    Charset              ::= '\\S' 
                         :   (AstCharsetNotWhitespace)

    Charset              ::= '\\d' 
                         :   (AstCharsetDigits)

    Charset              ::= '\\D' 
                         :   (AstCharsetNotDigits)

    Char                 ::= '\\\\' 
                         :   (AstCharsetEscape)

    Char                 ::= '$' 
                         :   (AstCharsetAltNewline)

    Char                 ::= '\\n' 
                         :   (AstCharsetNewline)

    Char                 ::= '\\r' 
                         :   (AstCharsetCr)

    Char                 ::= '\\^' 
                         :   (AstCharsetCaret)

    Char                 ::= '\\-' 
                         :   (AstCharsetDash)

    Char                 ::= '\\$' 
                         :   (AstCharsetDollar)

    Char                 ::= '\\[' 
                         :   (AstCharsetLeftBracket)

    Char                 ::= '\\]' 
                         :   (AstCharsetRightBracket)

    Char                 ::= <char>
                         :   (AstCharsetChar, &1)


