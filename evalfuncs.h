/**************************************************************************************************************

    NAME
        evalfuncs.c

    DESCRIPTION
        Functions and constants implemented by the evaluate() function.
	This file is included by eval.c

    AUTHOR
        Christian Vigh, 09/2015.

    HISTORY
    [Version : 1.0]    [Date : 2015/09/20]     [Author : CV]
        Initial version.

 **************************************************************************************************************/


/*==============================================================================================================

	Some additional math constants.

  ==============================================================================================================*/
# define	M_PHI			1.6180339887498948482045868343


/*==============================================================================================================

        Helper functions.

  ==============================================================================================================*/
# ifndef	EVAL_TRIG_DEGREES
#	define  EVALUATOR_USE_DEGREES		1
# elif		EVAL_TRIG_DEGREES 
#	define	EVALUATOR_USE_DEGREES		1
# else
#	define  EVALUATOR_USE_DEGREES		0
# endif

int		evaluator_use_degrees		=  EVALUATOR_USE_DEGREES ;


static eval_double	eval_deg2rad ( eval_double  value )
   {
	return ( ( M_PI * value ) / 180 ) ;
    }

static eval_double	eval_rad2deg ( eval_double  value )
   {
	return ( ( 180 * value ) / M_PI ) ;
    }


static eval_double	eval_degrees ( eval_double  value )
   {
	if  ( evaluator_use_degrees )
		return ( eval_deg2rad ( value ) ) ;
	else 
		return ( value ) ;
    }


static eval_double	eval_factorial ( eval_double  value )
   {
	eval_double	result		=  1 ;
	int		i ;
	
	value	=  abs ( ( int ) value ) ;

	for  ( i = 2 ;  i  <=  value ; i ++ )
		result	*=  i ;

	return ( result ) ;
    }


/*==============================================================================================================

        Arithmetic functions implementation.
	All functions can safely assume that their number of arguments is correct (ie, conformant to what has
	been described in the corresponding evaluator_function_definition structure).

  ==============================================================================================================*/    

// abs(x) -
//	Returns the absolute value of x.
EVAL_PRIMITIVE ( abs )
   {
	return ( ( argv [0]  >=  0 ) ?  argv [0] : - argv [0] ) ;
    }


// sigma ( low, high [, step ] ) -
//	Sums all values between the specified range.
EVAL_PRIMITIVE ( sigma )
   {
	double		step	=  ( argc  ==  3 ) ?  argv [2] : 1 ;
	double		low	=  argv [0],
			high	=  argv [1] ;
	double		result ;


	result	=  ( ( high + low ) * ( ( high - low + 1 ) / step ) ) / 2 ;

	return ( result ) ;
    }


// avg ( x1 [, ..., xn] ) -
//	Computes the average of a list of values.
EVAL_PRIMITIVE ( avg )
   {
	int		i ;
	eval_double	result	=  0 ;

	for  ( i = 0 ; i  <  argc ; i ++ )
		result	+=  argv [i] ;

	return ( result / argc ) ;
    }

// var ( x1 [, ..., xn] ) -
//	Computes the variance of a list of values.
EVAL_PRIMITIVE ( var )
   {
	int		i ;
	eval_double	result	=  0 ;
	eval_double	avg	=  EVAL_FUNCTION_NAME ( avg ) ( argc, argv ) ;

	for  ( i = 0 ; i  <  argc ; i ++ )
		result	+=  pow ( ( argv [i] - avg ), 2 ) ;

	result	/=  argc ;

	return ( result ) ;
    }

// dev ( x1 [, ..., xn] ) -
//	Computes the standard deviation of a list of values.
EVAL_PRIMITIVE ( dev )
   {
	eval_double	result	=  sqrt ( EVAL_FUNCTION_NAME ( var ) ( argc, argv ) ) ;

	return ( result ) ;
    }

// arr ( n, p ) -
//	Computes the number of (ordered) arrangements of p elements within n.
EVAL_PRIMITIVE ( arr )
   {
	eval_double	result	=  eval_factorial ( argv [0] ) / eval_factorial ( argv [1] - argv [0] ) ;

	return ( result ) ;
    }

// comb ( n, p ) -
//	Computes the number of (unordered) arrangements of p elements within n.
EVAL_PRIMITIVE ( comb )
   {
	eval_double	result	=  eval_factorial ( argv [0] ) / 
					( eval_factorial ( argv [1] - argv [0] ) * eval_factorial ( argv [1] ) ) ;

	return ( result ) ;
    }

// dist ( x1, y1, x2, y2 ) -
//	Computes the distance between two points.
EVAL_PRIMITIVE ( dist )
   {
	eval_double	x1	=  argv [0],
			y1	=  argv [1],
			x2	=  argv [2],
			y2	=  argv [3] ;
	eval_double	result	=  sqrt ( pow ( x2 - x1, 2 ) + pow ( y2 - y1, 2 ) ) ;


	return ( result ) ;
    }

