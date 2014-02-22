#line 2 "u:\\hoshi\\raw\\CodeGenerator.cpp"
//
//  OpcodeType                                          
//  ----------                                          
//                                                      
//  Enum of the various opcodes in the virutal machine. 
//

#ifndef OPCODE_TYPE_H
#define OPCODE_TYPE_H

//
//  Namespace hoshi: Not indenting...
//

namespace hoshi 
{

enum OpcodeType : int
{
    OpcodeMinimum            =   0,
    OpcodeMaximum            =  37,
    OpcodeNull               =   0,
    OpcodeHalt               =   1,
    OpcodeLabel              =   2,
    OpcodeCall               =   3,
    OpcodeScanStart          =   4,
    OpcodeScanChar           =   5,
    OpcodeScanAccept         =   6,
    OpcodeScanToken          =   7,
    OpcodeScanError          =   8,
    OpcodeAstStart           =   9,
    OpcodeAstFinish          =  10,
    OpcodeAstNew             =  11,
    OpcodeAstForm            =  12,
    OpcodeAstLoad            =  13,
    OpcodeAstIndex           =  14,
    OpcodeAstChild           =  15,
    OpcodeAstChildSlice      =  16,
    OpcodeAstKind            =  17,
    OpcodeAstKindNum         =  18,
    OpcodeAstLocation        =  19,
    OpcodeAstLocationNum     =  20,
    OpcodeAstLexeme          =  21,
    OpcodeAstLexemeString    =  22,
    OpcodeAssign             =  23,
    OpcodeDumpStack          =  24,
    OpcodeAdd                =  25,
    OpcodeSubtract           =  26,
    OpcodeMultiply           =  27,
    OpcodeDivide             =  28,
    OpcodeUnaryMinus         =  29,
    OpcodeReturn             =  30,
    OpcodeBranch             =  31,
    OpcodeBranchEqual        =  32,
    OpcodeBranchNotEqual     =  33,
    OpcodeBranchLessThan     =  34,
    OpcodeBranchLessEqual    =  35,
    OpcodeBranchGreaterThan  =  36,
    OpcodeBranchGreaterEqual =  37
#line 50 "u:\\hoshi\\raw\\CodeGenerator.cpp"
};

} // namespace hoshi

#endif // OPCODE_TYPE_H
