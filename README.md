# INTRODUCTION #

The **eval** package provides a dynamic infix expression evaluator that can be used within C/C++ programs. It has been designed with the following goals in mind :

- *Ubiquity* : provides a means to easily evaluate expressions wherever your application needs it. 
- *Simplicity of use* : a single function, **evaluate()**, is the main entry point for evaluating an expression supplied as a string, such as :
 
		2 + ( log(127) / sqrt (5) ) 
- *Simplicity of build* : just compile the **eval.c** source file, and include **eval.h** in your source files wherever it is needed.
- *Extensibility* :
	- Core constants and functions are already integrated into the **eval** package, but you can add your own ones if you need to
	- User-variables can be specified in expressions ; you will need to specify a callback function to process their references
- *Compactness* : the **eval** package has been designed to use memory allocation whenever possible to minimize load-time memory footprint

# EXPRESSION EVALUATOR EXAMPLE #
maybe the simplest *main.c* program using the **eval** package could be :

	# include	<stdio.h>
	# include	"path/to/eval.h"

	void  main ( int  argc, char **  argv )
	   {
			double 		result ;

			if  ( argc  >  1 )
			   {
					if  ( evaluate ( argv [1], & result ) )
						printf ( "Result = %g\n", result ) ;
					else
						evaluate_perror ( ) ;
			    }
			else 
				fprintf ( stderr, "Missing expression as argument#1.\n" ) ;
	    }

then compile and execute it using :
	
	cc main.c eval.c
	./a.out 2+3

which will produce :

	5


# EXPRESSION SYNTAX #

Expressions can contain the following elements :

- Numbers
- Operators
- Parentheses for grouping expressions
- Constant names
- Function calls  
- References to registers (just like a programmable calculator would have)
- References to variables

All expressions manipulate floating-point values (doubles) and return a floating-point value.

An expression can be as simple as :

	1 + 2

or as complex as :

	( 1 + log ( $X1 ) * ( 1 / $X2 ) ) #!  + log (3) * #? + ( PI * #? )

Note that spaces are not significant.
> Whenever a letter appears in an expression, it is treated as case-insensitive data. Thus, the constant "pi" is equivalent to "PI", as well as the floating-point value 1e10 is equivalent to 1E10. 

The following paragraphs further describe the proper syntax of each element that can appear in an expression.


## NUMBERS ##

Numbers can be specified as :

- Sequences of digits : 1988
- Floating-point values : 3.14159, 3E10
- Integer values with a base specifier :
	- Binary : 0b1001 (= 9)
	- Octal : 0o377 or 0377 (= 255). Note that a number starting with zero and not followed by the 'o' (or 'O') character is considered to be expressed in the octal base only if it does not contain characters specific to floating-point values ("." or "e"). Thus, 0377.1 will be considered as 377.1 (and not 255 followed by .1) and 0377E1 will be considered as 03770.
	- Decimal : decimal values can be entered as is (without a leading zero) or using the base specifier "0d" ; for example, 0d0377 is 377, not 255.
	- Hexadecimal : hexadecimal values use the "0x" base specifier, such as in "0xFF".

## OPERATORS ##

Operators allow to apply a function on one or more operands ; they have two important properties :

- Whether they are unary or binary ; for example, the "~" operator (bitwise not of the operand located to its right)  is unary, as well as the "+" and "-" signs are when located in front of numbers ; but note that "+" and "-" are also binary operators, since they are used for addition and subtraction.
- Associativity : operators can be left- or right-associative.
 
### n-ary PROPERTY ###

Operators apply on a certain amount of operands.

Binary operators apply a function on the two operands that are located to their left and to their right ; for example :

	1 + 2

performs the addition of the numbers "1" and "2".

Unary operators can be infix (right-associative) :

	+2
	-37
	~0

(the first two examples give the sign of the value that follows them, while the "~" operator computes the bitwise NOT of the value following it).

But operators can also be postfix (left-associative) ; this is the case of the factorial operator "!" :

	3!		-> 6
	4!		-> 24
	5! 		-> 120
	6!		-> 720
	etc.

