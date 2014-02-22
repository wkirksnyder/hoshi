//
//  Hoshi Library                                                          
//  -------------                                                          
//                                                                         
//  This is the C# wrapper for the Hoshi Library. The library is really    
//  implemented in a C++ DLL, here what we have is a collection of proxies 
//  that call that DLL.                                                    
//

using System;
using System.Text;
using System.IO;
using System.Security;
using System.Threading;
using System.Collections.Generic;
using System.Runtime.InteropServices;

//
//  Namespace Hoshi: Not indenting...
//

namespace hoshi
{

//
//  Services 
//  -------- 
//
//  We need some services, primarily creating exceptions, which must be  
//  handled by asynchronous callbacks. I'm not sure this is the best way 
//  to program this, but what we're going to do is register callback     
//  handlers for each of those services here and call them as needed.    
//                                                                       
//  There is a similar concept in JNI, but there we initiate the process 
//  from the C++ side. In PInvoke we have to initiate it in .NET.        
//

internal unsafe class Services
{

    //
    //  GrammarError.
    //

    public delegate void GrammarErrorCreator(ref object exception,
                                             char *characters,
                                             int length);

    private static GrammarErrorCreator grammarErrorCreator = 
        delegate (ref object exception, char *characters, int length)
        {
            exception = new GrammarError(new string(characters, 0, length));
        };

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Services_register_grammar_error_creator"),
     SuppressUnmanagedCodeSecurity]
    private static extern void register_grammar_error_creator(
                                   GrammarErrorCreator creator);

    //
    //  SourceError.
    //

    public delegate void SourceErrorCreator(ref object exception,
                                            char *characters,
                                            int length);

    private static SourceErrorCreator sourceErrorCreator = 
        delegate (ref object exception, char *characters, int length)
        {
            exception = new SourceError(new string(characters, 0, length));
        };

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Services_register_source_error_creator"),
     SuppressUnmanagedCodeSecurity]
    private static extern void register_source_error_creator(
                                   SourceErrorCreator creator);

    //
    //  UnknownError.
    //

    public delegate void UnknownErrorCreator(ref object exception,
                                             char *characters,
                                             int length);

    private static UnknownErrorCreator unknownErrorCreator = 
        delegate (ref object exception, char *characters, int length)
        {
            exception = new UnknownError(new string(characters, 0, length));
        };

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Services_register_unknown_error_creator"),
     SuppressUnmanagedCodeSecurity]
    private static extern void register_unknown_error_creator(
                                   UnknownErrorCreator creator);

    //
    //  String.
    //

    public delegate void StringCreator(ref object target,
                                       char *characters,
                                       int length);

    private static StringCreator stringCreator = 
        delegate (ref object target, char *characters, int length)
        {
            target = new string(characters, 0, length);
        };

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Services_register_string_creator"),
     SuppressUnmanagedCodeSecurity]
    private static extern void register_string_creator(
                                   StringCreator creator);

    //
    //  register                                            
    //  --------                                            
    //                                                      
    //  This method registers each of our callback delegates.
    //

    public static void register()
    {
        register_grammar_error_creator(grammarErrorCreator);
        register_source_error_creator(sourceErrorCreator);
        register_unknown_error_creator(unknownErrorCreator);
        register_string_creator(stringCreator);
    }

}

//
//  Initializer                                                            
//  -----------                                                            
//                                                                         
//  We have to register services before we do anything else. So we include 
//  a static constructor on every other class that just calls this class.  
//

internal class Initializer
{
    
    private static volatile bool initializedPInvoke = false;
    private static object initializerLock = new object();

    public static void Initialize()
    {
        
        if (!initializedPInvoke)
        {

            lock (initializerLock)
            {
 
                if (!initializedPInvoke)
                {
                    Services.register();
                    initializedPInvoke = true;
                }

            }

        }

    }

}

//
//  DebugType                                                       
//  ----------                                                       
//                                                                   
//  We allow quite a few debugging options. It's the only way to get 
//  through such a large library.                                    
//

