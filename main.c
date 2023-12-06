#define _GNU_SOURCE
#define LOG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char *path;
    size_t row;
    size_t col;
} Location;

typedef enum {
    PREP_INCLUDE,
    PREP_DEFINE,
} TokenType;

typedef struct {
    char *txt;
    Location *loc;
} Token;

#define TOKEN_SIZE sizeof(Token)
#define MAX_TOKEN 64

typedef struct {
    Token *tokens[MAX_TOKEN];
    size_t count;
} Table;

void token_log(Token* t)
{
    if (t->loc == NULL) {
        printf("{ %s, (null) }\n", t->txt);
    } else printf("{ %s, { %s, %zu, %zu } }\n",
            t->txt, t->loc->path, t->loc->row, t->loc->col);
}

void table_log(Table* t)
{
    for (size_t tkcount = 0; tkcount < t->count; ++tkcount) {
        token_log(t->tokens[tkcount]);
    }
}

void alloc_error()
{
    printf("OS ERROR: can't allocate memory");
    exit(1);
}

const char* get_source_code(FILE *fd)
{
    char *source = (char*) malloc(1024);
    char *line = (char*) malloc(128);
    if (source == NULL || line == NULL) alloc_error();
    while (fgets(line, 128, fd) != NULL) strcat(source, line);
    free(line);
    return (const char*) source;
}

FILE* open_file(const char *file_path)
{
    FILE *fd = fopen(file_path, "r");
    if (fd == NULL) {
        printf("ERROR: cannot open file '%s'", file_path);
        exit(1);
    }
}

Token* create_token(const char *file_path, size_t row, size_t column)
{
    // Allocate memory for token
    Token *t = (Token*) malloc(sizeof(Token));
    t->loc = (Location*) malloc(sizeof(Location));
    if (t == NULL || t->loc == NULL) alloc_error();
    *t->loc = (Location) {(char*) file_path, row, column};
    return t;
}

int isvartype(char *text)
{
    if (strcmp(text, "int")) return 1;
    return 0;
}

#define CHP (source + c)
#define CH *(source + c)
#define INC(a, b) a++; b++;

Table* lex(const char *source, const char *file_path)
{
    Table *tbl = (Table*) malloc(sizeof(Table));
    if (tbl == NULL) alloc_error();
    size_t source_size = strlen(source);
    size_t row, col; 
    row = col = 1;
    size_t tkcount = 0;

    size_t c = 0;
    while (c < source_size) {
        if (CH == '\n') {
            col = 1;
            row++;
            c++;
        } else if (isspace(CH)) {
            INC(col, c);
        } else if (CH == '/' && *(CHP + 1) == '/') {
            while (CH != '\n') c++;
            col = 1;
        } else {
            char *text = (char*) malloc(64);
            if (text == NULL) alloc_error();
            size_t column = col;

            Token *t = create_token(file_path, row, column);
            tbl->tokens[tkcount++] = t;

            // Keyword or Identifier
            if (isalpha(CH)) {
                while (isalnum(CH)) {
                    strncat(text, CHP, 1);
                    INC(col, c);
                }
                t->txt = text;

            // Include literal
            } else if (CH == '<') {
                while (CH != '>') {
                    strncat(text, CHP, 1);
                    INC(col, c);
                }
                INC(col, c); // Skip '>'
                t->txt = text + sizeof(char);
                t->loc->col++;

            // String literal
            } else if (CH == '"') {
                INC(col, c); // Skip '"'
                while (CH != '"' || *(CHP - 1) == '\\') {
                    strncat(text, CHP, 1);
                    INC(col, c);
                }
                INC(col, c); // Skip '"'
                t->txt = text;
                t->loc->col++;

            // Numerical costant value
            } else if (isdigit(CH)) {
                while (isdigit(CH)) {
                    strncat(text, CHP, 1);
                    INC(col, c);
                }
                t->txt = text;

            // Symbols
            } else {
                *text = CH;
                t->txt = text;
                INC(col, c);

                // Preprocessor
                // if (*text == '#')
                //     while (isalpha(CH)) {
                //         strncat(t->txt, CHP, 1);
                //         INC(col, c);
                //     }
            }
        }
    }

    tbl->count = tkcount;
    return tbl;
}

int main()
{
    // TODO: recognize pointers, arrow symbols
    //       assign type for each token
    // TOCHECK: isvartype(), Preprocessor code
    Table* table;
    const char *file_path = "sources/hello_world.c";
    FILE *fd = open_file(file_path);
    const char *source = get_source_code(fd);
    fclose(fd);

    table = lex(source, file_path);
#ifdef LOG
    printf("Table size = %zu\n", table->count);
    table_log(table);
#endif

    return 0;
}
