hoshi
=====

Hoshi parser generator library. This isn't ready to share yet but in
case someone stumbles across this project I want to provide a
description of what it will do, what works now and the next steps.
Hopefully this will at least help you decide whether to check back
later.

Hoshi is a parser generator implemented as a native code library in
C++11. Most parser generators are implemented as code generators and
there are a number of advantages to that, but implementing as a
library has some advantages as well. 

The most important features are:

 - The parsing technology is LALR(k). One can get into almost
   religious discussions about the best parsing technology, but IMHO
   LR variants allow the most natural rules and make tree building the
   simplest.

 - The rules are given in BNF with a few extensions (grouping, '*',
   '?' and '+'). There is also a special syntax for operator
   precedence. All these extensions are just syntactic sugar and are
   translated into pure BNF internally at an early stage.

 - The scanner is a relatively simple DFA described in regular
   expressions. By providing a some default rules for tokens
   (anything in single quotes is its own regular expression) and a
   library of common symbols very little must be specified for a
   typical scanner.

 - Error recovery is fully automatic using an enhanced forward move
   technique. Relatively few spurious errors are reported no matter
   how dense errors are found in the source.

 - There is a notation for AST creation attached to each rule. If no
   specification is provided a concrete tree is returned but it's a
   simple matter to describe the AST desired.

 - There is a guard mechanism to allow some feedback from the parser
   to the scanner or even better, from a previous scanned token.

Everything described up to this point is working now in the source.
The next steps are:

 - Create wrappers in python, java and C# that call the native code
   library allowing the use of hoshi in those languages as well.

 - Write documentation. There are a lot of features here and it's
   unusable without documentation. But I want to get the multiple
   language support working first.

Once I get this far here are some things I may or may not do:

 - Extend the parsing technology to LR(k). This is a bit more
   powerful but I haven't convinced myself it's worth the effort. 

 - Make the parsers generated more efficient by removing LR(0) reduce
   states, or more likely removing chains of unit production. Again,
   I'm not convinced it's worth the effort because the parsers are
   pretty small and fast now.

As I write this I'm thinking I should be at the end of the
documentation stage by March of 2014. In the meantime you can take a
peek at DateTime.cpp in the tstsrc library to see a small sample and
get a bit more of an idea how this will be used.

