#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define n_elts(x) (int)(sizeof(x)/sizeof(x[0]))

#define OPERSTUFF_GET_ENUM 1
#include "operstuff.h"

#define EXPROPER_ADD	'+'	/* add */
#define EXPROPER_SUB	'-'	/* subtract */
#define EXPROPER_MUL	'*'	/* multiply */
#define EXPROPER_DIV	'/'	/* divide */
#define EXPROPER_AND	'&'	/* and */
#define EXPROPER_OR		'|'	/* inclusive or */
#define EXPROPER_XOR	'^'	/* exclusive or */
#define EXPROPER_NEG	'_'	/* negate (2's compliment) */
#define EXPROPER_COM	'~'	/* compliment (1's compliment) */
#define EXPROPER_USD	'?'	/* unsigned divide */
#define EXPROPER_SWAP	'='	/* swap bytes */
#define EXPROPER_MOD	'\\'	/* modulo */
#define EXPROPER_SHL	'<'	/* shift left */
#define EXPROPER_SHR	'>'	/* shift right */
#define EXPROPER_TST	'!'	/* test for condition (following char is case) */
#define EXPROPER_TST_NOT '!'	/* ...not */
#define EXPROPER_TST_AND '&'	/* ...logical and */
#define EXPROPER_TST_OR '|'	/* ...logical or */
#define EXPROPER_TST_LT	'<'	/* ...less than */
#define EXPROPER_TST_GT	'>'	/* ...greater than */
#define EXPROPER_TST_EQ	'='	/* ...equal */
#define EXPROPER_TST_NE '#'	/* ...not equal */
#define EXPROPER_TST_LE '['	/* ...less than or equal */
#define EXPROPER_TST_GE ']'	/* ...greater than or equal */
#define EXPROPER_TSTNM	'@'	/* test for condition without error message */
#define EXPROPER_PICK	'$'	/* dup n'th item on stack */
#define EXPROPER_PURGE	'#'	/* purge top of stack */
#define EXPROPER_XCHG	'`'	/* exchange top 2 items */
#define EXPROPER_IF		'('	/* if condit true, do until endif */
#define EXPROPER_ELSE	'"'	/* if condit false, do until endif */

static const char Opers[] = 
{
    EXPROPER_ADD,		// '+' */	/* add */
    EXPROPER_SUB,		// '-' */ /* subtract */
    EXPROPER_MUL,		//	'*'	/* multiply */
    EXPROPER_DIV,		//	'/'	/* divide */
    EXPROPER_AND,		//	'&'	/* and */
    EXPROPER_OR,		//	'|'	/* inclusive or */
    EXPROPER_XOR,		//	'^'	/* exclusive or */
    EXPROPER_NEG,		//	'_'	/* negate (2's compliment) */
    EXPROPER_COM,		//	'~'	/* compliment (1's compliment) */
    EXPROPER_USD,		//	'?'	/* unsigned divide */
    EXPROPER_SWAP,		//	'='	/* swap bytes */
    EXPROPER_MOD,		//	'\\'	/* modulo */
    EXPROPER_SHL,		//	'<'	/* shift left */
    EXPROPER_SHR,		//	'>'	/* shift right */
    EXPROPER_TST,		//	'!'	/* test for condition (following char is case) */
    EXPROPER_TST_NOT,	// '!'	/* ...not */
    EXPROPER_TST_AND,	// '&'	/* ...logical and */
    EXPROPER_TST_OR,	// '|'	/* ...logical or */
    EXPROPER_TST_LT,	//	'<'	/* ...less than */
    EXPROPER_TST_GT,	//	'>'	/* ...greater than */
    EXPROPER_TST_EQ,	//	'='	/* ...equal */
    EXPROPER_TST_NE,	// '#'	/* ...not equal */
    EXPROPER_TST_LE,	// '['	/* ...less than or equal */
    EXPROPER_TST_GE,	// ']'	/* ...greater than or equal */
    EXPROPER_TSTNM,		// '@'	/* test for condition without error message */
    EXPROPER_PICK,		// '$'	/* dup n'th item on stack */
    EXPROPER_PURGE,		// '#'	/* purge top of stack */
    EXPROPER_XCHG,		// '`'	/* exchange top 2 items */
    EXPROPER_IF,		// '('	/* if condit true, do until endif */
    EXPROPER_ELSE		// '"'	/* if condit false, do until endif */
};

#define NUL 0

static const OperType_t OperXlateCodeToType[] =
{
	NUL,			/* (space) */
	EXPRS_TERM_NOT,	/* ! (actually test; upper byte says what the test is) */
	NUL, 			/* " */
	NUL,			/* # */

	NUL,			/* $ */
	EXPRS_TERM_MOD, /* % */
	EXPRS_TERM_AND,	/* & */
	NUL,			/* ' */

	NUL,			/* ( */
	NUL,			/* ) */
	EXPRS_TERM_MUL, /* * */
	EXPRS_TERM_ADD,	/* + */

	NUL,			/* , */
	EXPRS_TERM_SUB,	/* - */
	NUL,			/* . */
	EXPRS_TERM_DIV,	/* / */

	NUL, NUL, NUL, NUL, NUL, NUL, NUL, NUL,  /* 0 1 2 3 4 5 6 7 */
	NUL, NUL, NUL, NUL, 					 /* 8 9 : ; */

	EXPRS_TERM_ASL,							 /* < */
	EXPRS_TERM_SWAP,						 /* = */
	EXPRS_TERM_ASR,							 /* > */
	EXPRS_TERM_DIV,							 /* ? (unsigned divide) */

	NUL, NUL, NUL, NUL, NUL, NUL, NUL, NUL,  /* @ A B C D E F G */
	NUL, NUL, NUL, NUL, NUL, NUL, NUL, NUL,  /* H I J K L M N O */
	NUL, NUL, NUL, NUL, NUL, NUL, NUL, NUL,  /* P Q R S T U V W */
	NUL, NUL, NUL,							 /* X Y Z */
	EXPRS_TERM_LE,							 /* [ */
	EXPRS_TERM_MOD,							 /* \ */
	EXPRS_TERM_GE,							 /* ] */
	EXPRS_TERM_XOR,							 /* ^ */
	EXPRS_TERM_MINUS,						 /* _ */

	NUL, NUL, NUL, NUL, NUL, NUL, NUL, NUL,  /* ` a b c d e f g */
	NUL, NUL, NUL, NUL, NUL, NUL, NUL, NUL,  /* h i j k l m n o */
	NUL, NUL, NUL, NUL, NUL, NUL, NUL, NUL,  /* p q r s t u v w */
	NUL, NUL, NUL, NUL,						 /* x y z { */
	EXPRS_TERM_OR,							 /* | */
	NUL, 									 /* } */
	EXPRS_TERM_COM,						 	 /* ~ */
	NUL										 /* DEL */
};

int main(int argc, char *argv[])
{
	int ii, idx;
	OperType_t oper;
	
	for (ii=0; ii < n_elts(Opers); ++ii)
	{
		oper = NUL;
		idx = Opers[ii];
		if ( idx < ' ' || idx > '~' )
		{
			printf("Opers[%2d]: EXPROPER of 0x%X ('%c') out of range\n", ii, idx, isprint(idx)?idx:'.');
			continue;
		}
		idx -= ' ';
		oper = OperXlateCodeToType[idx];
		printf("Opers[%2d]=%c -> %2d\n", ii, Opers[ii], oper);
	}
	return 0;
}