Function calls are considered as n-ary operators, *n* being the number of function parameters.


### ASSOCIATIVITY ###

Operator associativity helps resolve expression evaluation when potential ambiguities may arise and no grouping parentheses are specified. During this resolution process, the priorities of all the operators involved is taken into account.

Left associativity means that the following expression :

	2 + 3 * 4

will be reinterpreted as :

	2 + ( 3 * 4 )

Both the "+" and "\*" operators are left associative ; however, since "\*" has a greater priority than "+", it will be applied to its nearest operands, 3 and 4.

Another example is :

	2 * 3 + 4

which is rewritten as :
	
	( 2 * 3 ) + 4

Again, since "*" has a greater priority than "+", it will be applied to 2 and 3, and "+" will be applied to the result of "2\*3" then 4.

In the current implementation, binary right associative operators act as operators having much more priority than left associative operators ; this is the case for example of the exponentiation operator, "**" :

	2 * 3 ** 4

will give 162 as a result, while :

	2 * 3 ** 4 + 1

will give 163.

Unary operators apply the same way, whether they are left- or right-associative. The only difference is syntactic : right-associative unary operators appear BEFORE their operand, while left-associative operators appear AFTER it.

You can change associativity rules by enclosing parts of the expression with parentheses ; eg, if you want a result of 14 for the following expression :

	2 * 3 + 4 

just rewrite it this way :

	2 * ( 3 + 4 )

### OPERATOR LIST ###

The following binary operators are defined for mathematical operations :

- "+" 	:  Addition of two values
- "-" 	:  Subtraction of two values
- "/"	:  Division of one value by another 
- "\"	:  Integer division of one value by another 
- "**"	:  Exponentation. This is a right-associative operator.
- "%" 	:  Modulo

The following operators apply bitwise operations, after their operands have been converted to unsigned integers :

- "&"	:  Bitwise AND.
- "|" 	:  Bitwise OR.
- "^" 	:  Bitwise XOR.
- "~"	:  Bitwise NOT (see unary operators below).
- "<<"	:  Left-shifts a value by the specified number of bits.
- ">>"	:  Right-shifts (unsigned) a value by the specified number of bits.

The following unary operators are defined :

- "+"	:  Unary plus, such as : +4
- "-" 	:  Unary minus
- "~"	:  Bitwise NOT : ~0 will produce 0xFFFFFFFF.
- "!"	:  Factorial. This operator is left-associative, so it must occur AFTER its operand.

## CONSTANTS ##

The evaluator provides with a set of predefined constants. Constants are identified by their name, such as :

	2 * PI

Constant names are made of letters, digits and the underline sign, starting with a letter or an underline. They are case-insensitive.

Below is the list of predefined constants :

- *DBLMAX* : Max floating-point value 
- *DBLMIN* : Min floating-point value
- *E*   	: The value of E (2.71828182845904523536)
- *E_PI* : E^PI
- *INTMAX* : Max integer value
- *INTMIN* : Min integer value
- *LN2*	: ln(2)
- *LN10*	: ln(10)
- *LOG2E* : log2(e)
- *LOG10E*	: log10(e)
- *ONE\_PI*	: 1/PI
- *ONE\_SQRT\_2* : 1/sqrt(2)
- *PI*		: The value of PI (3.14159265358979323846)
- *PI\_2* 	: PI/2
- *PI\_4*	: PI/4
- *PI_E* : PI^E
- *PHI* : The Golden Ratio
- *SQRT\_2* : sqrt(2)
- *TWO\_PI* : 2/PI
- *TWO\_SQRT\_PI* : 2/sqrt(PI)
 
You can call the **evaluator\_register\_constants()** function for adding more constants before evaluating expressions (see the **API** section).

## FUNCTION CALLS ##

The evaluator provides with a set of predefined functions. Function calls are specified by a function name followed by optional parameters enclosed in parentheses. A comma is used to separate each function parameter. Note that even function calls without arguments must be followed with parentheses.

Some examples :

	2 * myfunc_with_no_param ( )
	2 * myfunc_with_1_param ( 17 )
	2 * myfunc_with_several_params ( 2, 3, 4, 5 )

