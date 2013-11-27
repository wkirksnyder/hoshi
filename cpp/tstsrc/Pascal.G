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

