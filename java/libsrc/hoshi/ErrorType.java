//#line 236 "u:\\hoshi\\raw\\JavaWrapper.java"
//
//  ErrorType                                                              
//  ---------                                                              
//                                                                         
//  We'll encode each error message with an enumerated type. For now we're 
//  not going to do anything with this but at some point we may want to    
//  add options like classifying them as warnings or errors, disabling and 
//  so forth.                                                              
//

package hoshi;

import java.lang.*;
import java.util.*;

public enum ErrorType {

    ErrorError,
    ErrorWarning,
    ErrorUnknownMacro,
    ErrorDupGrammarOption,
    ErrorDupToken,
    ErrorDupTokenOption,
    ErrorUnusedTerm,
    ErrorUndefinedNonterm,
    ErrorUnusedNonterm,
    ErrorUselessNonterm,
    ErrorUselessRule,
    ErrorReadsCycle,
    ErrorSymbolSelfProduce,
    ErrorLalrConflict,
    ErrorWordOverflow,
    ErrorCharacterRange,
    ErrorRegexConflict,
    ErrorDupAstItem,
    ErrorSyntax,
    ErrorLexical,
    ErrorAstIndex;
//#line 267 "u:\\hoshi\\raw\\JavaWrapper.java"

    private int value;
    private static HashMap<Integer, ErrorType> valueMap = new HashMap<Integer, ErrorType>();

    static {

        valueMap.put( 0, ErrorError);
        valueMap.put( 1, ErrorWarning);
        valueMap.put( 2, ErrorUnknownMacro);
        valueMap.put( 3, ErrorDupGrammarOption);
        valueMap.put( 4, ErrorDupToken);
        valueMap.put( 5, ErrorDupTokenOption);
        valueMap.put( 6, ErrorUnusedTerm);
        valueMap.put( 7, ErrorUndefinedNonterm);
        valueMap.put( 8, ErrorUnusedNonterm);
        valueMap.put( 9, ErrorUselessNonterm);
        valueMap.put(10, ErrorUselessRule);
        valueMap.put(11, ErrorReadsCycle);
        valueMap.put(12, ErrorSymbolSelfProduce);
        valueMap.put(13, ErrorLalrConflict);
        valueMap.put(14, ErrorWordOverflow);
        valueMap.put(15, ErrorCharacterRange);
        valueMap.put(16, ErrorRegexConflict);
        valueMap.put(17, ErrorDupAstItem);
        valueMap.put(18, ErrorSyntax);
        valueMap.put(19, ErrorLexical);
        valueMap.put(20, ErrorAstIndex);
//#line 285 "u:\\hoshi\\raw\\JavaWrapper.java"

    }

    public static ErrorType getErrorType(int value) {

        if (valueMap.containsKey(value)) {
            return valueMap.get(value);
        }

        return null;

    }

}