[Flags]
public enum DebugType : long
{
    DebugProgress    = 1 <<  0,
    DebugAstHandlers = 1 <<  1,
    DebugGrammar     = 1 <<  2,
    DebugGrammarAst  = 1 <<  3,
    DebugLalr        = 1 <<  4,
    DebugScanner     = 1 <<  5,
    DebugActions     = 1 <<  6,
    DebugICode       = 1 <<  7,
    DebugVCodeExec   = 1 <<  8,
    DebugScanToken   = 1 <<  9,
    DebugParseAction = 1 << 10
}

//
//  ErrorType                                                              
//  ---------                                                              
//                                                                         
//  We'll encode each error message with an enumerated type. For now we're 
//  not going to do anything with this but at some point we may want to    
//  add options like classifying them as warnings or errors, disabling and 
//  so forth.                                                              
//

public enum ErrorType : int
{
    ErrorError             =  0,
    ErrorWarning           =  1,
    ErrorUnknownMacro      =  2,
    ErrorDupGrammarOption  =  3,
    ErrorDupToken          =  4,
    ErrorDupTokenOption    =  5,
    ErrorUnusedTerm        =  6,
    ErrorUndefinedNonterm  =  7,
    ErrorUnusedNonterm     =  8,
    ErrorUselessNonterm    =  9,
    ErrorUselessRule       = 10,
    ErrorReadsCycle        = 11,
    ErrorSymbolSelfProduce = 12,
    ErrorLalrConflict      = 13,
    ErrorWordOverflow      = 14,
    ErrorCharacterRange    = 15,
    ErrorRegexConflict     = 16,
    ErrorDupAstItem        = 17,
    ErrorSyntax            = 18,
    ErrorLexical           = 19,
    ErrorAstIndex          = 20
}

//
//  GrammarError                                                 
//  ------------                                                 
//                                                               
//  Exception thrown when there is any kind of error in the grammar. 
//

public class GrammarError : Exception
{

    static GrammarError()
    {
        Initializer.Initialize();
    }

    public GrammarError(string message)
        : base(message)
    {
    }

}

//
//  SourceError                                                 
//  -----------                                                 
//                                                               
//  Exception thrown when there is any kind of error in the source. 
//

public class SourceError : Exception
{

    static SourceError()
    {
        Initializer.Initialize();
    }

    public SourceError(string message)
        : base(message)
    {
    }

}

//
//  UnknownError                                                 
//  ------------                                                 
//                                                               
//  Exception thrown when there is an unidentifiable error.
//

public class UnknownError : Exception
{

    static UnknownError()
    {
        Initializer.Initialize();
    }

    public UnknownError(string message)
        : base(message)
    {
    }

}

//
//  TypeConvert                                                           
//  -----------                                                           
//                                                                        
//  Miscellaneous conversion routines for abstract (language independent) 
//  types and marshalling utilities.
//

internal class TypeConvert
{

    //
    //  KindMapOut                                                         
    //  ----------                                                         
    //                                                                     
    //  Accept a C# Dictionary KindMap and marshall as a string. This is   
    //  the easiest way to transport an aggregate across the interlanguage 
    //  barrier.                                                           
    //

    public static string KindMapOut(Dictionary<string, int> kindMap)
    {

        StringWriter writer = new StringWriter();

        if (kindMap == null)
        {
            EncodeLong(writer, 0);
        }
        else
        {

            EncodeLong(writer, kindMap.Count);

            foreach (string key in kindMap.Keys)
            {
                EncodeString(writer, key);
                EncodeLong(writer, kindMap[key]);
            }

        }

        return writer.ToString();

    }

    //
    //  EncodeLong             
    //  ----------             
    //                         
    //  Marshall a long value. 
    //

    public static void EncodeLong(StringWriter writer, long value)
    {
        writer.Write(value + "|");
    }

    //
    //  DecodeLong               
    //  ----------               
    //                           
    //  Unmarshall a long value. 
    //

    public static long DecodeLong(StringReader reader)
    {

        StringBuilder buffer = new StringBuilder();
        long result = 0;

        try
        {

            for (;;)
            {
        
                char c = (char)reader.Read();

                if (c == '`')
                {
                    c = (char)reader.Read();
                }
                else if (c == '|')
                {
                    break;
                }
                
                buffer.Append(c);

            }
            
            long negative = 1;

            if (buffer[0] == '-')
            {
                negative = -1;
                buffer.Remove(0, 1);
            }

            if (long.TryParse(buffer.ToString(), out result))
            {
                result *= negative;
            }
            else
            {
                Console.Error.WriteLine("Internal Error: Number Format error");
                Environment.Exit(1);
            }

        }
        catch (IOException)
        {
            Console.Error.WriteLine("Internal Error: String decoding error");
            Environment.Exit(1);
        }

        return result;

    }

