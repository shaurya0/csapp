/*
 * CS:APP Data Lab
 */

/* Declare different function types */
typedef int32_t (*funct_t) (void);
typedef int32_t (*funct1_t)(int32_t);
typedef int32_t (*funct2_t)(int32_t, int32_t);
typedef int32_t (*funct3_t)(int32_t, int32_t, int32_t);

/* Combine all the information about a function and its tests as structure */
typedef struct {
    char *name;             /* String name */
    funct_t solution_funct; /* Function */
    funct_t test_funct;     /* Test function */
    int32_t args;               /* Number of function arguments */
    char *ops;              /* List of legal operators. Special case: "$" for floating point32_t */
    int32_t op_limit;           /* Max number of ops allowed in solution */
    int32_t rating;             /* Problem rating (1 -- 4) */
    int32_t arg_ranges[3][2];   /* Argument ranges. Always defined for 3 args, even if */
                            /* the function takes fewer. Special case: First arg */
			    /* must be set to {1,1} for f.p. puzzles */
} test_rec, *test_ptr;

extern test_rec test_set[];







