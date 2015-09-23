/**************************************************************************************************************

    NAME
        eval.h

    DESCRIPTION
        Header file for the evaluate() function.

    AUTHOR
        Christian Vigh, 09/2015.

    HISTORY
    [Version : 1.0]    [Date : 2015/09/17]     [Author : CV]
        Initial version.

 **************************************************************************************************************/

# ifndef	__EVAL_H__
#	define	__EVAL_H__

// Inclusion of <limits.h> is EXTREMELY important because it redefines structure alignment !
// Without it, the evaluator_constant_definition structure is 16 bytes long, while it takes 32 bytes after 
// <limits.h> has been included.
// If you use the evaluator package in, say, file main.c, which do not include <limits.h>, then main.c will
// see the evaluator_constant_definition structure as 16-bytes long, while eval.c will see it as 32-bytes long...
// This behavior has been found with GNU C.
# include	<limits.h>


/*==============================================================================================================
 *
 *  Limits.
 *
 *==============================================================================================================*/
# ifdef		LLONG_MIN

typedef		long long int		eval_int ;
typedef		long double		eval_double ;

#	define	EVAL_INTMIN		( ( eval_double ) LLONG_MIN )
#	define	EVAL_INTMAX		( ( eval_double ) LLONG_MAX )
#	define	EVAL_UINTMAX		( ( eval_double ) ULLONG_MAX )
#	define  EVAL_FLOATMIN		( ( eval_double ) LDBL_MIN )
#	define  EVAL_FLOATMAX		( ( eval_double ) LDBL_MAX )

# else

typedef		long int		eval_int ;
typedef		double			eval_double ;

#	define	EVAL_INTMIN		( ( eval_double ) INT_MIN )
#	define	EVAL_INTMAX		( ( eval_double ) INT_MAX )
#	define	EVAL_UINTMAX		( ( eval_double ) UINT_MAX )
#	define  EVAL_FLOATMIN		( ( eval_double ) DBL_MIN )
#	define  EVAL_FLOATMAX		( ( eval_double ) DBL_MAX )

# endif


/*==============================================================================================================
 *
 *  Platform-specific defines.
 *
 *==============================================================================================================*/
# ifdef		WIN32
#	define	strcasecompare(a,b)		_stricmp ( a, b )
# else
#	include <strings.h>
#	define	strcasecompare(a,b)		strcasecmp ( a, b )
# endif



/*==============================================================================================================

	Constant definition structures.

  ==============================================================================================================*/
# define	EVAL_NULL_CONSTANT		{ NULL, 0 }
# define	EVAL_CONSTANT_DEF( var )	evaluator_constant_definition  var [] = {
# define	EVAL_CONSTANT( name, value )	{ name, value },
# define	EVAL_CONSTANT_END		EVAL_NULL_CONSTANT }

typedef struct  evaluator_constant_definition
   {
	char *		name ;			// Constant name
	eval_double	value ;			// Constant value
    }  evaluator_constant_definition ;


/*==============================================================================================================

	Function definition macros, types & structures.

  ==============================================================================================================*/
# define	EVAL_NULL_FUNCTION		{ NULL, 0, 0, NULL }
# define	EVAL_FUNCTION_NAME(func)	eval_primitive_##func
# define	EVAL_PRIMITIVE(func)		static eval_double  EVAL_FUNCTION_NAME ( func ) ( int  argc, eval_double *  argv )

# define	EVAL_FUNCTION_DEF( var )	evaluator_function_definition  var [] = {
# define	EVAL_FUNCTION( name, minargs, maxargs, func )	\
						{ name, minargs, maxargs, EVAL_FUNCTION_NAME ( func ) },
# define	EVAL_FUNCTION_END		{ NULL, 0, 0, NULL } }


typedef eval_double	( * eval_function ) ( int  argc, eval_double *  argv ) ;


typedef struct  evaluator_function_definition
   {
	char *		name ;			// Function name
	int		min_args ;		// Min arguments
	int		max_args ;		// Max arguments
	eval_function	func ;			// Pointer to function
    }  evaluator_function_definition ;


/*==============================================================================================================

	Definitions for evaluator callbacks.

  ==============================================================================================================*/

// Status constants that can be returned by an eval_callback
# define	EVAL_CALLBACK_OK		0			// Variable exists
# define	EVAL_CALLBACK_UNDEFINED		-1			// Undefined variable

