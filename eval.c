/**************************************************************************************************************

    NAME
        eval.c

    DESCRIPTION
        Implements an expression evaluator, evaluate().
	Based on Wikipedia shunting-yard algorightm description :

		https://en.wikipedia.org/wiki/Shunting-yard_algorithm

    AUTHOR
        Christian Vigh, 09/2015.

    HISTORY
    [Version : 1.0]    [Date : 2015/09/17]     [Author : CV]
        Initial version.

 **************************************************************************************************************/

# define	_USE_MATH_DEFINES

# include   	<stdio.h>
# include 	<string.h>
# include 	<ctype.h>
# include 	<malloc.h>
# include 	<math.h>
# include	<float.h>
# include 	<stdarg.h>
# include	<stdlib.h>
# include	<time.h>

# include	"eval.h"
# include	"evalfuncs.h"


/*==============================================================================================================
 *
 *  Define the EVAL_DEBUG macro for debugging information.
 *
 *==============================================================================================================*/
# ifndef	EVAL_DEBUG
#	define	EVAL_DEBUG		0
# else
#	undef	EVAL_DEBUG
#	define	EVAL_DEBUG		1
# endif



/*==============================================================================================================
 *
 *  Redefine these macros if needed to integrate the eval package into your environment.
 *
 *==============================================================================================================*/

# ifndef	eval_malloc
#	define 	eval_malloc( size ) 		malloc ( size )
# endif

# ifndef	eval_realloc
#	define 	eval_realloc( p, size )		realloc ( p, size )
# endif

# ifndef	eval_strdup
#	define 	eval_strdup( p )		strdup ( p ) 
# endif

# ifndef	eval_strndup
#	define 	eval_strndup( s, size )		__eval_strndup__ ( s, size )
# endif

# ifndef	eval_free
#	define 	eval_free( p )			free ( p ) 
# endif

 
// Indicates whetyher empty strings are to be considered as an error or converted to zero
# ifndef	EVAL_DENY_EMPTY_STRINGS
#	define	EVAL_DENY_EMPTY_STRINGS			0
# else
#	undef	EVAL_DENY_EMPTY_STRINGS
#	define	EVAL_DENY_EMPTY_STRINGS			1
# endif


/*==============================================================================================================
 *
 *  Constants.
 *
 *==============================================================================================================*/

// Token classification
# define 	TOKEN_ERROR 			0x8000 			// Invalid character found
# define 	TOKEN_EOF 			0x0001			// Everything has been processed from the supplied input string
# define 	TOKEN_NUMBER			0x0002			// Number (integer or float)
# define 	TOKEN_NAME 			0x0004 			// Function name
# define 	TOKEN_OPERATOR			0x0008			// Operator
# define 	TOKEN_COMMA			0x0010 			// Argument separator for a function call
# define	TOKEN_LEFT_PARENT		0x0020			// Left and right parentheses
# define	TOKEN_RIGHT_PARENT		0x0040
# define	TOKEN_REGISTER_SAVE		0x0080			// Save last computed value ($x! construct)
# define	TOKEN_REGISTER_RECALL		0x0100			// Recall the specified computed value ($x?)
# define	TOKEN_VARIABLE			0x0200			// Variable reference

// Operators
# define	OP_PLUS 			 1
# define 	OP_MINUS 			 2
# define 	OP_MUL				 3
# define 	OP_DIV 				 4
# define 	OP_IDIV 			 5
# define 	OP_POWER 			 6
# define 	OP_MOD 				 7
# define 	OP_AND				 8
# define	OP_OR				 9
# define	OP_XOR		  		10
# define 	OP_NOT				11
# define	OP_UNARY_PLUS			12
# define	OP_UNARY_MINUS			13
# define	OP_SHL				14
# define	OP_SHR				15
# define	OP_FACTORIAL			16
# define	OP_LEFT_PARENT			50
# define	OP_RIGHT_PARENT			51
# define	OP_COMMA			52

// Operator associativity
# define 	ASSOC_NONE 			0
# define 	ASSOC_LEFT 			1
# define 	ASSOC_RIGHT 			2

// Stack initial sizes
# define 	OUTPUT_STACK_SIZE 		64
# define 	OPERATOR_STACK_SIZE 		32

// Registers, numbered from 0 to MAX_REGISTERS - 1 
# define	MAX_REGISTERS			64

// Max nested function calls
# define	MAX_NESTED_FUNCTION_CALLS	64

/*==============================================================================================================
 *
 *  Operator definitions.
 *
 *==============================================================================================================*/
typedef struct operator_token
   {
	char *		token ;
	int 		length ;
	int 		type ;
	int 		precedence:8 ;
	int 		associativity:2 ;
	int 		unary:1 ;
    }  operator_token ;


/*==============================================================================================================
 *
 *  Save/recall value constructs ($x! and $x?).
 *
 *==============================================================================================================*/
static eval_double	eval_registers		[ MAX_REGISTERS ] ;
static char		eval_registers_set	[ MAX_REGISTERS ] ;
static int		eval_last_register			=  -1 ;


/*==============================================================================================================
 *
 *  Stack definitions.
 *
 *==============================================================================================================*/

// A "compiled" expression stack is generated by the eval_parse() function, then interpreted by eval_compute().
// All stack entries have one of these types
# define 	STACK_ENTRY_NUMERIC 		0		// Numeric value
# define 	STACK_ENTRY_NAME 		1		// Constant name
# define 	STACK_ENTRY_OPERATOR 		2		// Operator
# define	STACK_ENTRY_REGISTER_SAVE	3		// Save value on top of stack to te specified register
# define	STACK_ENTRY_REGISTER_RECALL	4		// Push the specified register value on top of stack
# define	STACK_ENTRY_FUNCTION_CALL	5		// Start of a function call
# define	STACK_ENTRY_VARIABLE		6		// Variable reference

// Output stack entry definition
typedef struct eval_stack_entry
   {
	int 	type ;						// Stack entry type	
	union
	   {
		eval_double 		double_value ;		// Value
		char *			string_value ;		// Constant or function name
		operator_token *	operator_value ;	// Operator definition
		int			register_value ;	// Register

		struct						// Function call
		   {
			char *		name ;
			int		argc ;
		    } function_value ;
	    } value ;	
    }  eval_stack_entry ;
    

// Generic stack for values and operators
typedef struct  eval_stack
   {
	int 			size ;				// Current number of items in the stack
	int 			item_size ;			// Size of a stack item
	int 			last_item ;			// Last item of the stack
	eval_stack_entry *	data ;				// Stack data
    }  eval_stack ;



/*==============================================================================================================

        Error information.

  ==============================================================================================================*/    
int		evaluator_errno ;
char 		evaluator_error [ 1024 ] ;


/*==============================================================================================================

        Operator definitions.

  ==============================================================================================================*/    
static operator_token 		operators []	=
   {
	{ "+"	, 1, OP_PLUS		,  5, ASSOC_LEFT , 0	},
	{ "-"	, 1, OP_MINUS		,  5, ASSOC_LEFT , 0 	},
	{ "*"	, 1, OP_MUL		,  8, ASSOC_LEFT , 0 	},
	{ "/"	, 1, OP_DIV		,  8, ASSOC_LEFT , 0 	},
	{ "\\"	, 1, OP_IDIV		,  8, ASSOC_LEFT , 0 	},
	{ "**"	, 2, OP_POWER		,  9, ASSOC_RIGHT, 0	},
	{ "%"	, 1, OP_MOD		,  8, ASSOC_LEFT , 0 	},
	{ "&"	, 1, OP_AND		,  5, ASSOC_LEFT , 0 	},
	{ "|"	, 1, OP_OR		,  5, ASSOC_LEFT , 0 	},
	{ "^"	, 1, OP_XOR		,  5, ASSOC_LEFT , 0 	},
	{ "~"	, 1, OP_NOT		, 10, ASSOC_RIGHT, 1  	},
	{ "<<"	, 2, OP_SHL		,  5, ASSOC_LEFT , 0	},
	{ ">>"	, 2, OP_SHR		,  5, ASSOC_LEFT , 0	},
	{ "!"   , 1, OP_FACTORIAL	, 10, ASSOC_LEFT , 1    },
	
	{ NULL  , 0		,  0,           0, 0    }
    } ;

// Unary plus/minus need a special processing, since they use the same characters as their binary counterpart
static operator_token		unary_minus		=
	{ "-"	, 1, OP_UNARY_MINUS	, 10, ASSOC_RIGHT, 1	} ; 

