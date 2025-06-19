#ifndef _COMMON_H 
#define _COMMON_H 

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define CHECK_PTR(ptr)                                                             \
	do {                                                                           \
		if ((ptr) == NULL) {                                                       \
			fprintf(stderr, "\n  Error: Memory allocation failed: aborting\n\n");  \
			exit(EXIT_FAILURE);                                                    \
		}                                                                          \
	} while (0)

#define MAXTOKNCAP    32

enum token_type
{
	TT_UNKNOWN = -1,
	TT_NUMBER  = 0,
	TT_STRING  = '"',
	TT_CTEREF  = '@',
	TT_VARREF  = '$',
};

enum cell_type
{
	CT_EMPTY  = 0,
	CT_NUMBER = 1,
	CT_STRING = 2,
	CT_ERROR  = 3,
};

struct cell;

struct token
{
	struct
	{
		char     *context;
		uint16_t numberline;
		uint16_t offset;
		uint16_t length;
	} meta;

	union
	{
		/* Since we do not have string methods, whenever a token
		 * is a string, we can access its content by accessing
		 * meta.context + 1 and the length of that string is going to
		 * be meta.length - 1 (avoiding the quotes)
		 */
		long double  number;
		struct cell *ref;
	} as;
	
	enum token_type type;
};

struct cell
{
	struct token   stream[MAXTOKNCAP];
	enum cell_type type;
	uint16_t       nth_t;
};

struct sheet
{
	struct cell *grid;
	char        *filename;
	char        *source;
	size_t      size;
	uint16_t    rows, cols;
};

#endif