Function names are made of letters, digits and the underline sign, starting with a letter or an underline. They are case-insensitive.

Function calls can be nested, for example :

	abs ( cos ( 2 + ( 3 * 4 ) ) )

There is a limit of 64 nesting levels authorized in a function call. Note that parentheses used for grouping expression parts, such as "(3*4)" in the above example, do not count as a supplementary nesting level. 

Below is the list of predefined functions :

- *abs(x)* : Computes the absolute value of *x*.
- *acos(x)* : Arc cosine of *x*.
- *arr(n,p)* : Number of (ordered) arrangements of *p* values within *n*.
- *asin(x)* : Arc sine of *x*.
- *atan(x)* : Arc tangent of *x*.
- *atan2(y,x)* : Arc tangent of *y*/*x*.
- *avg(v1, [,..., vn])* : Average of the specified values.
- *ceil(x)* : Nearest integer value greater than or equal to *x*.
- *comb(n,p)* : Number of (unordered) combinations of *p* numbers within *n*.
- *cos(x)* : Cosine of *x*
- *cosh(x)* : Hyperbolic cosine of *x*
- *delta(a,b,c,sol)* : Computes the solutions of ax^2 + bx +c. *sol* is 0 for the first solution, and non-zero for the second (if defined).
- *dev(v1 [, ..., vn])* : Standard deviation of the specified values.
- *dist(x1,y1,x2,y2)* : Distance between points (*x1,y1)* and (*x2,y2*)
- *exp(x)* : Computes e^*x*     
- *fib(x)* : Fibonacci value for order *x*.
- *floor(x)* : Nearest integer value less than or equal to *x*.
- *log(x)* : Natural logarithm of *x*.
- *log2(x)* : Base 2 logarithm of *x*.
- *log10(x)* : Base 10 logarithm of *x*.
- *sigma(low,high[,step])* : Sum of all values between *low* and *high*, using the optional *step*, which defaults to 1.
- *sin(x)* : Sine of *x*.
- *sinh(x)* : Hyperbolic sine of *x*.
- *slope(x1,y1,x2,y2)* : Slope of a line traversing points (*x1,y1*), (*x2,y2*).
- *sqrt(x)* : Square root of *x*.
- *tan(x)* : Tangent of *x*.
- *tanh(x)* : Hyperbolic tangent of *x*.
- *var(v1[,...,vn])* : Variance of the specified set of values. 

You can call the **evaluator\_register\_functions()** function for adding more functions before evaluating expressions (see the **API** section).
	
## REGISTERS ##

The evaluator mimics some features of programmable calculators. You can save intermediate results early within an expression and recall them back later.

Suppose that you have a very complicated expression *S* :

	( 2 + log(17) / ( sqrt (2) + sqrt(3) ** e ) )

And you have a formula which uses *S* several times :

	S * ( abs ( pow (S, 2.3) ) ) - ( S / 2 )

You could write :

	( 2 + log(17) / ( sqrt (2) + sqrt(3) ** e ) ) #! * ( abs ( pow ( #?, 2.3 ) ) ) - ( #? / 2 )

The **#!** construct saves the last subexpression (ie, the last value computed on the stack) in the first available register, while **#?** is replaced with the value of the last register that has been set.

Up to 64 registers are available ; their id, which ranges from 0 to 63, can be specified in the **#!** and **#?** constructs ; for example :

	( 2 + 3 ) #12!

will save the value 5 into register 12, while the following :

	#12? * 7

will recall the value of register 12 (5) and multiply it by 7.

Recalling the value of a register which has not been previously set will generate an error.

The **#!** construct (register save) can be specified anywhere in an expression ; however, **#?** (register recall) MUST be specified where a number, a constant or an expression result (including a function call) is expected.

## VARIABLES ##

Variables can be specified in an expression ; however, it is the responsibility of the caller to provide for their value, when needed. For that, you need to call the **evaluate_ex()** function and provide a callback, instead of calling **evaluate()** (otherwise variable references will be flagged as syntax errors).

A variable reference starts with a "$" sign followed by a name, such as in :

	$MYVAR * 2

It is the responsibility of the callback function supplied to **evaluate_ex()** to return the appropriate value for the invoked variable. In this case, case-insensitivity, if desired, must be handled by the callback.

# API #

The evaluator API is designed to be as simple as possible. As a *Getting started* primer, all you need to do to evaluate an expression within your C program is :

1. Include file "eval.h"
2. Call the **evaluate()** function

Of course, you will also need to compile **eval.c** (the third file of the package, *evalfuncs.c*, is included by *eval.c*). See the **COMPILING THE EVALUATOR** section later in this document.

## FUNCTIONS ##

### int evaluate ( const char *  expression, double *  value ) ###

Evaluates the specified *expression* and sets *value* to the evaluated expression result.

Returns 1 if evaluation was successful, or 0 if an error occured. In this case, the **evaluator\_errno** and **evaluator\_error** variables will be set to the error code and error message, respectively. You can use the **evaluator_perror()** function to display an error message on *stderr*.

### int evaluate_ex ( const char *  expression, double *  value, eval\_callback  callback ) ###

Evaluates the specified *expression* and sets *value* to the evaluated expression result.

This function behaves the same as the **evaluate()** function, except that it allows for variable references which are to be processed by the specified *callback*.

The callback function has the following definition :

	typedef int		( * eval_callback ) ( char *  vname, eval_double *  value ) ;

Typically, it must set *value* to the value of the variable name specified by *vname* and return the constant EVAL\_CALLBACK\_OK if the variable exists ; or return EVAL\_CALLBACK\_UNDEFINED if the variable does not exist.

### void  evaluator_perror ( ) ###

Prints on *stderr* the last error code and message generated by a call to **evaluate()** or **evaluate_ex()**.

### int evaluator\_register\_constants	( const evaluator\_constant\_definition * definitions ) ###

Registers new constants for the evaluator. Existing constants will be overriden if they have the same name.

The last definition of the *definitions* array must have all its fields set to zero, to signal the end of the array.

See the **STRUCTURES** section for a description of the *evaluator\_constant\_definition* structure.

Also have a look at **CUSTOMIZING THE EVALUATOR** section for a step-by-step guide on adding new constants.

### int evaluator\_register\_functions	( const evaluator\_function\_definition * definitions ) ###

Registers new functions for the evaluator. Existing functions will be overriden if they have the same name.

The last definition of the *definitions* array must have all its fields set to zero, to signal the end of the array.

See the **STRUCTURES** section for a description of the *evaluator\_function\_definition* structure.

Also have a look at **CUSTOMIZING THE EVALUATOR** section for a step-by-step guide on adding new functions.

### const evaluator\_constant\_definition * evaluator\_get\_registered\_constants ( ) ###

Returns a pointer to the array containing the evaluator constant definitions.

### const evaluator\_function\_definition * evaluator\_get\_registered\_function ( ) ###

Returns a pointer to the array containing the evaluator function definitions.
 
## STRUCTURES ##

### evaluator\_constant\_definition structure ###

Holds the definition of a constant :

	typedef struct  evaluator_constant_definition
	   {
			char *		name ;			// Constant name
			eval_double	value ;			// Constant value
	    }  evaluator_constant_definition ;

### evaluator\_function\_definition structure ###

Holds the definition of a function :

	# define	EVAL_FUNCTION(func)		eval_primitive_##func
	# define	EVAL_PRIMITIVE(func)	static eval_double  EVAL_FUNCTION ( func ) ( int  argc, eval_double *  argv )
		
	typedef eval_double	( * eval_function ) ( int  argc, eval_double *  argv ) ;	
	
	typedef struct  evaluator_function_definition
	   {
			char *			name ;			// Function name
			int				min_args ;		// Min arguments
			int				max_args ;		// Max arguments
			eval_function	func ;			// Pointer to function
	    }  evaluator_function_definition ;

Fields have the following meaning :

- *name* : the name of the function, that can be specified in an expression.
- *min_args, max_args* : Minimum and maximum number of function arguments.
- *func* : A pointer to the function performing the computation.


## VARIABLES ##

The variables listed below can be set to customize the behavior of the evaluator.

### evaluator\_use\_degrees ###

When zero, trigonometric functions such as sin(), cos(), etc. use radians.
When non-zero (the default), trigonometric functions use degrees.

## RETURN CODES ##

The evaluator tries to provide as precise information as possible whenever a syntax or runtime error is encountered. The **evalute()** and **evaluate\_ex()** always return 1 when an expression has been successfully evaluated, and zero if evaluation failed.

In this case, the **evaluator\_errno** variable will contain the error condition, and **evaluator\_error** the error message (all of that can be printed to *stderr* using the **evaluator\_perror()** function).

The following error codes are defined :

- **E\_EVAL\_OK** : The expression has been sucessfully parsed
- **E\_EVAL\_UNEXPECTED\_CHARACTER** :  An unexpected character has been found
- **E\_EVAL\_INVALID\_NUMBER** :  Invalid number specified
- **E\_EVAL\_UNEXPECTED\_TOKEN** :  A valid token has been encountered in an invalid place
- **E\_EVAL\_UNEXPECTED\_NUMBER** :  A valid number has been encountered in an invalid place
- **E\_EVAL\_UNEXPECTED\_OPERATOR** :  A valid operator has been encountered in an invalid place
- **E\_EVAL\_STACK\_EMPTY** :  Internal error : stack has not enough arguments to apply the next operator
- **E\_EVAL\_UNDEFINED\_OPERATOR** :  Internal error : an undefined operator has been found (this means that a new operator has been added, but the eval\_compute() function has not been modified to process it)
- **E\_EVAL\_UNDEFINED\_TOKEN\_TYPE** :  Internal error : an undefined token type has been found (this means that a new token type has been added, but the eval\_compute() function has not been modified to process it)
- **E\_EVAL\_UNBALANCED\_PARENTHESES** :  Unbalanced parentheses
- **E\_EVAL\_UNEXPECTED\_RIGHT\_PARENT** :  Unmatched closing parenthesis
- **E\_EVAL\_UNDEFINED\_CONSTANT** :  Undefined constant
- **E\_EVAL\_UNEXPECTED\_NAME** :  Unexpected constant
- **E\_EVAL\_IMPLEMENTATION\_ERROR** :  Implementation caused inconsistent result
- **E\_EVAL\_INVALID\_REGISTER\_INDEX** :  Register index out of range
- **E\_EVAL\_REGISTER\_NOT\_SET** :  No value has been assigned to the specified register
- **E\_EVAL\_UNDEFINED\_FUNCTION** :  Undefined function
- **E\_EVAL\_UNTERMINATED\_FUNCTION\_CALL** :  Function call with unbalanced parentheses
- **E\_EVAL\_TOO\_MANY\_NESTED\_CALLS** :  Too many nested function calls
- **E\_EVAL\_UNEXPECTED\_ARG\_SEPARATOR** :  Argument separator in the wrong place
- **E\_EVAL\_INVALID\_FUNCTION\_ARGC** :  Invalid number of arguments specified during a function call
- **E\_EVAL\_BAD\_ARGUMENT\_COUNT** :  Not enough or too much arguments remain on stack to process function call
- **E\_EVAL\_UNDEFINED\_VARIABLE** :  Undefined variable
- **E\_EVAL\_VARIABLES\_NOT\_ALLOWED** : You have been using the evaluate() function and variable references are not allowed. Use evaluate_ex() instead.
- **E\_EVAL\_UNEXPECTED\_VARIABLE** : Unexpected constant found.

Note that the **E\_EVAL\_UNEXPECTED\_\*** error codes indicates an item (constant, name, variable reference, etc.) that is authorized but has been found in the wrong place within the expression to be evaluated. 

# CUSTOMIZING THE EVALUATOR #

The following methods are available to "customize" the evaluator :

- Adding new constants
- Adding new functions
- Allowing references to variables by providing a callback function to the **evaluate_ex()** function

## Adding new constants ##

Adding new constants can be done using the **evaluator\_register\_constants()** function ; its argument must be a pointer to an array of **evaluator\_constant\_definition** structures, which must end with an entry whose fields are all zeroes (use the **EVAL\_NULL\_CONSTANT macro for that).

The following code adds the **MYCONST** constant to the list of predefined constants :

	EVAL_CONSTANT_DEF ( mydef )
		EVAL_CONSTANT ( "MYCONST", 1234 )
	EVAL_CONSTANT_END ;

	...
 
	evaluator_register_constants ( mydefs ) ;

Note that you do not need to put a comma after a constant definition.

After registration, the following expression :

	evaluate ( "1 + MYCONST" ) ;

will give the following result :

	1235

## Adding new functions ##

Adding new functions can be done using the **evaluator\_register\_functions()** function ; its argument must be a pointer to an array of **evaluator\_function\_definition** structures, which must end with an entry whose fields are all zeroes (use the **EVAL\_NULL\_FUNCTION macro for that).

It is recommended to use the EVAL\_PRIMITIVE and EVAL\_FUNCTION macros to declare a function, since they are designed for preserving existing name space.

To add a custom function called **myfunc**, go through the following steps (note that functions that are made available for evaluation are called *primitives*).

All primitives have the following signature :

	eval_double 	myprimitive ( int  argc, eval_double *  argv )

where *argc* is the number of arguments passed to your function during evaluation, and *argv* the arguments themselves, which are numeric values. The **EVAL\_PRIMITIVE** macro can be used as a shortcut :

	EVAL_PRIMITIVE ( myprimitive )

The function definition array which can now be passed to the **evaluator\_register\_functions()** will have the following shape :

	EVAL_FUNCTION_DEF ( myfuncs )
		EVAL_FUNCTION ( "myprimitive", 1, 1, myprimitive )
	EVAL_FUNCTION_END ;

Now you can register your function :

	evaluate_register_functions ( myfuncs ) ;

and use it in an expression :

	evaluate ( "1 + myfunc ( 2 )" ) ;

which will give *5* as a result.

## Allowing usage of variables ##

Variables are only authorized when you call the **evaluate_ex()** function, specifying a callback function.

You need first to declare you callback, which will be called whenever a variable reference is found in the supplied expression ; the callback is passed the variable name in the *vname* parameter, and must set its value in the value pointed to by the *output* parameter :

	int  my_callback ( char *  vname, eval_double *  output )
	   {
			if  ( ! stricmp ( vname, "MYVAR" ) )
			   {
					* output 	=  2.17 ;
					
					return ( EVAL_CALLBACK_OK ) ;
			    }
			else
					return ( EVAL_CALLBACK_UNDEFINED ) ;
	    }

It must return one of the following results :

- *EVAL\_CALLBACK\_OK* : The specified variable exists, and the *output* parameter has been set to the variable value
- *EVAL\_CALLBACK\_UNDEFINED* : The specified variable does not exist ; the *output* parameter value will be ignored.

# COMPILING THE EVALUATOR #

Some constants can be specified for the compilation of the **eval.c** source file. They are listed below.

## EVAL\_TRIG\_DEGREES ##

If defined and set to a non-zero value, trigonometric functions will use degrees instead of radians by default.

If defined and set to zero, trigonometric functions will use radians by default.

If undefined, trigonometric functions will use degrees.

## EVAL\_DEBUG ##

If defined and set to a non-zero value, debugging information will be displayed. 

## EVAL\_DENY\_EMPTY\_STRINGS ##

If defined and set to a non-zero value, the **evaluate()** function will fail if an empty string is specified. Otherwise, it will return the value 0.

## Memory allocation ##

If you have your own memory allocation routines with error handling, you can define the following macros :

- **eval_malloc ( size )** : allocate a memory block of *size* bytes.
- **eval_realloc ( p, size )** : Takes pointer *p* and reallocates it so that it can hold *size* bytes.
- **eval_strdup ( s )** : Allocates a memory block and copies string *s* into it.
- **eval_strndup ( s, size )** : Duplicates a substring of *s*, of *size* characters.
- **eval_free ( p )** : Frees memory pointed to by *p*. 

If one of these macros is not defined, an internal version will be used (these macros can be defined individually, so you do not need to define them all if you have custom allocation functions).

# IMPLEMENTATION #

This implementation is inspired from the Djikstra shunting-yard algorithm, with some modifications :

- Lexical analysis is performed by the **eval\_lex()** function.
- Parsing is performed by the **eval\_parse()** function. This is where the output stack is built, with the help of the operator stack. Keep in mind the following facts :
	-  Binary operators that are declared to be right-associative are not really right-associative : they simply have a greater precedence than left-associative operators (thus, you can forget the right associativity of the '=' operator found in the C language, for example)
	-  Special processing is performed for the unary plus and minus signs, since they could be interpreted as their binary counterparts
	-  Special processing is also performed for unary left-associative operators, such as "!" (factorial) : they are immediately pushed onto the output stack and do not go to the operator stack.
	-  Since there is a separation between lexical analysis and parsing, more error cases can be identified
-  Once the **eval\_parse()** function has completed its work, it calls the **eval\_compute()** function to interpret output stack elements, which have been reordered so that operator and function call precedences are consistent with the input expression. Note that the output stack has its elements ordered in reverse-polish interpretation.

If the **EVAL\_DEBUG** macro is set to 1, the following functions will be available for debugging purposes :

- **eval\_dump\_constants()** : Displays a list of defined constants
- **eval\_dump\_functions()** : Displays a list of defined functions
- **eval\_dump\_stack( eval\_stack *  stack, char *  title )** : Dumps the contents of the specified stack ; the **eval_parse()** function will display the contents of the output stack if the **EVAL\_DEBUG** macro is defined.

Here are a few examples of the output stack before evaluation :

	2 + 3 * 4  ->
		DOUBLE   : 2
		DOUBLE   : 3
		DOUBLE   : 4
		OPERATOR : *
		OPERATOR : +

	4! + 17 ->
		DOUBLE   : 4
		OPERATOR : !
		DOUBLE   : 17
		OPERATOR : +

	f ( 1, ( 3 + log ( 6 ) ), g ( 1+4 ) ) ->
		DOUBLE   : 1
		DOUBLE   : 3
		DOUBLE   : 6
		FUNCTION : log
		OPERATOR : +
		DOUBLE   : 1
		DOUBLE   : 4
		OPERATOR : +
		FUNCTION : g
		FUNCTION : f

# COMPILING THE EXAMPLE PROGRAM #

An example program, **main.c**, has been supplied to test the evaluator. What it does is :

- Register two constants, *TESTC1* and *TESTC2*, equal to 100 and 200 respectively.
- Register a function, *by2()* which multiplies its argument by 2
- Use the **evaluate_ex()** function to provide a callback for variable references. The *$TIME* variable is authorized and set to the current Unix time value
- Then loop on :
	- Read an expression 
	- Dump the output stack before processing it
	- Display the result


## COMPILING ON WINDOWS ##

Use the Visual Studio 2010 project file provided with this package, then choose the *Generate solution* item in the *Generate* menu... or simply type *F5* to compile and run.

## COMPILING WITH CYGWIN ##

This paragraph assumes that Cygwin is installed on your computer, and that its */bin*, */usr/bin* directories are in your path.

Compile the package using gcc, then run it (the following example shows the output of the program after entering the expression *2+(4\*log(100))*) :

	C:\Eval> gcc -DEVAL_DEBUG main.c eval.c -lm
	C:\Eval> a.exe
	Expression evaluator tester. Press Enter to exit.
	Enter expression : 2+(4*log(100))
	Dumping output stack :
        DOUBLE   : 2
        DOUBLE   : 4
        DOUBLE   : 100
        FUNCTION : log
        OPERATOR : *
        OPERATOR : +
	[SUCCESS] result = 20.4207 (0x0000000000000014)
	Enter expression :
	done.

## COMPILING ON LINUX ##

Type the following commands :

	$ cc -DEVAL_DEBUG main.c eval.c -lm
	$ ./a.out

# TODO #
- Improve error detection when computation results return infinite or NaN values.
 