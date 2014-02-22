#line 2 "u:\\hoshi\\raw\\ErrorHandler.cpp"
//
//  ErrorHandler                                                          
//  ------------                                                          
//                                                                        
//  Handle all kinds of source errors, maintaining enough information for 
//  the user to debug his code. We'll need to keep track of these in a    
//  variety of contexts so we split this out into a class by itself.      
//

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
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

//
//  ErrorHandler                                                          
//  ------------                                                          
//
//  The ErrorHandler proper.
//                                                                        

class ErrorHandler
{
public:

    static const int min_error_severity = 100;

    explicit ErrorHandler(const Source& src) : src(src) {}

    static int get_severity(ErrorType error_type);
    static std::string get_tag(ErrorType error_type);
    
    void add_error(ErrorType error_type,
                   int64_t location,
                   const std::string& short_message,
                   const std::string& long_message = string_missing);

    int get_error_count() const;
    int get_warning_count() const;
    std::vector<ErrorMessage> get_error_messages();

    void dump_source(const Source& src,
                     std::ostream& os = std::cout,
                     int indent = 0) const;

    //
    //  Copying wouldn't cause an integrity error but it's almost       
    //  certainly a mistake.
    //

    ErrorHandler(const ErrorHandler&) = default;
    ErrorHandler(ErrorHandler&&) = default;
    ErrorHandler& operator=(const ErrorHandler&) = default;
    ErrorHandler& operator=(ErrorHandler&&) = default;

private:

    static std::string string_missing;

    static std::string error_tag_list[];
    static int error_severity_list[];

    const Source& src;
    std::vector<ErrorMessage> message_list;

};

} // namespace hoshi

#endif // ERROR_HANDLER_H
