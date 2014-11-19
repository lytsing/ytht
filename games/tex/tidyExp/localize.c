/*
  localize.c

  (c) 1998-2000 (W3C) MIT, INRIA, Keio University
  See tidy.c for the copyright notice.

  You should only need to edit this file and tidy.c
  to localize HTML tidy.
*/

#include "platform.h"
#include "html.h"

#define ACCESS_URL  "http://www.w3.org/WAI/GL"

char *release_date = "13th January 2000";

extern uint optionerrors;

/*
 This routine is the single point via which
 all output is written and as such is a good
 way to interface Tidy to other code when
 embedding Tidy in a GUI application.
*/
void tidy_out(FILE *fp, const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(fp, msg, args);
    va_end(args);
}

void ReadingFromStdin()
{
    fprintf(stderr, "Reading markup from standard input ...\n");
}

void ShowVersion(FILE *fp)
{
    tidy_out(fp, "HTML Tidy release date: %s\n"
            "See http://www.w3.org/People/Raggett for details\n", release_date);
}

void FileError(FILE *fp, const char *file)
{
    tidy_out(fp, "Can't open \"%s\"\n", file);
}

void ReportTag(Lexer *lexer, Node *tag)
{
    if (tag)
    {
        if (tag->type == StartTag)
            tidy_out(lexer->errout, "<%s>", tag->element);
        else if (tag->type == EndTag)
            tidy_out(lexer->errout, "</%s>", tag->element);
        else if (tag->type == DocTypeTag)
            tidy_out(lexer->errout, "<!DOCTYPE>");
        else if (tag->type == TextNode)
            tidy_out(lexer->errout, "plain text");
        else
            tidy_out(lexer->errout, "%s", tag->element);
    }
}

/* lexer is not defined when this is called */
void ReportUnknownOption(char *option)
{
    optionerrors++;
    fprintf(stderr, "Warning - unknown option: %s\n", option);
}

/* lexer is not defined when this is called */
void ReportBadArgument(char *option)
{
    optionerrors++;
    fprintf(stderr, "Warning - missing or malformed argument for option: %s\n", option);
}

void NtoS(int n, char *str)
{
    char buf[40];
    int i;

    for (i = 0;; ++i)
    {
        buf[i] = (n % 10) + '0';

        n = n /10;

        if (n == 0)
            break;
    }

    n = i;

    while (i >= 0)
    {
        str[n-i] = buf[i];
        --i;
    }

    str[n+1] = '\0';
}

void ReportPosition(Lexer *lexer)
{
    tidy_out(lexer->errout, "line %d", lexer->lines);
    tidy_out(lexer->errout, " column %d - ", lexer->columns);
}

void ReportEncodingError(Lexer *lexer, uint code, uint c)
{
    char buf[32];

    lexer->warnings++;

    if (ShowWarnings)
    {
        ReportPosition(lexer);

        if (code == WINDOWS_CHARS)
        {
            NtoS(c, buf);
            lexer->badChars |= WINDOWS_CHARS;
            tidy_out(lexer->errout, "Warning: replacing illegal character code %s", buf);
        }

        tidy_out(lexer->errout, "\n");
    }
}

void ReportEntityError(Lexer *lexer, uint code, char *entity, int c)
{
    lexer->warnings++;

    if (ShowWarnings)
    {
        ReportPosition(lexer);

        if (code == MISSING_SEMICOLON)
        {
            tidy_out(lexer->errout, "Warning: entity \"%s\" doesn't end in ';'", entity);
        }
        else if (code == UNKNOWN_ENTITY)
        {
            tidy_out(lexer->errout, "Warning: unescaped & or unknown entity \"%s\"", entity);
        }
        else if (code == UNESCAPED_AMPERSAND)
        {
            tidy_out(lexer->errout, "Warning: unescaped & which should be written as &amp;");
        }

        tidy_out(lexer->errout, "\n");
    }
}

