
push!(LOAD_PATH, "/dev/julia")

println("Kirk Kirk Kirk Kirk Kirk Kirk")

require("Dates.jl")
import Dates
using Dates

require("Hoshi.jl")
import Hoshi
using Hoshi

macro R_mstr(s)
    s
end

grammar = R"""
//
//  DateTime Grammar                                                       
//  ----------------                                                       
//                                                                         
//  Grammar recognizes a variety of date time formats creating an Ast with 
//  the individual elements. Note that we are only validating format here, 
//  not values.
//

options

    lookaheads = 4
    case_sensitive = false

rules

    DateTime       ::= SlashFormat | DashFormat | DotFormat 
                       | LongFormat | LongAltFormat 
                       | ShortFormat | ShortAltFormat
                       | ReutersFormat
    
    SlashFormat    ::= <integer> '/' <integer> '/' <integer> Time?
                   :   ( (Month &1 @1) (Day &3 @3) (Year &5 @5) $6._ )

    DashFormat     ::= <integer> '-' <integer> '-' <integer> Time?
                   :   ( (Month &3 @3) (Day &5 @5) (Year &1 @1) $6._ )

    DotFormat      ::= <integer> '.' <integer> '.' <integer> Time?
                   :   ( (Month &3 @3) (Day &1 @1) (Year &5 @5) $6._ )

    LongFormat     ::= ( DayName ','? )? ( MonthName | MonthAbv | May ) <integer> ','? <integer> Time?
                   :   ( $2 (Day &3 @3) (Year &5 @5) $6._ )

    LongAltFormat  ::= ( DayName ','? )? ( MonthName | MonthAbv | May ) <integer> Time <integer>
                   :   ( $2 (Day &3 @3) (Year &5 @5) $4._ )

    ShortFormat    ::= ( MonthAbv | May ) '-' <integer> '-' <integer> Time?
                   :   ( $1 (Day &3 @3) (Year &5 @5) $6._ )

    ShortAltFormat ::= <integer> '-' ( MonthAbv | May ) '-' <integer> Time?
                   :   ( $3 (Day &1 @1) (Year &5 @5) $6._ )

    ReutersFormat  ::= <integer> ( MonthAbv | May ) <integer> Time?
                   :   ( $2 (Day &1 @1) (Year &3 @3) $4._ )

    DayName        ::= 'mon' | 'monday' | 'tue' | 'tues' | 'tuesday' | 'wed' | 'wednesday'
                       | 'thu' | 'thur' | 'thurs' | 'thursday' | 'fri' | 'friday' 
                       | 'sat' | 'saturday' | 'sun' | 'sunday'

    MonthAbv       ::= 'jan'       : (Month &"1")
                  
    MonthAbv       ::= 'feb'       : (Month &"2")
                  
    MonthAbv       ::= 'mar'       : (Month &"3")
                  
    MonthAbv       ::= 'apr'       : (Month &"4")
                  
    May            ::= 'may'       : (Month &"5")
                  
    MonthAbv       ::= 'jun'       : (Month &"6")
                  
    MonthAbv       ::= 'jul'       : (Month &"7")
                  
    MonthAbv       ::= 'aug'       : (Month &"8")
                  
    MonthAbv       ::= 'sep'       : (Month &"9")
                  
    MonthAbv       ::= 'oct'       : (Month &"10")
                  
    MonthAbv       ::= 'nov'       : (Month &"11")
                  
    MonthAbv       ::= 'dec'       : (Month &"12")
                  
    MonthName      ::= 'january'   : (Month &"1")
                  
    MonthName      ::= 'february'  : (Month &"2")
                  
    MonthName      ::= 'march'     : (Month &"3")
                  
    MonthName      ::= 'april'     : (Month &"4")
                  
    MonthName      ::= 'june'      : (Month &"6")
                  
    MonthName      ::= 'july'      : (Month &"7")
                  
    MonthName      ::= 'august'    : (Month &"8")
                  
    MonthName      ::= 'september' : (Month &"9")
                  
    MonthName      ::= 'october'   : (Month &"10")
                  
    MonthName      ::= 'november'  : (Month &"11")
                  
    MonthName      ::= 'december'  : (Month &"12")
                  
    Time           ::= <integer> ':' <integer> ':' <integer> '.' <integer> AmPm
                   :   ( (Hour &1 @1) (Minute &3 @3) (Second &5 @5) (Millisecond &7 @7) $8 )
                  
    Time           ::= <integer> ':' <integer> '.' <integer> AmPm
                   :   ( (Minute &1 @1) (Second &3 @3) (Millisecond &5 @5) $6 )
                  
    Time           ::= <integer> '.' <integer> AmPm
                   :   ( (Second &1 @1) (Millisecond &3 @3) $4)
                  
    Time           ::= <integer> ':' <integer> ':' <integer> AmPm
                   :   ( (Hour &1 @1) (Minute &3 @3) (Second &5 @5) $6 )
                  
    Time           ::= <integer> ':' <integer> AmPm
                   :   ( (Hour &1 @1) (Minute &3 @3) $4)
                  
    AmPm           ::= 'am'        : (AmPm &"0")
                  
    AmPm           ::= 'pm'        : (AmPm &"12")
                  
    AmPm           ::= empty       : (AmPm &"0")
"""                  

