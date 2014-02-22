//#line 649 "u:\\hoshi\\raw\\JavaWrapper.java"
//
//  Parser                                                                 
//  ------                                                                 
//                                                                         
//  This is a modal parser object. It's created in an empty state. Then we 
//  generate a parser into it with a grammar. With that done we can use it 
//  to parse source.                                                       
//

package hoshi;

import java.io.*;
import java.lang.*;
import java.util.*;

public class Parser {

    private static final Initializer initializer = Initializer.getInitializer();

    private long thisHandle = 0;
    private HashMap<String, Integer> kindMap = null;
    private HashMap<Integer, String> kindIMap = null;

    //
    //  loadFile                                     
    //  --------                                     
    //                                               
    //  Utility method to load a file into a string. 
    //

    public static String loadFile(String fileName) {

        DataInputStream file = null;

        try {

            file = new DataInputStream(new FileInputStream(fileName));
            byte [] bytes = new byte[(int)((new File(fileName)).length())];

            file.readFully(bytes);

            return new String(bytes, "UTF-8");

        } catch (Exception e) {

            throw new hoshi.UnknownError("Internal Error: File input error");

        } finally {

            try {
                if (file != null) {
                    file.close();
                }
            } catch (Exception e) {
            }

        }

    }

    //
    //  Parser()                                       
    //  --------                                       
    //                                                 
    //  Create a parser instance in the Hoshi library. 
    //

    public Parser() {
        thisHandle = new_parser();
    }

    public Parser(Parser rhs) {
        thisHandle = clone_parser(rhs.thisHandle);
    }

    //
    //  ~Parser()                                                       
    //  ---------                                                       
    //                                                                  
    //  The Hoshi library keeps some storage for a parser. We have to   
    //  delete it when the garbage collector collects the Java version. 
    //

    protected void finalize() throws Throwable {
        delete_parser(thisHandle);
    }

//#line 918 "u:\\hoshi\\raw\\JavaWrapper.java"
    //
    //  isGrammarLoaded
    //  ---------------
    //  
    //  Check whether the parser has a grammer loaded.
    //

    public boolean isGrammarLoaded() {
        return is_grammar_loaded(thisHandle);
    }

    //
    //  isGrammarFailed
    //  ---------------
    //  
    //  Check whether the parser has a failed grammar.
    //

    public boolean isGrammarFailed() {
        return is_grammar_failed(thisHandle);
    }

    //
    //  isSourceLoaded
    //  --------------
    //  
    //  Check whether the parser has a source loaded.
    //

    public boolean isSourceLoaded() {
        return is_source_loaded(thisHandle);
    }

    //
    //  isSourceFailed
    //  --------------
    //  
    //  Check whether the parser has a grammer failed.
    //

    public boolean isSourceFailed() {
        return is_source_failed(thisHandle);
    }

    //
    //  generate
    //  --------
    //  
    //  Generate a parser from a grammar file.
    //
    
    public void generate(String source) {
        generate(source, new HashMap<String, Integer>());
    }
    
    public void generate(String source, HashMap<String, Integer> kindMap) {
        generate(source, kindMap, 0);
    }
    
    public void generate(String source, 
                         HashMap<String, Integer> kindMap, 
                         long debugFlags) {
        generate(thisHandle, source, TypeConvert.kindMapOut(kindMap), debugFlags);
        copyKindMap();
    }

    //
    //  parse
    //  -----
    //  
    //  Parse a source string saving the Ast and error messages.
    //

    public void parse(String source) {
        parse(source, 0);
    }
    
    public void parse(String source, long debugFlags) {
        parse(thisHandle, source, debugFlags);
    }

    //
    //  getAst
    //  ------
    //  
    //  Return the Ast.
    //
    
    public Ast getAst() {

        try {

            String marshalled = get_encoded_ast(thisHandle);
            StringReader reader = new StringReader(marshalled);
            kindMap = TypeConvert.decodeKindMap(reader);

            kindIMap = new HashMap<Integer, String>();
            for (String key: kindMap.keySet()) {
                kindIMap.put(kindMap.get(key), key);
            }

            Ast ast = TypeConvert.decodeAst(reader);

            return ast;

        } catch (hoshi.UnknownError e) {
            throw e;
        } catch (Throwable e) {
            System.err.println("Internal Error: Unexpected exception");
            Runtime.getRuntime().halt(1);
        }

        return null;

    }

    //
    //  dumpAst
    //  -------
    //  
    //  For debugging we'll provide a nice dump function.
    //
    
    public void dumpAst(Ast ast) {
        dumpAst(ast, 0);
    }
    
    public void dumpAst(Ast ast, int indent) {

        if (indent > 0) {
            System.out.print(String.format("%" + indent + "s", ""));
        }

        if (ast == null) {
            System.out.println("Nullptr");
            return;
        }

        System.out.print(getKindString(ast.getKind()) + "(" + ast.getKind() + ")");

        if (ast.getLexeme().length() > 0) {
            System.out.print(" [" + ast.getLexeme() + "]");
        }

        if (ast.getLocation() >= 0) {
            System.out.print(" @ " + ast.getLocation());
        }

        System.out.println();

        for (Ast child: ast.getChildren()) {
            dumpAst(child, indent + 4);
        }

    }

    //
    //  copyKindMap
    //  -----------
    //  
    //  Get the kind map from the native library.
    //
    
    public void copyKindMap() {

        try {

            String marshalled = get_encoded_kind_map(thisHandle);
            StringReader reader = new StringReader(marshalled);
            kindMap = TypeConvert.decodeKindMap(reader);

            kindIMap = new HashMap<Integer, String>();
            for (String key: kindMap.keySet()) {
                kindIMap.put(kindMap.get(key), key);
            }

        } catch (hoshi.UnknownError e) {
            throw e;
        } catch (Throwable e) {
            System.err.println("Internal Error: Unexpected exception");
            Runtime.getRuntime().halt(1);
        }

    }

