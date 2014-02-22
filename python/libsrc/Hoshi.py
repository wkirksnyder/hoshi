#
#  Hoshi                                                                   
#  -----                                                                   
#                                                                          
#  Hoshi is a parser generator written in C++. This file is a wrapper that 
#  uses ctypes to access Hoshi in a native library.                       
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
#  native library.                                                  
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
HOSHI.py_parser_generate.argtypes = [c_int64, 
                                     c_void_p, 
                                     c_char_p, 
                                     c_char_p, 
                                     c_int64]
HOSHI.py_parser_parse.argtypes = [c_int64, c_void_p, c_char_p, c_int64]
HOSHI.py_parser_get_encoded_ast.argtypes = [c_int64, c_void_p, c_void_p]
HOSHI.py_parser_get_encoded_kind_map.argtypes = [c_int64, c_void_p, c_void_p]
HOSHI.py_parser_get_kind.restype = c_int
HOSHI.py_parser_get_kind.argtypes = [c_int64, c_void_p, c_char_p]
HOSHI.py_parser_get_kind_force.restype = c_int
HOSHI.py_parser_get_kind_force.argtypes = [c_int64, c_void_p, c_char_p]
HOSHI.py_parser_get_kind_string.argtypes = [c_int64, 
                                            c_void_p, 
                                            c_void_p, 
                                            c_int]
HOSHI.py_parser_add_error.argtypes = [c_int64, 
                                      c_void_p, 
                                      c_int, 
                                      c_int64, 
                                      c_char_p, 
                                      c_char_p]
HOSHI.py_parser_get_error_count.restype = c_int
HOSHI.py_parser_get_error_count.argtypes = [c_int64, c_void_p]
HOSHI.py_parser_get_warning_count.restype = c_int
HOSHI.py_parser_get_warning_count.argtypes = [c_int64, c_void_p]
HOSHI.py_parser_get_encoded_error_messages.argtypes = [c_int64, 
                                                       c_void_p, 
                                                       c_void_p]
HOSHI.py_parser_get_source_list.argtypes = [c_int64, 
                                            c_void_p, 
                                            c_void_p, 
                                            c_char_p, 
                                            c_int]
HOSHI.py_parser_encode.argtypes = [c_int64, c_void_p, c_void_p]
HOSHI.py_parser_decode.argtypes = [c_int64, c_void_p, c_char_p, c_char_p]

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
#  kind_map_out
#  ------------
#                                                                
#  Convert a kind_map to marshalled form.
#

def kind_map_out(kind_map):
    if (kind_map == None):
        return string_out(encode_kind_map({}))
    return string_out(encode_kind_map(kind_map))

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
#  Encoders & Decoders                                                   
#  -------------------                                                   
#                                                                        
#  Data handled in this module is computed in a native code library.     
#  Sometimes it's better to leave it there and access it on demand, and  
#  sometimes it's better to marshall it in the library and bring it here 
#  in one call. These functions perform primitive un-marshalling         
#  operations that make it easier to un-marshall larger structures.      
#

def encode_int(value):
    return str(value) + '|'

def decode_int(marshalled, tail_ptr):

    buffer = ""
    while True:
        if marshalled[tail_ptr] == '`':
            tail_ptr += 1
        elif marshalled[tail_ptr] == '|':
            tail_ptr += 1
            break
        buffer += marshalled[tail_ptr]
        tail_ptr += 1

    return (int(buffer), tail_ptr)

def encode_string(value):

    buffer = ""
    for c in value:
        if c == '`' or c == '|':
            buffer += '`'
        buffer += c
    buffer += '|'

    return buffer
    
def decode_string(marshalled, tail_ptr):

    buffer = ""
    while True:
        if marshalled[tail_ptr] == '`':
            tail_ptr += 1
        elif marshalled[tail_ptr] == '|':
            tail_ptr += 1
            break
        buffer += marshalled[tail_ptr]
        tail_ptr += 1

    return (buffer, tail_ptr)

def encode_kind_map(kind_map):

    buffer = encode_int(len(kind_map))
    for key in kind_map:
        buffer += encode_string(key)    
        buffer += encode_int(kind_map[key])
    return buffer

def decode_kind_map(marshalled, tail_ptr):

    (size, tail_ptr) = decode_int(marshalled, tail_ptr)

    kind_map = dict()
    for i in range(0, size):
        (key, tail_ptr) = decode_string(marshalled, tail_ptr)
        (num, tail_ptr) = decode_int(marshalled, tail_ptr)
        kind_map[key] = num

    return (kind_map, tail_ptr)

def decode_ast(kind_map, marshalled, tail_ptr):

    (num_children, tail_ptr) = decode_int(marshalled, tail_ptr)
    if num_children < 0:
        return (None, tail_ptr)

    root = Ast()

    (kind, tail_ptr) = decode_int(marshalled, tail_ptr)
    root.set_kind(kind)

    (location, tail_ptr) = decode_int(marshalled, tail_ptr)
    root.set_location(location)

    (lexeme, tail_ptr) = decode_string(marshalled, tail_ptr)
    root.set_lexeme(lexeme)

    for i in range(0, num_children):
        (ast, tail_ptr) = decode_ast(kind_map, marshalled, tail_ptr)    
        root.set_child(i, ast)

    return (root, tail_ptr)

