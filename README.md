hoshi
=====

Hoshi parser generator library. This isn't ready to share yet but in
case someone stumbles across it I want to provide a description of
what it will do, what works now, and the next steps. Hopefully this
will help you decide whether to check back later.

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

Everything up to this point is done and working. The next steps are:

 - Write documentation. There are a lot of features here and it's
   unusable without documentation.

Once I get this far here are some things I may or may not do:

 - Extend the parsing methodology to LR(k). This is a bit more
   powerful than LALR(k) but I haven't convinced myself it's worth
   the effort. I haven't encountered any grammars that are LR(k)
   but are not LALR(k).

 - Make the parsers generated more efficient by removing LR(0) reduce
   states or removing chains of unit productions. Again, I'm not
   convinced it's worth the effort because the parsers are pretty
   small and fast now. A lot of the literature around parsing assumes
   much older technology where efficiency was more important than it
   is today.

 - More client interfaces. I'm not sure what else would be useful
   here. Although I don't know much about them, I think the Java
   library will cover the other jvm languages and C# will cover the
   other clr languages. C++ seems to be nearly the last man standing
   among the languages that generate native code. That leaves other
   scripting languages. 

As I write this I'm thinking I should be at the end of the
documentation stage by March of 2014. In the meantime you can see some
samples in the tstsrc directory. There are also more complex grammars
in the libsrc directory. Hoshi was used to implement itself through
bootstrapping, so you can find grammars for the overall grammar
syntax, for the regular expression syntax and for the character set
syntax in the libsrc directory.

I find the DateTime example interesting. It's not a very complex
grammar, and in fact it could be implemented with an elaborate regular
expression. It's simpler and clearer as a grammar, though. I'm
interested in how many other small applications of parsing technology
I'll find when I make use of it simple enough.

