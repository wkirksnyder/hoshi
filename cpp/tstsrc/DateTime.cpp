#include <mutex>
#include <iostream>
#include <chrono>
#include "Parser.H"

using namespace std;
using namespace chrono;
using namespace hoshi;

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

#pragma warning(disable : 1345)
const string grammarx = R"!(
options

    lookaheads = 4
    case_sensitive = false

rules

    DateTime       ::= SlashFormat | DashFormat | DotFormat 
                       | LongFormat | LongAltFormat 
                       | ShortFormat | ShortAltFormat
                       | ReutersFormat
    
    SlashFormat    ::= <integer> '/' <integer> '/' <integer> Time?
                   :   ( (Month &1) (Day &3) (Year &5) $6._ )

    DashFormat     ::= <integer> '-' <integer> '-' <integer> Time?
                   :   ( (Month &3) (Day &5) (Year &1) $6._ )

    DotFormat      ::= <integer> '.' <integer> '.' <integer> Time?
                   :   ( (Month &3) (Day &1) (Year &5) $6._ )

    LongFormat     ::= ( DayName ','? )? ( MonthName | MonthAbv | May ) <integer> ','? <integer> Time?
                   :   ( $2 (Day &3) (Year &5) $6._ )

    LongAltFormat  ::= ( DayName ','? )? ( MonthName | MonthAbv | May ) <integer> Time <integer>
                   :   ( $2 (Day &3) (Year &5) $4._ )

    ShortFormat    ::= ( MonthAbv | May ) '-' <integer> '-' <integer> Time?
                   :   ( $1 (Day &3) (Year &5) $6._ )

    ShortAltFormat ::= <integer> '-' ( MonthAbv | May ) '-' <integer> Time?
                   :   ( $3 (Day &1) (Year &5) $6._ )

    ReutersFormat  ::= <integer> ( MonthAbv | May ) <integer> Time?
                   :   ( $2 (Day &1) (Year &3) $4._ )

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
                   :   ( (Hour &1) (Minute &3) (Second &5) (Millisecond &7) $8 )
                  
    Time           ::= <integer> ':' <integer> '.' <integer> AmPm
                   :   ( (Minute &1) (Second &3) (Millisecond &5) $6 )
                  
    Time           ::= <integer> '.' <integer> AmPm
                   :   ( (Second &1) (Millisecond &3) $4)
                  
    Time           ::= <integer> ':' <integer> ':' <integer> AmPm
                   :   ( (Hour &1) (Minute &3) (Second &5) $6 )
                  
    Time           ::= <integer> ':' <integer> AmPm
                   :   ( (Hour &1) (Minute &3) $4)
                  
    AmPm           ::= 'am'        : (AmPm &"0")
                  
    AmPm           ::= 'pm'        : (AmPm &"12")
                  
    AmPm           ::= empty       : (AmPm &"0")
                  
##################################################################################
##################################################################################
##################################################################################
)!";
#pragma warning(default : 1345)

string grammar = grammarx.substr(0, grammarx.find("###"));

//
//  parse_time                                                           
//  ----------                                                           
//                                                                       
//  Convert a time string into a system_clock::timepoint. There are many 
//  formats for representing time as a string. This function should      
//  recognize a fairly rich subset.                                      
//

