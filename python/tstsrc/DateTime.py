
import Hoshi
import datetime

GRAMMAR = '''
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
'''                  

#
#  parse_time                                                            
#  ----------                                                            
#                                                                        
#  Convert a time string into a datetime.datetime oblect. There are many 
#  formats for representing time as a string. This function should       
#  recognize a fairly rich subset.                                       
#

#
#  I hope python is smart enough to only load a given module once. In that 
#  case we will create the parser during that load and copy it on each     
#  call. I need to learn more about how multithreading works in python...  
#

parser_ref = Hoshi.Parser()
try:
    parser_ref.generate(GRAMMAR)
except Hoshi.GrammarError as e:
    for message in parser_ref.get_error_messages():
        print(message.get_string())
    exit(1)

#
#  parse_time                                                       
#  ----------                                                       
#                                                                   
#  Function entry point. Convert a string into a datetime.datetime. 
#

def parse_time(s):

    #
    #  Make a local copy of the parser and parse the string. 
    #

    parser = Hoshi.Parser(parser_ref)
    try:
        parser.parse(s)
        root = parser.get_ast()
    except Hoshi.SourceError as e:
        return None

    #
    #  Read the individual components into a map and normalize them. 
    #

    date_time_element = dict()
    for kind in ["Year", "Month", "Day", "Hour", "Minute", "Second", "Millisecond", "AmPm"]:
        date_time_element[kind] = 0

    root = parser.get_ast()
    for child in root.get_children():
        date_time_element[parser.get_kind_string(child)] = int(child.get_lexeme())
        if parser.get_kind_string(child) == "Millisecond":
            for j in range(3, len(child.get_lexeme())):
                date_time_element["Millisecond"] /= 10
            for j in range(len(child.get_lexeme()), 3):
                date_time_element["Millisecond"] *= 10

    if date_time_element["Year"] <= 50:
        date_time_element["Year"] += 2000
    elif date_time_element["Year"] < 200:
        date_time_element["Year"] += 1900

    if date_time_element["Hour"] + date_time_element["AmPm"] < 24:
        date_time_element["Hour"] += date_time_element["AmPm"]

    #
    #  Construct the datetime and return. 
    #

    return datetime.datetime(date_time_element["Year"],
                             date_time_element["Month"],
                             date_time_element["Day"],
                             date_time_element["Hour"],
                             date_time_element["Minute"],
                             date_time_element["Second"],
                             int(date_time_element["Millisecond"] * 1000))
 
#
#  Test code. Remove this if you want to use this file for more than a 
#  test.                                                               
#

for s in ["11/19/13",
          "11/19/13 11:19:13.1234",
          "11/19/13 11:19:13.1234 PM",
          "Tue, November 19, 13 11:19.4321",
          "13-Feb-2014 3:15PM",
          "Thu Feb 13 15:15:00 2014"]:
    dt = parse_time(s)
    if (dt == None):
        print("%-35s *error*" % (s))
    else:
        print("%-35s %s" % (s, dt.isoformat(' ')))