// Declares a callback function
# define	EVAL_CALLBACK( func )		func ( char *  vname, eval_double *  value )

typedef int		( * eval_callback ) ( char *  vname, eval_double *  value ) ;


/*==============================================================================================================

	Macros & constants.

  ==============================================================================================================*/

// Error information
extern int	evaluator_errno ;		
extern char 	evaluator_error [] ;

// Error codes
# define	E_EVAL_OK					0
# define	E_EVAL_UNEXPECTED_CHARACTER			-1		// An unexpected character has been found
# define	E_EVAL_INVALID_NUMBER				-2		// Invalid number specified
# define	E_EVAL_UNEXPECTED_TOKEN				-3		// A valid token has been encountered in an invalid place
# define	E_EVAL_UNEXPECTED_NUMBER			-4		// A valid number has been encountered in an invalid place
# define	E_EVAL_UNEXPECTED_OPERATOR			-5		// A valid operator has been encountered in an invalid place
# define	E_EVAL_STACK_EMPTY				-6		// Internal error : stack has not enough arguments to apply the next operator
# define	E_EVAL_UNDEFINED_OPERATOR			-7		// Internal error : an undefined operator has been found (this means that a new operator has been
										// added, but the eval_compute() function has not been modified to process it)
# define	E_EVAL_UNDEFINED_TOKEN_TYPE			-8		// Internal error : an undefined token type has been found (this means that a new token type has been
										// added, but the eval_compute() function has not been modified to process it)
# define	E_EVAL_UNBALANCED_PARENTHESES			-9		// Unbalanced parentheses
# define	E_EVAL_UNEXPECTED_RIGHT_PARENT			-10		// Unmatched closing parenthesis
# define	E_EVAL_UNDEFINED_CONSTANT			-11		// Undefined constant
# define	E_EVAL_UNEXPECTED_NAME				-12		// Unexpected constant
# define	E_EVAL_IMPLEMENTATION_ERROR			-13		// Implementation caused inconsistent result
# define	E_EVAL_INVALID_REGISTER_INDEX			-14		// Register index out of range
# define	E_EVAL_REGISTER_NOT_SET				-15		// No value has been assigned to the specified register
# define	E_EVAL_UNDEFINED_FUNCTION			-16		// Undefined function
# define	E_EVAL_UNTERMINATED_FUNCTION_CALL		-17		// Function call with unbalanced parentheses
# define	E_EVAL_TOO_MANY_NESTED_CALLS			-18		// Too many nested function calls
# define	E_EVAL_UNEXPECTED_ARG_SEPARATOR			-19		// Argument separator in the wrong place
# define	E_EVAL_INVALID_FUNCTION_ARGC			-20		// Invalid number of arguments specified during a function call
# define	E_EVAL_BAD_ARGUMENT_COUNT			-21		// Not enough or too much arguments remain on stack to process function call
# define	E_EVAL_UNDEFINED_VARIABLE			-22		// Undefined variable
# define	E_EVAL_VARIABLES_NOT_ALLOWED			-23		// Variables are not allowed when calling the evaluate() function
# define	E_EVAL_UNEXPECTED_VARIABLE			-24		// Variable reference has been found in an incorrect place


/*==============================================================================================================
 *
 *  Declarations for debug mode.
 *
 *==============================================================================================================*/
# if	EVAL_DEBUG
extern void	eval_dump_constants ( ) ;
extern void	eval_dump_functions ( ) ;
# endif




/*==============================================================================================================

        Function prototypes.

  ==============================================================================================================*/

extern int					evaluator_use_degrees ;

extern int					evaluate				( const char *				expression,
											  double *				result ) ;

extern int					evaluate_ex				( const char *				expression,
											  double *				result,
											  eval_callback				callback ) ;

extern void					evaluator_perror			( ) ;

extern void 					evaluator_register_constants		( const evaluator_constant_definition *	definitions ) ;
extern void 					evaluator_register_functions		( const evaluator_function_definition *	definitions ) ;

extern const evaluator_constant_definition *	evaluator_get_registered_constants	( ) ;
extern const evaluator_function_definition *	evaluator_get_registered_functions	( ) ;

# endif		/*  __EVAL_H__  */