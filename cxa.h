/*   __              
 *  / /___ _____ ___ 
 * / __/ // (_-</ _ \
 * \__/\_, /___/ .__/
 *    /___/   /_/    
 */

#ifndef CXA_CXA_H
#define CXA_CXA_H

/* Defines if a flag does take an argument, for example
 * if the -d flag is defined as CXA_ARG_YES, then its
 * usage must be -d "argument", else cxa will detect
 * it as an error
 */
#define CXA_ARG_NON 0
#define CXA_ARG_YES 1
#define CXA_ARG_MAY 2

/* This must be always included within the array of flags
 * since this works as a the delimiter
 */
#define CXA_FINAL_FLAG    {NULL, 0, 0}

/* none:       do not worry
 * non_sense:  the current argv element does not make quite sense
 * undef_flag: unknonwn flag found
 * */
enum cxa_fatal
{
    cxa_fatal_none,
    cxa_fatal_non_sense,
    cxa_fatal_undef_flag,
    cxa_fatal_unnecessary_arg,
    cxa_fatal_arg_expected
};

/* flagname: a long name for the flag (--document)
 * id:       simple character representation (-D)
 * q_arg:    argument option
 */
struct cxa_flag
{
    char          *flagname;
    char          id;
    unsigned char q_arg;
};

/* flag:      pointer to the original flag
 * argument:  given argument (if any)
 */
struct cxa_found
{
    struct        cxa_flag *flag;
    char          *argument;
};

/* p_args:    positional arguments
 * found:     found flags
 * last:      last flag used (in case of error)
 * argc:      position within argv
 * no_found:  number of flags found
 * no_p_args: number of positional arguments
 * fatal:     cause of error (if any)
 */
struct cxa_res
{
    char         **p_args;
    struct       cxa_found *found;
    struct       cxa_found *last;
    unsigned int argc;
    unsigned int no_found;
    unsigned int no_p_args;
    enum         cxa_fatal fatal;
};

extern const char *const cxa_errors[5];

struct cxa_res *cxa_parse (const unsigned int, char**, const struct cxa_flag*);
void cxa_clean (struct cxa_res*);

#endif
