/*
 * CS:APP Data Lab
 *
 * btest.c - A test harness that checks a student's solution in bits.c
 *           for correctness.
 *
 * Copyright (c) 2001-2011, R. Bryant and D. O'Hallaron, All rights
 * reserved.  May not be used, modified, or copied without permission.
 *
 * This is an improved version of btest that tests large windows
 * around zero and tmin and tmax for int32_teger puzzles, and zero, norm,
 * and denorm boundaries for floating point32_t puzzles.
 *
 * Note: not 64-bit safe. Always compile with gcc -m32 option.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <setjmp.h>
#include <math.h>
#include "btest.h"

/* Not declared in some stdlib.h files, so define here */
float strtof(const char *nptr, char **endptr);

/*************************
 * Configuration Constants
 *************************/

/* Handle infinite loops by setting upper limit on execution time, in
   seconds */
#define TIMEOUT_LIMIT 10

/* For functions with a single argument, generate TEST_RANGE values
   above and below the min and max test values, and above and below
   zero. Functions with two or three args will use square and cube
   roots of this value, respectively, to avoid combinatorial
   explosion */
#define TEST_RANGE 500000

/* This defines the maximum size of any test value array. The
   gen_vals() routine creates k test values for each value of
   TEST_RANGE, thus MAX_TEST_VALS must be at least k*TEST_RANGE */
#define MAX_TEST_VALS 13*TEST_RANGE

/**********************************
 * Globals defined in other modules
 **********************************/
/* This characterizes the set of puzzles to test.
   Defined in decl.c and generated from templates in ./puzzles dir */
extern test_rec test_set[];

/************************************************
 * Write-once globals defined by command line args
 ************************************************/

/* Emit results in a format for autograding, without showing
   and counter-examples */
static int32_t grade = 0;

/* Time out after this number of seconds */
static int32_t timeout_limit = TIMEOUT_LIMIT; /* -T */

/* If non-NULL, test only one function (-f) */
static char* test_fname = NULL;

/* Special case when only use fixed argument(s) (-1, -2, or -3) */
static int32_t has_arg[3] = {0,0,0};
static unsigned argval[3] = {0,0,0};

/* Use fixed weight for rating, and if so, what should it  be? (-r) */
static int32_t global_rating = 0;

/******************
 * Helper functions
 ******************/

/*
 * Signal - installs a signal handler
 */
typedef void handler_t(int32_t);

handler_t *Signal(int32_t signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	perror("Signal error");
    return (old_action.sa_handler);
}

/*
 * timeout_handler - SIGALARM hander
 */
sigjmp_buf envbuf;
void timeout_handler(int32_t sig) {
    siglongjmp(envbuf, 1);
}

/*
 * random_val - Return random int32_teger value between min and max
 */
static int32_t random_val(int32_t min, int32_t max)
{
    double weight = rand()/(double) RAND_MAX;
    int32_t result = min * (1-weight) + max * weight;
    return result;
}

/*
 * gen_vals - Generate the int32_teger values we'll use to test a function
 */