void ReportAttrError(Lexer *lexer, Node *node, char *attr, uint code)
{
    lexer->warnings++;

    /* keep quiet after 6 errors */
    if (lexer->errors > 6)
        return;

    if (ShowWarnings)
    {
        /* on end of file adjust reported position to end of input */
        if (code == UNEXPECTED_END_OF_FILE)
        {
            lexer->lines = lexer->in->curline;
            lexer->columns = lexer->in->curcol;
        }

        ReportPosition(lexer);

        if (code == UNKNOWN_ATTRIBUTE)
            tidy_out(lexer->errout, "Warning: unknown attribute \"%s\"", attr);
        else if (code == MISSING_ATTRIBUTE)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " lacks \"%s\" attribute", attr);
        }
        else if (code == MISSING_ATTR_VALUE)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " attribute \"%s\" lacks value", attr);
        }
        else if (code == MISSING_IMAGEMAP)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " should use client-side image map");
            lexer->badAccess |= MISSING_IMAGE_MAP;
        }
        else if (code == BAD_ATTRIBUTE_VALUE)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " unknown attribute value \"%s\"", attr);
        }
        else if (code == XML_ATTRIBUTE_VALUE)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " has XML attribute \"%s\"", attr);
        }
        else if (code == UNEXPECTED_GT)
        {
            tidy_out(lexer->errout, "Error: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " missing '>' for end of tag");
            lexer->errors++;;
        }
        else if (code == UNEXPECTED_QUOTEMARK)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " unexpected or duplicate quote mark");
        }
        else if (code == REPEATED_ATTRIBUTE)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " repeated attribute \"%s\"", attr);
        }
        else if (code == PROPRIETARY_ATTR_VALUE)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " proprietary attribute value \"%s\"", attr);
        }
        else if (code == UNEXPECTED_END_OF_FILE)
        {
            tidy_out(lexer->errout, "Warning: end of file while parsing attributes");
        }
        else if (code == ID_NAME_MISMATCH)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " id and name attribute value mismatch");
        }

        tidy_out(lexer->errout, "\n");
    }
    else if (code == UNEXPECTED_GT)
    {
        ReportPosition(lexer);
        tidy_out(lexer->errout, "Error: ");
        ReportTag(lexer, node);
        tidy_out(lexer->errout, " missing '>' for end of tag\n");
        lexer->errors++;;
    }
}

void ReportWarning(Lexer *lexer, Node *element, Node *node, uint code)
{
    lexer->warnings++;

    /* keep quiet after 6 errors */
    if (lexer->errors > 6)
        return;

    if (ShowWarnings)
    {
        /* on end of file adjust reported position to end of input */
        if (code == UNEXPECTED_END_OF_FILE)
        {
            lexer->lines = lexer->in->curline;
            lexer->columns = lexer->in->curcol;
        }

        ReportPosition(lexer);

        if (code == MISSING_ENDTAG_FOR)
            tidy_out(lexer->errout, "Warning: missing </%s>", element->element);
        else if (code == MISSING_ENDTAG_BEFORE)
        {
            tidy_out(lexer->errout, "Warning: missing </%s> before ", element->element);
            ReportTag(lexer, node);
        }
        else if (code == DISCARDING_UNEXPECTED)
        {
            tidy_out(lexer->errout, "Warning: discarding unexpected ");
            ReportTag(lexer, node);
        }
        else if (code == NESTED_EMPHASIS)
        {
            tidy_out(lexer->errout, "Warning: nested emphasis ");
            ReportTag(lexer, node);
        }
        else if (code == COERCE_TO_ENDTAG)
        {
            tidy_out(lexer->errout, "Warning: <%s> is probably intended as </%s>",
                node->element, node->element);
        }
        else if (code == NON_MATCHING_ENDTAG)
        {
            tidy_out(lexer->errout, "Warning: replacing unexpected ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " by </%s>", element->element);
        }
        else if (code == TAG_NOT_ALLOWED_IN)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " isn't allowed in <%s> elements", element->element);
        }
        else if (code == DOCTYPE_AFTER_TAGS)
        {
            tidy_out(lexer->errout, "Warning: <!DOCTYPE> isn't allowed after elements");
        }
        else if (code == MISSING_STARTTAG)
            tidy_out(lexer->errout, "Warning: missing <%s>", node->element);
        else if (code == UNEXPECTED_ENDTAG)
        {
            tidy_out(lexer->errout, "Warning: unexpected </%s>", node->element);

            if (element)
                tidy_out(lexer->errout, " in <%s>", element->element);
        }
        else if (code == USING_BR_INPLACE_OF)
        {
            tidy_out(lexer->errout, "Warning: using <br> in place of ");
            ReportTag(lexer, node);
        }
        else if (code == INSERTING_TAG)
            tidy_out(lexer->errout, "Warning: inserting implicit <%s>", node->element);
        else if (code == CANT_BE_NESTED)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " can't be nested");
        }
        else if (code == PROPRIETARY_ELEMENT)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " is not approved by W3C");

            if (node->tag == tag_layer)
                lexer->badLayout |= USING_LAYER;
            else if (node->tag == tag_spacer)
                lexer->badLayout |= USING_SPACER;
            else if (node->tag == tag_nobr)
                lexer->badLayout |= USING_NOBR;
        }
        else if (code == OBSOLETE_ELEMENT)
        {
            if (element->tag && (element->tag->model & CM_OBSOLETE))
                tidy_out(lexer->errout, "Warning: replacing obsolete element ");
            else
                tidy_out(lexer->errout, "Warning: replacing element ");

            ReportTag(lexer, element);
            tidy_out(lexer->errout, " by ");
            ReportTag(lexer, node);
        }
        else if (code == TRIM_EMPTY_ELEMENT)
        {
            tidy_out(lexer->errout, "Warning: trimming empty ");
            ReportTag(lexer, element);
        }
        else if (code == MISSING_TITLE_ELEMENT)
            tidy_out(lexer->errout, "Warning: inserting missing 'title' element");
        else if (code == ILLEGAL_NESTING)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, element);
            tidy_out(lexer->errout, " shouldn't be nested");
        }
        else if (code == NOFRAMES_CONTENT)
        {
            tidy_out(lexer->errout, "Warning: ");
            ReportTag(lexer, node);
            tidy_out(lexer->errout, " not inside 'noframes' element");
        }
        else if (code == INCONSISTENT_VERSION)
        {
            tidy_out(lexer->errout, "Warning: html doctype doesn't match content");
        }
        else if (code == MALFORMED_DOCTYPE)
        {
            tidy_out(lexer->errout, "Warning: expected \"html PUBLIC\" or \"html SYSTEM\"");
        }
        else if (code == CONTENT_AFTER_BODY)
        {
            tidy_out(lexer->errout, "Warning: content occurs after end of body");
        }
        else if (code == MALFORMED_COMMENT)
        {
            tidy_out(lexer->errout, "Warning: adjacent hyphens within comment");
        }
        else if (code == BAD_COMMENT_CHARS)
        {
            tidy_out(lexer->errout, "Warning: expecting -- or >");
        }
        else if (code == BAD_XML_COMMENT)
        {
            tidy_out(lexer->errout, "Warning: XML comments can't contain --");
        }
        else if (code == BAD_CDATA_CONTENT)
        {
            tidy_out(lexer->errout, "Warning: '<' + '/' + letter not allowed here");
        }
        else if (code == INCONSISTENT_NAMESPACE)
        {
            tidy_out(lexer->errout, "Warning: html namespace doesn't match content");
        }
        else if (code == UNEXPECTED_END_OF_FILE)
        {
            tidy_out(lexer->errout, "Warning: unexpected end of file");
            ReportTag(lexer, element);
        }

        tidy_out(lexer->errout, "\n");
    }
}

