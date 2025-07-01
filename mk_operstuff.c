#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct
{
	char enumItem[64];
	char operItem[64];
	char precNorm[5];
	char precNone[5];
	char comment[64];
	char fixedComment[128];
} Line_t;

typedef struct
{
	char errName[64];
	char errDesc[128];
} Errors_t;

#define MAX_LINES (64)

int main(int argc, char *argv[])
{
	char *str, *end, buf[sizeof(Line_t)+5], oBuf[sizeof(Line_t)+5];
	int ii, datLineNo, numLines, numErrs;
	FILE *inF;
	Line_t lines[MAX_LINES], *lp;
	Errors_t errors[MAX_LINES], *ep;
	
	inF = fopen("operstuff.dat","r");
	if ( !inF )
	{
		perror("Failed to open operstuff.dat\n");
		return 1;
	}
	numLines = 0;
	numErrs = 0;
	datLineNo = 0;
	lp = lines;
	ep = errors;
	memset(lp,0,sizeof(lines));
	while ( numLines < MAX_LINES && numErrs < MAX_LINES && fgets(buf, sizeof(buf), inF) )
	{
		++datLineNo;
		end = strchr(buf,'\n');
		if ( !end )
		{
			fprintf(stderr,"operstuff.dat:%d: Malformed entry. No newline found: %s\n", datLineNo, buf);
			end = buf;
		}
		*end = 0;
		memcpy(oBuf,buf,sizeof(oBuf));
		if ( buf[0] == ';' )
			continue;
		if ( buf[0] != 'E' && buf[0] != 'B' )
		{
			printf("%s\n",buf);
			continue;
		}
		end = strchr(buf, ',');
		if ( !end )
		{
			fprintf(stderr,"operstuff.dat:%d: Malformed entry. No first comma found: %s\n", datLineNo, oBuf);
			fclose(inF);
			return 1;
		}
		*end = 0;	/* step on comma */
		if ( !strcmp(buf,"B") )
		{
			str = end+1;
			/* eat any leading whitespace */
			while ( isspace(*str) )
				++str;
			/* look for comma 2 */
			end = strchr(str, ',');
			if ( !end )
			{
				fprintf(stderr,"operstuff.dat:%d: Malformed entry. No second comma found: %s\n", datLineNo, oBuf);
				fclose(inF);
				return 1;
			}
			*end = 0;	/* step on comma */
			strncpy(ep->errName,str,sizeof(ep->errName)-1);
			str = end+1;
			/* eat any leading whitespace */
			while ( isspace(*str) )
				++str;
			strncpy(ep->errDesc,str,sizeof(ep->errDesc)-1);
			++ep;
			++numErrs;
			continue;
		}
		memcpy(lp->enumItem, buf, sizeof(lp->enumItem) - 1);
		/* start looking at char after comma */
		str = end+1;
		/* eat any leading whitespace */
		while ( isspace(*str) )
			++str;
		/* look for comma 2 */
		end = strchr(str, ',');
		if ( !end )
		{
			fprintf(stderr,"operstuff.dat:%d: Malformed entry. No second comma found: %s\n", datLineNo, oBuf);
			fclose(inF);
			return 1;
		}
		*end = 0;	/* step on comma */
		strncpy(lp->operItem, str, sizeof(lp->operItem) - 1);
		/* start looking at char after comma */
		str = end+1;
		/* eat any leading whitespace */
		while ( isspace(*str) )
			++str;
		/* look for comma 3 */
		end = strchr(str, ',');
		if ( !end )
		{
			fprintf(stderr,"operstuff.dat:%d: Malformed entry. No third comma found: %s\n", datLineNo, oBuf);
			fclose(inF);
			return 1;
		}
		/* step on comma */
		*end = 0;
		strncpy(lp->precNorm, str, sizeof(lp->precNorm) - 1);
		/* start looking at char after comma */
		str = end+1;
		/* eat any leading whitespace */
		while ( isspace(*str) )
			++str;
		/* look for comma 4 */
		end = strchr(str,',');
		if ( !end )
		{
			fprintf(stderr,"operstuff.dat:%d: Malformed entry. No fourth comma found: %s\n", datLineNo, oBuf);
			fclose(inF);
			return 1;
		}
		/* step on comma */
		*end = 0;
		strncpy(lp->precNone, str, sizeof(lp->precNone)-1);
		/* start looking at char after comma */
		str = end+1;
		/* eat any leading whitespace */
		while ( isspace(*str) )
			++str;
		strncpy(lp->comment, str, sizeof(lp->comment)-1);
		snprintf(lp->fixedComment, sizeof(lp->fixedComment) - 1, "/* %s: %s", lp->enumItem, lp->comment+3);
		++lp;
		++numLines;
	}
	fclose(inF);
	inF = NULL;
	fputs("#if OPERSTUFF_GET_ENUM\n"
		  "typedef enum\n{\n",stdout);
	lp = lines;
	for (ii=0; ii < numLines; ++ii, ++lp)
	{
		fprintf(stdout,"   %s%s\t%s\n", lp->enumItem, ii < numLines-1 ? ",":"", lp->comment );
	}
	fputs("} ExprsTermTypes_t;\n\n"
		  "typedef enum\n{\n"
		  ,stdout);
	ep = errors;
	for (ii=0; ii < numErrs; ++ii, ++ep)
	{
		fprintf(stdout, "   %s%s\t/* %s */\n", ep->errName, ii < numErrs - 1 ? "," : "", ep->errDesc);
	}
	fputs("} ExprsErrs_t;\n", stdout);
	fputs("#undef OPERSTUFF_GET_ENUM\n"
		  "#endif\n\n"
		  ,stdout);

	fputs("#if OPERSTUFF_GET_OTHERS\n"
		  "static const unsigned short OperXlate[] =\n"
		  "{\n"
		  ,stdout);
	lp = lines;
	for (ii=0; ii < numLines; ++ii, ++lp)
	{
		fprintf(stdout,"   %s%s\t%s\n", lp->operItem, ii < numLines-1 ? ",":"", lp->fixedComment );
	}
	fputs("};\n\n", stdout);
	fputs("static const ExprsPrecedence_t PrecedenceNormal[] =\n"
		  "{\n"
		  ,stdout);
	lp = lines;
	for (ii=0; ii < numLines; ++ii, ++lp)
	{
		fprintf(stdout, "   %s%s\t%s\n", lp->precNorm, ii < numLines - 1 ? "," : "", lp->fixedComment);
	}
	fputs("};\n\n"
		  "static const ExprsPrecedence_t PrecedenceNone[] =\n"
		  "{\n"
		  ,stdout);
	lp = lines;
	for (ii=0; ii < numLines; ++ii, ++lp)
	{
		fprintf(stdout, "   %s%s\t%s\n", lp->precNone, ii < numLines - 1 ? "," : "", lp->fixedComment);
	}
	fputs("};\n\n", stdout);
	fputs("static const char *ErrorDescriptions[] =\n{\n",stdout);
	ep = errors;
	for (ii=0; ii < numErrs; ++ii, ++ep)
	{
		fprintf(stdout, "   %s%s\t/* %s */\n", ep->errDesc, ii < numErrs - 1 ? "," : "", ep->errName);
	}
	fputs("};\n", stdout);
	fputs("#undef OPERSTUFF_GET_OTHERS\n"
		  "#endif\n"
		  ,stdout);
	return 0;	
}

