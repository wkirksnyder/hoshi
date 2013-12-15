#
#  Hoshi                                                                   
#  -----                                                                   
#                                                                          
#  Hoshi is a parser generator written in C++. This file is a wrapper that 
#  uses ctypes to access Hoshi in a dynamic library.                       
#

from ctypes import *
import os
import os.path

#
#  Setup the search path for the .dll. I'm not sure what the acceptable    
#  standards are for this, but I'm going to use the location of the Python 
#  source followed by the normal path.                                     
#

os.environ['PATH'] = os.path.dirname(__file__) + ';' + os.environ['PATH']
HOSHI = cdll.hoshi

#
#  Set the type attributes for the exported functions we need in the 
#  dynamic library.                                                  
#

HOSHI.py_get_exception_type.restype = c_int64
HOSHI.py_get_exception_type.argtypes = [c_void_p]
HOSHI.py_get_exception_length.restype = c_int64
HOSHI.py_get_exception_length.argtypes = [c_void_p]
HOSHI.py_get_exception_string.argtypes = [c_void_p, c_char_p]
HOSHI.py_get_string_length.restype = c_int64
HOSHI.py_get_string_length.argtypes = [c_void_p]
HOSHI.py_get_string_string.argtypes = [c_void_p, c_char_p]
HOSHI.py_parser_new_parser.restype = c_int64
HOSHI.py_parser_clone_parser.restype = c_int64
HOSHI.py_parser_clone_parser.argtypes = [c_int64]
HOSHI.py_parser_delete_parser.argtypes = [c_int64]
HOSHI.py_parser_is_grammar_loaded.restype = c_bool
HOSHI.py_parser_is_grammar_loaded.argtypes = [c_int64, c_void_p]
HOSHI.py_parser_is_grammar_failed.restype = c_bool
HOSHI.py_parser_is_grammar_failed.argtypes = [c_int64, c_void_p]
HOSHI.py_parser_is_source_loaded.restype = c_bool
HOSHI.py_parser_is_source_loaded.argtypes = [c_int64, c_void_p]
HOSHI.py_parser_is_source_failed.restype = c_bool
HOSHI.py_parser_is_source_failed.argtypes = [c_int64, c_void_p]
HOSHI.py_parser_generate_1.argtypes = [c_int64, c_void_p, c_char_p]
HOSHI.py_parser_parse.argtypes = [c_int64, c_void_p, c_char_p]
HOSHI.py_parser_get_encoded_ast.argtypes = [c_int64, c_void_p, c_void_p]
HOSHI.py_parser_get_error_count.restype = c_int
HOSHI.py_parser_get_error_count.argtypes = [c_int64, c_void_p]
HOSHI.py_parser_get_warning_count.restype = c_int
HOSHI.py_parser_get_warning_count.argtypes = [c_int64, c_void_p]
HOSHI.py_parser_add_error.argtypes = [c_int64, 
                                      c_void_p, 
                                      c_int, 
                                      c_int64, 
                                      c_char_p]
HOSHI.py_parser_get_encoded_error_messages.argtypes = [c_int64, 
                                                       c_void_p, 
                                                       c_void_p]
HOSHI.py_parser_get_source_list.argtypes = [c_int64, 
                                            c_void_p, 
                                            c_void_p, 
                                            c_char_p]
HOSHI.py_parser_encode.argtypes = [c_int64, c_void_p, c_void_p]
HOSHI.py_parser_decode.argtypes = [c_int64, c_void_p, c_char_p]

#
#  ExceptionType
#  -------------                                                       
#                                                                   
#  We'll have to pass exceptions back to the language-specific clients. 
#  This flag tells the receiver the kind of exception.                  
#

class ExceptionType:
    ExceptionNull    = 0
    ExceptionGrammar = 1
    ExceptionSource  = 2
    ExceptionUnknown = 3

