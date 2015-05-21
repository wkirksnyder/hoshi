//
//  DebugType                                                       
//  ----------                                                       
//                                                                   
//  We allow quite a few debugging options. It's the only way to get 
//  through such a large library.                                    
//

package hoshi;

import java.lang.*;
import java.util.*;

public enum DebugType {

    DebugProgress    (1 <<  0),
    DebugAstHandlers (1 <<  1),
    DebugGrammar     (1 <<  2),
    DebugGrammarAst  (1 <<  3),
    DebugLalr        (1 <<  4),
    DebugScanner     (1 <<  5),
    DebugActions     (1 <<  6),
    DebugICode       (1 <<  7),
    DebugVCodeExec   (1 <<  8),
    DebugScanToken   (1 <<  9),
    DebugParseAction (1 << 10);

    private long bits;

    DebugType(long bits) {
        this.bits = bits;
    }

    long debugNone() {
        return 0;
    }

    long debugAll() {
        return -1;
    }

    long debugFlags(DebugType ... types) {

        long bits = 0;
        for (DebugType debugType: types) {
            bits |= debugType.bits;
        }

        return bits;

    }

}
