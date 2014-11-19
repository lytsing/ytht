%{
#include <stdio.h>
#include <string.h>

extern int rowposn;
extern displaymode;
typedef char* string;
#define YYSTYPE string
%}



%token    CHAR STARTMATH STARTDMATH ENDMATH MI MN MO PLUS SUP SUB MROWOPEN MROWCLOSE LEFT RIGHT  FRAC MATHOP MOP MOL MOB SQRT ROOT BINOM UNDER OVER OVERBRACE UNDERBRACE UNDEROVER TENSOR MULTI ARRAY COLSEP ROWSEP ARRAYOPTS COLLAYOUT COLALIGN ROWALIGN ALIGN EQROWS EQCOLS ROWLINES COLLINES FRAME PADDING ATTRLIST ITALICS BOLD RM BB ST END BBLOWERCHAR BBUPPERCHAR CALCHAR FRAKCHAR CAL FRAK ROWOPTS TEXTSIZE SCSIZE SCSCSIZE DISPLAY TEXTSTY TEXTBOX TEXTSTRING CELLOPTS ROWSPAN COLSPAN THINSPACE MEDSPACE THICKSPACE QUAD NEGSPACE PHANTOM HREF UNKNOWNCHAR EMPTYMROW STATLINE TOGGLE FGHIGHLIGHT BGHIGHLIGHT SPACE INTONE INTTWO INTTHREE BAR VEC HAT CHECK TILDE DOT DDOT UNARYMINUS

%%

doc:  xmlmmlTermList {/* all processing done in body*/};

xmlmmlTermList:
{/* nothing - do nothing*/}
| char {/* proc done in body*/}
| expression {/* all proc. in body*/}
| xmlmmlTermList char {/* all proc. in body*/}
| xmlmmlTermList expression {/* all proc. in body*/};



char: CHAR {printf("%s", $1);};

expression: 
STARTMATH ENDMATH {/* empty math group - ignore*/}
| STARTDMATH ENDMATH {/* ditto */}
| STARTMATH compoundTermList ENDMATH  {printf("<math xmlns='http://www.w3.org/1998/Math/MathML'>\n%s\n</math>",$2);}
|   STARTDMATH compoundTermList ENDMATH  {printf("<math xmlns='http://www.w3.org/1998/Math/MathML' mode='display'>\n%s\n</math>",$2);};

compoundTermList:
compoundTerm  {sprintf((char *)$$, "%s\n", strdup((char *)$1));} 
|  compoundTermList compoundTerm
  {sprintf((char *)$$, "%s%s", strdup((char *)$1), strdup((char *)$2));};

compoundTerm: 
    mob SUB closedTerm SUP closedTerm 
{if (displaymode==1) {sprintf((char *)$$,"<munderover>%s %s %s</munderover>",strdup((char *)$1),strdup((char *)$3), strdup((char *)$5));} else  {sprintf((char *)$$,"<msubsup>%s %s %s</msubsup>",strdup((char *)$1),strdup((char *)$3), strdup((char *)$5));}}
|  mob SUB closedTerm {if (displaymode==1) {sprintf((char *)$$,"<munder>%s %s</munder>",strdup((char *)$1),strdup((char *)$3));} else {sprintf((char *)$$,"<msub>%s %s</msub>",strdup((char *)$1),strdup((char *)$3));}}
|   mob SUP closedTerm SUB closedTerm 
{if (displaymode==1) {sprintf((char *)$$,"<munderover>%s %s %s</munderover>",strdup((char *)$1),strdup((char *)$5), strdup((char *)$3));} else  {sprintf((char *)$$,"<msubsup>%s %s %s</msubsup>",strdup((char *)$1),strdup((char *)$5), strdup((char *)$3));}}
|  mob SUP closedTerm
{if (displaymode==1)     {sprintf((char *)$$,"<mover>%s %s</mover>", strdup((char *)$1),strdup((char *)$3));} else {sprintf((char *)$$,"<msup>%s %s</msup>", strdup((char *)$1),strdup((char *)$3));}}
|   closedTerm SUB closedTerm SUP closedTerm 
{sprintf((char *)$$,"<msubsup>%s %s %s</msubsup>",strdup((char *)$1),strdup((char *)$3), strdup((char *)$5)); }
|   closedTerm SUP closedTerm SUB closedTerm 
{sprintf((char *)$$,"<msubsup>%s %s %s</msubsup>",strdup((char *)$1),strdup((char *)$5), strdup((char *)$3)); }
|  closedTerm SUB closedTerm {sprintf((char *)$$,"<msub>%s %s</msub>",strdup((char *)$1),strdup((char *)$3));}
|  closedTerm SUP closedTerm
      {sprintf((char *)$$,"<msup>%s %s</msup>", strdup((char *)$1),strdup((char *)$3));}
