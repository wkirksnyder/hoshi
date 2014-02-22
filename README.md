hoshi
=====

Hoshi parser generator library. It's in pretty good shape now. I want
to provide a description of what it will do, what works now, and the
next steps. Hopefully this will help you decide whether to check back
later.

Hoshi is a parser generator implemented as a native code library in
C++11. Most parser generators are implemented as code generators and
there are a number of advantages to that, but implementing as a
library has some significant advantages as well. 

The most important features are:

 - The parsing methodology is LALR(k). One can get into almost
   religious discussions about the best parsing methodology, but IMHO
   LR variants allow the most natural rules and simplify tree
   building.

 - The rules are given in BNF with a few extensions (grouping, '|',
   '*', '?' and '+'). There is also a special syntax for operator
   precedence. All these extensions are just syntactic sugar
   translated into pure BNF internally.

 - The scanner is a DFA specified by regular expressions. By
   providing default rules for tokens (anything in single quotes
   is its own regular expression) and a library of common tokens
   very little must be specified for a typical scanner.

 - Error recovery is fully automatic using an enhanced forward move
   algorithm. Relatively few spurious errors are reported no matter
   how densely errors occur in the source.

 - There is a simple notation for AST creation attached to each rule.
   If no specification is provided a concrete tree is returned but
   it's a simple matter to describe the AST desired.

 - There is a guard mechanism to allow feedback from the parser
   to the scanner or from a previously scanned token.

 - The native library can be `wrapped' by small modules in other
   languages to allow the library to be used by those other languages.
   Those wrappers have been created in Java, C# and python so far.

 - There is a web page of documentation.

Everything up to this point works. The documentation could certainly
be better, but it's probably usable for an adventurous user.

The weakest part right now is some sort of distribution system but I
need to think about that a bit and maybe do some research. Github
seems to be designed for collaboration, not distribution. I need to
create pre-compiled versions of the source on several platforms and
host it somewhere. It's not really acceptable that someone interested
in using Hoshi in Java needs a C++ compiler to build it.

