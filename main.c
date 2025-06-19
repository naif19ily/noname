#include "common.h"
#include <errno.h>

#define MAX(a, b)    ((a) > (b) ? (a) : (b))

static void usage (void);
static void read_file (const char*, struct sheet*);

static void get_sheet_dimensions (struct sheet*);
static void process_content (struct sheet*);

static void process_raw_number (struct token*, size_t*, uint16_t*);
static void process_raw_string (struct token*, size_t*, uint16_t*);

int main (int argc, char **argv)
{
	if (argc == 1) { usage(); return 0; }

	struct sheet sheet = {0};

	read_file(argv[1], &sheet);
	get_sheet_dimensions(&sheet);

	sheet.grid = (struct cell*) calloc(sheet.rows * sheet.cols, sizeof(*sheet.grid));
	CHECK_PTR(sheet.grid);

	process_content(&sheet);
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

static void get_sheet_dimensions (struct sheet *sheet)
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

static void process_content (struct sheet *sheet)
{
	struct cell *thc = &sheet->grid[0];
	uint16_t offset = 0, numberline = 1;

	for (size_t i = 0; i < sheet->size; i++)
	{
		struct token *tht = &thc->stream[thc->nth_t];

		/* Meta information is useful for error handling, see
		 * an example in report_token function
		 */
		tht->meta.context    = sheet->source + i;
		tht->meta.numberline = numberline;
		tht->meta.offset     = offset;

		switch (sheet->source[i])
		{
			case ' ': case '\t': { offset++; continue; }
			case '|':            { thc++; offset++; continue; }
			case '\n':           { thc = &sheet->grid[numberline++ * sheet->rows]; offset = 0; continue; }

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			{
				process_raw_number(tht, &i, &offset);
				break;
			}

			case '"':
			{
				process_raw_string(tht, &i, &offset);
				break;
			}
		}
	}
}

static void process_raw_number (struct token *tht, size_t *aka_i, uint16_t *offset)
{
	char *ends;
	tht->as.number = strtold(tht->meta.context, &ends);

	size_t dx = ends - tht->meta.context;

	tht->type = TT_NUMBER;
	tht->meta.length = dx;

	*aka_i  += dx - 1;
	*offset += (uint16_t) dx;

	printf("(%d %d): %Lf\n", tht->meta.numberline, tht->meta.offset, tht->as.number);
}

static void process_raw_string (struct token *tht, size_t *aka_i, uint16_t *offset)
{
	size_t dx = 1;

	while (tht->meta.context[dx] != '"')
	{
		if (tht->meta.context[dx++] == 0)
		{
			/* TODO: inform */
		}
	}

	*aka_i += dx;
	*offset += (uint16_t) dx;

	tht->meta.length = (uint16_t) dx + 1;
	tht->type = TT_STRING;

	printf("(%d %d): %.*s\n", tht->meta.numberline, tht->meta.offset, tht->meta.length - 2, tht->meta.context + 1);
}