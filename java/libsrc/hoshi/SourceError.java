//#line 334 "u:\\hoshi\\raw\\JavaWrapper.java"
//
//  SourceError                                                 
//  -----------                                                 
//                                                               
//  Exception thrown when there is any kind of error in the source. 
//

package hoshi;

import java.lang.*;
import java.util.*;

public class SourceError extends RuntimeException {

    private static final Initializer initializer = Initializer.getInitializer();

    public SourceError(String message) {
        super(message);
    }

}
