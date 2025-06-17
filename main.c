/*   __              
 *  / /___ _____ ___ 
 * / __/ // (_-</ _ \
 * \__/\_, /___/ .__/
 *    /___/   /_/    
 */
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#define	CHECK_PTR(ptr)	do { if (ptr) break; err(EXIT_FAILURE, "cannot continue"); } while (0)

static char* read_file (const char*, size_t*);

int main (int argc, char **argv)
{
	if (argc == 1)
	{
		errx(EXIT_SUCCESS, "usage: tysp <filename>");
	}

	size_t length;
	char *src = read_file(argv[1], &length);

	return EXIT_SUCCESS;
}

static char* read_file (const char *filename, size_t *length)
{
	FILE *file = fopen(filename, "r");
	if (!file)
	{
		err(EXIT_FAILURE, "cannot open file");
	}

	fseek(file, 0, SEEK_END);
	*length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *src = (char*) calloc(*length + 1, sizeof(char));
	CHECK_PTR(src);

	const size_t read = fread(src, 1, *length, file);
	if (read != *length)
	{
		errx(EXIT_FAILURE, "only %ld bytes were read out of %ld", read, *length);
	}

	return src;
}
