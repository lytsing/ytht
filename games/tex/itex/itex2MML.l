%{

%}
%option noyywrap
%x MATH ATTRIBUTELIST BBOLD FRAKTUR CALLIGRAPHIC PLAINTEXT INT1 INT2 INT3 
%{
typedef char* string;
#define YYSTYPE string
#include "y.tab.h"
extern char* yylval;
int rowposn=0;
int lineno=1;
int displaymode=0;
%} 
%%
%{ if (rowposn==1) rowposn=0;
   if (rowposn==2) rowposn=1;
%}


<INITIAL>"$"   BEGIN(MATH); rowposn=2; displaymode=0; return STARTMATH;
<INITIAL>"\\[" BEGIN(MATH); rowposn=2; displaymode=1; return STARTDMATH;
<INITIAL>EOF    return 0;
<INITIAL>"\n" {lineno++; yylval=(char *)strdup(yytext); return CHAR;}
<INITIAL>"\\$" {yylval=(char *)strdup("$"); return CHAR;}
<INITIAL>.     {yylval=(char *)strdup(yytext); return CHAR;}


<MATH>{



"$"  |
"\\]"           {BEGIN(INITIAL); return ENDMATH;}

"&nbsp;" |
"<br>"        {;}
"^"            return SUP;
"_"            return SUB; 
"{"[\t\r\n ]*"}"   return EMPTYMROW;
"{"            {rowposn = 2; return MROWOPEN;}
"}"            return MROWCLOSE;

"\\left"       rowposn=2; return LEFT;
"\\right"      return RIGHT;


"\\rlap" |
"\\llap" |
"\\ulap" |
"\\dlap"               {/* Ignore */ ;}


"&amp;" |
"&"         {rowposn=2; return COLSEP;}

"\\\\"          rowposn=2; return ROWSEP;

[a-zA-Z]+       {yylval=(char *)strdup(yytext); return MI;}

[0-9]+          {yylval=(char *)strdup(yytext); return MN;}
"\\infty"   |
"\\infinity"     {yylval=(char *)strdup("&infin;"); return MN;}

"-"    {if (rowposn==1) return UNARYMINUS; else {yylval=(char *)strdup(yytext); return MO;}}


"("   |
"["   |
"+"   |
"."   |
","   |
"/"   |
":"   |
"!"   |
"="   |
"~"   |
";"   |
"'"   |
"?"   |
"#"   |
"`"   |
"|"   |
"%"   |
"*"        {yylval=(char *)strdup(yytext); return MOL;}

")"   |
"]"        {yylval=(char *)strdup(yytext); return MO;}

"\\alpha"   |
"\\beta" |
"\\gamma" |
"\\delta" |
"\\zeta" |
"\\eta"  |
"\\theta" |
"\\iota"  |
"\\kappa" |
"\\lambda" |
"\\mu"     |
"\\nu"    |
"\\xi"   |
"\\pi"     |
"\\rho"   |
"\\sigma"  |
"\\tau"   |
"\\upsilon" |
"\\phi"   |
"\\chi"  |
"\\psi"  |
"\\omega"     {yylval=(char *)strdup(yytext); yylval[0]='&'; yylval[yyleng]=';'; yylval[yyleng+1]='\0'; return MI;}

"\\omicron"  {yylval=(char *)strdup("&oscr;"); return MI;}

"\\epsilon"  {yylval=(char *)strdup("&epsi;"); return MI;}
"\\varepsilon"  {yylval=(char *)strdup("&epsiv;"); return MI;}
"\\vartheta"  {yylval=(char *)strdup("&thetav;"); return MI;}
"\\varpi"  {yylval=(char *)strdup("&pi;"); return MI;}
"\\varrho"  {yylval=(char *)strdup("&rhov;"); return MI;}
"\\varsigma"  {yylval=(char *)strdup("&sigmav;"); return MI;}
"\\varphi"  {yylval=(char *)strdup("&phiv;"); return MI;}

"\\cdot"   {yylval=(char *)strdup("&sdot;"); return MO;}

"\\Delta"  |
"\\Gamma"  |
"\\Lambda" |
"\\Pi"     |
"\\Phi"    |
"\\Psi"    |
"\\Sigma"  |
"\\Theta"  |
"\\Xi"     |
"\\Upsilon" |
"\\Omega"     {yylval=(char *)strdup(yytext); yylval[0]='&'; yylval[yyleng]=';'; yylval[yyleng+1]='\0'; return MI;}

"\\Alpha"   |
"\\Beta" |
"\\Zeta" |
"\\Eta"  |
"\\Iota"  |
"\\Kappa" |
"\\Mu"     |
"\\Nu"    |
"\\Rho"   |
"\\Tau"   {yylval=(char *)strdup(yytext+1); yylval[1]='\0'; return MI;}


"\\iff" |
"\\Longleftrightarrow"  {yylval=(char *)strdup("&iff;"); return MOL;}

"\\Leftrightarrow"  {yylval=(char *)strdup("&Leftrightarrow;"); return MOL;}

"\\impliedby" |
"\\Leftarrow"   {yylval=(char *)strdup("&Leftarrow;"); return MOL;}

"\\implies" |
"\\Rightarrow"   {yylval=(char *)strdup("&Rightarrow;"); return MOL;}

"\\hookleftarrow" {yylval=(char *)strdup("&hookleftarrow;"); return MOL;}

"\\embedsin" |
"\\hookrightarrow"  {yylval=(char *)strdup("&hookrightarrow;"); return MOL;}

"\\longleftarrow"   {yylval=(char *)strdup("&longleftarrow;"); return MOL;}

"\\longrightarrow"  {yylval=(char *)strdup("&longrightarrow;"); return MOL;}

"\\leftarrow"   {yylval=(char *)strdup("&leftarrow;"); return MOL;}

"\\to"    |
"\\rightarrow"  {yylval=(char *)strdup("&rightarrow;"); return MOL;}

"\\leftrightarrow"  {yylval=(char *)strdup("&leftrightarrow;"); return MOL;}

"\\mapsto"  |
"\\map"       {yylval=(char *)strdup("&map;"); return MOL;}

"\\nearrow" |
"\\nearr"    {yylval=(char *)strdup("&nearr;"); return MOL;}

"\\nwarrow" |
"\\nwarr"     {yylval=(char *)strdup("&nwarr;"); return MOL;}

"\\searrow" |
"\\searr"     {yylval=(char *)strdup("&searr;"); return MOL;}

"\\swarrow" |
"\\swarr"     {yylval=(char *)strdup("&swarr;"); return MOL;}

"\\neArrow" |
"\\neArr"    {yylval=(char *)strdup("&neArr;"); return MOL;}

"\\nwArrow" |
"\\nwArr"     {yylval=(char *)strdup("&nwArr;"); return MOL;}

"\\seArrow" |
"\\seArr"     {yylval=(char *)strdup("&seArr;"); return MOL;}

"\\swArrow" |
"\\swArr"     {yylval=(char *)strdup("&swArr;"); return MOL;}

"\\downarrow" |
"\\darr"     {yylval=(char *)strdup("&downarrow;"); return MOL;}

"\\uparrow" |
"\\uparr"    {yylval=(char *)strdup("&uparrow;"); return MOL;}

"\\downuparrow" |
"\\updownarrow" |
"\\duparr" |
"\\updarr"  {yylval=(char *)strdup("&updownarrow;"); return MOL;}

"\\cdots"   {yylval=(char *)strdup("&sdot; &sdot; &sdot;"); return MO;}

"\\ddots"   {yylval=(char *)strdup("&dtdot;"); return MO;}

"\\ldots"   {yylval=(char *)strdup("&ltdot;"); return MO;}

"\\vdots"   {yylval=(char *)strdup("&vellip;"); return MO;}


"\\cup"  |
"\&union;" |
"\\union"          {yylval=(char *)strdup("&cup;"); return MOL;}

"\\bigcup" |
"\\Union"  |
"\&Union;"      {yylval=(char *)strdup("&Union;"); return MOB;}

"\\{"            {yylval=(char *)strdup("{"); return MOL;}
"\\}"            {yylval=(char *)strdup("}"); return MO;}




"\\in"           {yylval=(char *)strdup("&Element;"); return MOL;}

"\\|"      {yylval=(char *)strdup("&DoubleVerticalBar;"); return MOL;}

"\\rfloor" |
"\\rceil"  |
"\\rang"   |
"\\rangle"  {yylval=(char *)strdup(yytext); yylval[0]='&'; yylval[yyleng]=';'; yylval[yyleng+1]='\0'; return MO;}


"\\lfloor" |
"\\lceil" |
"\\lang" |
"\\langle" |
"\\gt"   |
"\\lt"   |
"\\subset"  |
"\\subseteq" |
"\\prec"     |
"\\parallel"  |
"\\nparallel" |
"\\perp"  |
"\\gg" |
"\\geq" |
"\\ge"  |
"\\le"  |
"\\leq" |
"\\nsubset" |
"\\nsubseteq" |
"\\notin" |
"\\ni" |
"\\nmid" |
"\\preceq" |
"\\npreceq" |
"\\ll" |
"\\ngeq" |
"\\nleq" |
"\\supset" |
"\\supseteq" |
"\\approx" |
"\\sim" |
"\\triangleleft" |
"\\cong" |
"\\succ" |
"\\nsucc" |
"\\ngtr" |
"\\nsupset" |
"\\nsupseteq" |
"\\propto" |
"\\equiv" |
"\\triangleright" |
"\\ncong" |
"\\succeq" |
"\\nsucceq" |
"\\amalg" |
"\\setminus" |
"\\smallsetminus" |
"\\pm" |
"\\mp" |
"\\cap" |
"\\uplus" |
"\\sqcap" |
"\\sqcup" |
"\\ominus" |
"\\ast" |
"\\oslash" |
"\\star" |
"\\diamond" |
"\\wr" |
"\\dagger" |
"\\ddagger" |
"\\ltimes" |
"\\div" |
"\\rtimes" |
"\\Im" |
"\\bot" |
"\\emptyset" |
"\\prime" |
"\\therefore" |
"\\flat" |
"\\vee" |
"\\Re" |
"\\hbar" |
"\\because" |
"\\sharp" |
"\\bullet" |
"\\aleph" |
"\\nexists" |
"\\imath" |
"\\not" |
"\\top" |
"\\wp" |
"\\natural" |
"\\square" |
"\\angle" |
"\\forall" |
"\\jmath" |
"\\ell" |
"\\bigtriangleup" |
"\\bigtriangledown" |
"\\beth" |
"\\clubsuit" |
"\\diamondsuit" |
"\\heartsuit" |
"\\nabla" |
"\\triangle" |
"\\nprec"  {yylval=(char *)strdup(yytext); yylval[0]='&'; yylval[yyleng]=';'; yylval[yyleng+1]='\0'; return MOL;}

"\\partial" {yylval=(char *)strdup("&PartialD;"); return MOL;}
"\\land"       {yylval=(char *)strdup("&and;"); return MOL;}
"\\circ" {yylval=(char *)strdup("&compfn;"); return MO;}

"\\qed" {yylval=(char *)strdup("&squf;"); return MO;}

"\\pmod" {yylval=(char *)strdup("&nbsp; mod "); return MO;}

"\\bottom" {yylval=(char *)strdup("&perp;"); return MOL;}

"\\exists" {yylval=(char *)strdup("&exist;"); return MOL;}

"\\neg" {yylval=(char *)strdup("&not;"); return MOL;}

"\\neq" |
"\\ne"   {yylval=(char *)strdup("&ne;"); return MOL;}

"\\mid"  {yylval=(char *)strdup("&VerticalBar;"); return MOL;}

"\\int"    |
"\\integral"     {yylval=(char *)strdup("&Integral;"); return MOL;}

"\\iint"   |
"\\doubleintegral" {yylval=(char *)strdup("&Int;"); return MOL;}

"\\iiint"   |
"\\tripleintegral" {yylval=(char *)strdup("&tint;"); return MOL;}

"\\iiiint"   |
"\\quadrupleintegral" {yylval=(char *)strdup("&qint;"); return MOL;}

"\\oint"    |
"\\conint"  |
"\\contourintegral"   {yylval=(char *)strdup("&conint;"); return MOL;}

"\\times"   {yylval=(char *)strdup("&times;"); return MOL;}

"\\sum"          {yylval=(char *)strdup("&Sum;"); return MOB;}

"\\prod" |
"\\product"      {yylval=(char *)strdup("&prod;"); return MOB;}

"\\coprod"  |
"\\coproduct"    {yylval=(char *)strdup("&coprod;"); return MOB;}

"\\otimes"       {yylval=(char *)strdup("&otimes;"); return MOL;}
"\\Otimes"  |
"\\bigotimes"     {yylval=(char *)strdup("&otimes;"); return MOB;}


"\\oplus"       {yylval=(char *)strdup("&oplus;"); return MOL;}
"\\Oplus"   |
"\\bigoplus"    {yylval=(char *)strdup("&oplus;"); return MOB;}


"\\wedge"       {yylval=(char *)strdup("&wedge;"); return MOL;}
"\\Wedge"  |
"\\bigwedge"       {yylval=(char *)strdup("&xwedge;"); return MOB;}

"\\lim" {yylval=(char *)strdup(yytext); ++yylval; return MOB;}

"\\frac"         {return FRAC;}

"\\binom"        {return BINOM;}

"\\tensor"       {return TENSOR;}

"\\multiscripts" {return MULTI;}

"\\overbrace"     {return OVERBRACE;}

"\\underbrace"   {return UNDERBRACE;}

"\\overline" |
"\\closure" |
"\\bar" |
"\\widebar"      {return BAR;}

"\\vec"  |
"\\widevec"   {return VEC;}

"\\dot"       {return DOT;}

"\\ddot"      {return DDOT;}

"\\tilde"  |
"\\widetilde"   {return TILDE;}


"\\check"  |
"\\widecheck"   {return CHECK;}


"\\hat"  |
"\\widehat"   {return HAT;}

"\\underset"     {return UNDER;}

"\\overset"      {return OVER;}

"\\underoverset"  {return UNDEROVER;}

"\\sqrt"         {return SQRT;}

"\\root"         {return ROOT;}

"\\space"        {BEGIN(INT1); return SPACE;}

"\\text"          {BEGIN(PLAINTEXT); return TEXTBOX;}

"\\statusline"    {BEGIN(PLAINTEXT); return STATLINE;}

"\\toggle"        {return TOGGLE;}

"\\fghilight" |
"\\fghighlight"   {BEGIN(ATTRIBUTELIST); return FGHIGHLIGHT;}

"\\bghilight" |
"\\bghighlight"   {BEGIN(ATTRIBUTELIST); return BGHIGHLIGHT;}


"\\displaystyle"  {rowposn=2; return DISPLAY;}

"\\textstyle"     {rowposn=2; return TEXTSTY;}

"\\textsize"     {return TEXTSIZE;}

"\\scriptsize"     {return SCSIZE;}

"\\scriptscriptsize"     {return SCSCSIZE;}

"\\mathit"       {return ITALICS;}

"\\mathbf"       {return BOLD;}

"\\mathrm"       {return RM;}

"\\mathbb"       {BEGIN(BBOLD); return BB;}

"\\mathfr"       {BEGIN(FRAKTUR); return FRAK;}

"\\mathcal"       {BEGIN(CALLIGRAPHIC); return CAL;}

"\\array"        {return ARRAY;}

"\\arrayopts"    {return ARRAYOPTS;}

"\\colalign"     {BEGIN(ATTRIBUTELIST); return COLALIGN;}

"\\collayout"   {BEGIN(ATTRIBUTELIST); return COLLAYOUT;}

"\\rowalign"     {BEGIN(ATTRIBUTELIST); return ROWALIGN;}

"\\align"     {BEGIN(ATTRIBUTELIST); return ALIGN;}

"\\equalrows"     {BEGIN(ATTRIBUTELIST); return EQROWS;}

"\\equalcols"     {BEGIN(ATTRIBUTELIST); return EQCOLS;}

"\\rowlines"     {BEGIN(ATTRIBUTELIST); return ROWLINES;}

"\\collines"     {BEGIN(ATTRIBUTELIST); return COLLINES;}

"\\frame"     {BEGIN(ATTRIBUTELIST); return FRAME;}

"\\padding"     {BEGIN(ATTRIBUTELIST); return PADDING;}

"\\rowopts"    {return ROWOPTS;}

"\\cellopts"   {return CELLOPTS;} 

"\\rowspan"     {BEGIN(ATTRIBUTELIST); return ROWSPAN;}

"\\colspan"     {BEGIN(ATTRIBUTELIST); return COLSPAN;}


"\\," |
"\\thinspace"   {return THINSPACE;}

"\\medspace"    {return MEDSPACE;}

"\\thickspace"  {return THICKSPACE;}

"\\quad"        {return QUAD;}

"\\!" |
"\\negspace"    {return NEGSPACE;}

"\\phantom"     {return PHANTOM;}

"\\href"        {BEGIN(PLAINTEXT); return HREF;}


"\\mathop"       {BEGIN(PLAINTEXT); return MATHOP;}

"\\"[a-zA-Z]+    {yylval=(char *)strdup(yytext); ++yylval; return MOP;}

"\&"[a-zA-Z]+";"  {yylval=(char *)strdup(yytext); return MI;}

"\n"             lineno++;
[\t ]+           ;


.              {return UNKNOWNCHAR;}
}