// Same for parentheses
static operator_token		left_parenthesis	=
	{ "("	, 1, OP_LEFT_PARENT	, 50, ASSOC_NONE , 0	} ;

static operator_token		right_parenthesis	=
	{ ")"	, 1, OP_RIGHT_PARENT	, 50, ASSOC_NONE , 0	} ;

// And for comma
static operator_token		comma_operator		=  
	{ ","	, 1, OP_COMMA	, 50, ASSOC_NONE , 0	} ;


/*==============================================================================================================
 *
 *  Structures for constant and function definitions.
 *
 *==============================================================================================================*/

// Increments for constant and function stores
# define	PRIMITIVE_INCREMENT		64
# define	ARGUMENT_INCREMENT		64
# define	NEXT_INCREMENT(x,incr)		( ( ( (x) + (incr) - 1 ) / (incr) ) * (incr) )


// Some kind of "base" class for constant and function definitions
typedef struct  primitive_definition
   {
	char *		name ;
    }  primitive_definition ;


// Implements either a list of constants or functions
typedef struct  primitive_list 
   {
	int		length ;
	int		item_count ;
	int		item_size ;
	void *		data ;
    }  primitive_list ;

// Constants and functions list
static primitive_list		eval_constant_definitions	=  { 0, 0, sizeof ( evaluator_constant_definition ), NULL },
				eval_function_definitions	=  { 0, 0, sizeof ( evaluator_function_definition ), NULL } ;


/*==============================================================================================================

    eval_skip_spaces -
	Skips spaces from the current position.

  ==============================================================================================================*/    
static char *	eval_skip_spaces ( char *  str, int *  line, int *  character )
   {
	while  ( * str  &&  isspace ( * str ) )
	   {
		* character ++ ;

		if  ( * str  ==  '\n' )
		   {
			* line ++ ;
			* character	=  1 ;
		    }
		else if  ( * str  !=  '\r' )
			* character ++ ;

		str ++ ;
	    }

	return ( str ) ;
     }


/*==============================================================================================================

    eval_register -
        Called for registering constants & functions.

  ==============================================================================================================*/
static int	__eval_primitive_sort__ ( const void *  a, const void *  b )
   {
	return
	   (
		strcmp 
		   (
			( ( primitive_definition * ) a ) -> name,
			( ( primitive_definition * ) b ) -> name
		    )
	    ) ;
    }


static void	eval_register ( primitive_list *  list, const void *  definitions )
   {
	int		count		=  0 ;
	int		total_count ;
	int		copy_size ;
	int		new_length ;
	const void *	p		=  definitions ;
	void *		q ;


	// First, count the number of new definitions ; the last definition must be an empty one (ie, at least the
	// 'name' member must be null)
	while  ( ( ( primitive_definition * ) p ) -> name  !=  NULL )
	   {
		count ++ ;
		p		=  ( void * ) ( ( ( char * ) p ) + list -> item_size ) ;
	    }

	// Compute the total number of primitives to be stored
	total_count	=  list -> item_count + count ;

	// Total number of bytes to copy
	copy_size	=  count * list -> item_size ;

	// If the total number of primitives is greater than the actual size of the primitive list,
	// then we will have some allocation work to perform
	if  ( total_count  >  list -> length )
	   {
		new_length	=  NEXT_INCREMENT ( total_count, PRIMITIVE_INCREMENT ) ;

		if  ( list -> length )   
		   {
			list -> length	=  new_length ;
			list -> data	=  eval_realloc ( list -> data, new_length * list -> item_size ) ;
		    }
		else
		   {
			list -> length	 =  new_length ;
			list -> data	 =  eval_malloc ( new_length * list -> item_size ) ;
		    }
	    }

	// Make 'q' point to the first slot after the very last primitive of the list
	q	=  ( void * ) ( ( ( char * ) ( list -> data ) ) + ( list -> item_count * list -> item_size ) ) ;

	// Now copy the definitions after the last primitive of the list 
	memcpy ( q, definitions,  copy_size ) ;
	list -> item_count	+=  count ;

	// And finally, sort primitive list
	qsort ( list -> data, list -> item_count, list -> item_size, __eval_primitive_sort__ ) ;
    }


/*==============================================================================================================

    eval_find_primitive -
        Finds a primitive (constant or function).

  ==============================================================================================================*/
static primitive_definition *	eval_find_primitive ( primitive_list *  list, char *  value )
   {
	int			low		=  0,
				high		=  list -> item_count - 1,
				middle ;
	int			cmpresult ;
	primitive_definition *	p ;


	while  ( low  <=  high )
	   {
		middle		=  low + ( high - low ) / 2 ;
		p		=  ( primitive_definition * ) ( ( ( char * ) list -> data ) + ( list -> item_size * middle ) ) ;
		cmpresult	=  strcasecompare ( p -> name, value ) ;


		if  ( cmpresult  <  0 )
			low	=  middle + 1 ;
		else if  ( cmpresult  >  0 )
			high	=  middle - 1 ;
		else
			return ( p ) ;
	    }

	return ( NULL ) ;
    }


# if	EVAL_DEBUG
/*==============================================================================================================

	Debug functions for dumping constant and function definitions.

  ==============================================================================================================*/
void	eval_dump_constants ( )
   {
	int					i ;
	evaluator_constant_definition *		p	=  ( evaluator_constant_definition * ) eval_constant_definitions. data ;


	printf ( "Defined constants :\n" ) ;

	for  ( i = 0 ; i  <  eval_constant_definitions. item_count ; i ++, p ++ )
	   {
		printf ( "\t%-32s = %lg\n", p -> name, ( double ) p -> value ) ;
	    }
    }


void	eval_dump_functions ( )
   {
	int					i ;
	evaluator_function_definition *		p	=  ( evaluator_function_definition * ) eval_function_definitions. data ;


	printf ( "Defined functions :\n" ) ;

	for  ( i = 0 ; i  <  eval_function_definitions. item_count ; i ++, p ++ )
	   {
		printf ( "\t%-32s (%d..%d)\n", p -> name, p -> min_args, p -> max_args ) ;
	    }
    }


static void	eval_dump_stack ( eval_stack *  stack, char *  title )
   {
	int		i ;


	printf ( "Dumping %s :\n", title ) ;

	for  ( i = 0 ; i  <=  stack -> last_item ; i ++ )
	   {
		printf ( "\t" ) ;

		switch ( stack -> data [i]. type )
		   {
			case	STACK_ENTRY_NUMERIC :
				printf ( "NUMBER   : %lg\n", ( double ) stack -> data [i]. value. double_value ) ;
				break ;

			case	STACK_ENTRY_NAME :
				printf ( "CONSTANT : %s\n", stack -> data [i]. value. string_value ) ;
				break ;

			case	STACK_ENTRY_OPERATOR :
				printf ( "OPERATOR : %s\n", stack -> data [i]. value. operator_value -> token ) ;
				break ;

			case	STACK_ENTRY_REGISTER_SAVE :
				printf ( "REGSAVE  : %d\n", stack -> data [i]. value. register_value ) ;
				break ;

			case	STACK_ENTRY_REGISTER_RECALL :
				printf ( "REGCALL  : %d\n", stack -> data [i]. value. register_value ) ;
				break ;

			case	STACK_ENTRY_FUNCTION_CALL :
				printf ( "FUNCTION : %s\n", stack -> data [i]. value. string_value ) ;
				break ;

			case	STACK_ENTRY_VARIABLE :
				printf ( "VARIABLE : %s\n", stack -> data [i]. value. string_value ) ;
				break ;

			default :
				printf ( "UNKNOWN  : type = %d\n", stack -> data [i]. type ) ;
		    }
	    }
    }
# endif


/*==============================================================================================================
 *
 *  eval_initialize -
 *	Initializes the eval package.
 *
 *==============================================================================================================*/	
static int	eval_initialized		=  0 ;

static int	__eval_sort_operators__ ( const void *  a, const void *  b )
   {
	return
	   (
		( ( operator_token * ) b ) -> length -
		( ( operator_token * ) a ) -> length 
	    ) ;
    }


