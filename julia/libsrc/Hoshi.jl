#
#  Hoshi                                                                   
#  -----                                                                   
#                                                                          
#  Hoshi is a parser generator written in C++. This file is a wrapper that 
#  accesses Hoshi as a native library.
#

module Hoshi

    #
    #  DebugType                                                       
    #  ----------                                                       
    #                                                                   
    #  We allow quite a few debugging options. It's the only way to get 
    #  through such a large library.                                    
    #

    export DebugType
    baremodule DebugType
        const DebugProgress    =     1
        const DebugAstHandlers =     2
        const DebugGrammar     =     4
        const DebugGrammarAst  =     8
        const DebugLalr        =    16
        const DebugScanner     =    32
        const DebugActions     =    64
        const DebugICode       =   128
        const DebugVCodeExec   =   256
        const DebugScanToken   =   512
        const DebugParseAction =  1024
    end

    #
    #  ExceptionType                                                        
    #  -------------                                                        
    #                                                                       
    #  We'll have to pass exceptions back to the language-specific clients. 
    #  This flag tells the receiver the kind of exception.                  
    #                                                                       

    baremodule ExceptionType
        const ExceptionNull    =  0
        const ExceptionGrammar =  1
        const ExceptionSource  =  2
        const ExceptionUnknown =  3
    end

    #
    #  Exception types
    #  ---------------
    #                                                                 
    #  These are the julia versions of the exceptions thrown in C++. 
    #

    export GrammarError
    type GrammarError <: Exception
        msg::String
    end
    Base.showerror(io::IO, e::GrammarError) = print(io, e.msg)

    export SourceError
    type SourceError <: Exception
        msg::String
    end
    Base.showerror(io::IO, e::SourceError) = print(io, e.msg)

    export UnknownError
    type UnknownError <: Exception
        msg::String
    end
    Base.showerror(io::IO, e::UnknownError) = print(io, e.msg)

    #
    #  Parser()                                                       
    #  --------                                                       
    #                                                                 
    #  Create a parser in the Hoshi library either by cloning or from 
    #  scratch.                                                       
    #

    export Parser
    type Parser

        #
        #  Instance variables. 
        #

        this_ptr::Ptr{Void}
        missing_kind_map::Bool
        kind_map::Dict{String, Int32}
        kind_imap::Dict{Int32, String}

        #
        #  Member function variables. 
        #

        is_grammar_loaded::Function
        is_grammar_failed::Function
        is_source_loaded::Function
        is_source_failed::Function
        generate::Function
        parse::Function
        get_ast::Function
        get_encoded_ast::Function
        dump_ast::Function
        copy_kind_map::Function
        get_encoded_kind_map::Function
        get_kind::Function
        get_kind_force::Function
        get_kind_string::Function
        add_error::Function
        get_error_count::Function
        get_warning_count::Function
        get_error_messages::Function
        get_encoded_error_messages::Function
        get_source_list::Function
        encode::Function
        decode::Function

        #
        #  Internal constructor. 
        #

        function Parser(args...)

            this = new()

            if length(args) == 0

                this.this_ptr = ccall((:jl_parser_new_parser, "libhoshi"),
                                      Ptr{Void},
                                      ())

                this.kind_map = Dict{String, Int32}()
                this.kind_imap = Dict{Int32, String}()
                this.missing_kind_map = true

            elseif length(args) == 1 && typeof(args[1]) == Parser

                this.this_ptr = ccall((:jl_parser_clone_parser, "libhoshi"),
                                      Ptr{Void},
                                      (Ptr{Void},),
                                      args[1].this_ptr)

                this.kind_map = args[1].kind_map
                this.kind_imap = args[1].kind_imap
                this.missing_kind_map = args[1].missing_kind_map

            else

                throw(ArgumentError("Invalid arguments to Hoshi.Parser ctor"))

            end

            #
            #  finalizer                                   
            #  ---------                                   
            #                                              
            #  Free the memory used in the native library. 
            #

            finalizer(this, function(in::Parser)

                ccall((:jl_parser_delete_parser, "libhoshi"),
                      Void,
                      (Ptr{Void},),
                      this.this_ptr)

            end)

            #
            #  is_grammar_loaded
            #  -----------------
            #  
            #  Check whether the parser has a grammer loaded.
            #
        
            this.is_grammar_loaded = function()
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result::UInt8 = ccall((:jl_parser_is_grammar_loaded, "libhoshi"),
                                      UInt8,
                                      (Ptr{Void}, Ptr{Cptrdiff_t}),
                                      this.this_ptr, 
                                      exception_handle)
                
                check_exceptions(exception_handle)
                
                return result
                
            end
        
            #
            #  is_grammar_failed
            #  -----------------
            #  
            #  Check whether the parser has a failed grammar.
            #
        
            this.is_grammar_failed = function()
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result::UInt8 = ccall((:jl_parser_is_grammar_failed, "libhoshi"),
                                      UInt8,
                                      (Ptr{Void}, Ptr{Cptrdiff_t}),
                                      this.this_ptr, 
                                      exception_handle)
                
                check_exceptions(exception_handle)
                
                return result
                
            end
        
            #
            #  is_source_loaded
            #  ----------------
            #  
            #  Check whether the parser has a source loaded.
            #
        
            this.is_source_loaded = function()
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result::UInt8 = ccall((:jl_parser_is_source_loaded, "libhoshi"),
                                      UInt8,
                                      (Ptr{Void}, Ptr{Cptrdiff_t}),
                                      this.this_ptr, 
                                      exception_handle)
                
                check_exceptions(exception_handle)
                
                return result
                
            end
        
            #
            #  is_source_failed
            #  ----------------
            #  
            #  Check whether the parser has a grammer failed.
            #
        
            this.is_source_failed = function()
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result::UInt8 = ccall((:jl_parser_is_source_failed, "libhoshi"),
                                      UInt8,
                                      (Ptr{Void}, Ptr{Cptrdiff_t}),
                                      this.this_ptr, 
                                      exception_handle)
                
                check_exceptions(exception_handle)
                
                return result
                
            end
        
            #
            #  generate
            #  --------
            #  
            #  Generate a parser from a grammar file.
            #
            
            this.generate = function(source::String, args...)
                
                if length(args) > 2
                    throw(ArgumentError("Too many arguments for Hoshi.generate"))
                end
                
                kind_map::Dict{String, Int32} = Dict{String, Int32}()
                
                if length(args) > 0
                    if isa(args[1], Dict{String, Int32})
                        kind_map = args[1]
                    else
                        throw(ArgumentError("Invalid type for kind_map in Hoshi.generate"))
                    end
                end
                
                debug_flags::Int64 = 0
                
                if length(args) > 1
                    if isa(args[2], Int64)
                        debug_flags = args[2]
                    else
                        throw(ArgumentError("Invalid type for debug_flags in Hoshi.generate"))
                    end
                end
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                ccall((:jl_parser_generate, "libhoshi"),
                      Void,
                      (Ptr{Void}, Ptr{Cptrdiff_t}, Ptr{Uint8}, Ptr{Void}, Int64),
                      this.this_ptr, 
                      exception_handle, 
                      string_out(source), 
                      kind_map_out(kind_map), 
                      debug_flags)
                
                check_exceptions(exception_handle)
                
                this.copy_kind_map()
        
            end
        
            #
            #  parse
            #  -----
            #  
            #  Parse a source string saving the Ast and error messages.
            #
        
            this.parse = function(source::String, args...)
                
                if length(args) > 1
                    throw(ArgumentError("Too many arguments for Hoshi.Parse"))
                end
                
                debug_flags::Int64 = 0
                
                if length(args) > 0
                    if isa(args[1], Int64)
                        debug_flags = args[1]
                    else
                        throw(ArgumentError("Invalid type for debug_flags in Hoshi.Parse"))
                    end
                end
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                ccall((:jl_parser_parse, "libhoshi"),
                      Void,
                      (Ptr{Void}, Ptr{Cptrdiff_t}, Ptr{Uint8}, Int64),
                      this.this_ptr, 
                      exception_handle, 
                      string_out(source), 
                      debug_flags)
                
                check_exceptions(exception_handle)
                
            end
        
            #
            #  get_ast
            #  -------
            #  
            #  Return the Ast.
            #
            
            this.get_ast = function()
        
                marshalled::String = this.get_encoded_ast()
                (kind_map, tail_ptr) = decode_kind_map(marshalled, 1)
                (ast, tail_ptr) = decode_ast(kind_map, marshalled, tail_ptr)
        
                return ast
        
            end
        
            #
            #  get_encoded_ast
            #  ---------------
            #  
            #  Return the Ast encoded as a string. We use this method to pass entire
            #  trees back to the caller to facilitate interlanguage calls.
            #
        
            this.get_encoded_ast = function()
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result_ptr = Array(Cptrdiff_t, 1)
                result_handle = convert(Ptr{Cptrdiff_t}, result_ptr)
                
                ccall((:jl_parser_get_encoded_ast, "libhoshi"),
                      Void,
                      (Ptr{Void}, Ptr{Cptrdiff_t}, Ptr{Cptrdiff_t}),
                      this.this_ptr, 
                      exception_handle, 
                      result_handle)
                
                check_exceptions(exception_handle)
                
                return string_result_in(result_handle)
                
            end
        
            #
            #  dump_ast
            #  --------
            #  
            #  For debugging we'll provide a nice dump function.
            #
            
            this.dump_ast = function(ast::Ast, args...)
        
                indent::Int
        
                if length(args) > 1
                    throw(ArgumentError("Too many arguments for Hoshi.dump_ast"))
                end
                        
                if length(args) > 0
                    indent = int(args[1])
                else
                    indent = 0
                end
        
                line = " " ^ indent * this.get_kind_string(ast.kind) * "(" * string(ast.kind) * ")"
        
                if ast.lexeme != ""
                    line *= " [" * ast.lexeme * "]"
                end
        
                if ast.location >= 0
                    line *= " @ " * string(ast.location)
                end
        
                println(line)
        
                for child in ast.children
                    this.dump_ast(child, indent + 4)
                end
        
            end
        
            #
            #  copy_kind_map
            #  -------------
            #  
            #  Get the kind map from the native library.
            #
            
            this.copy_kind_map = function()
        
                marshalled = this.get_encoded_kind_map()
                (this.kind_map, tail_ptr) = decode_kind_map(marshalled, 1)
        
                this.kind_imap = Dict{Int32, String}()
        
                for key in keys(this.kind_map)
                    this.kind_imap[this.kind_map[key]] = key
                end
        
                this.missing_kind_map = false
        
            end
        
            #
            #  get_encoded_kind_map
            #  --------------------
            #  
            #  Return the kind map encoded as a string. We use this method to pass the
            #  kind map to the caller to facilitate interlanguage calls.
            #
        
            this.get_encoded_kind_map = function()
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result_ptr = Array(Cptrdiff_t, 1)
                result_handle = convert(Ptr{Cptrdiff_t}, result_ptr)
                
                ccall((:jl_parser_get_encoded_kind_map, "libhoshi"),
                      Void,
                      (Ptr{Void}, Ptr{Cptrdiff_t}, Ptr{Cptrdiff_t}),
                      this.this_ptr, 
                      exception_handle, 
                      result_handle)
                
                check_exceptions(exception_handle)
                
                return string_result_in(result_handle)
                
            end
        
            #
            #  get_kind
            #  --------
            #  
            #  Get the integer code for a given string.
            #
            
            this.get_kind = function(kind_string::String)
        
                if this.missing_kind_map
                    this.copy_kind_map()
                end
        
                if !haskey(this.kind_map, kind_string)
                    return -1
                end
        
                return this.kind_map[kind_string]
        
            end
        
            #
            #  get_kind_force
            #  --------------
            #  
            #  Get the integer code for a given string. If it doesn't exist then install it.
            #
            
            this.get_kind_force = function(kind_string::String)
        
                if this.missing_kind_map
                    this.copy_kind_map()
                end
        
                if !haskey(this.kind_map, kind_string)
        
                    exception_ptr = Array(Cptrdiff_t, 1)
                    exception_ptr[1] = 0
                    exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                    
                    ccall((:jl_parser_get_kind_force, "hoshi"),
                          Int32,
                          (Ptr{Void}, Ptr{Cptrdiff_t}, Ptr{Uint8}),
                          this.this_ptr, 
                          exception_handle, 
                          string_out(kind_string))
                    
                    check_exceptions(exception_handle)
        
                    this.copy_kind_map()
        
                end
        
                return this.kind_map[kind_string]
        
            end
        
            #
            #  get_kind_string
            #  ---------------
            #  
            #  Get the text name for a numeric kind code.
            #
            
            this.get_kind_string = function(kind::Int32)
        
                if this.missing_kind_map
                    this.copy_kind_map()
                end
        
                if !haskey(this.kind_imap, kind)
                    return "Unknown"
                end
        
                return this.kind_imap[kind]
        
            end
        
            #
            #  add_error
            #  ---------
            #  
            #  Add another error to the message list. This is provided so that clients
            #  can use the parser message handler for all errors, not just parsing
            #  errors.
            #
        
            this.add_error = function(error_type::Int32, 
                                      location::Int64, 
                                      short_message::String, 
                                      args...)
                
                if length(args) > 1
                    throw(ArgumentError("Too many arguments for Hoshi.add_error"))
                end
                
                long_message::String = 0
                
                if length(args) > 0
                    if isa(args[1], String)
                        long_message = args[1]
                    else
                        throw(ArgumentError("Invalid type for long_message in Hoshi.add_error"))
                    end
                end
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                ccall((:jl_parser_add_error, "libhoshi"),
                      Void,
                      (Ptr{Void}, Ptr{Cptrdiff_t}, int, Int64, Ptr{Uint8}, Ptr{Uint8}),
                      this.this_ptr, 
                      exception_handle, 
                      error_type_out(error_type), 
                      location, 
                      string_out(short_message), 
                      string_out(long_message))
                
                check_exceptions(exception_handle)
                
            end
        
            #
            #  get_error_count
            #  ---------------
            #  
            #  Return the number of error messages over the error threshhold.
            #
        
            this.get_error_count = function()
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result::Int32 = ccall((:jl_parser_get_error_count, "libhoshi"),
                                      Int32,
                                      (Ptr{Void}, Ptr{Cptrdiff_t}),
                                      this.this_ptr, 
                                      exception_handle)
                
                check_exceptions(exception_handle)
                
                return result
                
            end
        
            #
            #  get_warning_count
            #  -----------------
            #  
            #  Return the number of error messages under the error threshhold.
            #
        
            this.get_warning_count = function()
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result::Int32 = ccall((:jl_parser_get_warning_count, "libhoshi"),
                                      Int32,
                                      (Ptr{Void}, Ptr{Cptrdiff_t}),
                                      this.this_ptr, 
                                      exception_handle)
                
                check_exceptions(exception_handle)
                
                return result
                
            end
        
            #
            #  get_error_messages
            #  ------------------
            #  
            #  Return the error messages.
            #
            
            this.get_error_messages = function()
        
                marshalled::String = this.get_encoded_error_messages()
        
                (len, tail_ptr) = decode_int(marshalled, 1)
                messages = Array(ErrorMessage, 0)
                sizehint(messages, len)
        
                while len > 0
                    (message, tail_ptr) = decode_error_message(marshalled, tail_ptr)
                    push!(messages, message)
                    len -= 1
                end
        
                return messages
        
            end
        
            #
            #  get_encoded_error_messages
            #  --------------------------
            #  
            #  Return the error messages encoded as a string. We use this method to pass
            #  entire lists back to the caller to facilitate interlanguage calls.
            #
        
            this.get_encoded_error_messages = function()
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result_ptr = Array(Cptrdiff_t, 1)
                result_handle = convert(Ptr{Cptrdiff_t}, result_ptr)
                
                ccall((:jl_parser_get_encoded_error_messages, "libhoshi"),
                      Void,
                      (Ptr{Void}, Ptr{Cptrdiff_t}, Ptr{Cptrdiff_t}),
                      this.this_ptr, 
                      exception_handle, 
                      result_handle)
                
                check_exceptions(exception_handle)
                
                return string_result_in(result_handle)
                
            end
        
            #
            #  get_source_list
            #  ---------------
            #  
            #  Return a source list with embedded messages.
            #
        
            this.get_source_list = function(source::String, args...)
                
                if length(args) > 1
                    throw(ArgumentError("Too many arguments for Hoshi.get_source_list"))
                end
                
                indent::Int32 = 0
                
                if length(args) > 0
                    if isa(args[1], Int32)
                        indent = args[1]
                    else
                        throw(ArgumentError("Invalid type for indent in Hoshi.get_source_list"))
                    end
                end
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result_ptr = Array(Cptrdiff_t, 1)
                result_handle = convert(Ptr{Cptrdiff_t}, result_ptr)
                
                ccall((:jl_parser_get_source_list, "libhoshi"),
                      Void,
                      (Ptr{Void}, Ptr{Cptrdiff_t}, Ptr{Cptrdiff_t}, Ptr{Uint8}, Int32),
                      this.this_ptr, 
                      exception_handle, 
                      result_handle, 
                      string_out(source), 
                      indent)
                
                check_exceptions(exception_handle)
                
                return string_result_in(result_handle)
                
            end
        
            #
            #  encode
            #  ------
            #  
            #  Create a string encoding of a Parser.
            #
        
            this.encode = function()
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                result_ptr = Array(Cptrdiff_t, 1)
                result_handle = convert(Ptr{Cptrdiff_t}, result_ptr)
                
                ccall((:jl_parser_encode, "libhoshi"),
                      Void,
                      (Ptr{Void}, Ptr{Cptrdiff_t}, Ptr{Cptrdiff_t}),
                      this.this_ptr, 
                      exception_handle, 
                      result_handle)
                
                check_exceptions(exception_handle)
                
                return string_result_in(result_handle)
                
            end
        
            #
            #  decode
            #  ------
            #  
            #  Decode a previously created string into a parser
            #
            
            this.decode = function(str::String, args...)
                
                if length(args) > 1
                    throw(ArgumentError("Too many arguments for Hoshi.decode"))
                end
                
                kind_map::Dict{String, Int32} = Dict{String, Int32}()
                
                if length(args) > 0
                    if isa(args[1], Dict{String, Int32})
                        kind_map = args[1]
                    else
                        throw(ArgumentError("Invalid type for kind_map in Hoshi.decode"))
                    end
                end
                
                exception_ptr = Array(Cptrdiff_t, 1)
                exception_handle = convert(Ptr{Cptrdiff_t}, exception_ptr)
                
                ccall((:jl_parser_decode, "libhoshi"),
                      Void,
                      (Ptr{Void}, Ptr{Cptrdiff_t}, Ptr{Uint8}, Ptr{Void}),
                      this.this_ptr, 
                      exception_handle, 
                      string_out(str), 
                      kind_map_out(kind_map))
                
                check_exceptions(exception_handle)
                
                this.copy_kind_map()
        
            end
        
            return this

        end

    end

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

    export Ast
    type Ast

        #
        #  Instance variables. 
        #

        kind::Int32
        lexeme::String
        location::Int64
        children::Array{Ast, 1}

        #
        #  Internal constructor. 
        #

        function Ast()

            this = new()

            this.kind = 0
            this.lexeme = ""
            this.location = 0
            this.children = Array(Ast, 0)

            return this

        end

    end

    #
    #  ErrorType                                                               
    #  ----------                                                              
    #                                                                          
    #  We'll encode each error message with an enumerated type. For now we're  
    #  not going to do anything with this but at some point we may want to add 
    #  options like classifying them as warnings or errors, disabling and so   
    #  forth.                                                                  
    #

    baremodule ErrorType
        const ErrorProgress          =   0
        const ErrorAstHandlers       =   1
        const ErrorGrammar           =   2
        const ErrorGrammarAst        =   3
        const ErrorLalr              =   4
        const ErrorScanner           =   5
        const ErrorActions           =   6
        const ErrorICode             =   7
        const ErrorVCodeExec         =   8
        const ErrorScanToken         =   9
        const ErrorParseAction       =  10
    end

    #
    #  ErrorMessage                                                           
    #  ------------                                                           
    #                                                                         
    #  Both the parser generator and the parser can detect and return errors  
    #  in this form. We only create errors on the C++ side but we provide the 
    #  ability to pull them all to the python side for processing.            
    #

    export ErrorMessage
    type ErrorMessage

        #
        #  Instance variables. 
        #

        message_type::Int32
        tag::String
        severity::Int32
        location::Int64
        line_num::Int64
        column_num::Int64
        source_line::String
        short_message::String
        long_message::String
        string::String

        #
        #  Internal constructor. 
        #

        function ErrorMessage()

            this = new()

            message_type = 0
            tag = ""
            severity = 0
            location = 0
            line_num = 0
            column_num = 0
            source_line = ""
            short_message = ""
            long_message = ""
            string = ""

            return this

        end

    end

    #
    #  string_out                                           
    #  ----------                                           
    #                                                       
    #  Convert a julia String value onto a char*.
    #

    function string_out(str::Any)

        if isa(str, String)
            return convert(Ptr{Uint8}, str)
        end

        throw(ArgumentError("Invalid arguments to Hoshi.string_out"))

    end

    #
    #  string_result_in                                             
    #  ----------------
    #                                                                
    #  Convert the returned value for a string into a julia string. 
    #

    function string_result_in(result_ptr::Ptr{Cptrdiff_t})
        
        length = ccall((:jl_get_string_length, "libhoshi"),
                       Int32,
                       (Ptr{Cptrdiff_t},),
                       result_ptr)

        if length < 0
            return ""
        end

        buf = Array(Uint8, length)

        ccall((:jl_get_string_string, "libhoshi"),
              Void,
              (Ptr{Cptrdiff_t}, Ptr{Uint8}),
              result_ptr,
              convert(Ptr{Uint8}, buf))

        return bytestring(pointer(buf))

    end
    
    #
    #  kind_map_out
    #  ------------
    #                                                                
    #  Convert a kind_map to marshalled form.
    #

    function kind_map_out(kind_map::Dict{String, Int32})
        return string_out(encode_kind_map(kind_map))
    end

    #
    #  check_exceptions                                             
    #  ----------------
    #                                                                
    #  Check for any exceptions and throw any found.
    #

    function check_exceptions(exception_handle)

        exception_type = ccall((:jl_get_exception_type, "libhoshi"),
                               Int32,
                               (Ptr{Cptrdiff_t},),
                               exception_handle)

        if exception_type < 0 || exception_type == ExceptionType.ExceptionNull
            return
        end

        length = ccall((:jl_get_exception_length, "libhoshi"),
                       Int32,
                       (Ptr{Cptrdiff_t},),
                       exception_handle)

        if length < 0
            return
        end

        buf = Array(Uint8, length)

        ccall((:jl_get_exception_string, "libhoshi"),
              Void,
              (Ptr{Cptrdiff_t}, Ptr{Uint8}),
              exception_handle,
              convert(Ptr{Uint8}, buf))

        what = bytestring(pointer(buf))

        if exception_type == ExceptionType.ExceptionGrammar
            throw(GrammarError(what))
        elseif exception_type == ExceptionType.ExceptionSource
            throw(SourceError(what))
        elseif exception_type == ExceptionType.ExceptionUnknown
            throw(UnknownError(what))
        end

    end
    
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

    function encode_int(value)
        return @sprintf("%d|", value)
    end

    function decode_int(marshalled::String, tail_ptr)

        buffer = ""

        while true

            if marshalled[tail_ptr] == '`'
                tail_ptr += 1
            elseif marshalled[tail_ptr] == '|'
                tail_ptr += 1
                break
            end

            buffer = buffer * string(marshalled[tail_ptr])
            tail_ptr += 1

        end

        return (int(buffer), tail_ptr)

    end

    function encode_string(value::String)

        buffer = ""

        for c in value

            if c == '`' || c == '|'
                buffer = buffer * string('`')
            end

            buffer = buffer * string(c)

        end

        buffer = buffer * string('|')

        return buffer

    end
    
    function decode_string(marshalled::String, tail_ptr)

        buffer = ""

        while true

            if marshalled[tail_ptr] == '`'
                tail_ptr += 1
            elseif marshalled[tail_ptr] == '|'
                tail_ptr += 1
                break
            end

            buffer = buffer * string(marshalled[tail_ptr])
            tail_ptr += 1

        end

        return (buffer, tail_ptr)

    end

    function encode_kind_map(kind_map::Dict{String, Int32})

        buffer = encode_int(length(keys(kind_map)))
     
        for key in keys(kind_map)
            buffer += encode_string(key)    
            buffer += encode_int(kind_map[key])
        end

        return buffer

    end

    function decode_kind_map(marshalled::String, tail_ptr)

        (size, tail_ptr) = decode_int(marshalled, tail_ptr)

        kind_map = Dict{String, Int32}()

        for i = 1:size
            (key, tail_ptr) = decode_string(marshalled, tail_ptr)
            (num, tail_ptr) = decode_int(marshalled, tail_ptr)
            kind_map[key] = convert(Int32, num)
        end

        return (kind_map, tail_ptr)

    end

    function decode_ast(kind_map::Dict{String, Int32}, marshalled::String, tail_ptr)

        (num_children, tail_ptr) = decode_int(marshalled, tail_ptr)
        if num_children < 0
            return (None, tail_ptr)
        end

        root = Ast()
        sizehint(root.children, num_children)

        (root.kind, tail_ptr) = decode_int(marshalled, tail_ptr)
        (root.location, tail_ptr) = decode_int(marshalled, tail_ptr)
        (root.lexeme, tail_ptr) = decode_string(marshalled, tail_ptr)

        for i in 1:num_children
            (ast, tail_ptr) = decode_ast(kind_map, marshalled, tail_ptr)    
            push!(root.children, ast)
        end

        return (root, tail_ptr)

    end

    function decode_error_message(marshalled::String, tail_ptr)

        message = ErrorMessage()

        (message.message_type, tail_ptr) = decode_int(marshalled, tail_ptr)
        (message.tag, tail_ptr) = decode_string(marshalled, tail_ptr)
        (message.severity, tail_ptr) = decode_int(marshalled, tail_ptr)
        (message.location, tail_ptr) = decode_int(marshalled, tail_ptr)
        (message.line_num, tail_ptr) = decode_int(marshalled, tail_ptr)
        (message.column_num, tail_ptr) = decode_int(marshalled, tail_ptr)
        (message.source_line, tail_ptr) = decode_string(marshalled, tail_ptr)
        (message.short_message, tail_ptr) = decode_string(marshalled, tail_ptr)
        (message.long_message, tail_ptr) = decode_string(marshalled, tail_ptr)
        (message.string, tail_ptr) = decode_string(marshalled, tail_ptr)

        return (message, tail_ptr)

    end

end