|  SUB closedTerm 
{(char *)$$=malloc(strlen((char *)$2)+20); sprintf((char *)$$, "<msub><mo></mo>%s</msub>",strdup((char *)$2));}
|  SUP closedTerm 
{(char *)$$=malloc(strlen((char *)$2)+20); sprintf((char *)$$, "<msup><mo></mo>%s</msup>",strdup((char *)$2));}
|  closedTerm {sprintf((char *)$$, strdup((char *)$1));};



closedTerm: 
array
|   unaryminus
|    mi {sprintf((char *)$$,"<mi>%s</mi>",strdup((char *)$1));}
|   mn {sprintf((char *)$$,"<mn>%s</mn>",strdup((char *)$1));}
|   mo 
|   tensor
|   multi
|   mfrac
|   binom
|   msqrt 
|   mroot
|   munder
|   mover
|   bar
|   vec
|   hat
|   dot
|   ddot
|   check
|   tilde
|   moverbrace
|   munderbrace
|   munderover
|   emptymrow
|   displaystyle
|   textstyle
|   textsize
|   scriptsize
|   scriptscriptsize
|   italics
|   bold
|   roman
|   bbold
|   frak
|   cal
|   space
|   textstring
|   thinspace
|   medspace
|   thickspace
|   quad
|   negspace
|   phantom
|   href
|   statusline
|   toggle
|   fghighlight
|   bghighlight
|   MROWOPEN compoundTermList MROWCLOSE 
         {(char *)$$=malloc(strlen((char *)$2)+20); sprintf((char *)$$,"<mrow>%s</mrow>",strdup((char *)$2));};
|   LEFT compoundTermList RIGHT mo
         {(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$4)+20); sprintf((char *)$$,"<mrow>%s %s</mrow>",strdup((char *)$2),strdup((char *)$4));};
|   LEFT compoundTermList RIGHT
         {(char *)$$=malloc(strlen((char *)$2)+20); sprintf((char *)$$,"<mrow>%s</mrow>",strdup((char *)$2));};
|   unrecognized;

unrecognized: UNKNOWNCHAR
  {(char *)$$=malloc(40); sprintf((char *)$$,"<merror>Unknown character</merror>");};

unaryminus: UNARYMINUS {(char *)$$=malloc(50); sprintf((char *)$$,"<mo lspace=\"thinthinmathspace\" rspace=\"0em\">-</mo>");};

mi: MI;
mn: MN;
mob: MOB {rowposn=2; (char *)$$=malloc(strlen((char *)$1)+55); sprintf((char *)$$,"<mo lspace=\"thinmathspace\" rspace=\"thinmathspace\">%s</mo>", strdup((char *)$1));};

mo: mob
|   MO {(char *)$$=malloc(strlen((char *)$1)+10); sprintf((char *)$$,"<mo>%s</mo>",strdup((char *)$1));};
|    MOL {rowposn=2; (char *)$$=malloc(strlen((char *)$1)+10); sprintf((char *)$$,"<mo>%s</mo>",strdup((char *)$1));};
|  MOP {rowposn=2; (char *)$$=malloc(strlen((char *)$1)+45); sprintf((char *)$$,"<mo lspace=\"0em\" rspace=\"thinmathspace\">%s</mo>", strdup((char *)$1));};
|  MATHOP TEXTSTRING {rowposn=2; (char *)$$=malloc(strlen((char *)$2)+45); sprintf((char *)$$,"<mo lspace=\"0em\" rspace=\"thinmathspace\">%s</mo>", strdup((char *)$2));};



