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

const string grammar = R"!(
tokens

    <string>                  : regex = ''' ' ( \\ [^\n] | [^'\\\n] )* ' |
                                            " ( \\ [^\n] | [^"\\\n] )* " '''

    <comment>                 : template = <pascal_comment>

rules

   ProgramList                ::= Program+

   Program                    ::= ProgramHeading ';' Block '.'

   ProgramHeading             ::= 'program' <identifier>
                                  OptionalProgramParameterList

   OptionalProgramParameterList
                              ::= ProgramParameterList
                              |   empty

   ProgramParameterList       ::= '(' IdentifierList ')'

   Block                      ::= ConstantDefinitionPart
                                  TypeDefinitionPart
                                  VariableDeclarationPart
                                  ProcedureAndFunctionDeclarationPart
                                  CompoundStatement

   ConstantDefinitionPart     ::= 'const' ConstantDefinitionList ';'
                    
   ConstantDefinitionPart     ::= empty

   ConstantDefinitionList     ::= ConstantDefinitionList ';'
                                  ConstantDefinition

   ConstantDefinitionList     ::= ConstantDefinition

   TypeDefinitionPart         ::= 'type' TypeDefinitionList ';'

   TypeDefinitionPart         ::= empty

   TypeDefinitionList         ::= TypeDefinitionList ';' TypeDefinition

   TypeDefinitionList         ::= TypeDefinition

   VariableDeclarationPart    ::= 'var' VariableDeclarationList ';'

   VariableDeclarationPart    ::= empty

   VariableDeclarationList    ::= VariableDeclarationList ';'
                                  VariableDeclaration

   VariableDeclarationList    ::= VariableDeclaration

   ProcedureAndFunctionDeclarationPart
                              ::= ProcedureAndFunctionDeclarationList ';'
                              |   empty

   ProcedureAndFunctionDeclarationList
                              ::= ProcedureAndFunctionDeclarationList
                                  ';' ProcedureOrFunctionDeclaration
                              |   ProcedureOrFunctionDeclaration

   ProcedureOrFunctionDeclaration
                              ::= ProcedureDeclaration
                              |   FunctionDeclaration

   ConstantDefinition         ::= <identifier> '=' Constant

   TypeDefinition             ::= <identifier> '=' Type

   VariableDeclaration        ::= IdentifierList ':' Type

   ProcedureDeclaration       ::= ProcedureHeading ';' Block

   ProcedureDeclaration       ::= ProcedureHeading ';' 'directive'

   FunctionDeclaration        ::= FunctionHeading ';' Block

   FunctionDeclaration        ::= FunctionHeading ';' 'directive'

   ProcedureHeading           ::= ProcedureRwd <identifier>
                                  '(' FormalParameterList ')'
                              |   ProcedureRwd <identifier> 

   ProcedureRwd               ::= 'procedure' 

   FunctionHeading            ::= FunctionRwd <identifier>
                                  '(' FormalParameterList ')' ':' ResultType
                              |   FunctionRwd <identifier> ':' ResultType

   FunctionRwd                ::= 'function' 

   FormalParameterList        ::= FormalParameterList ';'
                                  FormalParameterSection
                              |   FormalParameterSection

   FormalParameterSection     ::= ValueParameterSpecification
                              |   VariableParameterSpecification

   ValueParameterSpecification
                              ::= IdentifierList ':' Type

   VariableParameterSpecification
                              ::= 'var' IdentifierList ':' Type

   CompoundStatement          ::= 'begin' StatementSequence 'end'

   StatementSequence          ::= StatementSequence ';' Statement
                              |   Statement

   Statement                  ::= SimpleStatement
                              |   StructuredStatement

   SimpleStatement            ::= EmptyStatement
                              |   AssignmentStatement
                              |   ProcedureStatement

   StructuredStatement        ::= CompoundStatement
                              |   CaseStatement
                              |   'if' Expression 'then'
                                     RestrictedStatement
                                  'else'
                                     Statement
                              |   'if' Expression 'then' Statement
                              |   'while' Expression 'do' Statement
                              |   'for' <identifier> ':=' Expression
                                  'to' Expression 'do' Statement
                              |   'for' <identifier> ':=' Expression
                                  'downto' Expression 'do' Statement

   RestrictedStatement        ::= SimpleStatement
                              |   CompoundStatement
                              |   CaseStatement
                              |   'if' Expression 'then'
                                     RestrictedStatement
                                  'else'
                                     RestrictedStatement
                              |   'while' Expression 'do' RestrictedStatement
                              |   'for' <identifier> ':=' Expression
                                  'to' Expression 'do' RestrictedStatement

   EmptyStatement             ::= empty

   AssignmentStatement        ::= Variable ':=' Expression

   ProcedureStatement         ::= <identifier>
                                  '(' ActualOrWriteParameterList ')'
                              |   <identifier>

   CaseStatement              ::= 'case' Expression 'of' CaseList 'end'
                              |   'case' Expression 'of' CaseList ';' 'end'
                              |   'case' Expression 'of' CaseList ';'
                                  'otherwise' Statement OptionalSemiColon 'end'

   CaseList                   ::= CaseList ';' Case
                              |   Case

   Case                       ::= ConstantList ':' Statement

   ConstantList               ::= ConstantList ',' Constant
                              |   Constant

   Type                       ::= SimpleType
                              |   StructuredType

   SimpleType                 ::= OrdinalType

   StructuredType             ::= OptionalPacked UnpackedStructuredType

   OptionalPacked             ::= 'packed'
                              |   empty

   OrdinalType                ::= EnumeratedType
                              |   SubrangeType
                              |   <identifier>

   UnpackedStructuredType     ::= ArrayType
                              |   RecordType
                              |   FileType

   EnumeratedType             ::= '(' IdentifierList ')'

   SubrangeType               ::= Constant '..' Constant

   ArrayType                  ::= 'array' '[' OrdinalType ']' 'of' Type

   RecordType                 ::= 'record' FieldList 'end'

   FileType                   ::= 'file' 'of' Type

   ResultType                 ::= <identifier>

   FieldList                  ::= FixedPart OptionalSemiColon
                              |   FixedPart ';' VariantPart
                              |   VariantPart
                              |   empty

   FixedPart                  ::= FixedPart ';' RecordSection
                              |   RecordSection

   VariantPart                ::= 'case' VariantSelector 'of' VariantList
                                  OptionalSemiColon

   VariantList                ::= VariantList ';' Variant
                              |   Variant

   RecordSection              ::= IdentifierList ':' Type

   VariantSelector            ::= <identifier> ':' Type
                              |   Type

   Variant                    ::= Constant ':' '(' FieldList ')' 

   Constant                   ::= OptionalSign UnsignedNumber
                              |   '+' <identifier>
                              |   '-' <identifier>
                              |   <identifier>
                              |   <string>

   ExpressionList             ::= ExpressionList ',' Expression 
                              |   Expression

   Expression                 ::= SimpleExpression '=' SimpleExpression
                              |   SimpleExpression '<>' SimpleExpression
                              |   SimpleExpression '<' SimpleExpression
                              |   SimpleExpression '<=' SimpleExpression
                              |   SimpleExpression '>' SimpleExpression
                              |   SimpleExpression '>=' SimpleExpression
                              |   SimpleExpression

   SimpleExpression           ::= SimpleExpression '+' Term
                              |   SimpleExpression '-' Term
                              |   SimpleExpression 'or' Term
                              |   Term

   Term                       ::= Term '*' Factor
                              |   Term '/' Factor
                              |   Term 'and' Factor
                              |   Term 'mod' Factor
                              |   Term 'div' Factor
                              |   Factor

   Factor                     ::= UnsignedConstant

   Factor                     ::= Variable
                              |   FunctionReference 
                              |   'not' Factor
                              |   '(' Expression ')'
                              |   Sign Factor

   UnsignedConstant           ::= UnsignedNumber 
                              |   <string>

   FunctionReference          ::= <identifier> 
                                  '(' ActualOrWriteParameterList ')'

   Variable                   ::= <identifier>

   Variable                   ::= IndexedVariable
                              |   FieldDesignator

   IndexedVariable            ::= Variable '[' ExpressionList ']'

   FieldDesignator            ::= Variable '.' <identifier>

   ActualOrWriteParameterList ::= ActualOrWriteParameterList ','
                                  ActualOrWriteParameter
                              |   ActualOrWriteParameter

   ActualOrWriteParameter     ::= Expression
                              |   Expression ':' Expression

   UnsignedNumber             ::= <integer>

   IdentifierList             ::= IdentifierList ',' <identifier>
                              |   <identifier>

   Sign                       ::= '+'
                              |   '-'

   OptionalSign               ::= Sign
                              |   empty

   OptionalSemiColon          ::= ';'
                              |   empty
)!";