static void  eval_initialize ( )
   {
	// Do nothing if initialization has been already performed
	if  ( eval_initialized )
		return ;

	// Register default constants and functions
	eval_register ( & eval_constant_definitions, default_constant_definitions ) ;
	eval_register ( & eval_function_definitions, default_function_definitions ) ;

	// Sort operator by descending length
	qsort ( operators, ( sizeof ( operators ) - 1 ) / sizeof ( operator_token ), sizeof ( operator_token ), __eval_sort_operators__ ) ;
	
	// All done
	eval_initialized	=  1 ;
    }


static void	eval_instance_initialize ( )
   {
	time_t		seed	=  time ( NULL ) ;

	// Initialize the pseudo-random number generator
	srand ( ( int ) seed ) ;

	// Reset last error code
	evaluator_errno		=  E_EVAL_OK ;
	* evaluator_error	=  '\0' ;

	// Reset register values
	memset ( eval_registers, 0, sizeof ( eval_registers ) ) ;
	memset ( eval_registers_set, 0, sizeof ( eval_registers_set ) ) ;
	eval_last_register	=  -1 ;
    }


/*==============================================================================================================
 *
 *  eval_error -
 *	Sets the evaluator_errno and evaluator_error variables.
 *
 *==============================================================================================================*/	
static void  eval_error ( int  err, int  line, int character, char *  fmt, ... )
   {
	va_list 	ap ;
	int		length ;
	   
	   
	evaluator_errno		=  err ;

	if  ( line  ==  -1 )
		length		=  sprintf ( evaluator_error, "Eval error : " ) ;
	else
		length		=  sprintf ( evaluator_error, "Eval error [line#%d, col#%d] : ", line, character + 1 ) ;

	va_start ( ap, fmt ) ;
	vsprintf ( evaluator_error + length, fmt, ap ) ;
	va_end ( ap ) ;
    }
    

/*==============================================================================================================
 *
 *  __eval_strndup__ -
 *	Duplicates a fixed-length string.
 *
 *==============================================================================================================*/	
static char *  __eval_strndup__ ( char *  s, int size )
   {
	char *		p	=  eval_malloc ( size ) ;

	strncpy ( p, s, size ) ;

	return ( p ) ;
    }
    


/*==============================================================================================================
 *
 *   	Stack functions.
 *
 *==============================================================================================================*/	

static void * 	eval_stack_alloc ( int  size, int  item_size )
   {
	int 			byte_count 		=  size * sizeof ( eval_stack_entry * ) ;
	eval_stack * 		stack 			=  ( eval_stack * ) eval_malloc ( sizeof ( eval_stack ) ) ;   
	   
	
	stack -> size	 	=  size ;
	stack -> item_size 	=  item_size ;
	stack -> last_item	=  -1 ;
	stack -> data 		=  eval_malloc ( byte_count ) ;
	   
	return ( stack ) ;
   }
   
   
static void 	eval_stack_realloc ( eval_stack *  stack, int  new_size )
   {
	int	byte_count 	=  new_size * stack -> item_size ;
	   
	   
	stack -> data 	=  ( eval_stack_entry * ) eval_realloc ( stack -> data, byte_count ) ;
	stack -> size 	=  new_size ;
    }
    
    
static void  	eval_stack_push ( eval_stack *  stack, void *  item )
   {
	if  ( stack -> last_item + 1  >=  stack -> size )
		eval_stack_realloc ( stack, stack -> size * 2 ) ;
	
	memcpy ( stack -> data + ( ++ stack -> last_item ), ( eval_stack_entry * ) item, sizeof ( eval_stack_entry ) ) ;
    }
   
    
static void * 	eval_stack_pop ( eval_stack *  stack )
   {
	if  ( stack -> last_item  <  0 )
		return ( NULL ) ;
	else
		return ( stack -> data + ( stack -> last_item -- ) ) ;
    }
    
    
static void 	eval_stack_free ( eval_stack *  stack )
   {
	int		i ;

	for  ( i = 0 ; i  <=  stack -> last_item ; i ++ )
	   {
		switch ( stack -> data [i]. type )
		   {
			case	STACK_ENTRY_NAME :
				eval_free ( stack -> data [i]. value. string_value ) ;
				break ;

			case	STACK_ENTRY_FUNCTION_CALL :
				eval_free ( stack -> data [i]. value. function_value. name ) ;
				break ;
		    }
	    }

	eval_free ( stack -> data ) ;
	eval_free ( stack ) ;
    }
 

static int	eval_stack_is_empty ( eval_stack *  stack )
   {
	return ( stack -> last_item  <  0 ) ;
    }


/*==============================================================================================================
 *
 *  eval_double_value -
 *	Converts a string to a double.
 *	At that point, no sign is present and we are sure that incorrect mixing between integer and double
 *	syntax are absent ; for example, we will never see a string such as "0xFF.01E10".
 *	Returns 1 if parsing is OK, 0 otherwise.
 *
 *==============================================================================================================*/	
static int	eval_double_value ( char *  str, int  length, eval_double *  result )
   {
	char *		tail ;
	eval_double	numeric_value ;


	* result	=  0 ;

# if	EVAL_DENY_EMPTY_STRINGS
	if  ( ! * str )
		return ( 0 ) ;
# else
	if  ( ! * str )
	   {
		* result	=  0 ;

		return ( 1 ) ;
	    }
# endif

	// Process integer values that have a base specifier or that start with zero
	if  ( * str  ==  '0'  &&  length  >  1 )
	   {
		char *		p		=  str + 1 ;
		int		base		=  10 ;
		double		value		=  0 ;
		char		ch, ich ;


		switch ( toupper ( * p ) )
		   {
			case	'B'	:  base	 =   2 ; break ;
			case	'O'	:  base	 =   8 ; break ;
			case	'D'	:  base  =  10 ; break ;
			case	'X'	:  base  =  16 ; break ;

			default :
				if  ( * p  >=  '0'  &&  * p  <=  '7' )
					base	=  8 ;
				else
					return ( 0 ) ;
		    }

		p ++ ;

		while  ( * p ) 
		   {
			ich	=  toupper ( * p ) ;
			ch	=  ( ich  >=  'A' ) ?  ich - 'A' + 10 : ich - '0' ;

			value   =  ( value * base ) + ch ;
			p ++ ;
		    }

		* result	=  value ;

		return ( 1 ) ;
	    }

	// Floating-point value - convert it
	numeric_value	=  strtod ( str, & tail ) ;

	// Fail if supplied input string does not completely match a float
	if  ( tail  !=  str + length )
		return ( 0 ) ;

	// Match found : return the numeric result
	* result	=  numeric_value ;

	return ( 1 ) ;
    }

    
/*==============================================================================================================
 *
 *  eval_lex -
 *	Parses the supplied input string to retrieve the next token.
 *
 *==============================================================================================================*/	