emptymrow: EMPTYMROW {(char *)$$=malloc(14); sprintf((char *)$$,"<mrow></mrow>");};

space: SPACE ST INTONE END ST INTTWO END ST INTTHREE END
{(char *)$$=malloc(strlen((char *)$3)+strlen((char *)$6)+strlen((char *)$9)+40); sprintf((char *)$$,"<mspace height=\"%sex\" depth=\"%sex\" width=\"%sem\"/>",strdup((char *)$3),strdup((char *)$6),strdup((char *)$9));};

statusline: STATLINE TEXTSTRING closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+60); sprintf((char *)$$,"<maction actiontype=\"statusline\">%s<mtext>%s</mtext></maction>",strdup((char *)$3),strdup((char *)$2));};

toggle: TOGGLE closedTerm closedTerm closedTerm closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+60); sprintf((char *)$$,"<maction actiontype=\"toggle\" selection=\"2\">%s %s</maction>",strdup((char *)$2),strdup((char *)$3));};

fghighlight: FGHIGHLIGHT ATTRLIST closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+70); sprintf((char *)$$,"<maction actiontype=\"highlight\" other='color=%s'>%s</maction>",strdup((char *)$2),strdup((char *)$3));};

bghighlight: BGHIGHLIGHT ATTRLIST closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+70); sprintf((char *)$$,"<maction actiontype=\"highlight\" other='background=%s'>%s</maction>",strdup((char *)$2),strdup((char *)$3));};

textstring: TEXTBOX TEXTSTRING 
{(char *)$$=malloc(strlen((char *)$2)+16); sprintf((char *)$$,"<mtext>%s</mtext>",strdup((char *)$2));};

displaystyle: DISPLAY closedTerm
{(char *)$$=malloc(strlen((char *)$2)+38); sprintf((char *)$$,"<mstyle displaystyle=\"true\">%s</mstyle>",strdup((char *)$2));};

textstyle: TEXTSTY closedTerm
{(char *)$$=malloc(strlen((char *)$2)+40); sprintf((char *)$$,"<mstyle displaystyle=\"false\">%s</mstyle>",strdup((char *)$2));};

textsize: TEXTSIZE closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mstyle scriptlevel=\"0\">%s</mstyle>",strdup((char *)$2));};

scriptsize: SCSIZE closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mstyle scriptlevel=\"1\">%s</mstyle>",strdup((char *)$2));};

scriptscriptsize: SCSCSIZE closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mstyle scriptlevel=\"2\">%s</mstyle>",strdup((char *)$2));};

italics: ITALICS closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mstyle fontstyle=\"italic\">%s</mstyle>",strdup((char *)$2));};

bold: BOLD closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mstyle fontweight=\"bold\">%s</mstyle>",strdup((char *)$2));};

roman: RM closedTerm
{(char *)$$=malloc(strlen((char *)$2)+60); sprintf((char *)$$,"<mstyle fontstyle=\"normal\" fontweight=\"normal\">%s</mstyle>",strdup((char *)$2));};

bbold: BB ST bbletters END
{(char *)$$=malloc(strlen((char *)$3)+10); sprintf((char *)$$,"<mi>%s</mi>",strdup((char *)$3));};

