//
//  ErrorHandler                                                          
//  ------------                                                          
//                                                                        
//  Handle all kinds of source errors, maintaining enough information for 
//  the user to debug his code. We'll need to keep track of these in a    
//  variety of contexts so we split this out into a class by itself.      
//

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "ErrorHandler.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;

//
//  Missing argument flag. 
//

string ErrorHandler::string_missing{""};

//
//  List of error type names.
//

string ErrorHandler::error_tag_list[] =
{
    "Error",
    "Warning",
    "UnknownMacro",
    "DupGrammarOption",
    "DupToken",
    "DupTokenOption",
    "UnusedTerm",
    "UndefinedNonterm",
    "UnusedNonterm",
    "UselessNonterm",
    "UselessRule",
    "ReadsCycle",
    "SymbolSelfProduce",
    "LalrConflict",
    "WordOverflow",
    "CharacterRange",
    "RegexConflict",
    "DupAstItem",
    "Syntax",
    "Lexical",
    "AstIndex" 
};

//
//  List of error type severities.
//

int ErrorHandler::error_severity_list[] =
{
    100,  // Error
      0,  // Warning
    100,  // UnknownMacro
    100,  // DupGrammarOption
    100,  // DupToken
    100,  // DupTokenOption
      0,  // UnusedTerm
    100,  // UndefinedNonterm
      0,  // UnusedNonterm
    100,  // UselessNonterm
      0,  // UselessRule
    100,  // ReadsCycle
    100,  // SymbolSelfProduce
    100,  // LalrConflict
    100,  // WordOverflow
    100,  // CharacterRange
    100,  // RegexConflict
    100,  // DupAstItem
    100,  // Syntax
    100,  // Lexical
    100   // AstIndex
};

//
//  get_tag                              
//  -------                              
//                                            
//  Look up the tag in our static table. 
//

string ErrorHandler::get_tag(ErrorType error_type)
{
    
    if (error_type < ErrorType::ErrorMinimum ||
        error_type > ErrorType::ErrorMaximum)
    {
        return "Unknown";
    }

    return error_tag_list[error_type];

}

//
//  get_severity                              
//  ------------                              
//                                            
//  Look up the severity in our static table. 
//

int ErrorHandler::get_severity(ErrorType error_type)
{
    
    if (error_type < ErrorType::ErrorMinimum ||
        error_type > ErrorType::ErrorMaximum)
    {
        return 100;
    }

    return error_severity_list[error_type];

}

//
//  add_error                                    
//  ---------                                    
//                                               
//  Create a new message and add it to the list. 
//

void ErrorHandler::add_error(ErrorType error_type,
                             int64_t location,
                             const std::string& short_message,
                             const std::string& long_message)
{

    int64_t line_num = -1;
    int64_t column_num = -1;
    string line{""};

    src.get_source_position(location, line_num, column_num, line);

    ErrorMessage error_message;
    error_message.error_type = error_type;
    error_message.location = location;
    error_message.line_num = line_num;
    error_message.column_num = column_num;
    error_message.source_line = line;
    error_message.short_message = short_message;

    if (&long_message != &string_missing)
    {
        error_message.long_message = long_message;
    }
    else
    {
        error_message.long_message = short_message;
    }
    
    message_list.push_back(error_message);

}

//
//  get_error_count                                          
//  ---------------                                          
//                                                           
//  Return the number of messages over the error threshhold. 
//

int ErrorHandler::get_error_count() const
{

    int count = 0;

    for (auto msg: message_list)
    {

        if (get_severity(msg.error_type) >= min_error_severity)
        {
            count++;
        }

    }

    return count;

}

//
//  get_warning_count                                         
//  -----------------                                         
//                                                            
//  Return the number of messages below the error threshhold. 
//

int ErrorHandler::get_warning_count() const
{

    int count = 0;

    for (auto msg: message_list)
    {

        if (get_severity(msg.error_type) < min_error_severity)
        {
            count++;
        }

    }

    return count;

}

//
//  get_error_messages                                   
//  ------------------                                   
//                                                       
//  Return the list of error messages in location order. 
//

vector<ErrorMessage> ErrorHandler::get_error_messages()
{

    for (int i = 1; i < message_list.size(); i++)
    {

        for (int j = i;
             j > 0 && message_list[j].location < message_list[j - 1].location;
             j--)
        {
            swap(message_list[j], message_list[j - 1]);
        }

    }

    return message_list;

}

//
//  ErrorMessage Accessors                      
//  ----------------------                      
//                                           
//  Very simple accesors for error messages. 
//

ErrorType ErrorMessage::get_type() const
{
    return error_type;
}

string ErrorMessage::get_tag() const
{
    return ErrorHandler::get_tag(error_type);
}