#
#  ErrorType                                                               
#  ----------                                                              
#                                                                          
#  We'll encode each error message with an enumerated type. For now we're  
#  not going to do anything with this but at some point we may want to add 
#  options like classifying them as warnings or errors, disabling and so   
#  forth.                                                                  
#

class ErrorType:
    ErrorProgress          = 1 <<  0
    ErrorAstHandlers       = 1 <<  1
    ErrorGrammar           = 1 <<  2
    ErrorGrammarAst        = 1 <<  3
    ErrorLalr              = 1 <<  4
    ErrorScanner           = 1 <<  5
    ErrorActions           = 1 <<  6
    ErrorICode             = 1 <<  7
    ErrorVCodeExec         = 1 <<  8
    ErrorScanToken         = 1 <<  9
    ErrorParseAction       = 1 << 10

#
#  DebugType                                                       
#  ----------                                                       
#                                                                   
#  We allow quite a few debugging options. It's the only way to get 
#  through such a large library.                                    
#

class DebugType:
    DebugProgress    = 1 <<  0
    DebugAstHandlers = 1 <<  1
    DebugGrammar     = 1 <<  2
    DebugGrammarAst  = 1 <<  3
    DebugLalr        = 1 <<  4
    DebugScanner     = 1 <<  5
    DebugActions     = 1 <<  6
    DebugICode       = 1 <<  7
    DebugVCodeExec   = 1 <<  8
    DebugScanToken   = 1 <<  9
    DebugParseAction = 1 << 10

#
#  Exception classes                                              
#  -----------------                                              
#                                                                 
#  These are the python versions of the exceptions thrown in C++. 
#

class GrammarError(Exception):
    pass

class SourceError(Exception):
    pass

class UnknownError(Exception):
    pass

#
#  int_out                                         
#  -------                                         
#                                                    
#  Convert an arbitrary python value onto a c_int. 
#

def int_out(value):
    try:
        if isinstance(value, int):
            return c_int(value)
        else:
            return c_int(int(value))
    except:
        return c_int(0)

#
#  int64_out                                         
#  ---------                                         
#                                                    
#  Convert an arbitrary python value onto a c_int64. 
#

def int64_out(value):
    try:
        if isinstance(value, int):
            return c_int64(value)
        else:
            return c_int64(int(value))
    except:
        return c_int64(0)

#
#  string_out                                           
#  ----------                                           
#                                                       
#  Convert an arbitrary python value onto a c_char_p. 
#

def string_out(value):
    try:
        if isinstance(value, bytes):
            return c_char_p(value)
        elif isinstance(value, str):
            return c_char_p(value.encode("utf-8"))
        else:
            return c_char_p(str(value).encode("utf-8"))
    except:
        return c_char_p(b"")

#
#  parser_handle_out                                 
#  -----------------                                 
#                                                    
#  Convert an arbitrary python value onto a c_int64. 
#

def parser_handle_out(value):
    try:
        if isinstance(value, int):
            return c_int64(value)
        else:
            return c_int64(int(value))
    except:
        return c_int64(0)

#
#  string_result_in                                             
#  ----------------
#                                                                
#  Convert the returned value for a string into a python string. 
#

def string_result_in(result_ptr):
    try:
        length = HOSHI.py_get_string_length(byref(result_ptr))
        if length < 0:
            return ""
        buf = create_string_buffer(b'', length)
        HOSHI.py_get_string_string(byref(result_ptr), buf)
        return str(buf.value.decode("utf-8"))
    except:
        return ""
    
#
#  check_exceptions                                             
#  ----------------
#                                                                
#  Check for any exceptions and raise any found.
#