bbletters:
bbletter {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| bbletters bbletter {(char *)$$=malloc(strlen((char *)$1)+strlen((char *)$2)); sprintf((char *)$$,"%s%s",strdup((char *)$1),strdup((char *)$2));};

bbletter:
BBLOWERCHAR {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| BBUPPERCHAR {(char *)$$=malloc(strlen((char *)$1)+6); sprintf((char *)$$,"&%sopf;", strdup((char *)$1));};

frak: FRAK ST frakletters END
{(char *)$$=malloc(strlen((char *)$3)+10); sprintf((char *)$$,"<mi>%s</mi>",strdup((char *)$3));};

frakletters:
frakletter {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| frakletters frakletter {(char *)$$=malloc(strlen((char *)$1)+strlen((char *)$2)); sprintf((char *)$$,"%s%s",strdup((char *)$1),strdup((char *)$2));};

frakletter:
FRAKCHAR {(char *)$$=malloc(strlen((char *)$1)+6); sprintf((char *)$$,"&%sfr;", strdup((char *)$1));};

cal: CAL ST calletters END
{(char *)$$=malloc(strlen((char *)$3)+10); sprintf((char *)$$,"<mi>%s</mi>",strdup((char *)$3));};

calletters:
calletter {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| calletters calletter {(char *)$$=malloc(strlen((char *)$1)+strlen((char *)$2)); sprintf((char *)$$,"%s%s",strdup((char *)$1),strdup((char *)$2));};

calletter:
CALCHAR {(char *)$$=malloc(strlen((char *)$1)+6); sprintf((char *)$$,"&%sscr;", strdup((char *)$1));};

thinspace: THINSPACE 
   {(char *)$$=malloc(34); sprintf((char *)$$, "<mspace width=\"thinmathspace\"/>");};

medspace: MEDSPACE 
   {(char *)$$=malloc(36); sprintf((char *)$$, "<mspace width=\"mediummathspace\"/>");};

thickspace: THICKSPACE 
   {(char *)$$=malloc(36); sprintf((char *)$$, "<mspace width=\"thickmathspace\"/>");};

quad: QUAD 
   {(char *)$$=malloc(38); sprintf((char *)$$, "<mspace width=\"verythickmathspace\"/>");};

negspace: NEGSPACE
   {(char *)$$=malloc(30); sprintf((char *)$$, "<mspace width=\"-0.1667 em\"/>");};

phantom: PHANTOM closedTerm
{(char *)$$=malloc(strlen((char *)$2)+22); sprintf((char *)$$,"<mphantom>%s</mphantom>",strdup((char *)$2));};

href: HREF TEXTSTRING closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+70); sprintf((char *)$$,"<mrow xlink:type=\"simple\" xlink:show=\"replace\" xlink:href=\"%s\">%s</mrow>",strdup((char *)$2),strdup((char *)$3));};

tensor:  TENSOR closedTerm MROWOPEN subsupList MROWCLOSE
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$4)+32); sprintf((char *)$$,"<mmultiscripts>%s%s</mmultiscripts>",strdup((char *)$2),strdup((char *)$4));}
| TENSOR closedTerm subsupList MROWCLOSE
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+32); sprintf((char *)$$,"<mmultiscripts>%s%s</mmultiscripts>",strdup((char *)$2),strdup((char *)$3));}
| TENSOR closedTerm subsupList 
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+32); sprintf((char *)$$,"<mmultiscripts>%s%s</mmultiscripts>",strdup((char *)$2),strdup((char *)$3));};


multi:  MULTI MROWOPEN subsupList MROWCLOSE closedTerm MROWOPEN subsupList MROWCLOSE
{(char *)$$=malloc(strlen((char *)$3)+strlen((char *)$5)+strlen((char *)$7)+48); sprintf((char *)$$,"<mmultiscripts>%s%s<mprescripts/>%s</mmultiscripts>",strdup((char *)$5),strdup((char *)$7),strdup((char *)$3));};


subsupList: subsupTerm {sprintf((char *)$$, "%s", strdup((char *)$1));} 
| subsupList subsupTerm
  {sprintf((char *)$$, "%s %s", strdup((char *)$1), strdup((char *)$2));};

subsupTerm: 
SUB closedTerm SUP closedTerm {(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$4)+2); sprintf((char *)$$,"%s %s",strdup((char *)$2),strdup((char *)$4));}
|
SUB closedTerm {(char *)$$=malloc(strlen((char *)$2)+10); sprintf((char *)$$,"%s <none/>",strdup((char *)$2));}
|
SUP closedTerm {(char *)$$=malloc(strlen((char *)$2)+10); sprintf((char *)$$,"<none/> %s",strdup((char *)$2));};



