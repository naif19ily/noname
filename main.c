#include "common.h"
#include <errno.h>
#include <ctype.h>
#include <string.h>

#define MAX(a, b)    ((a) > (b) ? (a) : (b))

enum cell_error_type
{
	CET_BOUNDS = 0,      /* Reference given is out of table's bounds */
	CET_INFSTR = 1,      /* String is not terminated                 */
	CET_TOOTOK = 2,      /* Cell has more than MAXTOKCAP tokens      */
	CET_NSENSE = 3,      /* Cell's content does not make sense       */
};

static void usage (void);
static void read_file (const char*, struct sheet*);

static void get_sheet_dimensions (struct sheet*);
static void process_content (struct sheet*);

static void process_raw_number (struct token*, size_t*, uint16_t*);
static bool process_raw_string (struct token*, size_t*, uint16_t*);

static bool process_raw_reference (struct token*, size_t*, uint16_t*, const enum token_type, const uint16_t, const uint16_t);
static void set_cell_to_error (struct cell*, const enum cell_error_type);

static void make_sense_of (struct sheet*);
static void finally_print (struct sheet*);

int main (int argc, char **argv)
{
	if (argc == 1) { usage(); return 0; }

	struct sheet sheet = {0};

	read_file(argv[1], &sheet);
	get_sheet_dimensions(&sheet);

	/* Creating sheet structure */
	sheet.ncells = sheet.rows * sheet.cols;
	sheet.grid = (struct cell*) calloc(sheet.ncells, sizeof(*sheet.grid));
	CHECK_PTR(sheet.grid);

	/* Setting default width but all columns */
	sheet.widths = (uint16_t*) calloc(sheet.cols, sizeof(*sheet.widths));	
	CHECK_PTR(sheet.widths);
	for (uint16_t i = 0; i < sheet.cols; i++) { sheet.widths[i] = DEFAULTWIDTH; }

	process_content(&sheet);
	make_sense_of(&sheet);

	finally_print(&sheet);
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
		const char ch = sheet->source[i];

		if (ch == ' ' || ch == '\t') { offset++; continue; }
		if (ch == '|')               { thc++; offset++; continue; }
		if (ch == '\n')              { thc = &sheet->grid[numberline++ * sheet->rows]; offset = 0; continue; }
		if (thc->type == CT_ERROR)   { offset++; continue; }

		struct token *tht = &thc->stream[thc->nth_t];

		tht->meta.context    = sheet->source + i;
		tht->meta.numberline = numberline;
		tht->meta.offset     = offset;

		switch (ch)
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
				process_raw_number(tht, &i, &offset);
				break;
			}

			case '"':
			{
				const bool success = process_raw_string(tht, &i, &offset);
				if (success == false) { set_cell_to_error(thc, CET_INFSTR); }
				break;
			}

			case '@':
			case '$':
			{
				const bool success = process_raw_reference(tht, &i, &offset, (enum token_type) sheet->source[i], sheet->rows, sheet->cols);
				if (success == false) { set_cell_to_error(thc, CET_BOUNDS); }
				break;
			}

			case '-':
			{
				if (isdigit(sheet->source[i + 1])) { process_raw_number(tht, &i, &offset); break; }
				else
				{
					tht->type = (enum token_type) ch;
					tht->meta.length = 1;
					offset++;
					break;
				}
			}

			case '+':
			case '*':
			case '/':
			case '(':
			case ')':
			case '=':
			case '^':
			case 'v':
			case '<':
			case '>':
			{
				tht->type = (enum token_type) ch;
				tht->meta.length = 1;
				offset++;
				break;
			}
		}

		if (++thc->nth_t == MAXTOKNCAP) { set_cell_to_error(thc, CET_TOOTOK); }
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
}

static bool process_raw_string (struct token *tht, size_t *aka_i, uint16_t *offset)
{
	size_t dx = 1;

	while (tht->meta.context[dx] != '"')
	{
		if (tht->meta.context[dx++] == 0)
		{
			return false;
		}
	}

	*aka_i += dx;
	*offset += (uint16_t) dx + 1;

	tht->meta.length = (uint16_t) dx + 1;
	tht->type = TT_STRING;

	return true;
}

static bool process_raw_reference (struct token *tht, size_t *aka_i, uint16_t *offset, const enum token_type type, const uint16_t rows, const uint16_t cols)
{
	size_t dx = 1;
	char *aux = tht->meta.context + 1;

	if (!isalpha(*aux))   { goto get_row; }
	while (isalpha(*aux)) { aux++; dx++; }

	bool less25 = true;
	aux--;

	do {
		if (less25 == true) tht->as.ref.col = tolower(*aux) - 'a' + 1;
		else tht->as.ref.col += 26 * (tolower(*aux) - 'a' + 1);
		aux--;

		less25 = false;
	} while (*aux != (char) type);

	/* Subtract one from column since it starts counting from 1, we
	 * want to start from 0
	 */
	tht->as.ref.col--;
	aux += dx;

get_row:
	if (!isdigit(*aux)) { goto fini; }

	char *ends;
	tht->as.ref.row = (uint16_t) strtol(aux, &ends, 10);

	dx += ends - aux;

fini:
	*aka_i += dx - 1;
	*offset += (uint16_t) dx;
	tht->type = type;

	return ((tht->as.ref.row < rows) && (tht->as.ref.col < cols));
}

static void set_cell_to_error (struct cell *thc, const enum cell_error_type type)
{
	static const char *const errors[] =
	{
		"BOUNDS!",
		"INFSTR!",
		"TOOTOK!",
		"NSENSE!",
	};

	thc->as.text.text   = errors[type];
	thc->as.text.length = 7;

	thc->type = CT_ERROR;
}

static void make_sense_of (struct sheet *sheet)
{
	uint16_t rvis = 0, cvis = 0, r_offset = 0;
	struct cell *thc = &sheet->grid[0];

	while (rvis < sheet->rows)
	{
		if (cvis == sheet->cols)
		{
			thc->fin_col = true;
			cvis = 0;
			r_offset = ++rvis * sheet->rows;
			continue;
		}

		uint16_t *ccwidth = &sheet->widths[cvis], thw = 0;

		thc = &sheet->grid[r_offset + cvis++];
		if (thc->nth_t == 0) { continue; }

		struct token *head = &thc->stream[0];
		switch (head->type)
		{
			case TT_NUMBER:
			{	
				thc->as.number = head->as.number;
				thc->type = CT_NUMBER;

				thw = head->meta.length;
				break;
			}
			case TT_STRING:
			{
				thc->as.text.text  = head->meta.context + 1;
				thc->as.text.length = head->meta.length - 2;

				thc->type = CT_TEXT;
				thw = thc->as.text.length;
				break;
			}

			default:
			{

			}
		}
		*ccwidth = MAX(*ccwidth, thw);
	}
}

static void finally_print (struct sheet *sheet)
{
	uint16_t nthcol = 0;

	for (uint16_t i = 0; i < sheet->ncells; i++)
	{
		struct cell *thc = &sheet->grid[i];
		const uint16_t width = sheet->widths[nthcol++];

		switch (thc->type)
		{
			case CT_NUMBER:
			{
				printf("%*.1Lf ", width, thc->as.number);
				break;
			}
			case CT_TEXT:
			{
				printf("%-*.*s ", width, thc->as.text.length, thc->as.text.text);
				break;
			}
			case CT_EMPTY:
			{
				printf("%-*c ", width, ' ');
				break;
			}
		}

		if (thc->fin_col == true) { putchar('\n'); nthcol = 0; }
	}
}

// SPECIFIC NUMBER OF DECIMALS