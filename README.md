Galvinise
=========

Galvinise is a tin-plating, I mean templating library.

Using
=====

Given a file foo.gvz, running 'galv foo.gvz' produces 'foo'.  Any other extensions are not replaced.


Inline Values
=============

{{ some lua code }} evaluaates the lua code.  It produces no output by
default.  If it returns a value, that value is coerced to a string, and
printed in the output.

$foo prints the value of foo.  (See predefined values below).
If you want to call a function, you can do $foo(arg1, arg2, et, al).  The
function invocation goes until it reaches the matching closing ).

{{include "file"}} includes a file.

A $ followed by whitespace produces a $.


Predefined Values
=================

'$GALVINISE' is defined to be 'Galvinise <version>'.

'$GALVINISE_VERSION' returns the the version number.

'$PANTS' returns the current pants state.

Future Work
===========

   * hould foo.gvz.html produce foo.html?  Yes probably.
   * How to print a $ value which is not code.