mfrac:   FRAC closedTerm closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+20); sprintf((char *)$$,"<mfrac>%s%s</mfrac>",strdup((char *)$2),strdup((char *)$3));};

binom: BINOM closedTerm closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+35); sprintf((char *)$$,"<mfrac linethickness=\"0\">%s%s</mfrac>",strdup((char *)$2),strdup((char *)$3));};

munderbrace:  UNDERBRACE closedTerm
{(char *)$$=malloc(strlen((char *)$2)+38); sprintf((char *)$$,"<munder>%s<mo>&UnderBrace;</mo></munder>",strdup((char *)$2));};

moverbrace:  OVERBRACE closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mover>%s<mo>&OverBrace;</mo></mover>",strdup((char *)$2));};

bar: BAR closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mover>%s<mo>&OverBar;</mo></mover>",strdup((char *)$2));};

vec: VEC closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mover>%s<mo>&RightVector;</mo></mover>",strdup((char *)$2));};

dot: DOT closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mover>%s<mo>&dot;</mo></mover>",strdup((char *)$2));};

ddot: DDOT closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mover>%s<mo>&Dot;</mo></mover>",strdup((char *)$2));};

tilde: TILDE closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mover>%s<mo>&tilde;</mo></mover>",strdup((char *)$2));};

check: CHECK closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mover>%s<mo>&macr;</mo></mover>",strdup((char *)$2));};


hat: HAT closedTerm
{(char *)$$=malloc(strlen((char *)$2)+36); sprintf((char *)$$,"<mover>%s<mo>&Hat;</mo></mover>",strdup((char *)$2));};

msqrt:  SQRT closedTerm
{(char *)$$=malloc(strlen((char *)$2)+20); sprintf((char *)$$,"<msqrt>%s</msqrt>",strdup((char *)$2));};

mroot: ROOT closedTerm closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+20); sprintf((char *)$$,"<mroot>%s%s</mroot>",strdup((char *)$3),strdup((char *)$2));};

munder: UNDER closedTerm closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+22); sprintf((char *)$$,"<munder>%s%s</munder>",strdup((char *)$3),strdup((char *)$2));};

mover: OVER closedTerm closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+20); sprintf((char *)$$,"<mover>%s%s</mover>",strdup((char *)$3),strdup((char *)$2));};

munderover: UNDEROVER closedTerm closedTerm closedTerm
{(char *)$$=malloc(strlen((char *)$2)+strlen((char *)$3)+strlen((char *)$4)+26); sprintf((char *)$$,"<munderover>%s%s%s</munderover>",strdup((char *)$4),strdup((char *)$2),strdup((char *)$3));};


array:  ARRAY MROWOPEN tableRowList MROWCLOSE
{(char *)$$=malloc(strlen((char *)$3)+28); sprintf((char *)$$,"<mrow><mtable>%s</mtable></mrow>",strdup((char *)$3));}
| ARRAY MROWOPEN ARRAYOPTS MROWOPEN arrayopts MROWCLOSE tableRowList MROWCLOSE
{(char *)$$=malloc(strlen((char *)$5)+strlen((char *)$7)+30); sprintf((char *)$$,"<mrow><mtable %s>%s</mtable></mrow>",strdup((char *)$5),strdup((char *)$7));};

arrayopts: anarrayopt {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));};
|
arrayopts anarrayopt
{(char *)$$=malloc(strlen((char *)$1)+strlen((char *)$2)+2);sprintf((char *)$$,"%s %s",strdup((char *)$1),strdup((char *)$2));};