static int32_t gen_vals(int32_t test_vals[], int32_t min, int32_t max, int32_t test_range, int32_t arg)
{
    int32_t i;
    int32_t test_count = 0;

    /* Special case: If the user has specified a specific function
       argument using the -1, -2, or -3 flags, then simply use this
       argument and return */
    if (has_arg[arg]) {
	test_vals[0] = argval[arg];
	return 1;
    }

    /*
     * Special case: Generate test vals for floating point32_t functions
     * where the input argument is an unsigned bit-level
     * representation of a float. For this case we want to test the
     * regions around zero, the smallest normalized and largest
     * denormalized numbers, one, and the largest normalized number,
     * as well as inf and nan.
     */
    if ((min == 1 && max == 1)) {
	unsigned smallest_norm = 0x00800000;
	unsigned one = 0x3f800000;
	unsigned largest_norm = 0x7f000000;

	unsigned inf = 0x7f800000;
	unsigned nan =  0x7fc00000;
	unsigned sign = 0x80000000;

	/* Test range should be at most 1/2 the range of one exponent
	   value */
	if (test_range > (1 << 23)) {
	    test_range = 1 << 23;
	}

	/* Functions where the input argument is an unsigned bit-level
	   representation of a float. The number of tests generated
	   inside this loop body is the value k referenced in the
	   comment for the global variable MAX_TEST_VALS. */

	for (i = 0; i < test_range; i++) {
	    /* Denorms around zero */
	    test_vals[test_count++] = i;
	    test_vals[test_count++] = sign | i;

	    /* Region around norm to denorm transition */
	    test_vals[test_count++] = smallest_norm + i;
	    test_vals[test_count++] = smallest_norm - i;
	    test_vals[test_count++] = sign | (smallest_norm + i);
	    test_vals[test_count++] = sign | (smallest_norm - i);

	    /* Region around one */
	    test_vals[test_count++] = one + i;
	    test_vals[test_count++] = one - i;
	    test_vals[test_count++] = sign | (one + i);
	    test_vals[test_count++] = sign | (one - i);

	    /* Region below largest norm */
	    test_vals[test_count++] = largest_norm - i;
	    test_vals[test_count++] = sign | (largest_norm - i);
	}

	/* special vals */
	test_vals[test_count++] = inf;        /* inf */
	test_vals[test_count++] = sign | inf; /* -inf */
	test_vals[test_count++] = nan;        /* nan */
	test_vals[test_count++] = sign | nan; /* -nan */

	return test_count;
    }


    /*
     * Normal case: Generate test vals for int32_teger functions
     */

    /* If the range is small enough, then do exhaustively */
    if (max - MAX_TEST_VALS <= min) {
	for (i = min; i <= max; i++)
	    test_vals[test_count++] = i;
	return test_count;
    }

    /* Otherwise, need to sample.  Do so near the boundaries, around
       zero, and for some random cases. */
    for (i = 0; i < test_range; i++) {

	/* Test around the boundaries */
	test_vals[test_count++] = min + i;
	test_vals[test_count++] = max - i;

	/* If zero falls between min and max, then also test around zero */
	if (i >= min && i <= max)
	    test_vals[test_count++] = i;
	if (-i >= min && -i <= max)
	    test_vals[test_count++] = -i;

	/* Random case between min and max */
	test_vals[test_count++] = random_val(min, max);

    }
    return test_count;
}

/*
 * test_0_arg - Test a function with zero arguments
 */
static int32_t test_0_arg(funct_t f, funct_t ft, char *name)
{
    int32_t r = f();
    int32_t rt = ft();
    int32_t error =  (r != rt);

    if (error && !grade)
	printf("ERROR: Test %s() failed...\n...Gives %d[0x%x]. Should be %d[0x%x]\n", name, r, r, rt, rt);

    return error;
}

/*
 * test_1_arg - Test a function with one argument
 */
static int32_t test_1_arg(funct_t f, funct_t ft, int32_t arg1, char *name)
{
    funct1_t f1 = (funct1_t) f;
    funct1_t f1t = (funct1_t) ft;
    int32_t r, rt, error;

    r = f1(arg1);
    rt = f1t(arg1);
    error = (r != rt);
    if (error && !grade)
	printf("ERROR: Test %s(%d[0x%x]) failed...\n...Gives %d[0x%x]. Should be %d[0x%x]\n", name, arg1, arg1, r, r, rt, rt);

    return error;
}

/*
 * test_2_arg - Test a function with two arguments
 */
static int32_t test_2_arg(funct_t f, funct_t ft, int32_t arg1, int32_t arg2, char *name)
{
    funct2_t f2 = (funct2_t) f;
    funct2_t f2t = (funct2_t) ft;
    int32_t r = f2(arg1, arg2);
    int32_t rt = f2t(arg1, arg2);
    int32_t error = (r != rt);

    if (error && !grade)
	printf("ERROR: Test %s(%d[0x%x],%d[0x%x]) failed...\n...Gives %d[0x%x]. Should be %d[0x%x]\n", name, arg1, arg1, arg2, arg2, r, r, rt, rt);

    return error;
}

/*
 * test_3_arg - Test a function with three arguments
 */
static int32_t test_3_arg(funct_t f, funct_t ft,
		      int32_t arg1, int32_t arg2, int32_t arg3, char *name)
{
    funct3_t f3 = (funct3_t) f;
    funct3_t f3t = (funct3_t) ft;
    int32_t r = f3(arg1, arg2, arg3);
    int32_t rt = f3t(arg1, arg2, arg3);
    int32_t error = (r != rt);

    if (error && !grade)
	printf("ERROR: Test %s(%d[0x%x],%d[0x%x],%d[0x%x]) failed...\n...Gives %d[0x%x]. Should be %d[0x%x]\n", name, arg1, arg1, arg2, arg2, arg3, arg3, r, r, rt, rt);

    return error;
}

