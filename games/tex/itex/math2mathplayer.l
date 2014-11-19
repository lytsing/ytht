%{
#include <stdio.h>
%}

/* Prevent the need for linking with -lfl */
/*%option noyywrap*/
%x MATHML

%%
<INITIAL>"<math "	{printf("<m:math ");BEGIN(MATHML);}
<INITIAL>.		{printf("%s", yytext);}

<MATHML>{
"<\/math>"	{printf("</m:math>");BEGIN(INITIAL);}
"<\/"		{printf("</m:");}
"<"		{printf("<m:");}
.		{printf("%s", yytext);}
}

%%
	
int main()
{

	yylex(); /* start the analysis*/
}

int yywrap()
{
	return 1;
}