const string source = R"!(
(*
 *  Ripley-Druseikis test suite.
 *)

program p(input,output);
begin
    if not leftpush(listdata[p] then writeln(' queue overflow.');
end.
 
program p(input,output);
begin
    if(link[i]=0)and(rightpop(x) then i:=x;
end.
 
program p(input,output);
begin
    if count[listdata[sub] := 0 then
       begin
           f := listdata[sub];
        end;
end.
 
program hunter\input,output'?
    var q:integer;
begin
end.
 
program p(input,output);
    var  i,prime,check,numb:real,a:array\1..6' of real?
begin
end.
 
program p(input,output);
begin
    for i  1 to 6 do x:=1
end.
 
program p(input,output);
begin
    check: 1?
    begin
       while check)' prime do x:=1
    end
end.

program p(input,output);
begin
    if prime check then x:=1
end.
 
program p(input,output);
begin
    writeln (' variable = ',n;
end.
 
program hunter[input,output];
    var q:integer;
begin
end.
 
program p(input,output);
    var i:real;
    const  a[1] 10;a[2] 15;a[3] 25;a[4] 3;?a[5] 50;a[6] 75;
begin
end.
 
program p(input,output);
begin
    if prime/check trunc(prime/check) then x:=1
end.
 
program p(input,output);
begin
    writeln(no. of primes less than@,a[i],@ is @,numb);
end.
 
program csc2201 (input, output);
    const pi := 3.14159;
    var q:integer;
begin
end.
 
program p(input,output);
    i := b;
    while i > 0 do x:=1
end.
 
program p(input,output);
begin
    xfact := if x = 0 then 1.0 else x * xfact (x - 1);
end.
 
program p(input,output);
begin
    repeat  (* until loop is found *)
        then
        x:=1
    until x=y ;
    x

program p(input,output);
    function power(m,n: integer): integer;
    begin
       if n<= then power := else power := m*power(m,m-1)
    end;
begin
end.
 
program p(input,output);
    begin
       if x<=0 then fact := 1 ense fact := x*fact(x-1)
    end;
begin
end.
 
program p(input,output);
    const  pi]= 3.14529;
    var q:integer;
begin
end.
 
program p(input,output);
begin
    f;=if x=0 then 1 else s * f(x-1);
end.
 
program p(input,output);
begin
    begin
       pfact]=m'x * exp(-m) / f
    end;
end.
 
program p(input,output);
begin
    if x=1 then
       x:=1
    else
       begin
           x:=1
       end
    else ;
end.
 
program p(input,output);
    var nodata, norels, noreturned, nosorted,
       sparedata, spareptr: integer
    function topsort (countlink: order; var sortvector: sorted;
           x:integer);
    begin
    end;
begin
end.
 
program reid(input,output);
    const a[1]=10;a[2]=15;a[3]=25;a[4]==35;a[5]=50;a[6]=75;
    var q:integer;
begin
end.

program p(input,output);
begin
    hs:=sqrt(2*pi*x)*x**x*exp(-x)
end.
 
program p(input,output);
begin
    repeat
       x:=1
    until-[sqrt(i);
end.
 
program p(input,output);
    procedure pr;
    begin
       x:=1;
       begin
           x:=1
       end ;
    var  fact, fact2: real ;
begin
end.
 
program p(input,output);
    function factorial( var: x: integer): integer]
       var q:integer;
    begin
    end;
begin
end.
 
program p(input,output);
begin
    begin
        write(x,m,pfact(fact,x,m),pstirl(stirl,x,m),pfact-pstirl)
end.
 
program p(input,output);
begin
    begin
       count := 0;
       go to 2
    end;
end.
 
program p(input,output);
begin
    while i<: n do x:=1
end.
 
program p(input,output);
    var x,m,i integer; fx,sx,lx real;
begin
    x:=1
end.
 
program p(input,output);
begin
    label 1;
    read(x);
    x:=1
end.
 
program p(input,output);
begin
99  prcount := prcount;
    x:=1
end.
 
program p(input,output);
    procedure pr;
    begin
        x:=1
    end ;
    var  fact, fact2: real ;
begin
    x:=1
end.
 
program p(input,output);
begin
    fact := fact*fact2*exp(-number) ;
    write ('stirling approximation =' , fact) ;
x:=1
end.
 
program p(input,output);
begin
    begin
       x:=1
    end ;
    procedure  stirling ;
    begin
       x:=1
    end;
x:=1
end.
 
program p(input,output);
    var x,m,i,p,j : integer;
    fx,sx,lx : real;
1 : begin;
    x:=1
end.
 
program p(input,output);
begin
    arr :=[2..n];
    ko nt:= 0;
    next :=2;
    x:=1
end.
 
program p(input,output);
    procedure factorial (a);
        var q:integer;
    begin
       x:=1
    end;
begin
end.
 
program p(input,output);
    type word = array (1..10) of char;
    var q:integer;
begin
    x:=1
end.
 
program p(input,output);
begin
    readln ( list_i?);
    x:=1
end.
 
progeam lab2(input,output);
    var q:integer;
begin
    x:=1
end.
 
program p(input,output);
    procedure x;
    begin
       x:=1
    end.
begin
    m := 0;
    null := "          ";
end.
 
program p(input,output);
begin
    read(letter);
1:  if letter<>'.' and letter<>' ' then
        x:=1
end.
 
program p(input,output);
    var label, loc, index, rsize: integer;
begin
    x:=1
end.
 
program p(input,output);
begin
    if list[index] < list[loc] ;
    then
        x:=1
end.
 
program p(input,output);
begin
    pack(buf,1,a);
    list(t):= a;
    x:=1
end.
 
program p(input,output);
    var key,record : array[1..limit] if akfa;
begin
    x:=1
end.
 
program p(input,output);
    procedure x;
    begin
        writeln ('   p(x,m) = ', w2 , '   for x stirling.') ;
    procedure factr(n: integer  ; var factor : integer ) ;
    begin
       x:=1
    end;
begin
    x:=1
end.
 
program p(input,output);
begin;
    procedure factr(n: integer  ; var factor : integer ) ;
    begin
        x:=1
    end ;
    x:=1
end.
 
program sort (input,output);
    const limit=100;
    limitp1=limit+1;
    var q:integer;
begin
    x:=1
end.
 
program alphaorder(input,output);
    var word:array[1..10] of char; a:alpha; pack(word,1,a);
begin
    x:=1
end.

program p(input,output);
begin repeat
    writeln(' ------------------');
    untill eof(input);
         x:=1
end.
 
program p(input,output);
begin
    ch[1]:=letter;
    for j:=2 to 10 do ch[j]'=' ';
       x:=1
end.
 
program p(input,output);
begin
    hi(h):=word;
    if h<=1 then
       begin
           writeln(?error sort?,cntr,h,l,);
           goto 1; *abnrm*
        end ;
    x:=1
end.
 
program p(input,output);
    begin
       writeln('  ',t,'   ',list[t]);
    end;
end.
 
program p(input,output);
    type alfa = packed array [1-10] of char;
    var q:integer;
begin
    x:=1
end.
 
program p(input,output);
  matrixknown (name:char, lower: boolean, var pointer: integer):boolean;
    var q:integer;
begin
    x:=1
end.

program p(input,output);
    procedure factorial (x:integer;var fact:integer):integer;
        var q:integer;
    begin
    end;
begin
end.
 
program p(input,output);
begin
    if x:= 0 then x:=1
end.
 
program p(input,output);
begin
    if x=0 then
       fact = 1
    else
       begin
            x:=1
       end
end.
)!";

int main() 
{

    Parser parser;
    try
    {

        parser.generate(grammar, map<string, int>(),
                        static_cast<DebugType>(0)
                        | DebugType::DebugProgress
                       );

        parser.parse(source, 
                     static_cast<DebugType>(0)
                     | DebugType::DebugProgress
                    );

    }
    catch (GrammarError& e)
    {
        cout << "Grammar errors:" << endl;
        parser.dump_source(grammar, cout);
    }
    catch (SourceError& e)
    {
        cout << "Source errors:" << endl;
        parser.dump_source(source, cout);
    }
    catch (exception& e)
    {
        cout << "Exception: " << e.what() << endl;
    }
        
}