/*
 * test_function - Test a function.  Return number of errors
 */
static int32_t test_function(test_ptr t) {
    int32_t test_counts[3];    /* number of test values for each arg */
    int32_t args = t->args;    /* number of function arguments */
    int32_t arg_test_range[3]; /* test range for each argument */
    int32_t i, a1, a2, a3;
    int32_t errors = 0;

    /* These are the test values for each arg. Declared with the
       static attribute so that the array will be allocated in bss
       rather than the stack */
    static int32_t arg_test_vals[3][MAX_TEST_VALS];

    /* Sanity check on the number of args */
    if (args < 0 || args > 3) {
	printf("Configuration error: invalid number of args (%d) for function %s\n", args, t->name);
	exit(1);
    }

    /* Assign range of argument test vals so as to conserve the total
       number of tests, independent of the number of arguments */
    if (args == 1) {
	arg_test_range[0] = TEST_RANGE;
    }
    else if (args == 2) {
	arg_test_range[0] = pow((double)TEST_RANGE, 0.5);  /* sqrt */
	arg_test_range[1] = arg_test_range[0];
    }
    else {
	arg_test_range[0] = pow((double)TEST_RANGE, 0.333); /* cbrt */
	arg_test_range[1] = arg_test_range[0];
	arg_test_range[2] = arg_test_range[0];
    }

    /* Sanity check on the ranges */
    if (arg_test_range[0] < 1)
	arg_test_range[0] = 1;
    if (arg_test_range[1] < 1)
	arg_test_range[1] = 1;
    if (arg_test_range[2] < 1)
	arg_test_range[2] = 1;

    /* Create a test set for each argument */
    for (i = 0; i < args; i++) {
	test_counts[i] =  gen_vals(arg_test_vals[i],
				   t->arg_ranges[i][0], /* min */
				   t->arg_ranges[i][1], /* max */
				   arg_test_range[i],
				   i);

    }

    /* Handle timeouts in the test code */
    if (timeout_limit > 0) {
	int32_t rc;
	rc = sigsetjmp(envbuf, 1);
	if (rc) {
	    /* control will reach here if there is a timeout */
	    errors = 1;
	    printf("ERROR: Test %s failed.\n  Timed out after %d secs (probably infinite loop)\n", t->name, timeout_limit);
	    return errors;
	}
	alarm(timeout_limit);
    }


    /* Test function has no arguments */
    if (args == 0) {
	errors += test_0_arg(t->solution_funct, t->test_funct, t->name);
	return errors;
    }

    /*
     * Test function has at least one argument
     */

    /* Iterate over the values for first argument */

    for (a1 = 0; a1 < test_counts[0]; a1++) {
	if (args == 1) {
	    errors += test_1_arg(t->solution_funct,
				 t->test_funct,
				 arg_test_vals[0][a1],
				 t->name);

	    /* Stop testing if there is an error */
	    if (errors)
		return errors;
	}
	else {
	    /* if necessary, iterate over values for second argument */
	    for (a2 = 0; a2 < test_counts[1]; a2++) {
		if (args == 2) {
		    errors += test_2_arg(t->solution_funct,
					 t->test_funct,
					 arg_test_vals[0][a1],
					 arg_test_vals[1][a2],
					 t->name);

		    /* Stop testing if there is an error */
		    if (errors)
			return errors;
		}
		else {
		    /* if necessary, iterate over vals for third arg */
		    for (a3 = 0; a3 < test_counts[2]; a3++) {
			errors += test_3_arg(t->solution_funct,
					     t->test_funct,
					     arg_test_vals[0][a1],
					     arg_test_vals[1][a2],
					     arg_test_vals[2][a3],
					     t->name);

			/* Stop testing if there is an error */
			if (errors)
			    return errors;
		    } /* a3 */
		}
	    } /* a2 */
	}
    } /* a1 */


    return errors;
}

/*
 * run_tests - Run series of tests.  Return number of errors
 */