<ATTRIBUTELIST>"{"[ \t\n\r #a-zA-Z0-9.\-]*"}"  {BEGIN(MATH); yylval=(char *)strdup(yytext); yylval[0]='"'; yylval[yyleng-1]='"'; return ATTRLIST;}

<BBOLD>{
"\{"  {return ST;}
"\}"  {BEGIN(MATH); return END;}
[a-z] {yylval=(char *)strdup(yytext); return BBLOWERCHAR;}
[A-Z]   {yylval=(char *)strdup(yytext); return BBUPPERCHAR;}
[\t\n\r ]+       ;
.           {return yytext[0];}
}

<CALLIGRAPHIC>{
"\{"  {return ST;}
"\}"  {BEGIN(MATH); return END;}
[a-zA-Z]   {yylval=(char *)strdup(yytext); return CALCHAR;}
[\t\n\r ]+       ;
.           {return yytext[0];}
}

<FRAKTUR>{
"\{"  {return ST;}
"\}"  {BEGIN(MATH); return END;}
[a-zA-Z]   {yylval=(char *)strdup(yytext); return FRAKCHAR;}
[\t\n\r ]+       ;
.           {return yytext[0];}
}

<PLAINTEXT>{
\{[^\}]*\}  {yylval=(char *)strdup(yytext+1); yylval[yyleng-2]='\0'; BEGIN(MATH); return TEXTSTRING;}
}