system_clock::time_point parse_time(const string& source)
{

    //
    //  Components we must parse out of the string. 
    //

    enum DateTimeType
    {
        Year,
        Month,
        Day,
        Hour,
        Minute,
        Second,
        Millisecond,
        AmPm,
        DateTimeTypeSize
    };

    //
    //  Generating a parser is relatively expensive, copying one is cheap  
    //  and safe. Here we create a static master copy then copy it on each 
    //  call. This makes the function thread safe while maintaining good   
    //  efficiency.                                                        
    //

    static hoshi::Parser parser_ref;
    static volatile bool initialized = false;
    static mutex initialize_mutex;

    if (!initialized)
    {

        lock_guard<mutex> initialize_guard(initialize_mutex);

        if (!initialized)
        {

            try
            {

                map<string, int> kind_map;
                kind_map["Year"] = DateTimeType::Year;
                kind_map["Month"] = DateTimeType::Month;
                kind_map["Day"] = DateTimeType::Day;
                kind_map["Hour"] = DateTimeType::Hour;
                kind_map["Minute"] = DateTimeType::Minute;
                kind_map["Second"] = DateTimeType::Second;
                kind_map["Millisecond"] = DateTimeType::Millisecond;
                kind_map["AmPm"] = DateTimeType::AmPm;
                    
                parser_ref.generate(grammar, kind_map);

            }
            catch (hoshi::GrammarError& e)
            {
                parser_ref.dump_source(grammar, cerr);
                exit(1);
            }

            initialized = true;
        
        }

    }

    hoshi::Parser parser = parser_ref;

    //
    //  Initialize all the date components and parse the string. Pick the 
    //  resulting components out of the tree.                             
    //

    int date_time_element[DateTimeTypeSize];
    for (int i = 0; i < LENGTH(date_time_element); i++)
    {
        date_time_element[i] = 0;
    }

    try
    {

        parser.parse(source);
        Ast* root = parser.get_ast();

        for (int i = 0; i < root->get_num_children(); i++)
        {

            date_time_element[root->get_child(i)->get_kind()] =
                stoi(root->get_child(i)->get_lexeme());

            if (root->get_child(i)->get_kind() == DateTimeType::Millisecond)
            {

                for (int j = 3; j < root->get_child(i)->get_lexeme().length(); j++)
                {
                    date_time_element[DateTimeType::Millisecond] /= 10;
                }

                for (int j = root->get_child(i)->get_lexeme().length(); j < 3; j++)
                {
                    date_time_element[DateTimeType::Millisecond] *= 10;
                }

            }

        }

    }
    catch (hoshi::SourceError& e)
    {
        parser.dump_source(source, cout);
        throw invalid_argument("Invalid date-time");
    }
    
    //
    //  Normalize the numeric values and put them in the form 
    //  required by tm.                                              
    //

    if (date_time_element[Year] <= 50)
    {
        date_time_element[Year] += 2000;
    }
    else if (date_time_element[Year] < 200)
    {
        date_time_element[Year] += 1900;
    }

    if (date_time_element[Hour] + date_time_element[AmPm] < 24)
    {
        date_time_element[Hour] += date_time_element[AmPm];
    }

    date_time_element[Month]--;
    date_time_element[Day]++;

    //
    //  Use the C++ standard library to convert to a timepoint. 
    //

    struct tm tm;

    tm.tm_year = date_time_element[Year] - 1900;
    tm.tm_mon = date_time_element[Month];
    tm.tm_mday = date_time_element[Day];
    tm.tm_hour = date_time_element[Hour];
    tm.tm_min = date_time_element[Minute];
    tm.tm_sec = date_time_element[Second];
    tm.tm_isdst = 0;

    time_t result_time_t = mktime(&tm);

    if (result_time_t < 0)
    {
        throw invalid_argument("Invalid date-time");
    }

    result_time_t = result_time_t - timezone - 86400;

    //
    //  The library routine doesn't handle milliseconds so add them in. 
    //

    return system_clock::from_time_t(result_time_t) +
           milliseconds(date_time_element[Millisecond]);

}

//
//  Test Driver. 
//

int main() 
{

    string test_case[] =
    {
        "11/19/13",
        "11/19/13 11:19:13.1234",
        "11/19/13 11:19:13.1234 PM",
        "Tue, November 19, 13 11:19.4321",
        "13-Feb-2014 3:15PM",
        "Thu Feb 13 15:15:00 2014"
    };

    for (int i = 0; i < LENGTH(test_case); i++)
    {

        string ptime;

        try
        {
            time_t t = system_clock::to_time_t(parse_time(test_case[i]));
            ptime = asctime(gmtime(&t));
        }
        catch (invalid_argument& e)
        {
            ptime = "*error*\n";
        }

        cout << left << setw(35) << test_case[i] << " " << ptime;

    }

}