static int eval_lex ( char *  str, char **  startp, char **  endp, void **  op, int *  line, int *  character )
   {
	static int	register_id ;
	int 		i ;
	int 		token 		=  TOKEN_EOF ;
	   
	   
	str	=  eval_skip_spaces ( str, line, character ) ;
	
	if  ( ! * str )
		return ( token ) ;
	
	// *startp will point to the start of the token (and end,... guess what ?)
	* startp 	=  str ;
	
	// If an operator has been found, *op will point to the operator description structure
	* op 		=  NULL ;
			
	// Name found (maybe a function name)
	if  ( isalpha ( * str )  ||  * str  ==  '_' )
	   {
		while  ( isalnum ( * str )  ||  * str  ==  '_' )
			str ++ ;
		
		token 		=  TOKEN_NAME ;
	    }
	// Number, either an integer with an optional base or a float
	// At that point we don't try to convert anything, just verify that the number is correct
	else if  ( isdigit ( * str ) )
	   {
		int 	found_base 	=  0,
			found_dot 	=  0,
			found_exp	=  0 ;
		int 	base 		=  10 ;   
		char 	ch, ich ;
		

		if  ( * str  ==  '0'  &&  isxdigit ( str [1] ) )
		   {
			char * 	p 	=  str + 1 ;
			int  	found 	=  0 ;

			// If we find a number starting with zero, then this may be an integer represented in 
			// octal, unless we find something specific to a float (a decimal point or the exponentiation
			// character, E)
			while  ( * p )
			   {
				switch ( * p ) 
				   {
					case 	'.' :
					case 	'E' : 
					case 	'e' :
						found 	=  1 ;
						break ;
				    }
				    
				if  ( found )
					break ;
				
				p ++ ;
			    }
			
			if  ( ! found )
			   {
				base 	=  8 ;
				   
				str ++ ;
			    }
		    }
		   
		// Loop through input number
		token 	=  TOKEN_NUMBER ;

		while  ( * str )
		   {
			ch 	=  toupper ( * str ) ;
			   
			switch  ( ch )
			   {
				// Base specifier
				case 	'X' :		// Hex
				case	'B' :		// Binary
				case	'D' :		// Decimal
				case	'O' :		// Octal
					if  ( found_dot  ||  found_exp  ||  found_base )
					   {
						token 	=  TOKEN_ERROR ;
						goto  NumberEnd ;
					    }
		
					switch ( ch )
					   {
						case	'X'	:  base		=  16 ; break ;
						case	'D'	:  base		=  10 ; break;
						case	'O'	:  base		=   8 ; break ;
						case	'B'	:  base		=   2 ; break ;
					    }
					
					found_base 	=  1 ;
					break ;
					    
				// Decimal point
				case 	'.' :
					if  ( found_dot  ||  found_exp  ||  found_base )
					   {
						token 	=  TOKEN_ERROR ;
						goto  NumberEnd ;
					    }
		
					found_dot 	=  1 ;
					break ;
					    
				// Exponentiation character ; can be interpreted in two ways, depending on
				// whether we saw a hex base specifier (0x) or we are handling a float
				case 	'E' :
					if  ( ! found_base )
					   {
						if  ( found_dot  ||  found_exp )
						   {
							token 	=  TOKEN_ERROR ;
							goto  NumberEnd ;
						    }
		
						found_exp 	=  1 ;
						ch 		=  * ( str + 1 ) ;
						    
						if  ( ch  ==  '+'  ||  ch  ==  '-' )
							str ++ ;
						
						break ;
					    }
					// Not a float ; this means that "E" may be a hex digit so intentionally fall
					// through the next case below
					    
				// Other character : our token fetching task is complete
				default :
					if  ( ( ch  >=  '0'  &&  ch  <= '9' )  ||  ( ch  >=  'A'  &&  ch  <=  'F' ) )
					   {
						ich 	=  ( ch  >=  'A' ) ?  ( ch - 'A' + 10 ) : ch - '0' ;

						if  ( ich  >=  base )
						   {
							token 	=  TOKEN_ERROR ;
							goto  NumberEnd ;
						    }
					    }
					else
						goto NumberEnd ;
			    }			
			    
			str ++ ;
		    }
		    
NumberEnd : ;

	    }
	// Left parenthesis
	else if  ( * str  ==  '(' )
	   {
		token	=  TOKEN_LEFT_PARENT ;
		str ++ ;
	    }
	// Right parenthesis 
	else if  ( * str  ==  ')' )
	   {
		token	=  TOKEN_RIGHT_PARENT ;
		str ++ ;
	    }
	// $name notation : variable name whose value must be supplied by the caller
	else if  ( * str  ==  '$' )
	   {
		// Check that some characters remain after the "$" sign
		if  ( * ( str + 1 ) )
		   {
			str ++ ;
			* startp	=  str ;

			// A variable name must start with a letter or an underline
			if  ( isalpha ( * str )  ||  * str  ==  '_' )
			   {
				while  ( * str  &&  ( isalnum ( * str )  ||  * str  ==  '_' ) )
					str ++ ;

				token	=  TOKEN_VARIABLE ;
			    }
			// Otherwise this is an error
			else
				token	=  TOKEN_ERROR ;
		    }
		// End of string after the "$" sign
		else
			token	=  TOKEN_ERROR ;
	    }
	// #x! or #x? notation : save to/restore from register
	else if  ( * str  ==  '#' )
	   {
		int	found_digits	=  0 ;

		register_id	=  0 ;
		str ++ ;

		str	=  eval_skip_spaces ( str, line, character ) ;

		while  ( isdigit ( * str )  )
		   {
			register_id	=  ( register_id * 10 ) + ( * str - '0' ) ;
			found_digits	=  1 ;
			str ++ ;
		    }

		str	=  eval_skip_spaces ( str, line, character ) ;

		if  ( ! found_digits )
			register_id	=  -1 ;

		if  ( * str  ==  '!' )
		   {
			token			=  TOKEN_REGISTER_SAVE ;
			* ( int ** ) op		=  & register_id ;
			str ++ ;
		    }
		else if  ( * str  ==  '?' )
		   {
			token			=  TOKEN_REGISTER_RECALL ;
			* ( int ** ) op		=  & register_id ;
			str ++ ;
		    }
		else
			token			=  TOKEN_ERROR ;
	    }
	// Function argument separator (comma)
	else if  ( * str  ==  ',' )
	   {
		token	=  TOKEN_COMMA ;
		str ++ ;
	    }
	// Other input : may be an operator
	else if  ( * str ) 
	   {
		int 	found 	=  0 ;
		   
		// Loop through operator definitions
		for  ( i = 0 ; operators [i]. token  !=  NULL ; i ++ )
		   {
			if  ( ! strncmp ( str, operators [i]. token, operators [i]. length ) )
			   {
				token 				 =  TOKEN_OPERATOR ;
				* ( ( operator_token ** ) op )	 =  operators + i ;
				found 				 =  1 ;
				str 				+=  operators [i]. length ;
				break ;
			    }
		    }
		    
		if  ( ! found )
			token 	=  TOKEN_ERROR ;
	    }
	
	// Set the end pointer to the character after the token found
	* endp 		 =  str ;
	* character	+=  str - * startp - 1 ;

	// All done, return
	return ( token ) ;
    }


/*==============================================================================================================
 *
 *  eval_compute -
 *	Performs the real computation of the expression evaluated by eval_parse.
 *
 *==============================================================================================================*/	