static int32_t run_tests()
{
    int32_t i;
    int32_t errors = 0;
    double point32_ts = 0.0;
    double max_point32_ts = 0.0;

    printf("Score\tRating\tErrors\tFunction\n");

    for (i = 0; test_set[i].solution_funct; i++) {
	int32_t terrors;
	double tscore;
	double tpoint32_ts;
	if (!test_fname || strcmp(test_set[i].name,test_fname) == 0) {
	    int32_t rating = global_rating ? global_rating : test_set[i].rating;
	    terrors = test_function(&test_set[i]);
	    errors += terrors;
	    tscore = terrors == 0 ? 1.0 : 0.0;
	    tpoint32_ts = rating * tscore;
	    point32_ts += tpoint32_ts;
	    max_point32_ts += rating;

	    if (grade || terrors < 1)
		printf(" %.0f\t%d\t%d\t%s\n",
		       tpoint32_ts, rating, terrors, test_set[i].name);

	}
    }

    printf("Total point32_ts: %.0f/%.0f\n", point32_ts, max_point32_ts);
    return errors;
}

/*
 * get_num_val - Extract hex/decimal/or float value from string
 */
static int32_t get_num_val(char *sval, unsigned *valp) {
    char *endp;

    /* See if it's an int32_teger or floating point32_t */
    int32_t ishex = 0;
    int32_t isfloat = 0;
    int32_t i;
    for (i = 0; sval[i]; i++) {
	switch (sval[i]) {
	case 'x':
	case 'X':
	    ishex = 1;
	    break;
	case 'e':
	case 'E':
	    if (!ishex)
		isfloat = 1;
	    break;
	case '.':
	    isfloat = 1;
	    break;
	default:
	    break;
	}
    }
    if (isfloat) {
	float fval = strtof(sval, &endp);
	if (!*endp) {
	    *valp = *(unsigned *) &fval;
	    return 1;
	}
	return 0;
    } else {
	long long int llval = strtoll(sval, &endp, 0);
	long long int upperbits = llval >> 31;
	/* will give -1 for negative, 0 or 1 for positive */
	if (!*valp && (upperbits == 0 || upperbits == -1 || upperbits == 1)) {
	    *valp = (unsigned) llval;
	    return 1;
	}
	return 0;
    }
}


/*
 * usage - Display usage info
 */
static void usage(char *cmd) {
    printf("Usage: %s [-hg] [-r <n>] [-f <name> [-1|-2|-3 <val>]*] [-T <time limit>]\n", cmd);
    printf("  -1 <val>  Specify first function argument\n");
    printf("  -2 <val>  Specify second function argument\n");
    printf("  -3 <val>  Specify third function argument\n");
    printf("  -f <name> Test only the named function\n");
    printf("  -g        Compact output for grading (with no error msgs)\n");
    printf("  -h        Print32_t this message\n");
    printf("  -r <n>    Give uniform weight of n for all problems\n");
    printf("  -T <lim>  Set timeout limit to lim\n");
    exit(1);
}


/**************
 * Main routine
 **************/

int32_t main(int32_t argc, char *argv[])
{
    int32_t errors;
    char c;

    /* parse command line args */
    while ((c = getopt(argc, argv, "hgf:r:T:1:2:3:")) != -1)
        switch (c) {
        case 'h': /* help */
	    usage(argv[0]);
	    break;
	case 'g': /* grading option for autograder */
	    grade = 1;
	    break;
	case 'f': /* test only one function */
	    test_fname = strdup(optarg);
	    break;
	case 'r': /* set global rating for each problem */
	    global_rating = atoi(optarg);
	    if (global_rating < 0)
		usage(argv[0]);
	    break;
	case '1': /* Get first argument */
	    has_arg[0] = get_num_val(optarg, &argval[0]);
	    if (!has_arg[0]) {
		printf("Bad argument '%s'\n", optarg);
		exit(0);
	    }
	    break;
	case '2': /* Get first argument */
	    has_arg[1] = get_num_val(optarg, &argval[1]);
	    if (!has_arg[1]) {
		printf("Bad argument '%s'\n", optarg);
		exit(0);
	    }
	    break;
	case '3': /* Get first argument */
	    has_arg[2] = get_num_val(optarg, &argval[2]);
	    if (!has_arg[2]) {
		printf("Bad argument '%s'\n", optarg);
		exit(0);
	    }
	    break;
	case 'T': /* Set timeout limit */
	    timeout_limit = atoi(optarg);
	    break;
	default:
	    usage(argv[0]);
	}

    if (timeout_limit > 0) {
	Signal(SIGALRM, timeout_handler);
    }

    /* test each function */
    errors = run_tests();

    return 0;
}
