//
//  Pascal Test                                                            
//  -----------                                                            
//                                                                         
//  In general Pascal isn't all that interesting, but the Ripley Druseikis 
//  test suite is the best database of naturally occuring parse errors I   
//  know of. This tests the error recovery mechanism on that test suite.   
//

#include <typeinfo>
#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "Parser.H"

using namespace std;
using namespace hoshi;

int main() 
{

    Parser parser;
    try
    {

        parser.generate(SourceFile("Pascal.G"), map<string, int>(),
                        static_cast<DebugType>(0)
                        | DebugType::DebugProgress
                       );

        parser.parse(SourceFile("Pascal.S"), 
                     static_cast<DebugType>(0)
                     | DebugType::DebugProgress
                    );

    }
    catch (GrammarError& e)
    {
        cout << "Grammar errors:" << endl;
        parser.dump_source(SourceFile("Pascal.G"), cout);
    }
    catch (SourceError& e)
    {
        cout << "Source errors:" << endl;
        parser.dump_source(SourceFile("Pascal.S"), cout);
    }
    catch (exception& e)
    {
        cout << "Exception: " << e.what() << endl;
    }
        
}