static int	eval_compute ( eval_stack *  stack, eval_double *  output, eval_callback  callback )
   {
	eval_double *		value_stack ;				// Stack of intermediary floating point values
	int			value_stack_top		=  -1 ;
	eval_stack_entry *	se ;					// Current stack entry
	operator_token *	ot ;					// Current token
	int			i ;
	eval_double		value1,					// Result + values for binary operators
				value2, 
				result  ;
	int			status			=  1 ;		// Return code ; 1 = OK
	eval_double *		function_args		=  NULL ;	// Placeholder used to store function arguments
	int			function_args_max	=  0 ;


	// Ignore empty parse trees
	if  ( eval_stack_is_empty ( stack ) )
		return ( 0 ) ;

	// Allocate space for a new stack
	value_stack	=  ( eval_double * ) eval_malloc ( ( stack -> last_item + 1 ) * sizeof ( eval_double ) ) ;

	// Loop through expression tree items
	for  ( i = 0 ; i  <=  stack -> last_item ; i ++ )
	   {
		se	=  stack -> data + i ;

		switch  ( se -> type )
		   {
			// Push numeric entries onto the value stack
			case	STACK_ENTRY_NUMERIC :
				value_stack [ ++ value_stack_top ]	=  
				result					= se -> value. double_value ;
				break ;

			// Apply operators ; One value is popped off the stack for unary operators, and two for binary ones
			case	STACK_ENTRY_OPERATOR :
				// Get current operator
				ot	=  se -> value. operator_value ;

				// Check that enough elements remain on the stack
				if  ( ( ot -> unary  &&  value_stack_top  <  0 )  ||
				      ( ! ot -> unary  &&  value_stack_top  <  1 ) )
				   {
					eval_error ( E_EVAL_STACK_EMPTY, -1, -1, "Stack does not contain enough elements to process the '%s' operator",
							se -> value. operator_value -> token ) ;

					return ( 0 ) ;
				    }

				// Pop one or two values from the stack, depending on whether the operator is unary or binary
				if  ( ot -> unary )
					value1	=  value_stack [ value_stack_top -- ] ;
				else
				   {
					value1	=  value_stack [ value_stack_top -- ] ;
					value2	=  value_stack [ value_stack_top -- ] ;
				    }

				// Process the operator
				switch ( ot -> type )
				   {
					case	OP_PLUS :
						result	=  value2 + value1 ;
						break ;

					case	OP_MINUS :
						result	=  value2 - value1 ;
						break ;

					case	OP_MUL :
						result	=  value1 * value2 ;
						break ;

					case	OP_DIV :
						result	=  value2 / value1 ;
						break ;

					case	OP_IDIV :
						result	=  floor ( value2 / value1 ) ;
						break ;

					case	OP_POWER :
						result	=  pow ( value2, value1 ) ;
						break ;

					case	OP_MOD :
						result	=  fmod ( value2, value1 ) ;
						break ;

					case	OP_AND :
						result	=  ( eval_double ) ( ( ( eval_int ) value1 )  &  ( ( eval_int ) value2 ) );
						break ;

					case	OP_OR :
						result	=  ( eval_double ) ( ( ( eval_int ) value1 )  |  ( ( eval_int ) value2 ) ) ;
						break ;

					case	OP_XOR :
						result	=  ( eval_double ) ( ( ( eval_int ) value1 )  ^  ( ( eval_int ) value2 ) ) ;
						break ;

					case	OP_NOT :
						result	=  ( eval_double ) ( ~( ( eval_int ) value1 ) ) ;
						break ;

					case	OP_UNARY_PLUS :
						break ;

					case	OP_UNARY_MINUS :
						result	=  -value1 ;
						break ;

					case	OP_SHL :
						result	=  ( eval_double ) ( ( ( eval_int ) value2 )  <<  ( ( eval_int ) value1 ) ) ;
						break ;

					case	OP_SHR :
						result	=  ( eval_double ) ( ( ( eval_int ) value2 )  >>  ( ( eval_int ) value1 ) ) ;
						break ;

					case	OP_FACTORIAL :
						result	=  eval_factorial ( value1 ) ;
						break ;

					// Paranoia : Changes have been made to the supported operator list, but not reflected here
					default :
						eval_error ( E_EVAL_UNDEFINED_OPERATOR,  -1, -1, "Undefined operator '%s' found", ot -> token ) ;
						status	=  0 ;
						goto  ComputeEnd ;
				    }

				value_stack [ ++ value_stack_top ]	=  result ;
				break ;

			// Constant name
			case	STACK_ENTRY_NAME :
			   {
				evaluator_constant_definition *		def ;	
				
				
				def	=  ( evaluator_constant_definition * ) eval_find_primitive ( 
											& eval_constant_definitions, 
											se -> value. string_value ) ;


				if  ( def  !=  NULL )
				   {
					value_stack [ ++ value_stack_top ]	=  
					result					=  def -> value ;
				    }
				else
				   {
					eval_error ( E_EVAL_UNDEFINED_CONSTANT, -1, -1, "Undefined constant '%s'", 
							se -> value. string_value ) ;
					status	=  0 ;

					goto  ComputeEnd ;
				    }

				break ;
			    }

			// Variable reference
			case	STACK_ENTRY_VARIABLE :
			   {
				eval_double 	callback_result ;
				int		callback_status		=  callback ( se -> value. string_value, & callback_result ) ;

				if  ( callback_status  ==  EVAL_CALLBACK_UNDEFINED )
				   {
					eval_error ( E_EVAL_UNDEFINED_VARIABLE, -1, -1, "Undefined variable '%s'",
							se -> value. string_value ) ;
					status	=  0 ;

					goto  ComputeEnd ;
				    }

				value_stack [ ++ value_stack_top ]	=  
				result					=  callback_result ;
				break ;
			    }

			// Register recall
			// The correctness of the register number has been checked in eval_parse() so we know here that
			// the register number is ok
			case	STACK_ENTRY_REGISTER_RECALL :
			   {
				int	regnum		=  se -> value. register_value ;

				if  ( regnum  <  0 )
				   {
					if  ( eval_last_register  >=  0 )
						regnum	=  eval_last_register ;
				    }

				if  ( regnum  >=  0  &&  eval_registers_set [ regnum ] )
					value_stack [ ++ value_stack_top ]	=  
					result					=  eval_registers [ regnum ] ;
				else
				   {
					   eval_error ( E_EVAL_INVALID_REGISTER_INDEX, -1, -1,  "Register #%d has not been assigned any value", ( int ) regnum ) ;
					   status	=  0 ;
					   goto ComputeEnd ;
				    }

				break ;
			    }

			// Register save 
			//	Save the last value of the value stack into the specified register
			case	STACK_ENTRY_REGISTER_SAVE :
			   {
				int	regnum		=  se -> value. register_value ;

				if  ( regnum  <  0 )
				   {
					if  ( eval_last_register  <  0 )
						regnum	=  0 ;
					else
						regnum	=  eval_last_register + 1 ;
				    }

				eval_registers [ regnum ]	=  value_stack [ value_stack_top ] ;
				eval_registers_set [ regnum ]	=  ( char ) 1 ;
				eval_last_register		=  regnum ;
				break ;
			    }

			// Function call
			case	STACK_ENTRY_FUNCTION_CALL :
			   {
				evaluator_function_definition *		def ;	
				int					argc	=  se -> value. function_value. argc ;
				
				
				def	=  ( evaluator_function_definition * ) eval_find_primitive ( 
											& eval_function_definitions, 
											se -> value. function_value. name ) ;

				// Function exists
				if  ( def  !=  NULL )
				   {
					int	stack_arguments			=  value_stack_top + 1 ;

					// Not enough or too much parameters specified : generate an error
					if  ( stack_arguments  <  argc )
					   {
						eval_error ( E_EVAL_IMPLEMENTATION_ERROR, -1, -1, "Not enough parameters (%d) remain on stack for function %s()",
								stack_arguments, def -> name ) ;
						status	=  0 ;

						goto  ComputeEnd ;
					    }
					// Argument count falls within the acceptable range of arguments
					// Note that if arguments on the stack are less than the number of arguments specified
					// during the function call, then there is an implementation error
					else if  ( argc  >=  def -> min_args  &&  argc  <=  def -> max_args )
					   {
						int		j ;

						// Ensure that the function_args array has enough entries to store function arguments
						if  ( argc + 1  >  function_args_max )
						   {
							int	count		=  NEXT_INCREMENT ( argc, ARGUMENT_INCREMENT ) + 1 ;

							if  ( function_args  ==  NULL )
								function_args	=  ( eval_double * ) eval_malloc ( count * sizeof ( eval_double ) ) ;
							else
								function_args	=  ( eval_double * ) eval_realloc ( function_args, count * sizeof ( eval_double ) ) ;
						     }

						// Collect arguments in a separate array
						for  ( j = argc - 1 ; j  >=  0 ; j -- )
							function_args [j]	=  value_stack [ value_stack_top -- ] ;

						// Call the function
						value_stack [ ++ value_stack_top ]	=  
						result					=  def -> func ( argc, function_args ) ;
					    }
					else
					   {
						eval_error ( E_EVAL_BAD_ARGUMENT_COUNT, -1, -1, "Bad number of arguments (%d) for function %s() ;"
								" authorized range is %d..%d",
								argc, def -> name, def -> min_args, def -> max_args ) ;
						status	=  0 ;

						goto  ComputeEnd ;
					    }
				    }
				// Undefined function name
				else
				   {
					eval_error ( E_EVAL_UNDEFINED_FUNCTION, -1, -1, "Undefined function '%s'", 
							se -> value. function_value. name ) ;
					status	=  0 ;

					goto  ComputeEnd ;
				    }

				break ;
			    }

			// Paranoia : Changes have been made to the supported token list, but not reflected here
			default :
				eval_error ( E_EVAL_UNDEFINED_TOKEN_TYPE, -1, -1, "Undefined token type '#%d'", se -> type ) ;
				status	=  0 ;
				goto  ComputeEnd ;
		    }
	    }

	// Final result
	if  ( value_stack_top  >  0 )			// More than one value on the stack : there must be a programmation error here...
	   {
		eval_error ( E_EVAL_IMPLEMENTATION_ERROR, -1, -1, "Value stack should hold at most one value" ) ;
		status		=  0 ;

		goto  ComputeEnd ;
	    }

	// Free any memory used to hold function arguments
	if  ( function_args  !=  NULL )
		eval_free ( function_args ) ;

	* output	=  result ;

ComputeEnd :
	return ( status ) ;
    }


/*==============================================================================================================
 *
 *  eval_parse -
 *	Grammatical analyzer for expressions.
 *
 *==============================================================================================================*/	
# define	DEFAULT_TOKEN_BUFFER_SIZE	64

