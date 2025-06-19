#include "common.h"

#define MAX(a, b)    ((a) > (b) ? (a) : (b))

static void usage (void);
static void read_file (const char*, struct sheet*);

static void process_sheet (struct sheet*);

int main (int argc, char **argv)
{
	if (argc == 1) { usage(); return 0; }

	struct sheet sheet = {0};

	read_file(argv[1], &sheet);
	process_sheet(&sheet);

	sheet.grid = (struct cell*) calloc(sheet.rows * sheet.cols, sizeof(*sheet.grid));
	CHECK_PTR(sheet.grid);

	return 0;
}

static void usage (void)
{
	printf("\n  Usage: tysp <filename>\n");
	printf("    filename: The name of the file to process.\n\n");
}

static void read_file (const char *filename, struct sheet *sheet)
{
	sheet->filename = (char*) filename;
	FILE *file = fopen(filename, "r");

	if (!file)
	{
		fprintf(stderr, "\n  Error: Could not open file '%s'.\n\n", filename);
		exit(EXIT_FAILURE);
	}

	fseek(file, 0, SEEK_END);
	sheet->size = ftell(file);
	fseek(file, 0, SEEK_SET);

	sheet->source = (char*) calloc(sheet->size + 1, sizeof(*sheet->source));
	CHECK_PTR(sheet->source);

	const size_t read = fread(sheet->source, 1, sheet->size, file);
	if (read != sheet->size)
	{
		fprintf(stderr, "\n  Error: Only %ld B out of %ld B were read.\n\n", read, sheet->size);
		exit(EXIT_FAILURE);
	}

	fclose(file);
}

static void process_sheet (struct sheet *sheet)
{
	bool instr = false;
	uint16_t cols = 0;

	for (size_t i = 0; i < sheet->size; i++)
	{
		switch (sheet->source[i])
		{
			case '"':
			{
				instr = !instr;
				break;
			}
			case '|':
			{
				if (instr == false) cols++;
				break;
			}
			case '\n':
			{
				sheet->cols = MAX(sheet->cols, cols);
				cols = 0;
				sheet->rows++;
				break;
			}
		}
	}
	sheet->cols = MAX(sheet->cols, cols);
}