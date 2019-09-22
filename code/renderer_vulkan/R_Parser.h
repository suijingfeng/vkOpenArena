#ifndef R_PARSER_H_
#define R_PARSER_H_

char* R_ParseExt(char** data_p, qboolean allowLineBreaks);
int R_Compress( char *data_p );
int R_GetCurrentParseLine( void );
void R_BeginParseSession(const char* const name);
qboolean R_SkipBracedSection(char ** program, int depth);
void R_SkipRestOfLine ( char **data );

#endif