static int  eval_parse ( const char *		str, 
			 eval_double *		value, 
			 eval_stack *		output_stack, 
			 eval_stack *		operator_stack, 
			 eval_callback		callback ) 
   { 
	char * 			startp,						// Start and end of next token in the input string
	     *			endp ;
	void *			param ;						// Data returned by the eval_lex() function
	operator_token *	op ;						// Operator token (may be returned by eval_lex)
	int 			token ;						// Token value
	int 			last_token 		=  TOKEN_EOF ;		// Last seen token value
	int 			status 			=  1 ;			// Return value (0 = bad...)
	eval_stack_entry 	stack_entry ;					// Entry to be added onto the stack
	char *			current_token		=  ( char * ) eval_malloc ( DEFAULT_TOKEN_BUFFER_SIZE ) ;
	int			current_token_length	=  0,			// A copy of the current token in the input string, its length and max length seen so far
				max_token_length	=  DEFAULT_TOKEN_BUFFER_SIZE ;
	int			character		=  0,			// Current character and line position
				line			=  1 ;
	int			inert_token ;					// Set to 1 when "inert" constructs, such as #! have been found
										// In this case, the last_token variable will keep its original value
	eval_stack_entry *	se ;
	int			parentheses_nesting	[ MAX_NESTED_FUNCTION_CALLS ] ;
	int			function_args		[ MAX_NESTED_FUNCTION_CALLS ] ;
	int			nesting_level		=  0 ;


	parentheses_nesting [0]		=  0 ;

	// The default value of an expression evaluation is not something which may look like 'undefined', but zero
	* value		=  0 ;

	// Retrieve tokens one by one from the input string
	while  ( * str )
	   {
		token 			=  eval_lex ( ( char * ) str, & startp, & endp, & param, & line, & character ) ;
		inert_token		=  0 ;

		// Always hold the current token in a nul-terminated string
		// The main purpose is to use it easily for error messages
		current_token_length	=  endp - startp ;

		if  ( current_token_length + 1  >=  max_token_length ) 
		   {
			max_token_length	*=  2 ;
			eval_free ( current_token ) ;
			current_token		 =  ( char * ) eval_malloc ( max_token_length ) ;
		    }

		strncpy ( current_token, startp, current_token_length ) ;
		current_token [ current_token_length ]	=  '\0' ;

		// Process current token
		switch ( token )
		   {
			// End of string : stop parsing
			case 	TOKEN_EOF :
				goto ParseEnd ;
			
			// Unexpected character found
			case 	TOKEN_ERROR :
				eval_error ( E_EVAL_UNEXPECTED_CHARACTER, line, character, "Unexpected character '%c'", * endp ) ;
				status 	=  0 ;
				goto ParseEnd ;
			
			// Number found 
			case 	TOKEN_NUMBER :
			   {
				eval_double		numeric_value ;
				int 			converted ;
				

				// A number can occur only :
				// - At the start of an expression
				// - After an operator
				// - In a function call, after an opening parenthesis or a comma
				if  ( ! ( last_token & ( TOKEN_EOF | TOKEN_OPERATOR | TOKEN_COMMA | TOKEN_LEFT_PARENT ) ) )
				   {
					eval_error ( E_EVAL_UNEXPECTED_NUMBER, line, character, "Unexpected number '%s'", current_token ) ;
					status	=  0 ;

					goto  ParseEnd ;
				    }

				// Convert current token to a double value
				converted	=  eval_double_value ( current_token, current_token_length, & numeric_value ) ;

				// Conversion successful
				if  ( converted )
				   {
					stack_entry. type 			=  STACK_ENTRY_NUMERIC ;
					stack_entry. value. double_value 	=  numeric_value ;
					   
					eval_stack_push ( output_stack, & stack_entry ) ;
				    }
				// Some extraneous characters did not get interpreted correctly...
				else
				   {
					eval_error ( E_EVAL_INVALID_NUMBER, line, character, "Invalid numeric value '%s'", current_token ) ;
					status 	=  0 ;
					   
					goto  ParseEnd ;
				    }
				
				break ;
			    }
			    
			// Operators
			case  	TOKEN_OPERATOR :
				op			=  ( operator_token * ) param ;
				stack_entry. type 	=  STACK_ENTRY_OPERATOR ;

				// A plus/minus sign is considered as unary if it follows one of the following construct :
				// - The start of the string
				// - An operator (eg : 2+-3)
				// - An opening parenthesis (eg: func(-3))
				// The unary plus is silently ignored, since it does not affect its right-part value
				if  ( last_token  &  ( TOKEN_OPERATOR | TOKEN_EOF | TOKEN_LEFT_PARENT ) )
				   {
					if  ( op -> type  ==  OP_PLUS )
						break ;
					else if  ( op -> type  ==  OP_MINUS )
						op	=  & unary_minus ;
					else if  ( ! op -> unary )
					   {
						eval_error ( E_EVAL_UNEXPECTED_OPERATOR, line, character, "Unexpected operator '%s'", current_token ) ;
						status	=  0 ;

						goto  ParseEnd ;
					    }
				    }  
				// Otherwise, operators can only follow :
				// - A number
				// - A constant name 
				// - A closing parenthesis
				else if  ( ! ( last_token & ( TOKEN_NUMBER | TOKEN_NAME | TOKEN_VARIABLE | TOKEN_RIGHT_PARENT ) ) )
				   {
					eval_error ( E_EVAL_UNEXPECTED_OPERATOR, line, character, "Unexpected operator '%s'", current_token ) ;
					status	=  0 ;

					goto  ParseEnd ;
				    }
				// Consider unary left-associative operators as inert : since they apply to their left operand (a number, a constant or a right parent)
				// they are pushed immediately onto the output stack
				else if  ( op -> unary  &&  op -> associativity  ==  ASSOC_LEFT )
				   {
					stack_entry. value. operator_value	=  op ;
					eval_stack_push ( output_stack, & stack_entry ) ;
					inert_token	=  1 ;

					break ;
				    }

				/***
					We have found operator op1. While there is an operator op2 in the operator stack, and either :
					- op1 is left-associative and its precedence is less than or equal to op2 precedence, OR
					- op1 is right-associative and its precedence is greater than op2 precedence
					then we will pop op2 off the operator stack onto the output queue
				 ***/
				while  ( ! eval_stack_is_empty ( operator_stack ) )
				   {
					operator_token *	previous_op	=  operator_stack -> data [ operator_stack -> last_item ]. value. operator_value ;


					if  ( ( ( op -> associativity  ==  ASSOC_LEFT   &&  op -> precedence  <=  previous_op -> precedence )   ||
					        ( op -> associativity  ==  ASSOC_RIGHT  &&  op -> precedence  >   previous_op -> precedence ) )	&&
					      previous_op -> type  !=  OP_LEFT_PARENT )
					   {
						eval_stack_pop ( operator_stack ) ;
						stack_entry. value. operator_value	=  previous_op ;
						eval_stack_push ( output_stack, & stack_entry ) ;
					    }
					else 
						break ;
				    }

				// Now that precedence rules have been applied, we can push op1 to the operator stack 
				stack_entry. value. operator_value	=  op ;
				eval_stack_push ( operator_stack, & stack_entry ) ;

				break ;

			// Left parenthesis : consider it as a non-associative operator with the highest precedence or to a function call
			case	TOKEN_LEFT_PARENT :
				// Last token was a name, so this is a function call 
				// We consider function calls with n arguments as n-ary operators ; thus , f(1,2,3,4) will give on the stack :
				// - STACK_ENTRY_NUMBER (1)
				// - STACK_ENTRY_NUMBER (2)
				// - STACK_ENTRY_NUMBER (3)
				// - STACK_ENTRY_NUMBER (4)
				// - STACK_ENTRY_FUNCTION_CALL, argc = 4
				if  ( last_token  &  TOKEN_NAME ) 
				   {
					eval_stack_entry *		se ;

					if  ( nesting_level + 1  >  MAX_NESTED_FUNCTION_CALLS )
					   {
						eval_error ( E_EVAL_TOO_MANY_NESTED_CALLS, line, character, "Too many nested function calls" ) ;
						status	=  0 ;

						goto  ParseEnd ;
					    }

					se		=  ( eval_stack_entry * ) eval_stack_pop ( output_stack ) ; 
					se -> type	=  STACK_ENTRY_FUNCTION_CALL ;
					eval_stack_push ( operator_stack, se ) ;

					parentheses_nesting [ ++ nesting_level ]	=  1 ;
					function_args [ nesting_level ]			=  0 ;
				    }
				// Otherwise, this is simply for expression grouping
				else if  ( last_token  &  ( TOKEN_EOF | TOKEN_LEFT_PARENT | TOKEN_OPERATOR | TOKEN_COMMA ) )
				   {
					stack_entry. type			=  STACK_ENTRY_OPERATOR ;
					stack_entry. value. operator_value	=  & left_parenthesis ;
					eval_stack_push ( operator_stack, & stack_entry ) ;

					parentheses_nesting [ nesting_level ] ++ ;
				    }
				else
				   {
					eval_error ( E_EVAL_UNEXPECTED_TOKEN, line, character, "Unexpected opening parenthesis" ) ;
					status	=  0 ;

					goto  ParseEnd ;
				    }

				break ;

			/***
				Right parenthesis :
				- Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue.
				- Pop the left parenthesis from the stack, but not onto the output queue.
				- If the token at the top of the stack is a function token, pop it onto the output queue.
				- If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
			 ***/
			case	TOKEN_RIGHT_PARENT :
			   {
				int		found_left	=  0 ;

				// Closing parenthesis ends an expression grouping, not a function call
				if  ( ( last_token  &  ( TOKEN_NUMBER | TOKEN_RIGHT_PARENT | TOKEN_NAME | TOKEN_VARIABLE | TOKEN_LEFT_PARENT ) ) )
				   {
					// Count one more argument if previous token was not a left parenthesis 
					if  ( ! ( last_token  &  ( TOKEN_LEFT_PARENT ) ) )
						function_args [ nesting_level ] ++ ;

					// Push all operators until an opening parenthesis has been found
					while  ( ! eval_stack_is_empty ( operator_stack ) )
					   {
						eval_stack_entry *	se		=  ( eval_stack_entry * ) eval_stack_pop ( operator_stack ) ;
						operator_token *	previous_op	=  se -> value. operator_value ;

						if  ( se -> type  ==  STACK_ENTRY_FUNCTION_CALL )
						   {
							se -> value. function_value. argc	=  function_args [ nesting_level ] ;
							eval_stack_push ( output_stack, se ) ;
							found_left	=  1 ;
							break ;
						    }
						else if  ( previous_op -> type  ==  OP_LEFT_PARENT )
						   {
							found_left	=  1 ;
							break ;
						    }
						else
						   {
							eval_stack_push ( output_stack, se ) ;
						    }
					    }
				    }

				// A nested level greater than zero means that we are in a nested function call ; 
				// When the parentheses nesting within this call reaches 1, then this means that the 
				// closing parenthesis we just encountered is finishing the call
				if  ( ! ( last_token & TOKEN_COMMA )  &&  nesting_level > 0  &&  parentheses_nesting [ nesting_level ]  ==  1 )
					found_left				=  1 ;

				// Opening parenthesis not found
				if  ( ! found_left ) 
				   {
					eval_error ( E_EVAL_UNEXPECTED_RIGHT_PARENT, line, character, "Unexpected closing parenthesis" ) ;
					status	=  0 ;

					goto  ParseEnd ;
				    }

				parentheses_nesting [ nesting_level ] -- ;

				if  ( ! parentheses_nesting [ nesting_level ] )
				   {
					if  ( nesting_level  >  0 )
						nesting_level -- ;
				    }

				break ;
			    }

			// Name : either a constant or a function
			case	TOKEN_NAME :
				if  ( ! ( last_token & ( TOKEN_EOF | TOKEN_OPERATOR | TOKEN_COMMA | TOKEN_LEFT_PARENT ) ) )
				   {
					eval_error ( E_EVAL_UNEXPECTED_NAME, line, character, "Unexpected name '%s'", current_token ) ;
					status	=  0 ;

					goto  ParseEnd ;
				    }

				stack_entry. type			=  STACK_ENTRY_NAME ;
				stack_entry. value. string_value	=  eval_strdup ( current_token ) ;
				eval_stack_push ( output_stack, & stack_entry ) ;
				break ;

			// Variable name
			case	TOKEN_VARIABLE :
				if  ( callback  ==  NULL )
				   {
					eval_error ( E_EVAL_VARIABLES_NOT_ALLOWED, line, character, 
						"Variable references are not allowed when you use the evaluate() function.\n" 
						"Use the evaluate_ex() function instead (referenced variable : %s)",
						current_token ) ;
					status	=  0 ;

					goto  ParseEnd ;
				    }
				else if  ( ! ( last_token & ( TOKEN_EOF | TOKEN_OPERATOR | TOKEN_COMMA | TOKEN_LEFT_PARENT ) ) )
				   {
					eval_error ( E_EVAL_UNEXPECTED_VARIABLE, line, character, "Unexpected variable reference '%s'", current_token ) ;
					status	=  0 ;

					goto  ParseEnd ;
				    }

				stack_entry. type			=  STACK_ENTRY_VARIABLE ;
				stack_entry. value. string_value	=  eval_strdup ( current_token ) ;
				eval_stack_push ( output_stack, & stack_entry ) ;

				break ;

			// Save to register
			case	TOKEN_REGISTER_SAVE :
			   {
				int		register_id		=  * ( int * ) param ;


				if  ( register_id  >=  MAX_REGISTERS )
				   {
					eval_error ( E_EVAL_INVALID_REGISTER_INDEX, line, character, "Invalid register index %d (range is 0..%d)",
							register_id, MAX_REGISTERS - 1 ) ;
					status	=  0 ;

					goto  ParseEnd ;
				    }

				stack_entry. type			=  STACK_ENTRY_REGISTER_SAVE ;
				stack_entry. value. register_value	=  register_id ;
				eval_stack_push ( output_stack, & stack_entry ) ;

				// This token is inert and won't be remembered during the parsing
				inert_token	=  1 ;
				break ;
			    }

			// Recall register value
			case	TOKEN_REGISTER_RECALL :
			   {
				int		register_id		=  * ( int * ) param ;


				if  ( ! ( last_token & ( TOKEN_EOF | TOKEN_OPERATOR | TOKEN_COMMA | TOKEN_LEFT_PARENT ) ) )
				   {
					eval_error ( E_EVAL_UNEXPECTED_TOKEN, line, character, "Unexpected register '%s' value recall", current_token ) ;
					status	=  0 ;

					goto  ParseEnd ;
				    }

				if  ( register_id  >=  MAX_REGISTERS )
				   {
					eval_error ( E_EVAL_INVALID_REGISTER_INDEX, line, character, "Invalid register index %d (allowed range is 0..%d)",
							register_id, MAX_REGISTERS - 1 ) ;
					status	=  0 ;

					goto  ParseEnd ;
				    }

				stack_entry. type			=  STACK_ENTRY_REGISTER_RECALL ;
				stack_entry. value. register_value	=  register_id ;
				eval_stack_push ( output_stack, & stack_entry ) ;
				break ;
			    }

			// Function argument separator (comma)
			case	TOKEN_COMMA :
				if  ( last_token  &  ( TOKEN_NUMBER | TOKEN_NAME | TOKEN_RIGHT_PARENT ) )
				   {
					int		found_parent	=  0 ;

					function_args [ nesting_level ] ++ ;

					// Pop every operator off the operator stack and push it onto the output stack until a function call or a
					// left parenthesis has been found. A call like : f ( 1, 2+3, 4 ) will finally look on the stack as :
					//	1 2 3 + 4 f 
					// Once all the operations will be applied, the stack will look like :
					//	1 5 4 f
					// And since "f" is a 3-ary operator (because "f" was called with three arguments), the resulting stack will be :
					//	x
					// where "x" is the result of f(1, 5, 4)
					while  ( ! eval_stack_is_empty ( operator_stack ) )
					   {
						eval_stack_entry *	se		=  ( eval_stack_entry * ) eval_stack_pop ( operator_stack ) ;

						// A function call is a stop condition for this loop, but we need to push it onto the output stack
						if  ( se -> type  ==  STACK_ENTRY_FUNCTION_CALL )
						   {
							eval_stack_push ( operator_stack, se ) ;
							found_parent	=  1 ;
							break ;
						    }
						// Left parenthesis is also a stop condition, but we do not push it onto the output stack
						else if  ( se -> value. operator_value -> type  ==  OP_LEFT_PARENT ) 
						   {
							found_parent	=  1 ;
							break ;
						    }
						// Other operators : push the onto the output stack
						else
							eval_stack_push ( output_stack, se ) ;
					    }

					// Neither left parenthesis nor function call found on the operator stack
					if  ( ! found_parent )
					   {
						eval_error ( E_EVAL_UNEXPECTED_ARG_SEPARATOR, line, character, "Unexpected argument delimiter ',' found" ) ;
						status	=  0 ;

						goto  ParseEnd ;
					    }
				    }
				else
				   {
					eval_error ( E_EVAL_UNEXPECTED_ARG_SEPARATOR, line, character, "Unexpected argument separator" ) ;
					status	=  0 ;

					goto ParseEnd ;
				    }

				break ;

			// Unexpected token
			default :
				eval_error ( E_EVAL_UNEXPECTED_TOKEN, line, character, "Unexpected token #%d", token ) ;
				status 	=  0 ;
				goto  ParseEnd ;
		    }
		
		// Update pointer to current character 
		str 		=  endp ;

		// Remember last token type seen so far ; this helps providing some kind of syntax checking 
		// before computing the expression result
		if  ( ! inert_token )
			last_token 	=  token ;

		character	+=  endp - startp ;
	    }
	    
ParseEnd:
	// Possible error in expression
	if  ( ! status )			// Some error occured
		goto  ParseReturn ;
	else if  ( nesting_level )
	   {
		eval_error ( E_EVAL_UNTERMINATED_FUNCTION_CALL, line, character, "Unterminated function call" ) ;
		status	=  0 ;

		goto	ParseReturn ;
	    }
	else if  ( parentheses_nesting [0] )	// Unbalanced parentheses
	   {
		eval_error ( E_EVAL_UNBALANCED_PARENTHESES, line, character, "Unbalanced parentheses" ) ;
		status	=  0 ;

		goto  ParseReturn ;
	    }

	// Pop all the remaining elements from the operator stack to the output stack
	while  ( ( se = ( eval_stack_entry * ) eval_stack_pop ( operator_stack ) )  !=  NULL )
		eval_stack_push ( output_stack, se ) ;

# if	EVAL_DEBUG
	//eval_dump_constants ( ) ;
	//eval_dump_functions ( ) ;
	eval_dump_stack ( output_stack, "output stack" ) ;
# endif

	// Compute the result
	status	=  eval_compute ( output_stack, value, callback ) ;

ParseReturn :
	eval_free ( current_token ) ;
	    
	// All done, return
	return ( status ) ;
    }
 

