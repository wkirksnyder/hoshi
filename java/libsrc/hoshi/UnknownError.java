//
//  UnknownError                                                 
//  ------------                                                 
//                                                               
//  Error thrown when there is an unidentifiable error.
//

package hoshi;

import java.lang.*;
import java.util.*;

public class UnknownError extends RuntimeException {

    private static final Initializer initializer = Initializer.getInitializer();

    public UnknownError(String message) {
        super(message);
    }

}