def check_exceptions(exception_ptr):

    try:
        exception_type = HOSHI.py_get_exception_type(byref(exception_ptr))
        if exception_type < 0 or exception_type == ExceptionType.ExceptionNull:
            return
        length = HOSHI.py_get_exception_length(byref(exception_ptr))
        if length < 0:
            return
        buf = create_string_buffer(b'', length)
        HOSHI.py_get_exception_string(byref(exception_ptr), buf)
        what = str(buf.value.decode("utf-8"))
    except:
        raise UnknownError("Unknown exception")

    if exception_type == ExceptionType.ExceptionGrammar:
        raise GrammarError(what)
    elif exception_type == ExceptionType.ExceptionSource:
        raise SourceError(what)
    elif exception_type == ExceptionType.ExceptionUnknown:
        raise UnknownError(what)
    
#
#  Parser                         
#  ------                         
#                                 
#  This is the main Parser class. 
#

class Parser:

    #
    #  Parser()                                                       
    #  --------                                                       
    #                                                                 
    #  Create a parser in the Hoshi library either by cloning or from 
    #  scratch.                                                       
    #

    def __init__(self, *args):

        if len(args) == 0:
            self.this_handle = HOSHI.py_parser_new_parser()
            return

        if len(args) > 1 or not isinstance(args[1], Parser): 
            raise TypeError("A single Hoshi.Parser is required to clone")

        self.this_handle = HOSHI.py_parser_clone_parser(c_int64(args[0]))

    #
    #  ~Parser()                                                       
    #  ---------                                                       
    #                                                                 
    #  Delete the parser.
    #

    def __del__(self):
        HOSHI.py_parser_delete_parser(c_int64(self.this_handle))

    #
    #  is_grammar_loaded
    #  -----------------
    #  
    #  Check whether the parser has a grammer loaded.
    #

    def is_grammar_loaded(self):
        
        exception_ptr = c_void_p(None)
        
        result = HOSHI.py_parser_is_grammar_loaded(parser_handle_out(self.this_handle), 
                                                   byref(exception_ptr))
        
        check_exceptions(exception_ptr)
        
        return result
        
    #
    #  is_grammar_failed
    #  -----------------
    #  
    #  Check whether the parser has a failed grammar.
    #

    def is_grammar_failed(self):
        
        exception_ptr = c_void_p(None)
        
        result = HOSHI.py_parser_is_grammar_failed(parser_handle_out(self.this_handle), 
                                                   byref(exception_ptr))
        
        check_exceptions(exception_ptr)
        
        return result
        
    #
    #  is_source_loaded
    #  ----------------
    #  
    #  Check whether the parser has a source loaded.
    #

    def is_source_loaded(self):
        
        exception_ptr = c_void_p(None)
        
        result = HOSHI.py_parser_is_source_loaded(parser_handle_out(self.this_handle), 
                                                  byref(exception_ptr))
        
        check_exceptions(exception_ptr)
        
        return result
        
    #
    #  is_source_failed
    #  ----------------
    #  
    #  Check whether the parser has a grammer failed.
    #

    def is_source_failed(self):
        
        exception_ptr = c_void_p(None)
        
        result = HOSHI.py_parser_is_source_failed(parser_handle_out(self.this_handle), 
                                                  byref(exception_ptr))
        
        check_exceptions(exception_ptr)
        
        return result
        
    #
    #  generate
    #  --------
    #  
    #  Generate a parser from a grammar file. This version does not include a
    #  kind map for languages that do not have a switch on integer feature.
    #

    def generate(self, source, *args):
        
        if len(args) > 0:
            debug_flags = args[0]
        else:
            debug_flags = 0
        
        exception_ptr = c_void_p(None)
        
        HOSHI.py_parser_generate_1(parser_handle_out(self.this_handle), 
                                   byref(exception_ptr), 
                                   string_out(source), 
                                   int64_out(debug_flags))
        
        check_exceptions(exception_ptr)
        
    #
    #  parse
    #  -----
    #  
    #  Parse a source string saving the Ast and error messages.
    #

    def parse(self, source, *args):
        
        if len(args) > 0:
            debug_flags = args[0]
        else:
            debug_flags = 0
        
        exception_ptr = c_void_p(None)
        
        HOSHI.py_parser_parse(parser_handle_out(self.this_handle), 
                              byref(exception_ptr), 
                              string_out(source), 
                              int64_out(debug_flags))
        
        check_exceptions(exception_ptr)
        
    #
    #  get_encoded_ast
    #  ---------------
    #  
    #  Return the Ast encoded as a string. We use this method to pass entire
    #  trees back to the caller to facilitate interlanguage calls.
    #

    def get_encoded_ast(self):
        
        exception_ptr = c_void_p(None)
        result_ptr = c_void_p(None)
        
        HOSHI.py_parser_get_encoded_ast(parser_handle_out(self.this_handle), 
                                        byref(exception_ptr), 
                                        byref(result_ptr))
        
        check_exceptions(exception_ptr)
        
        return string_result_in(result_ptr)
        
    #
    #  get_error_count
    #  ---------------
    #  
    #  Return the number of error messages over the error threshhold.
    #

    def get_error_count(self):
        
        exception_ptr = c_void_p(None)
        
        result = HOSHI.py_parser_get_error_count(parser_handle_out(self.this_handle), 
                                                 byref(exception_ptr))
        
        check_exceptions(exception_ptr)
        
        return result
        
    #
    #  get_warning_count
    #  -----------------
    #  
    #  Return the number of error messages under the error threshhold.
    #

    def get_warning_count(self):
        
        exception_ptr = c_void_p(None)
        
        result = HOSHI.py_parser_get_warning_count(parser_handle_out(self.this_handle), 
                                                   byref(exception_ptr))
        
        check_exceptions(exception_ptr)
        
        return result
        
    #
    #  add_error
    #  ---------
    #  
    #  Add another error to the message list. This is provided so that clients
    #  can use the parser message handler for all errors, not just parsing
    #  errors.
    #

    def add_error(self, error_type, location, short_message, *args):
        
        if len(args) > 0:
            long_message = args[0]
        else:
            long_message = ""
        
        exception_ptr = c_void_p(None)
        
        HOSHI.py_parser_add_error(parser_handle_out(self.this_handle), 
                                  byref(exception_ptr), 
                                  error_type_out(error_type), 
                                  int64_out(location), 
                                  string_out(short_message), 
                                  string_out(long_message))
        
        check_exceptions(exception_ptr)
        
    #
    #  get_encoded_error_messages
    #  --------------------------
    #  
    #  Return the error messages encoded as a string. We use this method to pass
    #  entire trees back to the caller to facilitate interlanguage calls.
    #

    def get_encoded_error_messages(self):
        
        exception_ptr = c_void_p(None)
        result_ptr = c_void_p(None)
        
        HOSHI.py_parser_get_encoded_error_messages(parser_handle_out(self.this_handle), 
                                                   byref(exception_ptr), 
                                                   byref(result_ptr))
        
        check_exceptions(exception_ptr)
        
        return string_result_in(result_ptr)
        
    #
    #  get_source_list
    #  ---------------
    #  
    #  Return a source list with embedded messages.
    #

    def get_source_list(self, source, *args):
        
        if len(args) > 0:
            indent = args[0]
        else:
            indent = 0
        
        exception_ptr = c_void_p(None)
        result_ptr = c_void_p(None)
        
        HOSHI.py_parser_get_source_list(parser_handle_out(self.this_handle), 
                                        byref(exception_ptr), 
                                        byref(result_ptr), 
                                        string_out(source), 
                                        int_out(indent))
        
        check_exceptions(exception_ptr)
        
        return string_result_in(result_ptr)
        
    #
    #  encode
    #  ------
    #  
    #  Create a string encoding of a Parser.
    #

    def encode(self):
        
        exception_ptr = c_void_p(None)
        result_ptr = c_void_p(None)
        
        HOSHI.py_parser_encode(parser_handle_out(self.this_handle), 
                               byref(exception_ptr), 
                               byref(result_ptr))
        
        check_exceptions(exception_ptr)
        
        return string_result_in(result_ptr)
        
    #
    #  decode
    #  ------
    #  
    #  Decode a previously created string into a parser
    #

    def decode(self, str):
        
        exception_ptr = c_void_p(None)
        
        HOSHI.py_parser_decode(parser_handle_out(self.this_handle), 
                               byref(exception_ptr), 
                               string_out(str))
        
        check_exceptions(exception_ptr)
        
    #
    #  get_ast                                                            
    #  -------                                                            
    #                                                                     
    #  Fetch a marshalled form of the Ast from the native code module and 
    #  un-marshall it into a tree.                                        
    #

    def get_ast(self):

        try:
            marshalled = self.get_encoded_ast()
        except Exception as e:
            print(str(e))
            return None

        (kind_map, tail_ptr) = decode_kind_map(marshalled, 0)
        (ast, tail_ptr) = decode_ast(kind_map, marshalled, tail_ptr)

        return ast

    #
    #  get_error_messages                                                 
    #  ------------------                                                 
    #                                                                     
    #  Fetch a marshalled form of the error messages from the native code 
    #  module and un-marshall it into a list.                             
    #

    def get_error_messages(self):

        try:
            marshalled = self.get_encoded_error_messages()
        except Exception as e:
            print(str(e))
            return []

        (length, tail_ptr) = decode_int(marshalled, 0)
        messages = []

        while length > 0:

            message = ErrorMessage()

            (message_type, tail_ptr) = decode_int(marshalled, tail_ptr)
            message.set_message_type(message_type)

            (tag, tail_ptr) = decode_string(marshalled, tail_ptr)
            message.set_tag(tag)

            (severity, tail_ptr) = decode_int(marshalled, tail_ptr)
            message.set_severity(severity)

            (location, tail_ptr) = decode_int(marshalled, tail_ptr)
            message.set_location(location)

            (line_num, tail_ptr) = decode_int(marshalled, tail_ptr)
            message.set_line_num(line_num)

            (column_num, tail_ptr) = decode_int(marshalled, tail_ptr)
            message.set_column_num(column_num)

            (source_line, tail_ptr) = decode_string(marshalled, tail_ptr)
            message.set_source_line(source_line)

            (short_message, tail_ptr) = decode_string(marshalled, tail_ptr)
            message.set_short_message(short_message)

            (long_message, tail_ptr) = decode_string(marshalled, tail_ptr)
            message.set_long_message(long_message)

            (string, tail_ptr) = decode_string(marshalled, tail_ptr)
            message.set_string(string)

            messages.append(message)
            length -= 1

        return messages

