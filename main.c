/*   __              
 *  / /___ _____ ___ 
 * / __/ // (_-</ _ \
 * \__/\_, /___/ .__/
 *    /___/   /_/    
 */
#include "common.h"

static char* read_file (const char*, size_t*);
static void calc_dimens (struct sheet*);

int main (int argc, char **argv)
{
	if (argc == 1)
	{
		errx(EXIT_SUCCESS, "usage: tysp <filename>");
	}

	struct sheet sheet =
	{
		.length = 0,
		.src = read_file(argv[1], &sheet.length)
	};
	calc_dimens(&sheet);

	printf("%d %d\n", sheet.rows, sheet.cols);

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

static void calc_dimens (struct sheet *sheet)
{
	unsigned short cols = 0;
	bool instr = false;

	for (size_t i = 0; i < sheet->length; i++)
	{
		switch (sheet->src[i])
		{
			case '"': instr = !instr; break;
			case '|': if (!instr) cols++; break;
			case '\n':
				sheet->rows++;
				sheet->cols = (sheet->cols > cols) ? sheet->cols : cols;
				cols = 0;
				break;
		}
	}
	sheet->cols = (sheet->cols > cols) ? sheet->cols : cols;
}