    //
    //  getKind
    //  -------
    //  
    //  Get the integer code for a given string.
    //
    
    public int getKind(String kindString) {

        if (kindMap == null) {
            copyKindMap();
        }

        if (!kindMap.containsKey(kindString)) {
            return -1;
        }

        return kindMap.get(kindString);

    }

    //
    //  getKindForce
    //  ------------
    //  
    //  Get the integer code for a given string. If it doesn't exist then install
    //  it.
    //
    
    public int getKindForce(String kindString) {

        if (kindMap == null) {
            copyKindMap();
        }

        if (!kindMap.containsKey(kindString)) {
            get_kind_force(thisHandle, kindString);
            copyKindMap();
        }

        return kindMap.get(kindString);

    }

    //
    //  getKindString
    //  -------------
    //  
    //  Get the text name for a numeric kind code.
    //
    
    public String getKindString(int kind) {

        if (kindIMap == null) {
            copyKindMap();
        }

        if (!kindIMap.containsKey(kind)) {
            return "Unknown";
        }

        return kindIMap.get(kind);

    }

    //
    //  getKindString
    //  -------------
    //  
    //  Get the text name for an Ast.
    //
    
    public String getKindString(Ast ast) {
        return getKindString(ast.getKind());
    }

    //
    //  addError
    //  --------
    //  
    //  Add another error to the message list. This is provided so that clients
    //  can use the parser message handler for all errors, not just parsing
    //  errors.
    //

    public void addError(int errorType, long location, String shortMessage) {
        addError(errorType, location, shortMessage, "");
    }
    
    public void addError(int errorType, 
                         long location, 
                         String shortMessage, 
                         String longMessage) {
        
        add_error(thisHandle, 
                  errorType, 
                  location, 
                  shortMessage, 
                  longMessage);
        
    }

    //
    //  getErrorCount
    //  -------------
    //  
    //  Return the number of error messages over the error threshhold.
    //

    public int getErrorCount() {
        return get_error_count(thisHandle);
    }

    //
    //  getWarningCount
    //  ---------------
    //  
    //  Return the number of error messages under the error threshhold.
    //

    public int getWarningCount() {
        return get_warning_count(thisHandle);
    }

    //
    //  getErrorMessages
    //  ----------------
    //  
    //  Return the error messages.
    //
    
    public ArrayList<ErrorMessage> getErrorMessages() {

        ArrayList<ErrorMessage> messages = new ArrayList<ErrorMessage>();

        try {

            String marshalled = get_encoded_error_messages(thisHandle);
            StringReader reader = new StringReader(marshalled);

            int size = (int)TypeConvert.decodeLong(reader);

            while (size-- > 0) {
                messages.add(TypeConvert.decodeErrorMessage(reader));
            }

        } catch (hoshi.UnknownError e) {
            throw e;
        } catch (Throwable e) {
            System.err.println("Internal Error: Unexpected exception");
            Runtime.getRuntime().halt(1);
        }

        return messages;

    }

    //
    //  getSourceList
    //  -------------
    //  
    //  Return a source list with embedded messages.
    //

    public String getSourceList(String source) {
        return getSourceList(source, 0);
    }
    
    public String getSourceList(String source, int indent) {
        return get_source_list(thisHandle, source, indent);
    }

    //
    //  encode
    //  ------
    //  
    //  Create a string encoding of a Parser.
    //

    public String encode() {
        return encode(thisHandle);
    }

    //
    //  decode
    //  ------
    //  
    //  Decode a previously created string into a parser
    //
    
    public void decode(String str) {
        decode(str, new HashMap<String, Integer>());
    }
    
    public void decode(String str, HashMap<String, Integer> kindMap) {
        decode(thisHandle, str, TypeConvert.kindMapOut(kindMap));
        copyKindMap();
    }

//#line 919 "u:\\hoshi\\raw\\JavaWrapper.java"
    //
    //  Declarations of functions in the Hoshi native library. 
    //

    private static native long new_parser();
    private static native long clone_parser(long parserHandle);
    private static native void delete_parser(long parserHandle);
    private static native boolean is_grammar_loaded(long thisHandle);
    private static native boolean is_grammar_failed(long thisHandle);
    private static native boolean is_source_loaded(long thisHandle);
    private static native boolean is_source_failed(long thisHandle);
    private static native void generate(long thisHandle, 
                                        String source, 
                                        String kindMap, 
                                        long debugFlags);
    private static native void parse(long thisHandle, 
                                     String source, 
                                     long debugFlags);
    private static native String get_encoded_ast(long thisHandle);
    private static native String get_encoded_kind_map(long thisHandle);
    private static native int get_kind(long thisHandle, String kindString);
    private static native int get_kind_force(long thisHandle, 
                                             String kindString);
    private static native String get_kind_string(long thisHandle, int kind);
    private static native void add_error(long thisHandle, 
                                         int errorType, 
                                         long location, 
                                         String shortMessage, 
                                         String longMessage);
    private static native int get_error_count(long thisHandle);
    private static native int get_warning_count(long thisHandle);
    private static native String get_encoded_error_messages(long thisHandle);
    private static native String get_source_list(long thisHandle, 
                                                 String source, 
                                                 int indent);
    private static native String encode(long thisHandle);
    private static native void decode(long thisHandle, 
                                      String str, 
                                      String kindMap);
//#line 924 "u:\\hoshi\\raw\\JavaWrapper.java"

}
