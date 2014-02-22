//#line 391 "u:\\hoshi\\raw\\JavaWrapper.java"
//
//  TypeConvert                                                           
//  -----------                                                           
//                                                                        
//  Miscellaneous conversion routines for abstract (language independent) 
//  types and marshalling utilities.
//

package hoshi;

import java.lang.*;
import java.util.*;
import java.io.*;

class TypeConvert {

    //
    //  kindMapOut                                                       
    //  ----------                                                       
    //                                                                   
    //  Accept a Java HashMap of a KindMap and marshall as a string. 
    //  This is the easiest way to transport an aggregate across the     
    //  interlanguage barrier.                                           
    //

    public static String kindMapOut(HashMap<String, Integer> kindMap) {

        StringWriter writer = new StringWriter();

        encodeLong(writer, kindMap.size());

        for (String key: kindMap.keySet()) {
            encodeString(writer, key);
            encodeLong(writer, kindMap.get(key));
        }

        return writer.toString();

    }

    //
    //  encodeLong             
    //  ----------             
    //                         
    //  Marshall a long value. 
    //

    public static void encodeLong(StringWriter writer, long value) {
        writer.write(value + "|");
    }

    //
    //  decodeLong               
    //  ----------               
    //                           
    //  Unmarshall a long value. 
    //

    public static long decodeLong(StringReader reader) {

        StringBuilder buffer = new StringBuilder();
        long result = 0;

        try {

            for (;;) {
        
                char c = (char)reader.read();

                if (c == '`') {
                    c = (char)reader.read();
                } else if (c == '|') {
                    break;
                }
                
                buffer.append(c);

            }
            
            long negative = 1;

            if (buffer.charAt(0) == '-') {
                negative = -1;
                buffer.deleteCharAt(0);
            }

            result = negative * Long.parseLong(buffer.toString());

        } catch (IOException e) {
            System.err.println("Internal Error: String decoding error");
            Runtime.getRuntime().halt(1);
        } catch (NumberFormatException e) {
            System.err.println("Internal Error: Number Format error");
            Runtime.getRuntime().halt(1);
        }

        return result;

    }

    //
    //  encodeString       
    //  ------------       
    //                     
    //  Marshall a string. 
    //

    public static void encodeString(StringWriter writer, String value) {

        for (int i = 0, n = value.length(); i < n; i++) {
            
            char c = value.charAt(i);

            if (c == '`' || c == '|') {
                writer.write('`');
            }

            writer.write(c);

        }

        writer.write("|");

    }

    //
    //  decodeString       
    //  ------------       
    //
    //  Unmarshall a string. 
    //

    public static String decodeString(StringReader reader) {

        StringBuilder buffer = new StringBuilder();

        try {

            for (;;) {
        
                char c = (char)reader.read();

                if (c == '`') {
                    c = (char)reader.read();
                } else if (c == '|') {
                    break;
                }
                
                buffer.append(c);

            }
            
        } catch (IOException e) {
            System.err.println("Internal Error: String decoding error");
            Runtime.getRuntime().halt(1);
        }

        return buffer.toString();

    }

    //
    //  decodeErrorType                                
    //  ---------------                                
    //                                                 
    //  Unmarshall an error type.
    //

    public static ErrorType decodeErrorType(StringReader reader) {
        return ErrorType.getErrorType((int)decodeLong(reader));    
    }    

    //
    //  decodeKindMap          
    //  -------------          
    //                         
    //  Unmarshall a kind map. 
    //

    public static HashMap<String, Integer> decodeKindMap(StringReader reader) {

        HashMap<String, Integer> result = new HashMap<String, Integer>();

        int size = (int)decodeLong(reader);
        while (size-- > 0) {
            String key = decodeString(reader);
            int value = (int)decodeLong(reader);
            result.put(key, value);
        }

        return result;

    }

    //
    //  decodeAst               
    //  ---------               
    //                          
    //  Unmarshall an Ast node. 
    //

    public static Ast decodeAst(StringReader reader) {

        int numChildren = (int)decodeLong(reader);

        if (numChildren < 0) {
            return null;
        }

        Ast result = new Ast(numChildren);

        result.setKind((int)decodeLong(reader)); 
        result.setLocation(decodeLong(reader)); 
        result.setLexeme(decodeString(reader)); 
        
        for (int i = 0; i < numChildren; i++) {
            result.setChild(i, decodeAst(reader));
        }

        return result;

    }

    //
    //  decodeErrorMessage
    //  ------------------                    
    //                               
    //  Unmarshall an error message. 
    //

    public static ErrorMessage decodeErrorMessage(StringReader reader) {

        ErrorMessage message = new ErrorMessage();

        message.setType(decodeErrorType(reader));
        message.setTag(decodeString(reader));
        message.setSeverity((int)decodeLong(reader));
        message.setLocation(decodeLong(reader));
        message.setLineNum(decodeLong(reader));
        message.setColumnNum(decodeLong(reader));
        message.setSourceLine(decodeString(reader));
        message.setShortMessage(decodeString(reader));
        message.setLongMessage(decodeString(reader));
        message.setString(decodeString(reader));

        return message;

    }

}
