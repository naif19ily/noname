/*   __              
 *  / /___ _____ ___ 
 * / __/ // (_-</ _ \
 * \__/\_, /___/ .__/
 *    /___/   /_/    
 */

#include "cxa.h"

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#define macro_check_pointer(p)  do { if (p) break; err(EXIT_FAILURE, "cannot alloc"); } while (0)

const char *const cxa_errors[5] = {
    "",
    "element within argv does not make sense",
    "flag found was not defined as a program's option",
    "giving argument to a flag which already has its own",
    "flag was expecting an argument but none was given"
};

/* Attemps to classify the type of the current
 * element within argv
 */
enum argvs_kind
{
    argvs_non_sense,
    argvs_argument,
    argvs_long_flag,
    argvs_shrt_flag,
    argvs_term_flag,
};

/* Whenever `--` is found, it means there is not more flags, whatever
 * comes next must be trated as an optional argument
 */
static unsigned char __op_args_only = 0;

/* Stores the lengths of all flags in order to avoid re-computation
 * when `handle_long_flag` is called
 */
static unsigned int *__flag_lengths = NULL;

static struct cxa_res *make_res (void);
static void* realloc_q (const unsigned int, unsigned int*, void**, const size_t);

static void compute_lengths (const struct cxa_flag*);
static enum argvs_kind determinate_kind (const char*);

static enum cxa_fatal handle_long_flag (const char*, const struct cxa_flag*, struct cxa_found*);
static enum cxa_fatal handle_shrt_flag (const char, const struct cxa_flag*, struct cxa_found*);

struct cxa_res *cxa_parse (const unsigned int argc, char **argv, const struct cxa_flag *flags)
{
    struct cxa_res *res = make_res();
    compute_lengths(flags);

    unsigned int found_cap = 16, pargs_cap = 16;

    for (; (res->argc < argc) && (res->fatal == cxa_fatal_none); res->argc++)
    {
        if (__op_args_only == 1)
        {
            realloc_q(res->no_p_args, &pargs_cap, (void*) &res->p_args, sizeof(*res->p_args));
            res->p_args[res->no_p_args++] = argv[res->argc];
            continue;
        }

        const char *_argv = argv[res->argc];
        switch (determinate_kind(_argv))
        {
            case argvs_non_sense:
                res->fatal = cxa_fatal_non_sense;
                break;

            case argvs_long_flag:
                if (res->last != NULL && res->last->argument == NULL && res->last->flag->q_arg == CXA_ARG_YES)
                {
                    res->fatal = cxa_fatal_arg_expected;
                    break;
                }
                realloc_q(res->no_found, &found_cap, (void*) &res->found, sizeof(*res->found));
                res->last  = &res->found[res->no_found++];
                res->fatal = handle_long_flag(_argv + 2, flags, res->last);
                break;

            case argvs_shrt_flag:
                if (res->last != NULL && res->last->argument == NULL && res->last->flag->q_arg == CXA_ARG_YES)
                {
                    res->fatal = cxa_fatal_arg_expected;
                    break;
                }
                realloc_q(res->no_found, &found_cap, (void*) &res->found, sizeof(*res->found));
                res->last  = &res->found[res->no_found++];
                res->fatal = handle_shrt_flag(_argv[1], flags, res->last);
                break;

            case argvs_term_flag:
                __op_args_only = 1;
                break;

            case argvs_argument:
                if (res->last != NULL && res->last->argument == NULL && res->last->flag->q_arg != CXA_ARG_NON) res->last->argument = (char*) _argv;
                else res->fatal = cxa_fatal_unnecessary_arg;
                break;
        }
    }

    if (res->last && !res->last->argument && res->last->flag && res->last->flag->q_arg == CXA_ARG_YES) res->fatal = cxa_fatal_arg_expected;
    res->argc--;

    free(__flag_lengths);
    return res;
}

void cxa_clean (struct cxa_res *res)
{
    free(res->p_args);
    free(res->found);
    free(res);
}

static struct cxa_res *make_res (void)
{
    struct cxa_res *res = (struct cxa_res*) calloc(1, sizeof(struct cxa_res));
    macro_check_pointer(res);

    res->p_args = (char**) calloc(16, sizeof(char**));
    macro_check_pointer(res->p_args);

    res->found = (struct cxa_found*) calloc(16, sizeof(struct cxa_found));
    macro_check_pointer(res->found);

    res->last      = NULL;
    res->argc      = 1;
    res->no_found  = 0;
    res->no_p_args = 0;
    res->fatal     = cxa_fatal_none;

    return res;
}

static void* realloc_q (const unsigned int at, unsigned int *cap, void **arr, const size_t sz)
{
    if (at < *cap) return *arr;
    *cap *= 2;
    *arr = realloc(*arr, sz * *cap);
    return *arr;
}

static void compute_lengths (const struct cxa_flag *flags)
{
    unsigned int nf = 0;
    while (flags[nf].flagname != NULL) nf++;

    __flag_lengths = (unsigned int*) calloc(nf, sizeof(unsigned int));
    for (unsigned int i = 0; i < nf; i++)
    {
        __flag_lengths[i] = (unsigned int) strlen(flags[i].flagname);
    }
    macro_check_pointer(__flag_lengths);
}

static enum argvs_kind determinate_kind (const char *argv)
{
    if ((*argv == '-') && (argv[1] == '-'))
    {
        if (argv[2] == '\0') return argvs_term_flag;
        return isalnum(argv[2]) ? argvs_long_flag : argvs_non_sense;
    }
    if (*argv == '-')
    {
        return isalnum(argv[1]) ? argvs_shrt_flag : argvs_non_sense;
    }
    return argvs_argument;
}

static enum cxa_fatal handle_long_flag (const char *argv, const struct cxa_flag *flags, struct cxa_found *_flag)
{
    unsigned int eqs = 0, len = 0;
    for (; argv[len] != '\0'; len++) { if (argv[len] == '=') { eqs = len; } }

    const unsigned int thslen = (eqs == 0) ? len : eqs;
    struct cxa_flag *flag   = NULL;
    unsigned char found       = 0;

    for (unsigned int i = 0; (flags[i].flagname != NULL) && (found == 0); i++)
    {
        flag = (struct cxa_flag*) &flags[i];
        const unsigned int n = __flag_lengths[i] > thslen ? __flag_lengths[i] : thslen;
        if (strncmp(argv, flag->flagname, n) == 0) { found = 1; }
    }

    if (found == 0) return cxa_fatal_undef_flag;

    _flag->flag     = flag;
    _flag->argument = (eqs == 0) ? NULL : (char*) (argv + eqs + 1);
    return cxa_fatal_none;
}

static enum cxa_fatal handle_shrt_flag (const char id, const struct cxa_flag *flags, struct cxa_found *_flag)
{
    struct cxa_flag *flag = NULL;
    unsigned char found     = 0;

    for (unsigned int i = 0; (flags[i].flagname != NULL) && (found == 0); i++)
    {
        flag = (struct cxa_flag*) &flags[i];
        if (flags[i].id == id) { found = 1; }
    }

    if (found == 0) return cxa_fatal_undef_flag;

    _flag->flag     = flag;
    _flag->argument = NULL;
    return cxa_fatal_none;
}