void ReportError(Lexer *lexer, Node *element, Node *node, uint code)
{
    lexer->warnings++;

    /* keep quiet after 6 errors */
    if (lexer->errors > 6)
        return;

    lexer->errors++;

    ReportPosition(lexer);

    if (code == SUSPECTED_MISSING_QUOTE)
    {
        tidy_out(lexer->errout, "Error: missing quotemark for attribute value");
    }
    else if (code == DUPLICATE_FRAMESET)
    {
        tidy_out(lexer->errout, "Error: repeated FRAMESET element");
    }
    else if (code == UNKNOWN_ELEMENT)
    {
        tidy_out(lexer->errout, "Error: ");
        ReportTag(lexer, node);
        tidy_out(lexer->errout, " is not recognized!");
    }
    else if (code == UNEXPECTED_ENDTAG)  /* generated by XML docs */
    {
        tidy_out(lexer->errout, "Warning: unexpected </%s>", node->element);

        if (element)
            tidy_out(lexer->errout, " in <%s>", element->element);
    }

    tidy_out(lexer->errout, "\n");
}

void ErrorSummary(Lexer *lexer)
{
    /* adjust badAccess to that its null if frames are ok */
    if (lexer->badAccess & (USING_FRAMES | USING_NOFRAMES))
    {
        if (!((lexer->badAccess & USING_FRAMES) && !(lexer->badAccess & USING_NOFRAMES)))
            lexer->badAccess &= ~(USING_FRAMES | USING_NOFRAMES);
    }

    if (lexer->badChars)
    {
        if (lexer->badChars & WINDOWS_CHARS)
        {
            tidy_out(lexer->errout, "Characters codes for the Microsoft Windows fonts in the range\n");
            tidy_out(lexer->errout, "128 - 159 may not be recognized on other platforms. You are\n");
            tidy_out(lexer->errout, "instead recommended to use named entities, e.g. &trade; rather\n");
            tidy_out(lexer->errout, "than Windows character code 153 (0x2122 in Unicode). Note that\n");
            tidy_out(lexer->errout, "as of February 1998 few browsers support the new entities.\n\n");
        }
    }

    if (lexer->badForm)
    {
        tidy_out(lexer->errout, "You may need to move one or both of the <form> and </form>\n");
        tidy_out(lexer->errout, "tags. HTML elements should be properly nested and form elements\n");
        tidy_out(lexer->errout, "are no exception. For instance you should not place the <form>\n");
        tidy_out(lexer->errout, "in one table cell and the </form> in another. If the <form> is\n");
        tidy_out(lexer->errout, "placed before a table, the </form> cannot be placed inside the\n");
        tidy_out(lexer->errout, "table! Note that one form can't be nested inside another!\n\n");
    }
    
    if (lexer->badAccess)
    {
        if (lexer->badAccess & MISSING_SUMMARY)
        {
            tidy_out(lexer->errout, "The table summary attribute should be used to describe\n");
            tidy_out(lexer->errout, "the table structure. It is very helpful for people using\n");
            tidy_out(lexer->errout, "non-visual browsers. The scope and headers attributes for\n");
            tidy_out(lexer->errout, "table cells are useful for specifying which headers apply\n");
            tidy_out(lexer->errout, "to each table cell, enabling non-visual browsers to provide\n");
            tidy_out(lexer->errout, "a meaningful context for each cell.\n\n");
        }

        if (lexer->badAccess & MISSING_IMAGE_ALT)
        {
            tidy_out(lexer->errout, "The alt attribute should be used to give a short description\n");
            tidy_out(lexer->errout, "of an image; longer descriptions should be given with the\n");
            tidy_out(lexer->errout, "longdesc attribute which takes a URL linked to the description.\n");
            tidy_out(lexer->errout, "These measures are needed for people using non-graphical browsers.\n\n");
        }

        if (lexer->badAccess & MISSING_IMAGE_MAP)
        {
            tidy_out(lexer->errout, "Use client-side image maps in preference to server-side image\n");
            tidy_out(lexer->errout, "maps as the latter are inaccessible to people using non-\n");
            tidy_out(lexer->errout, "graphical browsers. In addition, client-side maps are easier\n");
            tidy_out(lexer->errout, "to set up and provide immediate feedback to users.\n\n");
        }

        if (lexer->badAccess & MISSING_LINK_ALT)
        {
            tidy_out(lexer->errout, "For hypertext links defined using a client-side image map, you\n");
            tidy_out(lexer->errout, "need to use the alt attribute to provide a textual description\n");
            tidy_out(lexer->errout, "of the link for people using non-graphical browsers.\n\n");
        }

        if ((lexer->badAccess & USING_FRAMES) && !(lexer->badAccess & USING_NOFRAMES))
        {
            tidy_out(lexer->errout, "Pages designed using frames presents problems for\n");
            tidy_out(lexer->errout, "people who are either blind or using a browser that\n");
            tidy_out(lexer->errout, "doesn't support frames. A frames-based page should always\n");
            tidy_out(lexer->errout, "include an alternative layout inside a NOFRAMES element.\n\n");
        }

        tidy_out(lexer->errout, "For further advice on how to make your pages accessible\n");
        tidy_out(lexer->errout, "see \"%s\". You may also want to try\n", ACCESS_URL);
        tidy_out(lexer->errout, "\"http://www.cast.org/bobby/\" which is a free Web-based\n");
        tidy_out(lexer->errout, "service for checking URLs for accessibility.\n\n");
    }

    if (lexer->badLayout)
    {
        if (lexer->badLayout & USING_LAYER)
        {
            tidy_out(lexer->errout, "The Cascading Style Sheets (CSS) Positioning mechanism\n");
            tidy_out(lexer->errout, "is recommended in preference to the proprietary <LAYER>\n");
            tidy_out(lexer->errout, "element due to limited vendor support for LAYER.\n\n");
        }

        if (lexer->badLayout & USING_SPACER)
        {
            tidy_out(lexer->errout, "You are recommended to use CSS for controlling white\n");
            tidy_out(lexer->errout, "space (e.g. for indentation, margins and line spacing).\n");
            tidy_out(lexer->errout, "The proprietary <SPACER> element has limited vendor support.\n\n");
        }

        if (lexer->badLayout & USING_FONT)
        {
            tidy_out(lexer->errout, "You are recommended to use CSS to specify the font and\n");
            tidy_out(lexer->errout, "properties such as its size and color. This will reduce\n");
            tidy_out(lexer->errout, "the size of HTML files and make them easier maintain\n");
            tidy_out(lexer->errout, "compared with using <FONT> elements.\n\n");
        }

        if (lexer->badLayout & USING_NOBR)
        {
            tidy_out(lexer->errout, "You are recommended to use CSS to control line wrapping.\n");
            tidy_out(lexer->errout, "Use \"white-space: nowrap\" to inhibit wrapping in place\n");
            tidy_out(lexer->errout, "of inserting <NOBR>...</NOBR> into the markup.\n\n");
        }
    }
}