anarrayopt:
collayout   {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| colalign     {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| rowalign   {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| align   {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| eqrows   {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| eqcols  {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| rowlines  {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| collines  {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| frame  {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| padding  {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));};

collayout:  COLLAYOUT ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+14); sprintf((char *)$$,"columnalign=%s",strdup((char *)$2));};

colalign: COLALIGN ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+14); sprintf((char *)$$,"columnalign=%s",strdup((char *)$2));};

rowalign: ROWALIGN ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+10); sprintf((char *)$$,"rowalign=%s",strdup((char *)$2));};

align: ALIGN ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+10); sprintf((char *)$$,"align=%s",strdup((char *)$2));};

eqrows: EQROWS ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+12); sprintf((char *)$$,"equalrows=%s",strdup((char *)$2));};

eqcols: EQCOLS ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+14); sprintf((char *)$$,"equalcolumns=%s",strdup((char *)$2));};

rowlines: ROWLINES ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+14); sprintf((char *)$$,"rowlines=%s",strdup((char *)$2));};

collines: COLLINES ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+14); sprintf((char *)$$,"columnlines=%s",strdup((char *)$2));};


frame: FRAME ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+14); sprintf((char *)$$,"frame=%s",strdup((char *)$2));};

padding: PADDING ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+28); sprintf((char *)$$,"rowspacing=%s columnspacing=%s",strdup((char *)$2),strdup((char *)$2));};


tableRowList: tableRow 
    {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| tableRowList ROWSEP tableRow {(char *)$$=malloc(strlen((char *)$1)+strlen((char *)$3)); sprintf((char *)$$,"%s %s",strdup((char *)$1), strdup((char *)$3));};

tableRow: simpleTableRow {(char *)$$=malloc(strlen((char *)$1)+12); sprintf((char *)$$,"<mtr>%s</mtr>",strdup((char *)$1));}
| optsTableRow {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));};

simpleTableRow: tableCell
  {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| simpleTableRow COLSEP tableCell
  {(char *)$$=malloc(strlen((char *)$1)+strlen((char *)$3)+2); sprintf((char *)$$,"%s %s",strdup((char *)$1), strdup((char *)$3));};

optsTableRow: ROWOPTS MROWOPEN rowopts MROWCLOSE simpleTableRow
 {(char *)$$=malloc(strlen((char *)$3)+strlen((char *)$5)+14); sprintf((char *)$$,"<mtr %s>%s</mtr>",strdup((char *)$3),strdup((char *)$5));};

rowopts: arowopt {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
|
rowopts arowopt
{(char *)$$=malloc(strlen((char *)$1)+strlen((char *)$2)+2);sprintf((char *)$$,"%s %s",strdup((char *)$1),strdup((char *)$2));};

arowopt:
colalign     {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| rowalign   {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));};

tableCell: 
     {(char *)$$=malloc(20); sprintf((char *)$$,"<mtd>&nbsp;</mtd>");}
| compoundTermList
 {(char *)$$=malloc(strlen((char *)$1)+12); sprintf((char *)$$,"<mtd>%s</mtd>",strdup((char *)$1));}
| CELLOPTS MROWOPEN cellopts MROWCLOSE compoundTermList
{(char *)$$=malloc(strlen((char *)$3)+strlen((char *)$5)+14); sprintf((char *)$$,"<mtd %s>%s</mtd>",strdup((char *)$3),strdup((char *)$5));};

cellopts: acellopt {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
|
cellopts acellopt
{(char *)$$=malloc(strlen((char *)$1)+strlen((char *)$2)+2);sprintf((char *)$$,"%s %s",strdup((char *)$1),strdup((char *)$2));};

acellopt:
colalign     {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| rowalign   {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| rowspan   {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));}
| colspan   {(char *)$$=malloc(strlen((char *)$1)); sprintf((char *)$$,"%s",strdup((char *)$1));};

rowspan: ROWSPAN ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+10); sprintf((char *)$$,"rowspan=%s",strdup((char *)$2));};

colspan: COLSPAN ATTRLIST
  {(char *)$$=malloc(strlen((char *)$1)+10); sprintf((char *)$$,"colspan=%s",strdup((char *)$2));};

%%





main()
{
		yyparse();
}