def decode_error_message(marshalled, tail_ptr):

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

    return message

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

        self.kind_map = None
        self.kind_imap = None

        if len(args) == 0:
            self.this_handle = HOSHI.py_parser_new_parser()
            return

        if len(args) > 1 or not isinstance(args[0], Parser): 
            raise TypeError("A single Hoshi.Parser is required to clone")

        self.this_handle = HOSHI.py_parser_clone_parser(c_int64(args[0].this_handle))

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
    #  Generate a parser from a grammar file.
    #
    
    def generate(self, source, *args):
        
        if len(args) > 0:
            kind_map = args[0]
        else:
            kind_map = None
        
        if len(args) > 1:
            debug_flags = args[1]
        else:
            debug_flags = 0
        
        exception_ptr = c_void_p(None)
        
        HOSHI.py_parser_generate(parser_handle_out(self.this_handle), 
                                 byref(exception_ptr), 
                                 string_out(source), 
                                 kind_map_out(kind_map), 
                                 int64_out(debug_flags))
        
        check_exceptions(exception_ptr)
        
        self.copy_kind_map()

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
    #  get_ast
    #  -------
    #  
    #  Return the Ast.
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
    #  dump_ast
    #  --------
    #  
    #  For debugging we'll provide a nice dump function.
    #
    
    def dump_ast(self, ast, *args):

        if len(args) > 0:
            indent = int(args[0])
        else:
            indent = 0

        line = " " * indent + self.get_kind_string(ast.kind) + "(" + str(ast.kind) + ")"

        if ast.lexeme != "":
            line += " [" + ast.lexeme + "]"

        if ast.location >= 0:
            line += " @ " + str(ast.location)

        print(line)

        for child in ast.children:
            self.dump_ast(child, indent + 4)

    #
    #  copy_kind_map
    #  -------------
    #  
    #  Get the kind map from the native library.
    #
    
    def copy_kind_map(self):

        try:

            marshalled = self.get_encoded_kind_map()
            (self.kind_map, tail_ptr) = decode_kind_map(marshalled, 0)

            self.kind_imap = {}
            for key in self.kind_map:
                self.kind_imap[self.kind_map[key]] = key

        except UnknownError as e:
            raise e
        except Exception as e:
            raise UnknownError(str(e))

    #
    #  get_encoded_kind_map
    #  --------------------
    #  
    #  Return the kind map encoded as a string. We use this method to pass the
    #  kind map to the caller to facilitate interlanguage calls.
    #

    def get_encoded_kind_map(self):
        
        exception_ptr = c_void_p(None)
        result_ptr = c_void_p(None)
        
        HOSHI.py_parser_get_encoded_kind_map(parser_handle_out(self.this_handle), 
                                             byref(exception_ptr), 
                                             byref(result_ptr))
        
        check_exceptions(exception_ptr)
        
        return string_result_in(result_ptr)
        
    #
    #  get_kind
    #  --------
    #  
    #  Get the integer code for a given string.
    #
    
    def get_kind(self, kind_string):

        if self.kind_map == None:
            self.copy_kind_map()

        if kind_string not in self.kind_map:
            return -1

        return self.kind_map[kind_string]

    #
    #  get_kind_force
    #  --------------
    #  
    #  Get the integer code for a given string. If it doesn't exist then install it.
    #
    
    def get_kind_force(self, kind_string):

        if self.kind_map == None:
            self.copy_kind_map()

        if kind_string not in self.kind_map:
            self.get_kind_force(self.this_handle, kind_string)
            self.copy_kind_map()

        return self.kind_map[kind_string]

    #
    #  get_kind_string
    #  ---------------
    #  
    #  Get the text name for a numeric kind code.
    #
    
    def get_kind_string(self, kind):

        if type(kind) is Ast:
            kind = kind.get_kind()

        if self.kind_imap == None:
            self.copy_kind_map()

        if kind not in self.kind_imap:
            return "Unknown"

        return self.kind_imap[kind]

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
            long_message = None
        
        exception_ptr = c_void_p(None)
        
        HOSHI.py_parser_add_error(parser_handle_out(self.this_handle), 
                                  byref(exception_ptr), 
                                  error_type_out(error_type), 
                                  int64_out(location), 
                                  string_out(short_message), 
                                  string_out(long_message))
        
        check_exceptions(exception_ptr)
        
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
    #  get_error_messages
    #  ------------------
    #  
    #  Return the error messages.
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
            (message, tail_ptr) = decode_error_message(marshalled, tail_ptr)
            messages.append(message)
            length -= 1

        return messages

    #
    #  get_encoded_error_messages
    #  --------------------------
    #  
    #  Return the error messages encoded as a string. We use this method to pass
    #  entire lists back to the caller to facilitate interlanguage calls.
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
    
    def decode(self, str, *args):
        
        if len(args) > 0:
            kind_map = args[0]
        else:
            kind_map = None
        
        exception_ptr = c_void_p(None)
        
        HOSHI.py_parser_decode(parser_handle_out(self.this_handle), 
                               byref(exception_ptr), 
                               string_out(str), 
                               kind_map_out(kind_map))
        
        check_exceptions(exception_ptr)
        
        self.copy_kind_map()

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