#
#  Decoders                                                              
#  --------                                                              
#                                                                        
#  Data handled in this module is computed in a native code library.     
#  Sometimes it's better to leave it there and access it on demand, and  
#  sometimes it's better to marshall it in the library and bring it here 
#  in one call. These functions perform primitive un-marshalling         
#  operations that make it easier to un-marshall larger structures.      
#

def decode_int(marshalled, tail_ptr):

    next_ptr = tail_ptr
    while marshalled[next_ptr] != "|":
        next_ptr += 1
    return (int(marshalled[tail_ptr:next_ptr]), next_ptr + 1)

def decode_string(marshalled, tail_ptr):

    (length, tail_ptr) = decode_int(marshalled, tail_ptr)
    return (marshalled[tail_ptr:tail_ptr + length], tail_ptr + length + 1)

def decode_kind_map(marshalled, tail_ptr):

    (size, tail_ptr) = decode_int(marshalled, tail_ptr)

    kind_map = dict()
    for i in range(0, size):
        (str, tail_ptr) = decode_string(marshalled, tail_ptr)
        (num, tail_ptr) = decode_int(marshalled, tail_ptr)
        kind_map[num] = str

    return (kind_map, tail_ptr);

def decode_ast(kind_map, marshalled, tail_ptr):

    if tail_ptr >= len(marshalled):
        return None

    root = Ast()

    (kind, tail_ptr) = decode_int(marshalled, tail_ptr)
    if kind in kind_map:
        root.set_kind(kind_map[kind])
    else:
        root.set_kind("Unknown")

    (lexeme, tail_ptr) = decode_string(marshalled, tail_ptr)
    root.set_lexeme(lexeme)

    (location, tail_ptr) = decode_int(marshalled, tail_ptr)
    root.set_location(location)

    (num_children, tail_ptr) = decode_int(marshalled, tail_ptr)
    for i in range(0, num_children):
        (ast, tail_ptr) = decode_ast(kind_map, marshalled, tail_ptr)    
        root.set_child(i, ast)

    return (root, tail_ptr);