    //
    //  EncodeString       
    //  ------------       
    //                     
    //  Marshall a string. 
    //

    public static void EncodeString(StringWriter writer, string value)
    {

        for (int i = 0, n = value.Length; i < n; i++)
        {
            
            char c = value[i];

            if (c == '`' || c == '|')
            {
                writer.Write('`');
            }

            writer.Write(c);

        }

        writer.Write("|");

    }

    //
    //  DecodeString       
    //  ------------       
    //
    //  Unmarshall a string. 
    //

    public static string DecodeString(StringReader reader)
    {

        StringBuilder buffer = new StringBuilder();

        try
        {

            for (;;)
            {
        
                char c = (char)reader.Read();

                if (c == '`')
                {
                    c = (char)reader.Read();
                }
                else if (c == '|')
                {
                    break;
                }
                
                buffer.Append(c);

            }
            
        }
        catch (IOException)
        {
            Console.Error.WriteLine("Internal Error: String decoding error");
            Environment.Exit(1);
        }

        return buffer.ToString();

    }

    //
    //  DecodeErrorType                                
    //  ---------------                                
    //                                                 
    //  Unmarshall an error type.
    //

    public static ErrorType DecodeErrorType(StringReader reader)
    {
        return (ErrorType)DecodeLong(reader);    
    }    

    //
    //  DecodeKindMap          
    //  -------------          
    //                         
    //  Unmarshall a kind map. 
    //

    public static Dictionary<string, int> DecodeKindMap(StringReader reader)
    {

        Dictionary<string, int> result = new Dictionary<string, int>();

        int size = (int)DecodeLong(reader);
        while (size-- > 0)
        {
            string key = DecodeString(reader);
            int value = (int)DecodeLong(reader);
            result[key] = value;
        }

        return result;

    }

    //
    //  DecodeAst               
    //  ---------               
    //                          
    //  Unmarshall an Ast node. 
    //

    public static Ast DecodeAst(StringReader reader)
    {

        int numChildren = (int)DecodeLong(reader);

        if (numChildren < 0)
        {
            return null;
        }

        Ast result = new Ast(numChildren);

        result.Kind = (int)DecodeLong(reader); 
        result.Location = DecodeLong(reader); 
        result.Lexeme = DecodeString(reader); 
        
        for (int i = 0; i < numChildren; i++)
        {
            result.SetChild(i, DecodeAst(reader));
        }

        return result;

    }

    //
    //  DecodeErrorMessage
    //  ------------------                    
    //                               
    //  Unmarshall an error message. 
    //

    public static ErrorMessage DecodeErrorMessage(StringReader reader)
    {

        ErrorMessage message = new ErrorMessage();

        message.ErrorType = DecodeErrorType(reader);
        message.Tag = DecodeString(reader);
        message.Severity = (int)DecodeLong(reader);
        message.Location = DecodeLong(reader);
        message.LineNum = DecodeLong(reader);
        message.ColumnNum = DecodeLong(reader);
        message.SourceLine = DecodeString(reader);
        message.ShortMessage = DecodeString(reader);
        message.LongMessage = DecodeString(reader);
        message.String = DecodeString(reader);

        return message;

    }

}

//
//  Parser                                                                 
//  ------                                                                 
//                                                                         
//  This is a modal parser object. It's created in an empty state. Then we 
//  generate a parser into it with a grammar. With that done we can use it 
//  to parse source.                                                       
//

public class Parser {

    private long thisHandle = 0;
    private Dictionary<string, int> kindMap;
    private Dictionary<int, string> kindIMap;

    //
    //  Parser()                                                       
    //  --------                                                       
    //                                                                 
    //  In the static constructor we call the Initializer to guarantee 
    //  that the dll is loaded.                                        
    //

    static Parser()
    {
        Initializer.Initialize();
    }