/*==============================================================================================================
 *
 *  evaluate -
 *	Expression analyzer.
 *
 *==============================================================================================================*/	
int	__evaluate__ ( const char *  str, double *  output, eval_callback  callback )
   {
	eval_stack * 		output_stack 	=  ( eval_stack * ) eval_stack_alloc ( OUTPUT_STACK_SIZE  , sizeof ( eval_stack_entry ) ) ;
	eval_stack *		operator_stack 	=  ( eval_stack * ) eval_stack_alloc ( OPERATOR_STACK_SIZE, sizeof ( eval_stack_entry ) ) ;
	int			status ;
	eval_double		result ;


	// Initialize package if needed
	if  ( ! eval_initialized )
		eval_initialize ( ) ;

	eval_instance_initialize ( ) ;

	// Parse expression
	status	  		=  eval_parse ( str, & result, output_stack, operator_stack, callback ) ;
	* output		=  ( double ) result ;

	// Free stack data
	eval_stack_free ( output_stack ) ;
	eval_stack_free ( operator_stack ) ;

	// All done, return
	return ( status ) ;
    }


int	evaluate ( const char *  str, double *  output )
   {
	return ( __evaluate__ ( str, output, NULL ) ) ;
    }


int	evaluate_ex ( const char *  str, double *  output, eval_callback  callback )
   {
	return ( __evaluate__ ( str, output, callback ) ) ;
    }


