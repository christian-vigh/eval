/**************************************************************************************************************

    NAME
        main.c

    DESCRIPTION
        Test file for the evaluate() function.

    AUTHOR
        Christian Vigh, 09/2015.

    HISTORY
    [Version : 1.0]    [Date : 2015/09/17]     [Author : CV]
        Initial version.

 **************************************************************************************************************/

# include	<stdio.h>
# include	<stdlib.h>
# include	"eval.h"

/*
# include	<float.h>
# include 	<stdarg.h>
*/

/* Custom constants */
EVAL_CONSTANT_DEF ( myconstants )
	EVAL_CONSTANT ( "TESTC1", 100 )
	EVAL_CONSTANT ( "TESTC2", 200 )
EVAL_CONSTANT_END ;

/* Custom functions */
EVAL_PRIMITIVE ( by2 )
   { return ( argv [0] * 2 ) ; } 

EVAL_FUNCTION_DEF ( myfunctions )
	EVAL_FUNCTION ( "by2", 1, 1, by2 )
EVAL_FUNCTION_END ;

/* Callback function */
EVAL_CALLBACK ( callback )
   {
	int	status	=  EVAL_CALLBACK_OK ;

	if  ( ! strcasecompare ( vname, "TIME" ) )
	   {
		time_t		now	=  time ( NULL ) ;

		* value		=  ( eval_double ) now ;
	    }
	else
		status	=  EVAL_CALLBACK_UNDEFINED ;
    }


void  main ( int  argc, char **  argv )
   {
	char  		buffer [1024] ;
	double 		value ;
	   

	// These initializations are necessary only if you plan to define your own constants and/or functions
	evaluator_register_constants ( myconstants ) ;
	evaluator_register_functions ( myfunctions ) ;

	printf ( "Expression evaluator tester. Press Enter to exit.\n" ) ;

	while  ( 1 )
	   {
		fflush ( stdin ) ;
		printf ( "Enter expression : " ) ;
		* buffer 	=  0 ;
		gets ( buffer ) ;
		
		if  ( ! * buffer  ||  * buffer  ==  '\n'  ||  * buffer  ==  '\r' ) 
		   {
			printf ( "done.\n" ) ;
			exit (0);
		    }
		
		if  ( evaluate_ex ( buffer, & value, callback ) )
			printf ( "[SUCCESS] result = %g (0x%.16lX)\n", value, ( eval_int ) value ) ;
		else
			evaluator_perror ( ) ;
	    }
    }