/*   __              
 *  / /___ _____ ___ 
 * / __/ // (_-</ _ \
 * \__/\_, /___/ .__/
 *    /___/   /_/    
 */
#ifndef _COMMON_H 
#define _COMMON_H 

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define	CHECK_PTR(ptr)  do { if (ptr) break; err(EXIT_FAILURE, "cannot continue"); } while (0)
#define MAXTPCELL       128

enum token_kind
{
	token_is_reference = '@',
	token_is_string    = '"',
	token_is_number    = 256,
};

struct token
{
	union
	{
		long double number;
	} as;
	char            *context;
	unsigned short  numline, offset;
	unsigned short  length;
	enum token_kind kind;
};

struct cell
{
	struct token   stream[MAXTPCELL];
	char           *src;
	unsigned short nth_t;
};

struct sheet
{
	struct cell    *grid;
	char           *src;
	size_t         length;
	unsigned short rows, cols;
};

#endif
