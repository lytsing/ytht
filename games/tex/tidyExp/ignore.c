%{
#include <stdio.h>
%}

/* Prevent the need for linking with -lfl */
/*%option noyywrap*/
%option caseless
%x SCRIPT IFRAME STYLE OBJECT NOUSE

%%
<INITIAL>"<script"[^>]*">"|"<SCRIPT"[^>]*">"    {BEGIN(SCRIPT);}
<INITIAL>"<IFRAME"[^>]*">"|"<iframe"[^>]*">"    {BEGIN(IFRAME);}
<INITIAL>"<STYLE"[^>]*">"|"<style"[^>]*">"      {BEGIN(STYLE);}
<INITIAL>"<!--"					{BEGIN(NOUSE);}
<INITIAL>"<OBJECT"[^>]*">"|"<object"[^>]*">"    {BEGIN(OBJECT);}
<INITIAL>"<body"[^>]*">"|"<BODY"[^>]*">"   {printf("<body>");};
<INITIAL>"<div"[^>]*">"|"<DIV"[^>]*">"        {;}
<INITIAL>"<\/div"[^>]*">"|"<\/DIV"[^>]*">"    {;}
<INITIAL>"<form"[^>]*">"|"<FORM"[^>]*">"         {;}
<INITIAL>"<\/form"[^>]*">"|"<\/FORM"[^>]*">"    {;}
<INITIAL>"<table"[^>]*">"|"<TABLE"[^>]*">"   {printf("<table>");};
<INITIAL>"<td"[^>]*">"|"<TD"[^>]*>">"   {printf("<td>");};
<INITIAL>"<tr"[^>]*">"|"<TR"[^>]*>">"   {printf("<tr>");};
<INITIAL>"<input"[^>]*">"|"<INPUT"[^>]*>">"   {;};
<INITIAL>"<meta"[^>]*">"|"<META"[^>]*>">"   {;};
<INITIAL>"<font"[^>]*">"|"<FONT"[^>]*">"        {;}
<INITIAL>"<\/font"[^>]*">"|"<\/FONT"[^>]*">"    {;}
<INITIAL>.      {printf("%s", yytext);}

<NOUSE>{
"-->"				                {BEGIN(INITIAL);}
.|\n    {;}
}
<OBJECT>{
"<\/OBJECT>"|"<\/object>"		       {BEGIN(INITIAL);}
.|\n	{;}
}

<SCRIPT>{
"<\/script>"|"<\/SCRIPT>"      {BEGIN(INITIAL);}
.|\n               {;}
}

<IFRAME>{
"<\/iframe>"|"<\/iframe>"      {BEGIN(INITIAL);}
.|\n               {;}
}

<STYLE>{
"<\/style>"|"<\/STYLE>"      {BEGIN(INITIAL);}
.|\n               {;}
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

