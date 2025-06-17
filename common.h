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

#define	CHECK_PTR(ptr)	do { if (ptr) break; err(EXIT_FAILURE, "cannot continue"); } while (0)


struct sheet
{
	char           *src;
	size_t         length;
	unsigned short rows, cols;
};

#endif
