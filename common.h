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
	CT_NUMBER = 2,
	CT_STRING = 3,
	CT_ERROR  = 4,
};

struct token
{
	enum token_type type;
};

struct cell
{
	enum cell_type type;
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