    //
    //  Parser()                                       
    //  --------                                       
    //                                                 
    //  Create a parser instance in the Hoshi library. 
    //

    public Parser()
    {
        thisHandle = new_parser();
    }

    public Parser(Parser rhs)
    {
        thisHandle = clone_parser(rhs.thisHandle);
    }

    //
    //  ~Parser()                                                       
    //  ---------                                                       
    //                                                                  
    //  The Hoshi library keeps some storage for a parser. We have to   
    //  delete it when the garbage collector collects the C# version. 
    //

    ~Parser()
    {
        delete_parser(thisHandle);
    }

    //
    //  IsGrammarLoaded
    //  ---------------
    //  
    //  Check whether the parser has a grammer loaded.
    //

    public bool IsGrammarLoaded()
    {
        
        object exception = null;
        
        bool result = is_grammar_loaded(thisHandle, ref exception);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
        return result;
        
    }

    //
    //  IsGrammarFailed
    //  ---------------
    //  
    //  Check whether the parser has a failed grammar.
    //

    public bool IsGrammarFailed()
    {
        
        object exception = null;
        
        bool result = is_grammar_failed(thisHandle, ref exception);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
        return result;
        
    }

    //
    //  IsSourceLoaded
    //  --------------
    //  
    //  Check whether the parser has a source loaded.
    //

    public bool IsSourceLoaded()
    {
        
        object exception = null;
        
        bool result = is_source_loaded(thisHandle, ref exception);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
        return result;
        
    }

    //
    //  IsSourceFailed
    //  --------------
    //  
    //  Check whether the parser has a grammer failed.
    //

    public bool IsSourceFailed()
    {
        
        object exception = null;
        
        bool result = is_source_failed(thisHandle, ref exception);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
        return result;
        
    }

    //
    //  Generate
    //  --------
    //  
    //  Generate a parser from a grammar file.
    //
    
    public void Generate(string source, 
                         Dictionary<string, int> kindMap = null, 
                         long debugFlags = 0)
    {
        
        object exception = null;
        
        generate(thisHandle, 
                 ref exception, 
                 source, 
                 TypeConvert.KindMapOut(kindMap), 
                 debugFlags);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
        CopyKindMap();

    }

    //
    //  Parse
    //  -----
    //  
    //  Parse a source string saving the Ast and error messages.
    //

    public void Parse(string source, long debugFlags = 0)
    {
        
        object exception = null;
        
        parse(thisHandle, ref exception, source, debugFlags);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
    }

    //
    //  GetAst
    //  ------
    //  
    //  Return the Ast.
    //
    
    public Ast GetAst()
    {

        try
        {

            object exception = null;
            object result = null;

            get_encoded_ast(thisHandle, ref exception, ref result);
        
            if (exception != null)
            {
                throw (Exception)exception;
            }

            string marshalled = (string)result;

            StringReader reader = new StringReader(marshalled);
            kindMap = TypeConvert.DecodeKindMap(reader);

            kindIMap = new Dictionary<int, string>();
            foreach (string key in kindMap.Keys)
            {
                kindIMap[kindMap[key]] = key;
            }

            Ast ast = TypeConvert.DecodeAst(reader);

            return ast;

        }
        catch (UnknownError e)
        {
            throw e;
        }
        catch (Exception)
        {
            Console.Error.WriteLine("Internal Error: Unexpected exception");
            Environment.Exit(1);
        }

        return null;

    }

    //
    //  DumpAst
    //  -------
    //  
    //  For debugging we'll provide a nice dump function.
    //
    
    public void DumpAst(Ast ast, int indent = 0)
    {

        if (indent > 0)
        {
            Console.Write("{0," + indent + "}", "");
        }

        if (ast == null)
        {
            Console.WriteLine("Nullptr");
            return;
        }

        Console.Write(GetKindString(ast.Kind) + "(" + ast.Kind + ")");

        if (ast.Lexeme.Length > 0)
        {
            Console.Write(" [" + ast.Lexeme + "]");
        }

        if (ast.Location >= 0)
        {
            Console.Write(" @ " + ast.Location);
        }

        Console.WriteLine();

        foreach (Ast child in ast.Children)
        {
            DumpAst(child, indent + 4);
        }

    }