void UnknownOption(FILE *errout, char c)
{
    tidy_out(errout, "unrecognized option -%c use -help to list options\n", c);
}

void UnknownFile(FILE *errout, char *program, char *file)
{
    tidy_out(errout, "%s: can't open file \"%s\"\n", program, file);
}

void NeedsAuthorIntervention(FILE *errout)
{
    tidy_out(errout, "This document has errors that must be fixed before\n");
    tidy_out(errout, "using HTML Tidy to generate a tidied up version.\n\n");
}

void MissingBody(FILE *errout)
{
    tidy_out(errout, "Can't create slides - document is missing a body element.\n");
}

void ReportNumberOfSlides(FILE *errout, int count)
{
    tidy_out(errout, "%d Slides found\n", count);
}

void GeneralInfo(FILE *errout)
{
    tidy_out(errout, "HTML & CSS specifications are available from http://www.w3.org/\n");
    tidy_out(errout, "To learn more about Tidy see http://www.w3.org/People/Raggett/tidy/\n");
    tidy_out(errout, "Please send bug reports to Dave Raggett care of <html-tidy@w3.org>\n");
    tidy_out(errout, "Lobby your company to join W3C, see http://www.w3.org/Consortium\n");
}

void HelloMessage(FILE *errout, char *date, char *filename)
{
    if (wstrcmp(filename, "stdin") == 0)
        tidy_out(errout, "\nTidy (vers %s) Parsing console input (stdin)\n", date);
    else
        tidy_out(errout, "\nTidy (vers %s) Parsing \"%s\"\n", date, filename);
}

