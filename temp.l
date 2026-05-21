%{
#include "y.tab.h"
#include <stdlib.h>
#include <string.h>

int line_number = 1;
/*פונקציית שגיאה */
void lex_error(const char* msg) {
    printf("Lexical Error: %s at text '%s' on line %d\n", msg, yytext, line_number);
    exit(1);
}
%}

/*ביטויים רגולריים*/
DIGIT       [0-9]
HEX_DIGIT   [0-9a-fA-F]
ALPHA       [a-zA-Z]
ALPHANUM    [a-zA-Z0-9_]
  
DECIMAL     0|[1-9]{DIGIT}*
HEX         0[xX]{HEX_DIGIT}+
REAL        ({DIGIT}+\.{DIGIT}*|\.{DIGIT}+)([eE][-+]?{DIGIT}+)?|{DIGIT}+[eE][-+]?{DIGIT}+
IDENTIFIER  {ALPHA}{ALPHANUM}*

%%

[ \t\r]+                    { /* התעלמות מרווחים וטאבים */ }
\n                          { line_number++;  }
\/\%([^\%]*|\%+[^/])*\%\/   { /* היערות */ 
                                for(int i = 0; yytext[i] != '\0'; i++) {
                                    if(yytext[i] == '\n') line_number++;
                                } }

 /*  מילות מפתח  */
"bool"          { return BOOL; }
"char"          { return CHAR; }
"int"           { return INT; }
"real"          { return REAL; }
"string"        { return TSTRING; }
"int*"          { return INT_PTR; }
"char*"         { return CHAR_PTR; }
"real*"         { return REAL_PTR; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"for"           { return FOR; }
"var"           { return VAR; }
"func"          { return FUNC; }
"proc"          { return PROC; }
"return"        { return RETURN; }
"null"          { return TOK_NULL; }
"true"          { return TOK_TRUE; }
"false"         { return TOK_FALSE; }

    /*  אופרטורים  */
"&&"            { return AND; }
"||"            { return OR; }
"=="            { return EQ; }
"!="            { return NEQ; }
">="            { return GE; }
"<="            { return LE; }
">"             { return GT; }
"<"             { return LT; }
"!"             { return NOT; }
"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return MULT; }
"/"             { return DIV; }
"="             { return ASSIGN; }
"&"             { return ADDRESS; }
"^"             { return DEREF; }
"|"             { return LENGTH; }

";"             { return ';'; }
":"             { return ':'; }
","             { return ','; }
"{"             { return '{'; }
"}"             { return '}'; }
"("             { return '('; }
")"             { return ')'; }
"["             { return '['; }
"]"             { return ']'; }


{DECIMAL}       { yylval.ival = atoi(yytext); return LIT_INT; }
{HEX}           { yylval.ival = strtol(yytext, NULL, 16); return LIT_INT; }
{REAL}          { yylval.rval = atof(yytext); return LIT_REAL; }

'([^'\n\\]|\\.)' { yylval.cval = yytext[1]; return LIT_CHAR; }

\"([^\"\n\\]|\\.)*\" { 
    yylval.sval = strdup(yytext); 
    return LIT_STRING; 
}
{IDENTIFIER}    { yylval.sval = strdup(yytext); return IDENTIFIER; }

.               { lex_error("Unknown character"); }

%%