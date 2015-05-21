//
//  Initializer                                                           
//  -----------                                                           
//                                                                        
//  We need to load a native library for most of the features of this    
//  package. That's done here and every other class must have an instance 
//  of this one to guarantee that was done.                               
//

package hoshi;

import java.lang.*;
import java.util.*;

public class Initializer {

    private static final Initializer initializer = new Initializer();

    private static boolean initializedJni = false;
    private static Object initializerLock = new Object();

    static Initializer getInitializer() {
        
        if (!initializedJni) {

            synchronized (initializerLock) {
 
                if (!initializedJni) {
                    initialize_jni();
                    initializedJni = true;
                }

            }

        }

        return initializer;

    }

    private static native void initialize_jni();

    static {
        System.loadLibrary("hoshi");
    }

}