    //
    //  CopyKindMap
    //  -----------
    //  
    //  Get the kind map from the native library.
    //
    
    public void CopyKindMap()
    {

        try
        {

            object exception = null;
            object result = null;
            
            get_encoded_kind_map(thisHandle, ref exception, ref result);
        
            if (exception != null)
            {
                throw (Exception)exception;
            }

            string marshalled = (string)result;

            StringReader reader = new StringReader(marshalled);
            kindMap = TypeConvert.DecodeKindMap(reader);

            kindIMap = new Dictionary<int, string>();
            foreach (string key in kindMap.Keys)
            {
                kindIMap[kindMap[key]] = key;
            }

        }
        catch (UnknownError e)
        {
            throw e;
        }
        catch (Exception)
        {
            Console.Error.WriteLine("Internal Error: Unexpected exception");
            Environment.Exit(1);
        }

    }

    //
    //  GetKind
    //  -------
    //  
    //  Get the integer code for a given string.
    //
    
    public int GetKind(string kindString)
    {

        if (kindMap == null)
        {
            CopyKindMap();
        }

        if (!kindMap.ContainsKey(kindString))
        {
            return -1;
        }

        return kindMap[kindString];

    }

    //
    //  GetKindForce
    //  ------------
    //  
    //  Get the integer code for a given string. If it doesn't exist then install
    //  it.
    //
    
    public int GetKindForce(string kindString)
    {

        if (kindMap == null)
        {
            CopyKindMap();
        }

        if (!kindMap.ContainsKey(kindString))
        {

            object exception = null;
            
            get_kind_force(thisHandle, ref exception, kindString);
        
            if (exception != null)
            {
                throw (Exception)exception;
            }

            CopyKindMap();

        }

        return kindMap[kindString];

    }

    //
    //  GetKindString
    //  -------------
    //  
    //  Get the text name for a numeric kind code.
    //
    
    public string GetKindString(int kind)
    {

        if (kindIMap == null)
        {
            CopyKindMap();
        }

        if (!kindIMap.ContainsKey(kind))
        {
            return "Unknown";
        }

        return kindIMap[kind];

    }

    //
    //  GetKindString
    //  -------------
    //  
    //  Get the text name for an Ast.
    //
    
    public string GetKindString(Ast ast)
    {
        return GetKindString(ast.Kind);
    }

    //
    //  AddError
    //  --------
    //  
    //  Add another error to the message list. This is provided so that clients
    //  can use the parser message handler for all errors, not just parsing
    //  errors.
    //

    public void AddError(int errorType, 
                         long location, 
                         string shortMessage, 
                         string longMessage = "")
    {
        
        object exception = null;
        
        add_error(thisHandle, 
                  ref exception, 
                  errorType, 
                  location, 
                  shortMessage, 
                  longMessage);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
    }

    //
    //  GetErrorCount
    //  -------------
    //  
    //  Return the number of error messages over the error threshhold.
    //

    public int GetErrorCount()
    {
        
        object exception = null;
        
        int result = get_error_count(thisHandle, ref exception);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
        return result;
        
    }

    //
    //  GetWarningCount
    //  ---------------
    //  
    //  Return the number of error messages under the error threshhold.
    //

    public int GetWarningCount()
    {
        
        object exception = null;
        
        int result = get_warning_count(thisHandle, ref exception);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
        return result;
        
    }

    //
    //  GetErrorMessages
    //  ----------------
    //  
    //  Return the error messages.
    //
    
    public List<ErrorMessage> GetErrorMessages()
    {

        List<ErrorMessage> messages = new List<ErrorMessage>();

        try
        {

            object exception = null;
            object result = null;
            
            get_encoded_error_messages(thisHandle, ref exception, ref result);
        
            if (exception != null)
            {
                throw (Exception)exception;
            }

            string marshalled = (string)result;

            StringReader reader = new StringReader(marshalled);

            int size = (int)TypeConvert.DecodeLong(reader);

            while (size-- > 0)
            {
                messages.Add(TypeConvert.DecodeErrorMessage(reader));
            }

        }
        catch (UnknownError e)
        {
            throw e;
        }
        catch (Exception)
        {
            Console.Error.WriteLine("Internal Error: Unexpected exception");
            Environment.Exit(1);
        }

        return messages;

    }