#
#  parse_time                                                            
#  ----------                                                            
#                                                                        
#  Convert a time string into a datetime.datetime oblect. There are many 
#  formats for representing time as a string. This function should       
#  recognize a fairly rich subset.                                       
#

parser_ref = Parser()
try

    parser_ref.generate(grammar)

catch e

    if isa(e, GrammarError)
        for message in parser_ref.get_error_messages()
            println(message.string)
        end
        error("Fatal grammar errors")
    else
        error("Unexpected excepion in Hoshi!")
    end

end

#
#  parse_time                                                       
#  ----------                                                       
#                                                                   
#  Function entry point. Convert a string into a datetime.datetime. 
#

function parse_time(s)

    #
    #  Make a local copy of the parser and parse the string. 
    #

    parser = Parser(parser_ref)

    try

        parser.parse(s)
        #parser.dump_ast(parser.get_ast(root), 0)

    catch e

        if isa(e, SourceError)
            for message in parser_ref.get_error_messages()
                println(message.string)
            end
            return DateTime(1,1,1,1,1,1,1)
        else
            error("Mysterious error in Hoshi parser")
        end

    end

    #
    #  Read the individual components into a map and normalize them. 
    #

    date_time_element = Dict{String, Int64}()
    for kind in ["Year", "Month", "Day", "Hour", "Minute", "Second", "Millisecond", "AmPm"] 
        date_time_element[kind] = 0
    end

    set_date_time_elements(parser, parser.get_ast(), date_time_element)

    if date_time_element["Hour"] + date_time_element["AmPm"] < 24
        date_time_element["Hour"] += date_time_element["AmPm"]
    end

    #
    #  Construct the datetime and return. 
    #

    return DateTime(date_time_element["Year"],
                    date_time_element["Month"],
                    date_time_element["Day"],
                    date_time_element["Hour"],
                    date_time_element["Minute"],
                    date_time_element["Second"],
                    date_time_element["Millisecond"])

end
 
#
#  set_date_time_elements                                                  
#  ----------------------                                                  
#                                                                          
#  This is more elaborate than it needs to be. I wanted to play with a way 
#  to avoid a 'case' statement, which Julia doesn't have.                  
#

function set_date_time_elements(parser::Parser,
                                root::Ast,
                                date_time_element::Dict{String, Int64})

    get(["SlashFormat" => function()
             for ast in root.children
                 set_date_time_elements(parser, ast, date_time_element)
             end
         end,
         "DashFormat" => function()
             for ast in root.children
                 set_date_time_elements(parser, ast, date_time_element)
             end
         end,
         "DotFormat" => function()
             for ast in root.children
                 set_date_time_elements(parser, ast, date_time_element)
             end
         end,
         "LongFormat" => function()
             for ast in root.children
                 set_date_time_elements(parser, ast, date_time_element)
             end
         end,
         "LongAltFormat" => function()
             for ast in root.children
                 set_date_time_elements(parser, ast, date_time_element)
             end
         end,
         "ShortFormat" => function()
             for ast in root.children
                 set_date_time_elements(parser, ast, date_time_element)
             end
         end,
         "ShortAltFormat" => function()
             for ast in root.children
                 set_date_time_elements(parser, ast, date_time_element)
             end
         end,
         "ReutersFormat" => function()
             for ast in root.children
                 set_date_time_elements(parser, ast, date_time_element)
             end
         end,
         "Year" => function()
             year = int(root.lexeme)
             if year <= 50
                 year += 2000
             elseif year < 200
                 year += 1900
             end
             date_time_element["Year"] = year
         end,
         "Month" => function()
             date_time_element["Month"] = int(root.lexeme)
         end,
         "Day" => function()
             date_time_element["Day"] = int(root.lexeme)
         end,
         "Hour" => function()
             date_time_element["Hour"] = int(root.lexeme)
         end,
         "Minute" => function()
             date_time_element["Minute"] = int(root.lexeme)
         end,
         "Second" => function()
             date_time_element["Second"] = int(root.lexeme)
         end,
         "Millisecond" => function()
             date_time_element["Millisecond"] = int(root.lexeme)
         end,
         "AmPm" => function()
             date_time_element["AmPm"] = int(root.lexeme)
         end
        ],
        parser.get_kind_string(root.kind), 
        function()
            println("Error!")
            parser.dump_ast(root)
        end
    )()

end

#
#  Test code. Remove this if you want to use this file for more than a 
#  test.                                                               
#

for s in ("11/19/13oops",
          "11/19/13 11:19:13.123",
          "11/19/13 11:19:13.123 PM",
          "Tue, November 19, 13 11:19.432",
          "13-Feb-2014 3:15PM",
          "Thu Feb 13 15:15:00 2014") 

    dt = parse_time(s)
    println(s * (" " ^ (36 - length(s))) * string(dt))

end