void ReportVersion(FILE *errout, char *filename, char *vers)
{
    tidy_out(errout, "\n\"%s\" appears to be %s\n", filename, (vers ? vers : "HTML proprietary"));
}

void ReportNumWarnings(FILE *errout, Lexer *lexer)
{
    if (lexer->warnings > 0)
        tidy_out(errout, "%d warnings/errors were found!\n\n", lexer->warnings);
    else
        tidy_out(errout, "no warnings or errors were found\n\n");
}

void HelpText(FILE *out, char *prog)
{
    tidy_out(out, "%s: file1 file2 ...\n", prog);
    tidy_out(out, "Utility to clean up & pretty print html files\n");
    tidy_out(out, "see http://www.w3.org/People/Raggett/tidy/\n");
    tidy_out(out, "options for tidy released on %s\n", release_date);
    tidy_out(out, "  -config <file>  set options from config file\n");
    tidy_out(out, "  -indent or -i   indent element content\n");
    tidy_out(out, "  -omit   or -o   omit optional endtags\n");
    tidy_out(out, "  -wrap 72        wrap text at column 72 (default is 68)\n");
    tidy_out(out, "  -upper  or -u   force tags to upper case (default is lower)\n");
    tidy_out(out, "  -clean  or -c   replace font, nobr & center tags by CSS\n");
    tidy_out(out, "  -raw            leave chars > 128 unchanged upon output\n");
    tidy_out(out, "  -ascii          use ASCII for output, Latin-1 for input\n");
    tidy_out(out, "  -latin1         use Latin-1 for both input and output\n");
    tidy_out(out, "  -iso2022        use ISO2022 for both input and output\n");
    tidy_out(out, "  -utf8           use UTF-8 for both input and output\n");
    tidy_out(out, "  -mac            use the Apple MacRoman character set\n");
    tidy_out(out, "  -numeric or -n  output numeric rather than named entities\n");
    tidy_out(out, "  -modify or -m   to modify original files\n");
    tidy_out(out, "  -errors or -e   only show errors\n");
    tidy_out(out, "  -quiet or -q    suppress nonessential output\n");
    tidy_out(out, "  -f <file>       write errors to <file>\n");
    tidy_out(out, "  -xml            use this when input is wellformed xml\n");
    tidy_out(out, "  -asxml          to convert html to wellformed xml\n");
    tidy_out(out, "  -slides         to burst into slides on h2 elements\n");
    tidy_out(out, "  -help   or -h   list command line options\n");
    tidy_out(out, "Input/Output default to stdin/stdout respectively\n");
    tidy_out(out, "Single letter options apart from -f may be combined\n");
    tidy_out(out, "as in:  tidy -f errs.txt -imu foo.html\n");
    tidy_out(out, "For further info on HTML see http://www.w3.org/MarkUp\n");
}


