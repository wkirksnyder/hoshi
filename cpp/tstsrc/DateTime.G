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