#
#  Ast (Abstract Syntax Tree)                                              
#  --------------------------                                              
#                                                                          
#  An abstract syntax tree holds the important syntactic elements from the 
#  source in an easily traversable form.                                   
#                                                                          
#  The Ast typically has a lot of nodes that we are likely to traverse     
#  multiple times, so for efficency we marshall them on the C++ side and   
#  bring them over as an entire tree. So this class is pretty complete     
#  here, without reaching back into C++.                                   
#

class Ast:

    def __init__(self):
        self.kind = "Error"
        self.lexeme = ""
        self.location = 0
        self.children = []

    def get_kind(self):
        return self.kind

    def set_kind(self, kind):
        self.kind = kind

    def get_lexeme(self):
        return self.lexeme

    def set_lexeme(self, lexeme):
        self.lexeme = lexeme

    def get_location(self):
        return self.location

    def set_location(self, location):
        self.location = location

    def get_children(self):
        return self.children

    def set_children(self, children):
        self.children = children

    def get_child(self, num):
        return self.children[num]

    def set_child(self, num, child):
        while len(self.children) <= num:
            self.children.append(None)
        self.children[num] = child

    def dump(self, *args):

        if len(args) > 0:
            indent = int(args[0])
        else:
            indent = 0

        line = " " * indent + self.kind

        if self.lexeme != "":
            line += " [" + self.lexeme + "]"

        if self.location >= 0:
            line += " @ " + str(self.location)

        print(line)

        for child in self.children:
            child.dump(indent + 4)