    //
    //  GetSourceList
    //  -------------
    //  
    //  Return a source list with embedded messages.
    //

    public string GetSourceList(string source, int indent = 0)
    {
        
        object exception = null;
        object result = null;
        
        get_source_list(thisHandle, 
                        ref exception, 
                        ref result, 
                        source, 
                        indent);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
        return (string)result;
        
    }

    //
    //  Encode
    //  ------
    //  
    //  Create a string encoding of a Parser.
    //

    public string Encode()
    {
        
        object exception = null;
        object result = null;
        
        encode(thisHandle, ref exception, ref result);
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
        return (string)result;
        
    }

    //
    //  Decode
    //  ------
    //  
    //  Decode a previously created string into a parser
    //
    
    public void Decode(string str, Dictionary<string, int> kindMap = null)
    {
        
        object exception = null;
        
        decode(thisHandle, ref exception, str, TypeConvert.KindMapOut(kindMap));
        
        if (exception != null)
        {
            throw (Exception)exception;
        }
        
        CopyKindMap();

    }

    //
    //  Native Function Declarations                           
    //  ----------------------------                           
    //                                                         
    //  Declarations of functions in the Hoshi native library. 
    //

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_new_parser"),
     SuppressUnmanagedCodeSecurity]
    private static extern long new_parser();

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_clone_parser"),
     SuppressUnmanagedCodeSecurity]
    private static extern long clone_parser(long parserHandle);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_delete_parser"),
     SuppressUnmanagedCodeSecurity]
    private static extern void delete_parser(long parserHandle);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_is_grammar_loaded"),
     SuppressUnmanagedCodeSecurity]
    private static extern bool is_grammar_loaded(long thisHandle, 
                                                 ref object exception);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_is_grammar_failed"),
     SuppressUnmanagedCodeSecurity]
    private static extern bool is_grammar_failed(long thisHandle, 
                                                 ref object exception);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_is_source_loaded"),
     SuppressUnmanagedCodeSecurity]
    private static extern bool is_source_loaded(long thisHandle, 
                                                ref object exception);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_is_source_failed"),
     SuppressUnmanagedCodeSecurity]
    private static extern bool is_source_failed(long thisHandle, 
                                                ref object exception);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_generate"),
     SuppressUnmanagedCodeSecurity]
    private static extern void generate(long thisHandle, 
                                        ref object exception, 
                                        [MarshalAs(UnmanagedType.LPWStr)] string source, 
                                        [MarshalAs(UnmanagedType.LPWStr)] string kindMap, 
                                        long debugFlags);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_parse"),
     SuppressUnmanagedCodeSecurity]
    private static extern void parse(long thisHandle, 
                                     ref object exception, 
                                     [MarshalAs(UnmanagedType.LPWStr)] string source, 
                                     long debugFlags);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_get_encoded_ast"),
     SuppressUnmanagedCodeSecurity]
    private static extern void get_encoded_ast(long thisHandle, 
                                               ref object exception, 
                                               ref object result);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_get_encoded_kind_map"),
     SuppressUnmanagedCodeSecurity]
    private static extern void get_encoded_kind_map(long thisHandle, 
                                                    ref object exception, 
                                                    ref object result);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_get_kind"),
     SuppressUnmanagedCodeSecurity]
    private static extern int get_kind(long thisHandle, 
                                       ref object exception, 
                                       [MarshalAs(UnmanagedType.LPWStr)] string kindString);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_get_kind_force"),
     SuppressUnmanagedCodeSecurity]
    private static extern int get_kind_force(long thisHandle, 
                                             ref object exception, 
                                             [MarshalAs(UnmanagedType.LPWStr)] string kindString);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_get_kind_string"),
     SuppressUnmanagedCodeSecurity]
    private static extern void get_kind_string(long thisHandle, 
                                               ref object exception, 
                                               ref object result, 
                                               int kind);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_add_error"),
     SuppressUnmanagedCodeSecurity]
    private static extern void add_error(long thisHandle, 
                                         ref object exception, 
                                         int errorType, 
                                         long location, 
                                         [MarshalAs(UnmanagedType.LPWStr)] string shortMessage, 
                                         [MarshalAs(UnmanagedType.LPWStr)] string longMessage);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_get_error_count"),
     SuppressUnmanagedCodeSecurity]
    private static extern int get_error_count(long thisHandle, 
                                              ref object exception);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_get_warning_count"),
     SuppressUnmanagedCodeSecurity]
    private static extern int get_warning_count(long thisHandle, 
                                                ref object exception);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_get_encoded_error_messages"),
     SuppressUnmanagedCodeSecurity]
    private static extern void get_encoded_error_messages(
        long thisHandle, 
        ref object exception, 
        ref object result);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_get_source_list"),
     SuppressUnmanagedCodeSecurity]
    private static extern void get_source_list(long thisHandle, 
                                               ref object exception, 
                                               ref object result, 
                                               [MarshalAs(UnmanagedType.LPWStr)] string source, 
                                               int indent);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_encode"),
     SuppressUnmanagedCodeSecurity]
    private static extern void encode(long thisHandle, 
                                      ref object exception, 
                                      ref object result);

    [DllImport("hoshi.dll", ExactSpelling=true,
               EntryPoint="csc_Parser_decode"),
     SuppressUnmanagedCodeSecurity]
    private static extern void decode(long thisHandle, 
                                      ref object exception, 
                                      [MarshalAs(UnmanagedType.LPWStr)] string str, 
                                      [MarshalAs(UnmanagedType.LPWStr)] string kindMap);

}

