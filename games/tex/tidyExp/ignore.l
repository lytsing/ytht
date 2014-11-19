%{
#include <stdio.h>
%}

/* Prevent the need for linking with -lfl */
/*%option noyywrap*/
%x SCRIPT NOUSEBLOCK

%%
<INITIAL>"<script"       {BEGIN(SCRIPT);}
<INITIAL>"<!--"          {BEGIN(NOUSEBLOCK);}
<INITIAL>.              {printf("%s", yytext);}

<SCRIPT>{
"<\/script>"      {BEGIN(INITIAL);}
.               {}
}

<NOUSEBLOCK>{
"-->"      {BEGIN(INITIAL);}
.               {}
}

%%

void main()
{

        yylex(); /* start the analysis*/
}

int yywrap()
{
        return 1;
}
