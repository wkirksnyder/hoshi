//
//  Source                                                                 
//  ------                                                                 
//                                                                         
//  This class is an abstraction that provides source text to the rest of  
//  the program. The remainder of the program should generally assume that 
//  when it sees an individual character it is in UCS-4/UTF-32, but        
//  strings are always returned as UTF-8. We're also including a few       
//  utilities here for handling those characters and strings. The          
//  implementation of this class needs work, but if we can maintain the    
//  abstraction we should be able to polish it eventually without changing 
//  the remainder of the program.                                          
//

#include <codecvt>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "Parser.H"

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

using namespace std;

//
//  char_length                                                          
//  -----------                                                          
//                                                                       
//  Find the length of an UTF-8 string in code points. We have to convert 
//  to UCS-4 first.                                                      
//

int64_t Source::char_length(const string& str)
{

    static wstring_convert<codecvt_utf8<char32_t>, char32_t> myconv;

    try
    {
        return myconv.from_bytes(str).length();
    }
    catch (...)
    {
        throw range_error("Hoshi source is not valid UTF-8");
    }

}

//
//  to_ascii_chop                                                          
//  -------------                                                          
//                                                                         
//  Convert a UTF-8 string to ASCII by chopping each code point at 7 bits. 
//  This is only really useful for debugging lists. We are throwing away a 
//  lot of information.                                                    
//

string Source::to_ascii_chop(const string& str)
{

    static wstring_convert<codecvt_utf8<char32_t>, char32_t> myconv;

    try
    {

        ostringstream ost;

        for (auto c: myconv.from_bytes(str))
        {

            if (c < 0x20)
	    {
                ost << ".";
            }
            else
            {
                ost << static_cast<char>(c & 0x7f);
            }

        }

        return ost.str();

    }
    catch (...)
    {
        throw range_error("Hoshi source is not valid UTF-8");
    }

}

//
//  to_ascii_cpp                                                          
//  ------------                                                          
//                                                                        
//  Convert a UTF-8 string to ASCII suitable for a C++ literal. Leave the 
//  UTF-8 basically intact, but escape values outside of printable ASCII. 
//

string Source::to_ascii_cpp(const string& str)
{

    ostringstream ost;

    for (auto c: str)
    {

        switch (c)
        {
            
            case '\\':  ost << "\\\\";    break;
            case '\0':  ost << "\\0";     break;
            case '\n':  ost << "\\n";     break;
            case '\r':  ost << "\\r";     break;
            case '\t':  ost << "\\t";     break;
            case '\b':  ost << "\\b";     break;
            case '\a':  ost << "\\a";     break;
            case '\f':  ost << "\\f";     break;
            case '\v':  ost << "\\v";     break;
            case '\"':  ost << "\\\x22";  break;

            default: 
            {

                if (c < 32 || c > 127)
                {
                    ost << "\\x" << hex << setfill('0') << setw(2) << static_cast<int>(c)
                        << dec << setfill(' ') << setw(0);
                }
                else
                {
                    ost << static_cast<char>(c & 0x7f);
                }

                break;

            }

        }

    }

    return ost.str();

}

//
//  to_utf32                                  
//  --------                                  
//                                            
//  Convert a UTF-8 encoded string to UTF-32. 
//

u32string Source::to_utf32(const string& str)
{

    static wstring_convert<codecvt_utf8<char32_t>, char32_t> myconv;

    try
    {
        return u32string(myconv.from_bytes(str).data());
    }
    catch (...)
    {
        throw range_error("Hoshi source is not valid UTF-8");
    }

}

//
//  length                                          
//  ------                                          
//                                                  
//  Return the number of code points in the source. 
//

int64_t Source::length() const
{
    return source.length();
}

//
//  get_char                                                             
//  --------                                                             
//                                                                       
//  Return a specific code point by location. We should be fancier about 
//  this so we don't need to store the entire source as code points, but 
//  this will do for now.                                                
//

char32_t Source::get_char(int64_t location) const
{

    if (location < 0)
    {
        location = source.length() + location;
    }

    if (location < 0 || location >= source.length())
    {
        return eof_char;
    }

    return source[location];

}

//
//  get_string                                                            
//  ----------                                                            
//                                                                        
//  Get a substring of the source and convert it to UTF-8. This is how we 
//  will extract lexemes.                                                 
//

string Source::get_string(int64_t first, int64_t last) const
{

    static wstring_convert<codecvt_utf8<char32_t>, char32_t> myconv;

    if (last < 0)
    {
        last = source.length() + 1 + last;
    }

    if (last < 0 || last > source.length())
    {
        last = source.length(); 
    }

    if (first < 0 || first >= last)
    {
        return "";
    }

    return myconv.to_bytes(source.data() + first, source.data() + last);

}

//
//  get_source_position                                                    
//  -------------------                                                    
//                                                                         
//  Decode a location for an error message. We would like to determine the 
//  line number, column number and source line.                            
//

void Source::get_source_position(int64_t location, 
                                 int64_t& line_num,
                                 int64_t& column_num,
                                 string& line) const
{

    if (location < 0)
    {
        location = source.length() + location;
    }

    if (location < 0 || location >= source.length())
    {
        line_num = -1;
        column_num = -1;
        line = "";
    }

    int64_t start = 0;
    for (start = location;
         start >= 0 && source[start] != '\n';
         start--);
    start++;
    
    int64_t end = 0;
    for (end = location;
         end < source.length() && source[end] != '\n' && source[end] != '\r';
         end++);
    
    line = get_string(start, end);

    line_num = 1;
    for (int64_t i = location; i >= 0; i--)
    {
        if (source[i] == '\n')
        {
            line_num++;
        }
    }

    column_num = char_length(get_string(start, location)) + 1;

}

//
//  Source()                                
//  --------                                
//                                              
//  Create a Source object from a UTF-8 string. 
//

Source::Source(const string& str)
{

    static wstring_convert<codecvt_utf8<char32_t>, char32_t> myconv;

    try
    {
        source = myconv.from_bytes(str);
    }
    catch (...)
    {
        throw range_error("Hoshi source is not valid UTF-8");
    }

}

//
//  SourceFile()                                
//  ------------                                
//                                              
//  Create a Source object from a UTF-8 file. 
//

SourceFile::SourceFile(const string& file_name)
{

    static wstring_convert<codecvt_utf8<char32_t>, char32_t> myconv;

    ifstream strm(file_name.c_str(), ifstream::binary);

    if (strm == nullptr)
    {
        ostringstream ost;
        ost << "Missing file: " << file_name;
        throw invalid_argument(ost.str());
    }

    strm.seekg(0, ios::end);
    int file_length = strm.tellg();
    strm.seekg(0, ios::beg);

    char *buffer = new char[file_length + 1];
    strm.read(buffer, file_length);
    strm.close();

    try
    {
        source = myconv.from_bytes(buffer, buffer + file_length);
    }
    catch (...)
    {
        throw range_error("Hoshi source is not valid UTF-8");
    }

    delete [] buffer;

}

} // namespace hoshi