//
//  Ast (Abstract Syntax Tree)                                              
//  --------------------------                                              
//                                                                          
//  An abstract syntax tree holds the important syntactic elements from the 
//  source in an easily traversable form.                                   
//                                                                          
//  The Ast typically has a lot of nodes that we are likely to traverse     
//  multiple times, so for efficency we marshall them on the C++ side and   
//  bring them over as an entire tree. So this class is pretty complete     
//  here, without reaching back into C++.                                   
//

public class Ast
{

    static Ast()
    {
        Initializer.Initialize();
    }

    private int kind;
    private long location;
    private string lexeme;
    private Ast [] children;

    public Ast(int numChildren)
    {
        kind = 0;
        location = -1;
        lexeme = "";
        children = new Ast[numChildren];
    }

    public int Kind
    {
        get { return kind; }
        set { kind = value; }
    }

    public long Location
    {
        get { return location; }
        set { location = value; }
    }

    public string Lexeme
    {
        get { return lexeme; }
        set { lexeme = value; }
    }

    public Ast [] Children
    {
        get { return children; }
        set { children = value; }
    }

    public Ast GetChild(int childNum)
    {
        return children[childNum];
    }

    public void SetChild(int childNum, Ast child)
    {
        children[childNum] = child;
    }

}

//
//  ErrorMessage                                                           
//  ------------                                                           
//                                                                         
//  Both the parser generator and the parser can detect and return errors  
//  in this form. We only create errors on the C++ side but we provide the 
//  ability to pull them all to the python side for processing.            
//

public class ErrorMessage
{

    static ErrorMessage()
    {
        Initializer.Initialize();
    }

    private ErrorType type;
    private string tag;
    private int severity;
    private long location;
    private long lineNum;
    private long columnNum;
    private string sourceLine;
    private string shortMessage;
    private string longMessage;
    private string stringDesc;

    public ErrorType ErrorType
    {
        get { return type; }
        set { type = value; }
    }

    public string Tag
    {
        get { return tag; }
        set { tag = value; }
    }

    public int Severity
    {
        get { return severity; }
        set { severity = value; }
    }

    public long Location
    {
        get { return location; }
        set { location = value; }
    }

    public long LineNum
    {
        get { return lineNum; }
        set { lineNum = value; }
    }

    public long ColumnNum
    {
        get { return columnNum; }
        set { columnNum = value; }
    }

    public string SourceLine
    {
        get { return sourceLine; }
        set { sourceLine = value; }
    }

    public string ShortMessage
    {
        get { return shortMessage; }
        set { shortMessage = value; }
    }

    public string LongMessage
    {
        get { return longMessage; }
        set { longMessage = value; }
    }

    public string String
    {
        get { return stringDesc; }
        set { stringDesc = value; }
    }

}

} // namespace hoshi

