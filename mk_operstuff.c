#include <stdio.h>
#include <string.h>

static const char License1[] =
"/*\n"
"    operstuff.h - Part of macxx, a cross assembler family for various micro-processors\n"
"    Copyright (C) 2025 David Shepperd\n"
"\n"
"    This program is free software: you can redistribute it and/or modify\n"
"    it under the terms of the GNU General Public License as published by\n"
"    the Free Software Foundation, either version 3 of the License, or\n"
"    (at your option) any later version.\n"
"\n";
static const char License2[] =
"    This program is distributed in the hope that it will be useful,\n"
"    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"    GNU General Public License for more details.\n"
"\n"
"    You should have received a copy of the GNU General Public License\n"
"    along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
"*/\n\n\n";
static const char Note[] =
"/**************************************************************************\n"
" * @note This file is produced by a separate program called mk_operstuff. *\n"
" * Any manual edits made to this file will likely be lost during the next *\n"
" * build. Edit mk_operstuff.c and/or operstuff.dat to make any necessary  *\n"
" * changes to these lists.                                                *\n"
" **************************************************************************/\n"
"\n\n\n"
;

typedef struct
{
	char enumItem[64];
	char operItem[64];
	char precNorm[5];
	char precNone[5];
	char comment[64];
	char fixedComment[128];
} Line_t;

#define MAX_LINES (32)

int main(int argc, char *argv[])
{
	char *str, *end, buf[sizeof(Line_t)+5];
	int ii, lineNo;
	FILE *inF;
	Line_t lines[MAX_LINES], *lp;
	
	inF = fopen("operstuff.dat","r");
	if ( !inF )
	{
		perror("Failed to open operstuff.dat\n");
		return 1;
	}
	lineNo = 0;
	lp = lines;
	memset(lp,0,sizeof(lines));
	while ( lineNo < MAX_LINES && fgets(buf, sizeof(buf), inF) )
	{
		++lineNo;
		end = strchr(buf,'\n');
		if ( !end )
		{
			fprintf(stderr,"operstuff.dat:%d: Malformed entry. No newline found: %s\n", lineNo, buf);
			fclose(inF);
			return 1;
		}
		*end = 0;
		if ( end == buf )
			break;
		str = buf;
		end = strchr(buf, '\t');
		if ( !end || end[-1] != ',')
		{
			fprintf(stderr,"operstuff.dat:%d: Malformed entry. No first tab or comma found: %s\n", lineNo, buf);
			fclose(inF);
			return 1;
		}
		end[-1] = 0;
		memcpy(lp->enumItem, str, sizeof(lp->enumItem)-1);
		str = end+1;
		end = strchr(str,'\t');
		if ( !end || end[-1] != ',')
		{
			fprintf(stderr,"operstuff.dat:%d: Malformed entry. No second tab or comma found: %s\n", lineNo, buf);
			fclose(inF);
			return 1;
		}
		end[-1] = 0;
		strncpy(lp->operItem, str, sizeof(lp->operItem) - 1);
		str = end+1;
		end = strchr(str,'\t');
		if ( !end || end[-1] != ',')
		{
			fprintf(stderr,"operstuff.dat:%d: Malformed entry. No third tab or comma found: %s\n", lineNo, buf);
			fclose(inF);
			return 1;
		}
		end[-1] = 0;
		strncpy(lp->precNorm, str, sizeof(lp->precNorm) - 1);
		str = end+1;
		end = strchr(str,'\t');
		if ( !end || end[-1] != ',')
		{
			fprintf(stderr,"operstuff.dat:%d: Malformed entry. No fourth tab or comma found: %s\n", lineNo, buf);
			fclose(inF);
			return 1;
		}
		end[-1] = 0;
		strncpy(lp->precNone, str, sizeof(lp->precNone)-1);
		str = end+1;
		strncpy(lp->comment, str, sizeof(lp->comment)-1);
		snprintf(lp->fixedComment, sizeof(lp->fixedComment) - 1, "/* %s: %s", lp->enumItem, lp->comment+3);
		++lp;
	}
	fclose(inF);
	inF = NULL;
	lineNo = lp-lines;
	fputs(License1, stdout);
	fputs(License2, stdout);
	fputs(Note, stdout);
	fputs("#if OPERSTUFF_GET_ENUM\n"
		  "typedef enum {\n",stdout);
	lp = lines;
	for (ii=0; ii < lineNo; ++ii, ++lp)
	{
		fprintf(stdout,"   %s%s\t%s\n", lp->enumItem, ii < lineNo-1 ? ",":"", lp->comment );
	}
	fputs("} OperType_t;\n"
		  "#undef OPERSTUFF_GET_ENUM\n"
		  "#endif\n\n",
		  stdout);

	fputs("#if OPERSTUFF_GET_OTHERS\n"
		  "static const unsigned short OperXlate[] =\n"
		  "{\n"
		  ,stdout);
	lp = lines;
	for (ii=0; ii < lineNo; ++ii, ++lp)
	{
		fprintf(stdout,"   %s%s\t%s\n", lp->operItem, ii < lineNo-1 ? ",":"", lp->fixedComment );
	}
	fputs("};\n\n", stdout);
	fputs("static const ExprsPrecedence_t PrecedenceNormal[] =\n"
		  "{\n"
		  ,stdout);
	lp = lines;
	for (ii=0; ii < lineNo; ++ii, ++lp)
	{
		fprintf(stdout, "   %s%s\t%s\n", lp->precNorm, ii < lineNo - 1 ? "," : "", lp->fixedComment);
	}
	fputs("};\n\n"
		  "static const ExprsPrecedence_t PrecedenceNone[] =\n"
		  "{\n"
		  ,stdout);
	lp = lines;
	for (ii=0; ii < lineNo; ++ii, ++lp)
	{
		fprintf(stdout, "   %s%s\t%s\n", lp->precNone, ii < lineNo - 1 ? "," : "", lp->fixedComment);
	}
	fputs("};\n\n"
		  "#undef OPERSTUFF_GET_OTHERS\n"
		  "#endif\n"
		  ,stdout);
	return 0;	
}

