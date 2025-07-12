#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "formats.h"

#define OP_ALL		(1)	/* A */
#define OP_NO_PP	(2)	/* N */
#define OP_ONLY_PP	(3)	/* O */
#define OP_LAST		(4) /* L */

typedef struct
{
	int op;  /* OP - one of A, N or O */
	int val; /* Val - 1 if no value is allowed */
	int opt; /* Opt - 1 if value is optional */
	int num; /* Num - 1 if value must be a number */
	int out; /* Out - 1 if param is an output file */
	int str; /* Str - 1 if value is a string */
	int neg; /* Neg - 1 if param is negatible */
	char enumName[32]; /* Enum - name of enum */
	char name[32]; /* Name - name of parameter */
	char outIdx[32]; /* OutIdx - if param is output file, the index of same */
	char comment[64]; /* Comment - comment to include */
	char fixedComment[128];
} Line_t;

typedef struct
{
	char str[128];	/* struct entry */
} Structs_t;

#define MAX_LINES (64)

int main(int argc, char *argv[])
{
	char *str, *end, inBuf[256], tokBuf[sizeof(inBuf)];
	int ii, datLineNo, numLines, numStructs, lastOp;
	FILE *inF;
	Line_t lines[MAX_LINES], *lp, *ll;
	Structs_t structs[MAX_LINES], *sp;
	
	inF = fopen("qualtbl.dat","r");
	if ( !inF )
	{
		perror("Failed to open qualtbl.dat\n");
		return 1;
	}
	numLines = 0;
	datLineNo = 0;
	lp = lines;
	memset(lp,0,sizeof(lines));
	sp = structs;
	numStructs = 0;
	memset(sp,0,sizeof(structs));
	while ( numLines < MAX_LINES && numStructs < MAX_LINES && fgets(inBuf, sizeof(inBuf), inF) )
	{
		++datLineNo;
		end = strchr(inBuf,'\n');
		if ( !end )
		{
			fprintf(stderr,"qualtbl.dat:%d: Malformed entry. No newline found: %s\n", datLineNo, inBuf);
			end = inBuf;
		}
		*end = 0;
		memcpy(tokBuf,inBuf,sizeof(tokBuf));
		tokBuf[sizeof(tokBuf)-1] = 0;
		if ( inBuf[0] == ';' )
			continue;
		if ( inBuf[0] == 'S' && inBuf[1] == ',' )
		{
			size_t len = strlen(inBuf+2);
			if ( len > sizeof(sp->str)-1 )
				len = sizeof(sp->str)-1;
			memcpy(sp->str,inBuf+2,len);
			++sp;
			++numStructs;
			continue;
		}
		if ( inBuf[0] == 'A' )
			lp->op = OP_ALL;
		else if ( inBuf[0] == 'N' )
			lp->op = OP_NO_PP;
		else if ( inBuf[0] == 'O' )
			lp->op = OP_ONLY_PP;
		else if ( (inBuf[0] == 'L') )
			lp->op = OP_LAST;
		if ( !lp->op )
		{
			printf("%s\n",inBuf);
			continue;
		}
		if ( inBuf[1] != ',' )
		{
			fprintf(stderr,"qualtbl.dat:%d: Malformed entry. No first comma found: %s\n", datLineNo, inBuf);
			fclose(inF);
			return 1;
		}
		ii = 0;
		str = strtok(tokBuf,",");
		for ( ; str; ++ii, str=strtok(NULL,",") )
		{
			switch (ii)
			{
			case 0:
				continue;
			case 1:
				lp->val = atoi(str);
				continue;
			case 2:
				lp->opt = atoi(str);
				continue;
			case 3:
				lp->num = atoi(str);
				continue;
			case 4:
				lp->out = atoi(str);
				continue;
			case 5:
				lp->str = atoi(str);
				continue;
			case 6:
				lp->neg = atoi(str);
				continue;
			case 7:
				while ( isspace(*str) )
					++str;
				strncpy(lp->enumName,str,sizeof(lp->enumName)-1);
				continue;
			case 8:
				while ( isspace(*str) )
					++str;
				strncpy(lp->name,str,sizeof(lp->name)-1);
				continue;
			case 9:
				while ( isspace(*str) )
					++str;
				strncpy(lp->outIdx,str,sizeof(lp->outIdx)-1);
				continue;
			case 10:
				while ( isspace(*str) )
					++str;
				strncpy(lp->comment,str,sizeof(lp->comment)-1);
				continue;
			default:
				fprintf(stderr,"qualtbl.dat:%d: Malformed entry. too many terms: %s\n", datLineNo, inBuf);
				fclose(inF);
				return 1;
			}
		}
		if ( !ii )
		{
			fprintf(stderr,"qualtbl.dat:%d: Malformed entry. strtok() failed to find term: %s\n", datLineNo, inBuf);
			fclose(inF);
			return 1;
		}
		if ( lp->comment[0] )
			snprintf(lp->fixedComment, sizeof(lp->fixedComment) - 1, "/* %s: %s", lp->enumName, lp->comment + 3);
		++lp;
		++numLines;
	}
	fclose(inF);
	inF = NULL;
	fprintf(stdout,"/* numLines=%d, __SIZEOF_SIZE_T__=%d, __SIZEOF_INT__=%d, __SIZEOF_LONG__=%d */\n",
			numLines,
			__SIZEOF_SIZE_T__,
			__SIZEOF_INT__,
			__SIZEOF_LONG__);
	fprintf(stdout,	"/* sizeof(char)=" FMT_SZ
					", sizeof(int)=" FMT_SZ
					", sizeof(long)=" FMT_SZ
					", sizeof(void *)=" FMT_SZ
					" */\n/* "
					"sizeof(int8_t)=" FMT_SZ 
					", sizeof(int16_t)=" FMT_SZ 
					", sizeof(int32_t)=" FMT_SZ
					" */\n/* "
					"sizeof(sizeof)=" FMT_SZ
					", sizeof(size_t)=" FMT_SZ
					", sizeof(time_t)=" FMT_SZ 
					" */\n\n"
				,sizeof(char)
				,sizeof(int)
				,sizeof(long)
				,sizeof(void *)
				,sizeof(int8_t)
				,sizeof(int16_t)
				,sizeof(int32_t)
				,sizeof(sizeof(char))
				,sizeof(size_t)
				,sizeof(time_t)
			);
	fputs("#if QUALTBL_GET_ENUM\n"
		  "typedef enum\n{\n",stdout);
	lp = lines;
	lastOp = 0;
	for (ii=0; ii < numLines; ++ii, ++lp)
	{
		if ( lp->op != lastOp )
		{
			if ( lastOp == OP_NO_PP || lastOp == OP_ONLY_PP )
				fputs("#endif\n",stdout);
			if ( lp->op == OP_NO_PP )
				fputs("#if !defined(MAC_PP)\n",stdout);
			else if ( lp->op == OP_ONLY_PP )
				fputs("#if defined(MAC_PP)\n",stdout);
		}
		fprintf(stdout, "    %s%s\t%s\n", lp->enumName, lp->op != OP_LAST ? "," : "", lp->comment);
		lastOp = lp->op;
	}
	if ( lastOp == OP_NO_PP || lastOp == OP_ONLY_PP )
		fputs("#endif\n",stdout);
	fputs("} Qualifiers_t;\n\n"
		  ,stdout);
	sp = structs;
	for (ii=0; ii < numStructs; ++ii, ++sp)
	{
		fprintf(stdout, "%s\n", sp->str);
	}
	fputs("\n#undef QUALTBL_GET_ENUM\n"
		  "#endif /* QUALTBL_GET_ENUM */\n\n"
		  ,stdout);

	fputs("#if QUALTBL_GET_OTHERS\n",stdout);
	fputs("\nQual_t qual_tbl[] = \n{\n",stdout);
	lp = lines;
	lastOp = 0;
	ll = NULL;
	for (ii=0; ii < numLines && lp->op != OP_LAST; ++ii, ll = lp, ++lp)
	{
		if ( ll )
			fprintf(stdout, ", %s\n", ll->comment);
		if ( lp->op != lastOp )
		{
			if ( lastOp == OP_NO_PP || lastOp == OP_ONLY_PP )
				fputs("#endif\n",stdout);
			if ( lp->op == OP_NO_PP )
				fputs("#if !defined(MAC_PP)\n",stdout);
			else if ( lp->op == OP_ONLY_PP )
				fputs("#if defined(MAC_PP)\n",stdout);
		}
#if 0
		#define QTBL(\
			noval,			/* t/f if no value is allowed */\
			optional,		/* t/f if value is optional */\
			number,			/* t/f is value must be a number */\
			output,			/* t/f if param is an output file */\
				string,			/* t/f if param is string (incl ws) */\
			negate,			/* t/f if param is negatible */\
			qual,			/* parameter mask */\
			name,			/* name of parameter */\
			index			/* output file index */\
		) {\
		noval,\
		optional,\
		number,\
		output,\
		string,\
		negate, \
		0, \
		0, \
		0, \
		qual,\
		name,\
		index,\
		0\
		}
#endif
		fprintf(stdout, "    { %d,%d,%d,%d,%d,%d,0,0,0,%s,%s,%s,NULL,0 }",
				lp->val,
				lp->opt,
				lp->num,
				lp->out,
				lp->str,
				lp->neg,
				lp->enumName,
				lp->name,
				lp->outIdx);
		lastOp = lp->op;
	}
	if ( ll )
		fprintf(stdout, " %s\n", ll->comment);
	if ( lastOp == OP_NO_PP || lastOp == OP_ONLY_PP )
		fputs("#endif\n",stdout);
	fputs("};\n"
		  "#undef QUALTBL_GET_OTHERS\n"
		  "#endif /* QUALTBL_GET_OTHERS */\n"
		  ,stdout);
	return 0;	
}

