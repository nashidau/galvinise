Galvinise
=========

Galvinise is a tin-plating, I mean templating library.

[![Build Status](https://travis-ci.org/nashidau/galvinise.svg?branch=master)](https://travis-ci.org/nashidau/galvinise)

Using
=====

Given a file foo.gvz, running 'galv foo.gvz' produces 'foo'.  Any other extensions are not replaced.


Inline Values
=============

{{ some lua code }} evaluates the lua code.  It produces no output by
default.  If it returns a value, that value is coerced to a string, and
printed in the output.

$foo prints the value of foo.  (See predefined values below).
If you want to call a function, you can do $foo(arg1, arg2, et, al).  The
function invocation goes until it reaches the matching closing ).

{-{ }-} is a comment.  No output will be produced.

{{include "file"}} includes a file.

A $ followed by whitespace or another $ produces a $.

{{ O("String format %d", 7) }} Outputs the formatted string.

{{ Oraw("x", "y", "z") }} Outputs xyz.

Predefined Values
=================

'$GALVINISE' is defined to be 'Galvinise <version>'.

'$GALVINISE_VERSION' returns the the version number.

'$PANTS' returns the current pants state.

Using Onion
===========

If you want to build with onion support use:
```
  USE_ONION=y make
```

Future Work
===========

   * Should foo.gvz.html produce foo.html?  Yes probably.
