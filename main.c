/*   __              
 *  / /___ _____ ___ 
 * / __/ // (_-</ _ \
 * \__/\_, /___/ .__/
 *    /___/   /_/    
 */
#include "common.h"

 // TODO: improve error handling and usage and stuff u know

static char* read_file (const char*, size_t*);
static void calc_dimens (struct sheet*);

static void breakdown_sheet (struct sheet*);
static void lex_number (const char*, size_t*, struct token*);

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

	sheet.grid = (struct cell*) calloc(sheet.rows * sheet.cols, sizeof(*sheet.grid));
	CHECK_PTR(sheet.grid);

	breakdown_sheet(&sheet);

	free(sheet.src);
	free(sheet.grid);

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

static void breakdown_sheet (struct sheet *sheet)
{
	unsigned short row = 0;
	signed short offset = 0;

	struct cell *cc = &sheet->grid[0];

	for (size_t i = 0; i < sheet->length; i++, offset++)
	{
		const char this = sheet->src[i];

		if (this == ' ' || this == '\t') { continue; }
		if (this == '|')                 { cc++; continue; }
		if (this == '\n')                { cc = &sheet->grid[++row *  sheet->rows]; offset = -1; continue; }

		if (cc->nth_t == MAXTPCELL)
		{
			/* TODO: error */
		}

		struct token *T =  &cc->stream[cc->nth_t++];
		T->context = sheet->src + i;
		T->numline = row + 1;
		T->offset  = (unsigned short) offset;

		switch (this)
		{
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
				lex_number(sheet->src, &i, T);
				break;
			}
		}

	}
}

static void lex_number (const char *src, size_t *aka_i, struct token *T)
{
	char *ends;
	T->as.number = strtold(src + *aka_i, &ends);
	T->kind = token_is_number;

	printf("token: <%d>: %Lf on %d at %d\n", T->kind, T->as.number, T->numline, T->offset);
}
