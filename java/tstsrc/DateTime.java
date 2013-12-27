//
//  DateTime Parser                                                       
//  ---------------                                                       
//                                                                        
//  Class contains a parser for Dates and times along with a test driver. 
//

import java.lang.*;
import java.util.*;
import hoshi.*;

public class DateTime {

    //
    //  DateTime component types. 
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
    }

    static DateTimeType [] dateTimeTypeValues = DateTimeType.values();

    //
    //  static constructor                                              
    //  ------------------                                              
    //                                                                  
    //  On startup create a parser template we can copy on each call to 
    //  parseTime. Copying is cheap, generating is expensive.           
    //

    static Parser parserRef = null;

    static {

        HashMap<String, Integer> kindMap = new HashMap<String, Integer>();

        for (DateTimeType dateTimeType: dateTimeTypeValues) {
            kindMap.put(dateTimeType.name(), dateTimeType.ordinal());
        }

        try {

            parserRef = new Parser();
            parserRef.generate(Parser.loadFile("DateTime.G"), kindMap);

        } catch (GrammarError e) {

            for (ErrorMessage m: parserRef.getErrorMessages()) {
                System.out.println(m.getString());
            }

            Runtime.getRuntime().halt(1);

        }
       
    }

    //
    //  parseTime                                                       
    //  ---------                                                       
    //                                                                  
    //  Convert a time string into a java.util.Date. There are many     
    //  formats for representing time as a string. This function should 
    //  recognize a fairly rich subset.                                 
    //

    public static Date parseTime(String source) {

        Parser parser = new Parser(parserRef);

        int [] dateTimeElement = new int[dateTimeTypeValues.length];
        for (int i = 0; i < dateTimeElement.length; i++) {
            dateTimeElement[i] = 0;
        }

        try {

            parser.parse(source);
            Ast root = parser.getAst();

            for (Ast ast: root.getChildren()) {

                try {
                    dateTimeElement[ast.getKind()] = 
                        Integer.parseInt(ast.getLexeme());
                } catch (NumberFormatException e) {
                    System.err.println("Internal Error: Number Format error");
                    Runtime.getRuntime().halt(1);
                }

            }

        } catch (SourceError e) {
            return null;
        }

        //
        //  Normalize the numeric values and put them in the form 
        //  required by Calendar;
        //

        if (dateTimeElement[DateTimeType.Year.ordinal()] <= 50)
        {
            dateTimeElement[DateTimeType.Year.ordinal()] += 2000;
        }
        else if (dateTimeElement[DateTimeType.Year.ordinal()] < 200)
        {
            dateTimeElement[DateTimeType.Year.ordinal()] += 1900;
        }

        if (dateTimeElement[DateTimeType.Hour.ordinal()] +
            dateTimeElement[DateTimeType.AmPm.ordinal()] < 24)
        {
            dateTimeElement[DateTimeType.Hour.ordinal()] +=
            dateTimeElement[DateTimeType.AmPm.ordinal()];
        }

        dateTimeElement[DateTimeType.Month.ordinal()]--;

        //
        //  Use the Gregorian calendar to construct the Date object. 
        //

        Calendar calendar = new GregorianCalendar();
        calendar.set(Calendar.YEAR, dateTimeElement[DateTimeType.Year.ordinal()]);
        calendar.set(Calendar.MONTH, dateTimeElement[DateTimeType.Month.ordinal()]);
        calendar.set(Calendar.DAY_OF_MONTH, dateTimeElement[DateTimeType.Day.ordinal()]);
        calendar.set(Calendar.HOUR, dateTimeElement[DateTimeType.Hour.ordinal()]);
        calendar.set(Calendar.MINUTE, dateTimeElement[DateTimeType.Minute.ordinal()]);
        calendar.set(Calendar.SECOND, dateTimeElement[DateTimeType.Second.ordinal()]);
        calendar.set(Calendar.MILLISECOND, dateTimeElement[DateTimeType.Millisecond.ordinal()]);

        return calendar.getTime();

    }

    //
    //  Test Driver. 
    //

    public static void main(String [] args) {

        String [] testCase = new String []
        {
            "11/19/13",
            "11/19/13 11:19:13.1234",
            "11/19/13 11:19:13.1234 PM",
            "Tue, November 19, 13 11:19.4321",
            "13-Feb-2014 3:15PM",
            "Thu Feb 13 15:15:00 2014"
        };

        for (int i = 0; i < testCase.length; i++) {

            String pDate;
        
            Date date = parseTime(testCase[i]);
            if (date == null) {
                pDate = "*error*";
            } else {
                pDate = date.toString();
            }
 
            System.out.println(String.format("%-35s %s", testCase[i], pDate));

        }

    }

}