int ErrorMessage::get_severity() const
{
    return ErrorHandler::get_severity(error_type);
}

int64_t ErrorMessage::get_location() const
{
    return location;
}

int64_t ErrorMessage::get_line_num() const
{
    return line_num;
}

int64_t ErrorMessage::get_column_num() const
{
    return column_num;
}

string ErrorMessage::get_source_line() const
{
    return source_line;
}

string ErrorMessage::get_short_message() const
{
    return short_message;
}

string ErrorMessage::get_long_message() const
{
    return long_message;
}

//
//  get_string                                                            
//  ----------                                                            
//                                                                        
//  Return a fairly long formatted string with the error message. This is 
//  a convenience function, but we hope clients will do their own         
//  formatting.                                                           
//

string ErrorMessage::get_string() const
{

    ostringstream ost;

    if (ErrorHandler::get_severity(error_type) < ErrorHandler::min_error_severity)
    {
        ost << "WARNING";
    }
    else
    {
        ost << "ERROR";
    }

    if (location < 0)
    {
        ost << ": ";
    }
    else
    {
        ost << " [" << line_num << ","
                    << column_num << "]: ";
    }

    if (line_num >= 0 &&
        source_line.length() > 0 &&
        source_line.length() <= 150)
    {
        ost << long_message << endl
            << source_line << endl 
            << setw(column_num - 1) << "" << setw(0) << "^";
    }
    else
    {
        ost << long_message;
    }

    return ost.str();

}

//
//  dump_source                                                            
//  -----------                                                            
//                                                                         
//  Create a source listing with error messages along with the lines       
//  containing the errors. This is very helpful in testing error recovery. 
//

void ErrorHandler::dump_source(const Source& src,
                               ostream& os,
                               int indent) const
{

    int64_t line_start = 0;
    int64_t line_end = 0;
    int line_number = 1;
    int message_next = 0;

    //
    //  Dump errors with an unknown location. 
    //

    for (;
         message_next < message_list.size() &&
             message_list[message_next].location < 0;
         message_next++)
    {

        const ErrorMessage& msg = message_list[message_next];

        if (ErrorHandler::get_severity(msg.get_type()) < ErrorHandler::min_error_severity)
        {
            os << "WARNING: ";
        }
        else
        {
            os << "ERROR: ";
        }

        os << msg.get_long_message()
           << endl;
            
    }

    if (message_next > 0)
    {
        os << endl;
    }

    //
    //  Now dump each line of source followed by the messages. 
    //

    while (line_start < src.length() || message_next < message_list.size())
    {

        //
        //  Dump one source line. 
        //

        for (line_end = line_start;
             line_end < src.length() && 
                 src.get_char(line_end) != '\r' &&
                 src.get_char(line_end) != '\n' &&
                 src.get_char(line_end) != 026;
            line_end++);

        os << setw(indent) << setfill(' ') << ""
           << setw(5) << right << line_number << setw(0) << "  "
           << Source::to_ascii_chop(src.get_string(line_start, line_end))
           << endl;

        line_number++;

        if (src.get_char(line_end) == '\r')
        {
            line_end++;
        }

        if (src.get_char(line_end) == '\n')
        {
            line_end++;
        }

        if (src.get_char(line_end) == 026)
        {
            line_end++;
        }

        //
        //  Create a line with carets under each error location. 
        //

        int carets_length = 0;
        for (int i = message_next;
             i < message_list.size() &&
                 (message_list[i].location < line_end || line_end >= src.length());
             i++)
        {

            int64_t line_num;
            int64_t column_num;
            string line;
            src.get_source_position(message_list[i].location, line_num, column_num, line);

            if (carets_length == 0)
            {
                os << setw(indent + 7) << setfill(' ') << "";
            }

            if (carets_length < column_num - 1)    
            {
                os << setw(column_num - carets_length - 1) << setfill(' ') << "";
                carets_length = column_num - 1;
            }

            if (carets_length < column_num)    
            {
                os << "^";
                carets_length = column_num;
            }

        }

        if (carets_length > 0)
        {
            os << endl;
        }

        //
        //  Dump the text of each message. 
        //

        for (int i = message_next;
             i < message_list.size() &&
                 (message_list[i].location < line_end || line_end >= src.length());
             i++)
        {

            const ErrorMessage& msg = message_list[i];

            if (ErrorHandler::get_severity(msg.get_type()) < ErrorHandler::min_error_severity)
            {
                os << "WARNING: ";
            }
            else
            {
                os << "ERROR: ";
            }

            os << msg.get_long_message()
               << endl;
            
            message_next++;

        }

        line_start = line_end;

    }

}

} // namespace hoshi


