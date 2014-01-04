//
//  DateTime Parser                                                       
//  ---------------                                                       
//                                                                        
//  Class contains a parser for Dates and times along with a test driver. 
//

using System;
using System.Text;
using System.IO;
using System.Security;
using System.Threading;
using System.Collections.Generic;
using hoshi;

public class DateTimeUtil
{

    //
    //  DateTime Grammar                                                       
    //  ----------------                                                       
    //                                                                         
    //  Grammar recognizes a variety of date time formats creating an Ast with 
    //  the individual elements. Note that we are only validating format here, 
    //  not values.
    //

    private const string grammar = @"

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

            MonthAbv       ::= 'jan'       : (Month &'1')
                          
            MonthAbv       ::= 'feb'       : (Month &'2')
                          
            MonthAbv       ::= 'mar'       : (Month &'3')
                          
            MonthAbv       ::= 'apr'       : (Month &'4')
                          
            May            ::= 'may'       : (Month &'5')
                          
            MonthAbv       ::= 'jun'       : (Month &'6')
                          
            MonthAbv       ::= 'jul'       : (Month &'7')
                          
            MonthAbv       ::= 'aug'       : (Month &'8')
                          
            MonthAbv       ::= 'sep'       : (Month &'9')
                          
            MonthAbv       ::= 'oct'       : (Month &'10')
                          
            MonthAbv       ::= 'nov'       : (Month &'11')
                          
            MonthAbv       ::= 'dec'       : (Month &'12')
                          
            MonthName      ::= 'january'   : (Month &'1')
                          
            MonthName      ::= 'february'  : (Month &'2')
                          
            MonthName      ::= 'march'     : (Month &'3')
                          
            MonthName      ::= 'april'     : (Month &'4')
                          
            MonthName      ::= 'june'      : (Month &'6')
                          
            MonthName      ::= 'july'      : (Month &'7')
                          
            MonthName      ::= 'august'    : (Month &'8')
                          
            MonthName      ::= 'september' : (Month &'9')
                          
            MonthName      ::= 'october'   : (Month &'10')
                          
            MonthName      ::= 'november'  : (Month &'11')
                          
            MonthName      ::= 'december'  : (Month &'12')
                          
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
                          
            AmPm           ::= 'am'        : (AmPm &'0')
                          
            AmPm           ::= 'pm'        : (AmPm &'12')
                          
            AmPm           ::= empty       : (AmPm &'0')

        ";

    //
    //  DateTime component types. 
    //

    enum DateTimeType : int
    {
        Year         = 0,
        Month        = 1,
        Day          = 2,
        Hour         = 3,
        Minute       = 4,
        Second       = 5,
        Millisecond  = 6,
        AmPm         = 7
    }

    //
    //  static constructor                                              
    //  ------------------                                              
    //                                                                  
    //  On startup create a parser template we can copy on each call to 
    //  ParseTime. Copying is cheap, generating is expensive.           
    //

    static Parser parserRef = null;

    static DateTimeUtil()
    {

        Dictionary<string, int> kindMap = new Dictionary<string, int>();

        for (int i = 0; i < Enum.GetValues(typeof(DateTimeType)).Length; i++)
        {
            kindMap[Enum.GetValues(typeof(DateTimeType)).GetValue(i).ToString()] =
                (int)Enum.GetValues(typeof(DateTimeType)).GetValue(i);
        }

        try
        {

            parserRef = new Parser();
            parserRef.Generate(grammar, kindMap);

        }
        catch (GrammarError)
        {

            foreach (ErrorMessage m in parserRef.GetErrorMessages())
            {
                Console.WriteLine(m.String);
            }

            Environment.Exit(1);

        }
       
    }

    //
    //  parseTime                                                       
    //  ---------                                                       
    //                                                                  
    //  Convert a time string into a System.DateTime. There are many     
    //  formats for representing time as a string. This function should 
    //  recognize a fairly rich subset.                                 
    //

    public static DateTime ParseTime(String source)
    {

        Parser parser = new Parser(parserRef);

        int length = 0;
        foreach (DateTimeType dateTimeType in Enum.GetValues(typeof(DateTimeType)))
        {
            if ((int)dateTimeType > length)
            {
                length = (int)dateTimeType;
            }
        }

        length++;

        int [] dateTimeElement = new int[length];
        foreach (DateTimeType dateTimeType in Enum.GetValues(typeof(DateTimeType)))
        {
            dateTimeElement[(int)dateTimeType] = 0;
        }

        try
        {

            parser.Parse(source);
            Ast root = parser.GetAst();

            foreach (Ast ast in root.Children) {

                if (!Int32.TryParse(ast.Lexeme, out dateTimeElement[ast.Kind]))
                {
                    Console.Error.WriteLine("Internal Error: Number Format error");
                    Environment.Exit(1);
                }

                if (ast.Kind == (int)DateTimeType.Millisecond)
                {

                    for (int i = 3; i < ast.Lexeme.Length; i++)
                    {
                        dateTimeElement[ast.Kind] /= 10;
                    }

                    for (int i = ast.Lexeme.Length; i < 3; i++)
                    {
                        dateTimeElement[ast.Kind] *= 10;
                    }

                }

            }

        }
        catch (SourceError)
        {
            throw new ArgumentException("Invalid date!");
        }

        //
        //  Normalize the numeric values and put them in the form 
        //  required by Calendar;
        //

        if (dateTimeElement[(int)DateTimeType.Year] <= 50)
        {
            dateTimeElement[(int)DateTimeType.Year] += 2000;
        }
        else if (dateTimeElement[(int)DateTimeType.Year] < 200)
        {
            dateTimeElement[(int)DateTimeType.Year] += 1900;
        }

        if (dateTimeElement[(int)DateTimeType.Hour] +
            dateTimeElement[(int)DateTimeType.AmPm] < 24)
        {
            dateTimeElement[(int)DateTimeType.Hour] +=
            dateTimeElement[(int)DateTimeType.AmPm];
        }

        //
        //  Construct the date time object.
        //

        return new DateTime(dateTimeElement[(int)DateTimeType.Year],
                            dateTimeElement[(int)DateTimeType.Month],
                            dateTimeElement[(int)DateTimeType.Day],
                            dateTimeElement[(int)DateTimeType.Hour],
                            dateTimeElement[(int)DateTimeType.Minute],
                            dateTimeElement[(int)DateTimeType.Second],
                            dateTimeElement[(int)DateTimeType.Millisecond]);

    }

    //
    //  Test Driver. 
    //

    public static void Main()
    {

        string [] testCase = new string []
        {
            "11/19/13",
            "11/39/13",
            "11/19/13 11:19:13.1234",
            "11/19/13 11:19:13.1234 PM",
            "Tue, November 19, 13 11:19.4321",
            "13-Feb-2014 3:15PM",
            "Thu Feb 13 15:15:00 2014"
        };

        for (int i = 0; i < testCase.Length; i++)
        {

            String pDate = "";
        
            try
            {
                DateTime date = ParseTime(testCase[i]);
                pDate = date.ToString("o");
            }
            catch (ArgumentException)
            {
                pDate = "*error*";
            }
 
            Console.WriteLine("{0,-35} {1}", testCase[i], pDate);

        }

    }

}
