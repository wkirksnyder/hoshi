//
//  Ast (Abstract Syntax Tree)                                              
//  --------------------------                                              
//                                                                          
//  An abstract syntax tree holds the important syntactic elements from the 
//  source in an easily traversable form.                                   
//                                                                          
//  The Ast typically has a lot of nodes that we are likely to traverse     
//  multiple times, so for efficency we marshall them on the C++ side and   
//  bring them over as an entire tree. So this class is pretty complete     
//  here, without reaching back into C++.                                   
//

package hoshi;

import java.io.*;
import java.lang.*;
import java.util.*;

public class Ast {

    private static final Initializer initializer = Initializer.getInitializer();

    private int kind;
    private long location;
    private String lexeme;
    private Ast [] children;

    public Ast(int numChildren) {
        kind = 0;
        location = -1;
        lexeme = "";
        children = new Ast[numChildren];
    }

    public int getKind() {
        return kind;
    }

    void setKind(int kind) {
        this.kind = kind;
    }
    
    public long getLocation() {
        return location;
    }

    void setLocation(long location) {
        this.location = location;
    }
    
    public String getLexeme() {
        return lexeme;
    }

    void setLexeme(String lexeme) {
        this.lexeme = lexeme;
    }
    
    public Ast [] getChildren() {
        return children;
    }

    void setChildren(Ast [] children) {
        this.children = children;
    }
    
    public Ast getChild(int childNum) {
        return children[childNum];
    }

    void setChild(int childNum, Ast child) {
        children[childNum] = child;
    }

}
