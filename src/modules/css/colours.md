Colours
=======

The CSS Colours module for Galvinise provides a set of helpers to create and
maintain colour themes and styles in CSS.

Creation
--------

*rgb(_r_, _g_, _b_)*
*rgba(_r_, _g_, _b_, _a_)*
	Create a new colour object with the specified colour values.
	The r,g,b values indicate red, green, blue values respectively and must
	be between 0 and 255.  The a value represents alpha and must be between
	0 and 1.  Invalid or missing a values will be treated as 1.

*rgb(_x_)*
	Creates a greyscale colour with initial r, g & b values equal to x.  a is 1.

*rgb(_x_, _a_)*
	Creates a greyscale colour with initial r, g & b values equal to x.  a is 1.
