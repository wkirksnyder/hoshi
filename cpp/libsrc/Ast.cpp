//
//  Ast (Abstract Syntax Tree)                                          
//  --------------------------                                          
//                                                                      
//  An abstract syntax tree holds the important syntactic elements from 
//  the source in an easily traversable form.                           
//                                                                      
//  This class has a lot of naked pointers. The convention is that a    
//  parent owns all its children. When we delete a node we delete the   
//  entire subtree. If we need to copy a pointer and claim ownership we 
//  must clone the subtree.                                             
//

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <map>
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
//  Constructor & Destructor                                               
//  ------------------------                                               
//                                                                         
//  Not that the number of children must be set in the constructor and can 
//  not be changed.                                                        
//

Ast::Ast(int num_children) : num_children(num_children)
{

    children = new Ast*[num_children];

    for (int i = 0; i < num_children; i++)
    {
        children[i] = nullptr;
    }

}

Ast::~Ast()
{

    for (int i = 0; i < num_children; i++)
    {
        delete children[i];
    }

    delete [] children;

}

//
//  Simple Accessors 
//  ---------------- 
//

int Ast::get_kind() const
{
    return kind;
}

void Ast::set_kind(int kind)
{
    this->kind = kind;
}

int64_t Ast::get_location() const
{
    return location;
}

void Ast::set_location(int64_t location)
{
    this->location = location;
}

std::string Ast::get_lexeme() const
{
    return lexeme;
}

void Ast::set_lexeme(const std::string& lexeme)
{
    this->lexeme = lexeme;
}

Ast* Ast::get_parent() const
{
    return parent;
}

int Ast::get_num_children() const
{
    return num_children;
}

Ast* Ast::get_child(int index) const
{
    return children[index];
}

void Ast::set_child(int index, Ast* ast)
{

    children[index] = ast;
    if (ast != nullptr)
    {
        ast->parent = this;
    }

}

//
//  clone                                                                 
//  -----                                                                 
//                                                                        
//  Clone a subtree. We use reference semantics for Ast's, so this is for 
//  the few times we need value semantics.                                
//

Ast* Ast::clone() const
{

    Ast* ast = new Ast(num_children);

    ast->kind = kind;
    ast->location = location;
    ast->lexeme = lexeme;
    ast->parent = nullptr;

    for (int i = 0; i < num_children; i++)
    {

        if (children[i] == nullptr)
        {
            ast->children[i] = nullptr;
        }
        else
        {
            ast->children[i] = children[i]->clone();
            ast->children[i]->parent = ast;
        }

    }

    return ast;

}

//
//  encode_cpp                                                               
//  ----------                                                               
//                                                                       
//  We want to be able to save an Ast to a C++ source file and load from 
//  the same. This helps us with various bootstrapping hurdles. Here we  
//  have the encode function.                                            
//

void Ast::encode_cpp(const Ast* root,
                     const Source* src,
                     const Parser& parser,
                     const string& file_name,
                     const string& identifier)
{

    ofstream os(file_name.c_str());

    //
    //  encode_ast                                          
    //  ----------                                          
    //                                                  
    //  Encode one node and recurively encode children. 
    //

    std::function<void(const Ast*, int)> encode_ast = 
        [&](const Ast* ast, int indent) -> void
    {

        if (indent > 0)
        {
            os << std::setw(indent) << "" << std::setw(0);
        }

        if (ast == nullptr)
        {
            os << "nullptr," << std::endl;
            return;
        }

        os << "\"" << parser.get_kind_string(ast->kind) << "\", ";

        os << "\"";

        for (auto c: ast->lexeme)
        {

            switch (c)
            {
                
                case '\\':  os << "\\\\";    break;
                case '\0':  os << "\\0";     break;
                case '\n':  os << "\\n";     break;
                case '\r':  os << "\\r";     break;
                case '\t':  os << "\\t";     break;
                case '\b':  os << "\\b";     break;
                case '\a':  os << "\\a";     break;
                case '\f':  os << "\\f";     break;
                case '\v':  os << "\\v";     break;
                case '\"':  os << "\\\x22";  break;

                default: 
                {

                    if (c < 32 || c > 127)
                    {
                        os << "\\x" << hex << setfill('0') << setw(2) << static_cast<int>(c)
                           << dec << setfill(' ') << setw(0);
                    }
                    else
                    {
                        os << static_cast<char>(c & 0x7f);
                    }

                    break;

                }

            }

        }

        os << "\", ";

        os << "\"" << ast->location << "\", ";

        os << "\"" << ast->num_children << "\", ";

        os << std::endl;

        for (int i = 0; i < ast->num_children; i++)
        {
            Ast* child = ast->children[i];
            encode_ast(child, indent + 4);
        }

    };

    //
    //  encode_cpp                     
    //  ----------                     
    //                                 
    //  The function body begins here. 
    //

    os << "static const char* " << identifier << "[] =" << endl
       << "{" << endl 
       << endl
       << "    //" << endl
       << "    //  Grammar source." << endl
       << "    //" << endl
       << endl;

    os << "    \"";

    for (auto c: src->get_string(0, src->length()))
    {
        
        switch (c)
        {
            
            case '\\':  os << "\\\\";    break;
            case '\0':  os << " ";       break;
            case '\r':  os << "\\r";     break;
            case '\t':  os << "\\t";     break;
            case '\b':  os << "\\b";     break;
            case '\a':  os << "\\a";     break;
            case '\f':  os << "\\f";     break;
            case '\v':  os << "\\v";     break;
            case '\"':  os << "\\\x22";  break;

            case '\n':
            {
                os << "\\n\"" << endl << "    \"";
                break;
            }
   
            default: 
            {

                if (c < 32 || c > 127)
                {
                    os << "\\x" << hex << setfill('0') << setw(2) << static_cast<int>(c)
                       << dec << setfill(' ') << setw(0);
                }
                else
                {
                    os << static_cast<char>(c & 0x7f);
                }

                break;

            }

        }

    }

    os << "\"," << endl;

    os << endl
       << "    //" << endl
       << "    //  Ast." << endl
       << "    //" << endl
       << endl;

    encode_ast(root, 4);

    os << "    nullptr" << endl 
       << "};" << endl;

    os.close();

}

//
//  decode_cpp                                                               
//  ----------                                                               
//                                                                       
//  We want to be able to save an Ast to a C++ source file and load from 
//  the same. This helps us with various bootstrapping hurdles. Here we  
//  have the decode function.                                            
//

void Ast::decode_cpp(Ast*& root,
                     Source*& src,
                     const Parser& parser,
                     const char* item[])
{

    //
    //  decode_ast                                          
    //  ----------                                          
    //                                                  
    //  Decode one node and recurively decode children. 
    //

    std::function<Ast*(int&)> decode_ast = [&](int& index) -> Ast*
    {
        
        if (item[index] == nullptr)
        {
            return nullptr;
        }

        string kind_string = item[index++];
        string lexeme = item[index++];
        int64_t location = atol(item[index++]);
        int num_children = atoi(item[index++]);

        Ast* ast = new Ast(num_children);
        ast->set_kind(parser.get_kind(kind_string));
        ast->set_location(location);
        ast->set_lexeme(lexeme);
        
        for (int i = 0; i < num_children; i++)
        {
            ast->set_child(i, decode_ast(index));
        }
            
        return ast;

    };

    //
    //  decode_cpp                     
    //  ----------                     
    //                                 
    //  The function body begins here. 
    //

    int index = 0;

    src = new Source(item[index++]);
    root = decode_ast(index);

}

} // namespace hoshi