// slope ( x1, y1, x2, y2 ) -
//	Computes the slope of a line traversing two points.
EVAL_PRIMITIVE ( slope )
   {
	eval_double	x1	=  argv [0],
			y1	=  argv [1],
			x2	=  argv [2],
			y2	=  argv [3] ;
	eval_double	result	=  ( y2 - y1 ) / ( x2 - x1 ) ;


	return ( result ) ;
    }

// fib ( x ) -
//	Computes fibonnaci value for order x.
EVAL_PRIMITIVE ( fib )
   {
	static eval_double	sqrt_5	=  2.2360679774997896964091736687313 ;
	eval_double		n	=  argv [0] ;
	eval_double		result	=  ( pow ( M_PHI, ( double ) n ) - pow ( -1 / M_PHI, ( double ) n ) ) / sqrt_5 ;

	return ( result ) ;
    }


// delta1, delta2 ( a, b, c ) -
//	Computes -b +/- sqrt ( b2 - 4ac )
//		 ------------------------
//		           2a
EVAL_PRIMITIVE ( delta1 )
   {
	eval_double	a	=  argv [0],
			b	=  argv [1],
			c	=  argv [2] ;
	eval_double	result ;

	result	=  ( - b + sqrt ( ( b * b ) - ( 4 * a * c ) ) ) / ( 2 * a ) ;

	return ( result ) ;
    }

EVAL_PRIMITIVE ( delta2 )
   {
	eval_double	a	=  argv [0],
			b	=  argv [1],
			c	=  argv [2] ;
	eval_double	result ;

	result	=  ( - b - sqrt ( ( b * b ) - ( 4 * a * c ) ) ) / ( 2 * a ) ;

	return ( result ) ;
    }


/*==============================================================================================================

        Math lib function wrappers.

  ==============================================================================================================*/

// acos ( X ) -
//	Arc cosine of x.
EVAL_PRIMITIVE ( acos )
   {
	return ( acos ( eval_degrees ( argv [0] ) ) ) ;
    }

// asin ( X ) -
//	Arc sine of x.
EVAL_PRIMITIVE ( asin )
   {
	return ( asin ( eval_degrees ( argv [0] ) ) ) ;
    }

// atan ( X ) -
//	Arc tangent of x.
EVAL_PRIMITIVE ( atan )
   {
	return ( atan ( eval_degrees ( argv [0] ) ) ) ;
    }

// atan2 ( Y, X ) -
//	Arc tangent of y/x.
EVAL_PRIMITIVE ( atan2 )
   {
	return ( atan2 ( eval_degrees ( argv [0] ), eval_degrees ( argv [1] ) ) ) ;
    }

// ceil ( x ) -
//	Rounds x to the next integer.
EVAL_PRIMITIVE ( ceil )
   {
	return ( ceil ( argv [0] ) ) ;
    }

// cos ( X ) -
//	Cosine of x.
EVAL_PRIMITIVE ( cos )
   {
	return ( cos ( eval_degrees ( argv [0] ) ) ) ;
    }

// cosh ( X ) -
//	Hyperbolic cosine of x.
EVAL_PRIMITIVE ( cosh )
   {
	return ( cosh ( eval_degrees ( argv [0] ) ) ) ;
    }

// exp ( X ) -
//	Exponential of x.
EVAL_PRIMITIVE ( exp )
   {
	return ( exp ( argv [0] ) ) ;
    }

// floor ( X ) -
//	Rounds x to the nearest lower integer value.
EVAL_PRIMITIVE ( floor )
   {
	return ( floor ( argv [0] ) ) ;
    }

// Log ( X ) -
//	Natural logarithm of x.
EVAL_PRIMITIVE ( log )
   {
	return ( log ( argv [0] ) ) ;
    }

// log2 ( X ) -
//	Base 2 logarithm of x.
EVAL_PRIMITIVE ( log2 )
   {
	static eval_double	ln2_value	=  0 ;

	if  ( ! ln2_value )
		ln2_value	=  log ( 2 ) ;

	return ( log ( argv [0] ) / ln2_value ) ;
    }

// log10 ( X ) -
//	Base 10 logarithm of x.
EVAL_PRIMITIVE ( log10 )
   {
	return ( log10 ( argv [0] ) ) ;
    }

// sin ( X ) -
//	Sine of x.
EVAL_PRIMITIVE ( sin )
   {
	return ( sin ( eval_degrees ( argv [0] ) ) ) ;
    }

// sinh ( X ) -
//	Hyperbolic sine of x.
EVAL_PRIMITIVE ( sinh )
   {
	return ( sinh ( eval_degrees ( argv [0] ) ) ) ;
    }

// sqrt ( X ) -
//	Square root of x.
EVAL_PRIMITIVE ( sqrt )
   {
	return ( sqrt ( argv [0] ) ) ;
    }

// tan ( X ) -
//	Tangent of x.
EVAL_PRIMITIVE ( tan )
   {
	return ( tan ( eval_degrees ( argv [0] ) ) ) ;
    }

