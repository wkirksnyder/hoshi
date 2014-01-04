//
//  GrammarError                                                 
//  ------------                                                 
//                                                               
//  Exception thrown when there is any kind of error in the grammar. 
//

package hoshi;

import java.lang.*;
import java.util.*;

public class GrammarError extends RuntimeException {

    private static final Initializer initializer = Initializer.getInitializer();

    public GrammarError(String message) {
        super(message);
    }

}