/*==============================================================================================================
 *
 *  evaluator_perror -
 *	Prints the last expression evaluation error message.
 *
 *==============================================================================================================*/	
static struct eval_error_code
   {
	char *		name ;
	int		code ;
    }  eval_error_codes []	=
   {
	{ "E_EVAL_OK"				, E_EVAL_OK				},
	{ "E_EVAL_UNEXPECTED_CHARACTER"		, E_EVAL_UNEXPECTED_CHARACTER		},
	{ "E_EVAL_INVALID_NUMBER"		, E_EVAL_INVALID_NUMBER			},
	{ "E_EVAL_UNEXPECTED_TOKEN"		, E_EVAL_UNEXPECTED_TOKEN		},
	{ "E_EVAL_UNEXPECTED_NUMBER"		, E_EVAL_UNEXPECTED_NUMBER		},
	{ "E_EVAL_UNEXPECTED_OPERATOR"		, E_EVAL_UNEXPECTED_OPERATOR		},
	{ "E_EVAL_STACK_EMPTY"			, E_EVAL_STACK_EMPTY			},
	{ "E_EVAL_UNDEFINED_OPERATOR"		, E_EVAL_UNDEFINED_OPERATOR		},
	{ "E_EVAL_UNDEFINED_TOKEN_TYPE"		, E_EVAL_UNDEFINED_TOKEN_TYPE		},
	{ "E_EVAL_UNBALANCED_PARENTHESES"	, E_EVAL_UNBALANCED_PARENTHESES		},
	{ "E_EVAL_UNEXPECTED_RIGHT_PARENT"	, E_EVAL_UNEXPECTED_RIGHT_PARENT	},
	{ "E_EVAL_UNDEFINED_CONSTANT"		, E_EVAL_UNDEFINED_CONSTANT		},
	{ "E_EVAL_UNEXPECTED_NAME"		, E_EVAL_UNEXPECTED_NAME		},
	{ "E_EVAL_IMPLEMENTATION_ERROR"		, E_EVAL_IMPLEMENTATION_ERROR		},
	{ "E_EVAL_INVALID_REGISTER_INDEX"	, E_EVAL_INVALID_REGISTER_INDEX		},
	{ "E_EVAL_REGISTER_NOT_SET"		, E_EVAL_REGISTER_NOT_SET		},
	{ "E_EVAL_UNDEFINED_FUNCTION"		, E_EVAL_UNDEFINED_FUNCTION		},
	{ "E_EVAL_UNTERMINATED_FUNCTION_CALL"	, E_EVAL_UNTERMINATED_FUNCTION_CALL	},
	{ "E_EVAL_TOO_MANY_NESTED_CALLS"	, E_EVAL_TOO_MANY_NESTED_CALLS		},
	{ "E_EVAL_UNEXPECTED_ARG_SEPARATOR"	, E_EVAL_UNEXPECTED_ARG_SEPARATOR	},
	{ "E_EVAL_INVALID_FUNCTION_ARGC"	, E_EVAL_INVALID_FUNCTION_ARGC		},
	{ "E_EVAL_BAD_ARGUMENT_COUNT"		, E_EVAL_BAD_ARGUMENT_COUNT		},
	{ "E_EVAL_UNDEFINED_VARIABLE"		, E_EVAL_UNDEFINED_VARIABLE		},
	{ "E_EVAL_VARIABLES_NOT_ALLOWED"	, E_EVAL_VARIABLES_NOT_ALLOWED		},
	{ "E_EVAL_UNEXPECTED_VARIABLE"		, E_EVAL_UNEXPECTED_VARIABLE		},

	{ NULL, 0 }
    } ;

static char *	eval_errnostr ( int  err ) 
   {
	static char			buffer [ 64 ] ;
	struct eval_error_code *	p	=  eval_error_codes ;


	while  ( p -> name  !=  NULL )
	   {
		if  ( p -> code  ==  err )
			return ( p -> name ) ;

		p ++ ;
	    }

	sprintf ( buffer, "errno = %d", err ) ;
	return ( buffer ) ;
    }


void  evaluator_perror ( )
   {
	if  ( evaluator_errno )
		fprintf ( stderr, "%s (%s) \n", evaluator_error, eval_errnostr ( evaluator_errno ) ) ;
    }


/*==============================================================================================================
 *
 *  evaluator_register_constants, evaluator_register_functions -
 *	Register new constants and functions.
 *
 *==============================================================================================================*/
void  evaluator_register_constants ( const evaluator_constant_definition *	newdefs )
   {
	eval_register ( & eval_constant_definitions, newdefs ) ;
    }

void  evaluator_register_functions ( const evaluator_function_definition *	newdefs )
   {
	eval_register ( & eval_function_definitions, newdefs ) ;
    }


const evaluator_constant_definition *	evaluator_get_registered_constants  ( )
   {
	return ( ( evaluator_constant_definition * ) eval_constant_definitions. data ) ;
    }


const evaluator_function_definition *	evaluator_get_registered_functions  ( )
   {
	return ( ( evaluator_function_definition * ) eval_function_definitions. data ) ;
    }