#
#  ErrorMessage                                                           
#  ------------                                                           
#                                                                         
#  Both the parser generator and the parser can detect and return errors  
#  in this form. We only create errors on the C++ side but we provide the 
#  ability to pull them all to the python side for processing.            
#

class ErrorMessage:

    def __init__(self):
        self.message_type = 0
        self.tag = ""
        self.severity = 0
        self.location = 0
        self.line_num = 0
        self.column_num = 0
        self.source_line = ""
        self.short_message = ""
        self.long_message = ""
        self.string = ""

    def get_message_type(self):
        return self.message_type

    def set_message_type(self, message_type):
        self.message_type = message_type

    def get_tag(self):
        return self.tag

    def set_tag(self, tag):
        self.tag = tag

    def get_severity(self):
        return self.severity

    def set_severity(self, severity):
        self.severity = severity

    def get_location(self):
        return self.location

    def set_location(self, location):
        self.location = location

    def get_line_num(self):
        return self.line_num

    def set_line_num(self, line_num):
        self.line_num = line_num

    def get_column_num(self):
        return self.column_num

    def set_column_num(self, column_num):
        self.column_num = column_num

    def get_source_line(self):
        return self.source_line

    def set_source_line(self, source_line):
        self.source_line = source_line

    def get_short_message(self):
        return self.short_message

    def set_short_message(self, short_message):
        self.short_message = short_message

    def get_long_message(self):
        return self.long_message

    def set_long_message(self, long_message):
        self.long_message = long_message

    def get_string(self):
        return self.string

    def set_string(self, string):
        self.string = string