// tanh ( X ) -
//	Hyperbolic tangent of x.
EVAL_PRIMITIVE ( tanh )
   {
	return ( tanh ( eval_degrees ( argv [0] ) ) ) ;
    }




/*==============================================================================================================

        Default constant definitions.

  ==============================================================================================================*/    
EVAL_CONSTANT_DEF ( default_constant_definitions )
	EVAL_CONSTANT ( "PI"		, 3.14159265358979323846	)
	EVAL_CONSTANT ( "PI_2"		, 1.57079632679489661923	)
	EVAL_CONSTANT ( "PI_4"		, 0.785398163397448309616	)
	EVAL_CONSTANT ( "E"		, 2.71828182845904523536	)
	EVAL_CONSTANT ( "LOG2E"		, 1.44269504088896340736	)
	EVAL_CONSTANT ( "LOG10E"	, 0.434294481903251827651	)
	EVAL_CONSTANT ( "LN2"		, 0.693147180559945309417	)
	EVAL_CONSTANT ( "LN10"		, 2.30258509299404568402	)
	EVAL_CONSTANT ( "ONE_PI"	, 0.318309886183790671538	)
	EVAL_CONSTANT ( "TWO_PI"	, 0.636619772367581343076	)
	EVAL_CONSTANT ( "TWO_SQRTPI"	, 1.12837916709551257390	)
	EVAL_CONSTANT ( "SQRT2"		, 1.41421356237309504880	)
	EVAL_CONSTANT ( "ONE_SQRT2"	, 0.707106781186547524401	)
	EVAL_CONSTANT ( "INTMIN"	, EVAL_INTMIN			)
	EVAL_CONSTANT ( "INTMAX"	, EVAL_INTMAX			)
	EVAL_CONSTANT ( "UINTMAX"	, EVAL_UINTMAX			)
	EVAL_CONSTANT ( "DBLMIN"	, EVAL_FLOATMIN			)
	EVAL_CONSTANT ( "DBLMAX"	, EVAL_FLOATMAX			)
	EVAL_CONSTANT ( "E_PI"		, 23.140692632779269006		)
	EVAL_CONSTANT ( "PI_E"		, 22.45915771836104547342715	)
	EVAL_CONSTANT ( "PHI"		, M_PHI				)
EVAL_CONSTANT_END ;


/*==============================================================================================================

        Function definitions.

  ==============================================================================================================*/    
EVAL_FUNCTION_DEF ( default_function_definitions )
	   EVAL_FUNCTION ( "abs"		,	1,		1, abs		)	
	   EVAL_FUNCTION ( "acos"		,	1,		1, acos		)
	   EVAL_FUNCTION ( "arr"		,	2,		2, arr 		)
	   EVAL_FUNCTION ( "asin"		,	1,		1, asin 	)
	   EVAL_FUNCTION ( "atan"		,	1,		1, atan 	)
	   EVAL_FUNCTION ( "atan2"		,	2,		2, atan 	)
	   EVAL_FUNCTION ( "ceil"		,	1,		1, ceil 	)
	   EVAL_FUNCTION ( "comb"		,	2,		2, comb 	)
	   EVAL_FUNCTION ( "cos"		,	1,		1, cos 		)
	   EVAL_FUNCTION ( "cosh"		,	1,		1, cosh 	)
	   EVAL_FUNCTION ( "delta1"		,	3,		3, delta1	)
	   EVAL_FUNCTION ( "delta2"		,	3,		3, delta2	)
	   EVAL_FUNCTION ( "dev"		,	1,     0x7FFFFFFF, dev 		)
	   EVAL_FUNCTION ( "dist"		,	4,		4, dist 	)
	   EVAL_FUNCTION ( "exp"		,	1,		1, exp 		)
	   EVAL_FUNCTION ( "fib"		,	1,		1, fib 		)
	   EVAL_FUNCTION ( "floor"		,	1,		1, floor 	)
	   EVAL_FUNCTION ( "log"		,	1,		1, log 		)
	   EVAL_FUNCTION ( "log2"		,	1,		1, log2		)
	   EVAL_FUNCTION ( "log10"		,	1,		1, log10	)
	   EVAL_FUNCTION ( "avg"		,	1,     0x7FFFFFFF, avg 		)
	   EVAL_FUNCTION ( "sigma"		,	2,		3, sigma	)
	   EVAL_FUNCTION ( "sin"		,	1,		1, sin		)
	   EVAL_FUNCTION ( "sinh"		,	1,		1, sinh		)
	   EVAL_FUNCTION ( "slope"		,	4,		4, slope	)
	   EVAL_FUNCTION ( "sqrt"		,	1,		1, sqrt		)
	   EVAL_FUNCTION ( "tan"		,	1,		1, tan		)
	   EVAL_FUNCTION ( "tanh"		,	1,		1, tanh		)
	   EVAL_FUNCTION ( "var"		,	1,     0x7FFFFFFF, var		)
EVAL_FUNCTION_END ;