<INT1>{ 
"\{"      {return ST;}
"\}"      {BEGIN(INT2); return END;}
[0-9]+    {yylval=malloc(yyleng +2); yylval=(char *)strdup(yytext); yylval[yyleng]=yylval[yyleng-1]; yylval[yyleng-1]='.'; yylval[yyleng+1]='\0'; return INTONE;}
[\t\n\r ]+   ;
.         {return yytext[0];}
}

<INT2>{
"\{"      {return ST;}
"\}"      {BEGIN(INT3); return END;}
[0-9]+    {yylval=malloc(yyleng +2); yylval=(char *)strdup(yytext); yylval[yyleng]=yylval[yyleng-1]; yylval[yyleng-1]='.'; yylval[yyleng+1]='\0';   return INTTWO;}
[\t\n\r ]+   ;
.         {return yytext[0];}
}

<INT3>{
"\{"      {return ST;}
"\}"      {BEGIN(MATH); return END;}
[0-9]+    {yylval=malloc(yyleng +2); yylval=(char *)strdup(yytext); yylval[yyleng]=yylval[yyleng-1]; yylval[yyleng-1]='.'; yylval[yyleng+1]='\0';  return INTTHREE;}
[\t\n\r ]+   ;
.         {return yytext[0];}
}



%%

void 
yyerror(s)
char *s;
{
	fprintf(stderr, "Line: %d Error: %s at token %s\n",lineno, s, yytext);
}

