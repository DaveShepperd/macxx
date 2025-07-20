/*
    exprs.h - part of a generic expression parser example
    Copyright (C) 2022 David Shepperd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _LIB_EXPRS_H_
#define _LIB_EXPRS_H_ (1)

#include <inttypes.h>

/**
 * @brief Example of an generic expression evaluator.
 *
 * This subsystem includes the optional use of symbols
 * maintained by an external symbol table manager.
 *
 * Expression operators use the syntax of the 'C' language. With
 * that in mind, an expression term can be either a unary or a
 * binary. A unary means the operator applies only the immediate
 * term on the right (i.e. -4 or !foo, etc.) where a binary
 * operator applies to one term on the left and one on the right
 * (i.e. a+b, 5*4, etc.). The unary operators are: +,-,! and ~.
 * The binary operators in decreasing precedence are **, *, /,
 * %, +, -, ^, |, &, <<, >>, <, <=, >, >=, ==, !=, &&, || and =.
 *
 * NOTE: the operator '**' available in this expression parser
 * is not available in the 'C' language but it means the operand
 * on the left is raised to the power of the operand on the
 * right. I.e. x**y = the term x is raised to the power of y.
 * And x**0.5 = the square root of x.
 *
 * A subexpression is built and stored in a stack. After all the
 * subexpressions (terms delimited with parenthesis), they are
 * evaluated left to right with operator precedence as defined
 * by the 'C' language.
 *
 * @note The 'C' operator '++' would mean add a unary positive
 * (i.e. 5++6 means add a +6 to 5) and '--' would mean subtract
 * a unary negative (i.e. 5--6 means subtract a negative 6 from
 * 5.
 */

#ifndef n_elts
#define n_elts(x) (int)(sizeof(x)/sizeof(x[0]))
#endif

#include "utils.h"

#define OPERSTUFF_GET_ENUM 1
#include "operstuff.h"

typedef enum
{
	ExprsPoolNull,
	ExprsPoolTerms,
	ExprsPoolString
} ExprsPoolID_t;

typedef struct
{
	void *mPoolTop;
	size_t mEntrySize;
	int mNumUsed;
	int mNumAvailable;
	int mMaxUsed;
	ExprsPoolID_t mPoolID;
} ExprsPool_t;

#define EXPRS_TERM_FLAG_LOCAL_SYMBOL	(0x001)	/* term is a local symbol */
#define EXPRS_TERM_FLAG_REGISTER		(0x002)	/* term is a register */
#define EXPRS_TERM_FLAG_COMPLEX			(0x004)	/* symbol value is complex */
#define EXPRS_TERM_FLAG_BYTE			(0x008)	/* term qualified as byte (68k) */
#define EXPRS_TERM_FLAG_WORD			(0x010)	/* term qualified as word (68k) */
#define EXPRS_TERM_FLAG_LONG			(0x020)	/* term qualified as long (68k) */

/** ExprsTerm_t - definition of the primitive contents of any
 *  individual term.
 **/
typedef struct
{
	ExprsTermTypes_t termType;	/*! the type of the term */
	const char *chrPtr;			/*! pointer to place in expression string where this term was found */
	int flags;					/*! 0 or more of the EXPRS_TERM_FLAG_xxx options */
	/* The actual term is one of the following: */
	union
	{
		int link;		/* Index to a different stack */
		int string;		/* Index into the string pool if type is string */
		int32_t s32;
		uint32_t u32;
#if 0
		double f64;
		int64_t s64;
		uint64_t u64;
#endif
		char oper[4];
		void *complex;
	} term;
} ExprsTerm_t;

/** ExprsStack_t - definition of a expression stack
 **/
typedef struct
{
	ExprsPool_t mTermsPool;			/* pool of terms for stack */
} ExprsStack_t;

/** ExprsMsgSeverity_t - define the severity of messages that
 *  may be emitted by the parser. */
typedef enum
{
	EXPRS_SEVERITY_INFO,
	EXPRS_SEVERITY_WARN,
	EXPRS_SEVERITY_ERROR,
	EXPRS_SEVERITY_FATAL
} ExprsMsgSeverity_t;

/** ExprsCallbacks_t - define the pointers to functions to
 *  callback for various parser primitives.
 *
 *  @note The memory callbacks are used only to get memory for
 *  	  the parser's internal use. Both memAlloc and memFree
 *  	  must contain a pointer or both must be NULL. If set to
 *  	  NULL, default functions using malloc/free will be
 *  	  used.
 *
 *  @note The msgOut callback is used by the parser to emit
 *  	  informational and error messages. If msgOut is set to
 *  	  NULL, a default function using stdout and stderr will
 *  	  be used.
 *
 *  @note The symGet/symSet members must contain a pointer or
 *  	  both must be set to NULL. If NULL, symbols appearing
 *  	  in an expression results in a parse error.
 *
 **/
typedef struct
{
	void *(*memAlloc)(void *memArg, size_t size);	/*! Memory allocation callback */
	void (*memFree)(void *memArg, void *memPtr);	/*! Memory free callback */
	void *memArg;									/*! Argument to pass to above memory callbacks */
	void (*msgOut)(void *msgArg, ExprsMsgSeverity_t severity, const char *msg);	/*! Message (error and info) output callback */
	void *msgArg;									/*! Argument to pass to msgOut callback */
} ExprsCallbacks_t;

typedef uint8_t ExprsPrecedence_t;

/** Exprs flags:
 */
#define EXPRS_FLG_USE_RADIX			0x00000001	/*! Use radix to figure out numbers */
#define EXPRS_FLG_NO_FLOAT			0x00000002	/*! No floating point allowed */
#define EXPRS_FLG_NO_STRING			0x00000004	/*! No quoted strings allowed */
#define EXPRS_FLG_NO_PRECEDENCE		0x00000008	/*! No operator precedence */
#define EXPRS_FLG_H_HEX				0x00000010	/*! Hex can be expressed with trailing 'h' or 'H' */
#define EXPRS_FLG_PRE_DOLLAR_HEX	0x00000020	/*! Hex can be expressed with leading '$' */
#define EXPRS_FLG_POST_DOLLAR_HEX	0x00000040	/*! Hex can be expressed with trailing '$' */
#define EXPRS_FLG_O_OCTAL			0x00000080	/*! Octal can be expressed with trailing 'o' or 'O' */
#define EXPRS_FLG_Q_OCTAL			0x00000100	/*! Octal can be expressed with trailing 'q' or 'Q' */
#define EXPRS_FLG_DOT_DECIMAL		0x00000200	/*! Decimal can be expressed with trailing '.' */
#define EXPRS_FLG_NO_POWER			0x00000400	/*! No exponent allowed */
#define EXPRS_FLG_SINGLE_QUOTE		0x00000800	/*! Allow single quoted chars (i.e. 'a vs. 'a' */
#define EXPRS_FLG_NO_LOGICALS		0x00001000	/*! No logical operators allowed (i.e. mac65 et al) */
#define EXPRS_FLG_SPECIAL_UNARY		0x00002000	/*! Enable special unary operators (i.e. mac65 et al) */
#define EXPRS_FLG_NO_ASSIGNMENT		0x00004000	/*! No symbol assignments */
#define EXPRS_FLG_WS_DELIMIT		0x00008000	/*! White space delimits non-operator terms */
#define EXPRS_FLG_SANITY			0x00010000	/*! Don't allow more than one bump in pool increments */
#define EXPRS_FLG_LOCAL_SYMBOLS		0x00020000	/*! Local symbols are expressed via decimalNumber$ (cannot be combined with POST_DOLLAR_HEX) */
#define EXPRS_FLG_DOT_SYMBOL		0x00040000	/*! Symbols can begin with leading period (.) (forces flag 0x2 = NO_FLOAT) */
#define EXPRS_FLG_PCNT_IS_REGISTER	0x00080000	/*! A unary '%' means term is a register */
#define EXPRS_FLG_OPEN_IS_END		0x00100000	/*! A lone open expression w/o leading operator is just the end */
#define EXPRS_FLG_CLOSE_IS_END		0x00200000	/*! A lone close expression is just the end */
#define EXPRS_FLG_NO_DOUBLE_PLAIN	0x00400000	/*! A second plain term terminates expression parse */
#define EXPRS_FLG_PCNT_REGISTER		0x00800000	/*! A percent sign signals register type */
#define EXPRS_FLG_LEN_QUALIFIERS	0x01000000	/*! Term can have length qualifiers (68k mode) */

/** ExprsDef_t - definition of expression stack internal
 *  variables. With the exception of userArg1 and userArg2
 *  these struct members should be thought of as read-only.
 **/
typedef struct
{
	void *userArg1;					/*! can be used for any purpose */
	void *userArg2;					/*! can be used for any purpose */
	ExprsCallbacks_t mCallbacks;	/*! callbacks to be used by the parser */
	unsigned int mVerbose;			/*! verbose flags */
	int mTermsPoolInc;				/*! term pool increment */
	int mStringPoolInc;				/*! string pool increment */
	ExprsStack_t mStack;			/*! expression stack */
	ExprsPool_t mStringPool;		/*! string pool */
	const char *mCurrPtr;			/*! Pointer to current place in expression string being processed */
	const char *mLineHead;			/*! Pointer to first character in expression string */
	uint32_t mFlags;				/*! Bit mask of EXPRS_FLG_xxx bits */
	int mRadix;						/*! Radix to use if EXPRS_FLG_USE_RADIX flag set */
	char mOpenDelimiter;			/*! Open expression delimiter */
	char mCloseDelimiter;			/*! Close expression delimiter */
	const ExprsPrecedence_t *precedencePtr; /*! Pointer to our precedence table */
	const unsigned short *chMaskPtr;/*! Pointer to cttbl */
} ExprsDef_t;

#ifndef EXPRS_MAX_NEST
#define EXPRS_MAX_NEST (128)		/*! A sanity check */
#endif

/** libExprsInit - Initialize an expression parser.
 *
 *  At entry:
 * @param callbacks - pointer to list of various callbacks the
 *  				parser is to use.
 * @param termIncs - sets the number of terms added when needed.
 *  			   If 0, a default is set to 32.
 * @param stringIncs - sets the number of bytes for strings
 *  					 added when needed. If 0, defaults to
 *  					 2048.
 *
 * At exit:
 * @return pointer to expression parser control struct or NULL
 *  	   if error (out of memory).
 *
 * @note The contents of the callbacks struct is copied into the
 *  	 ExprsDef_t struct. The struct pointed to by 'callbacks'
 *  	 does not need to remain in static memory.
 **/
extern ExprsDef_t *libExprsInit(const ExprsCallbacks_t *callbacks,  int termIncs, int stringIncs);

/** libExprsDestroy - free all the previously allocated
 *  memory used by the expression parser.
 *
 *  At entry:
 *  @param exprs - pointer as returned from previous call to
 *  			 libExprsInit.
 *
 *  At exit:
 *  @return 0 on success, non-zero on error. All the memory
 *  		allocated for use by the parser has been freed. See
 *  		errno for additional error information.
 **/
extern ExprsErrs_t libExprsDestroy(ExprsDef_t *exprs);

/** libExprsSetCallbacks - Alter the list of callbacks in use
 *  by the expression parser.
 *
 *  At entry:
 *  @param exprs - pointer to parser control as returned from
 *  			 libExprsInit().
 *  @param newPtr - pointer to new list of callbacks.
 *  @param oldPtrs - if not NULL, pointer to place into which
 *  			   the current pointers will be copied.
 *
 *  At exit:
 *  @return 0 on success, non-zero on error.
 *
 * @note The contents of newPtrs is copied into the struct
 *  	 pointed to by exprs. The struct pointed to by newPtrs
 *  	 does not need to be in static memory.
 **/
extern ExprsErrs_t libExprsSetCallbacks(ExprsDef_t *exprs, const ExprsCallbacks_t *newPtrs, ExprsCallbacks_t *oldPtrs);

/** libExprsSetVerbose - set the verbose flags.
 *
 *  At entry:
 *  @param exprs - pointer to parser control as returned from
 *  			 libExprsInit().
 *  @param newVal - verbose flags.
 *  @param oldValP - if not null, pointer to place to store old
 *  			   value.
 *
 *  At exit:
 * @return 0 on success else error code. Look in errno for
 *  	   further indication of error.
 **/
extern ExprsErrs_t libExprsSetVerbose(ExprsDef_t *exprs, unsigned int newVal, unsigned int *oldValP);

/** libExprsSetFlags - set the flags.
 *
 *  At entry:
 *  @param exprs - pointer to parser control as returned from
 *  			 libExprsInit().
 *  @param newVal - flags.
 *  @param oldValP - if not null, pointer to place to store old
 *  			   value.
 *
 *  At exit:
 * @return 0 on success else error code. Look in errno for
 *  	   further indication of error.
 **/
extern ExprsErrs_t libExprsSetFlags(ExprsDef_t *exprs, uint32_t newVal, uint32_t *oldValP);

/** libExprsSetRadix - set the default radix
 *
 *  At entry:
 *  @param exprs - pointer to parser control as returned from
 *  			 libExprsInit().
 *  @param newVal - default radix.
 *  @param oldValP - if not null, pointer to place to store old
 *  			   value.
 *
 *  At exit:
 * @return 0 on success else error code. Look in errno for
 *  	   further indication of error.
 **/

extern ExprsErrs_t libExprsSetRadix(ExprsDef_t *exprs, int newVal, int *oldValP);

/** libExprsSetOpenDelimiter - set the open delimiter
 *
 *  At entry:
 *  @param exprs - pointer to parser control as returned from
 *  			 libExprsInit().
 *  @param newVal - default open delimiter.
 *  @param oldValP - if not null, pointer to place to store old
 *  			   value.
 *
 *  At exit:
 * @return 0 on success else error code. Look in errno for
 *  	   further indication of error.
 **/
extern ExprsErrs_t libExprsSetOpenDelimiter(ExprsDef_t *exprs, char newVal, char *oldValP);

/** libExprsSetcloseDelimiter - set the close delimiter
 *
 *  At entry:
 *  @param exprs - pointer to parser control as returned from
 *  			 libExprsInit().
 *  @param newVal - open delimiter.
 *  @param oldValP - if not null, pointer to place to store old
 *  			   value.
 *
 *  At exit:
 * @return 0 on success else error code. Look in errno for
 *  	   further indication of error.
 **/
extern ExprsErrs_t libExprsSetCloseDelimiter(ExprsDef_t *exprs, char newVal, char *oldValP);

/** libExprsGetErrorStr - Translate a parser error into a
 *  text string.
 *
 *  At entry:
 *  @param errCode - code returned from an expression parser
 *  			   function.
 *
 *  At exit:
 *  @return null terminated text string.
 **/
extern const char *libExprsGetErrorStr(ExprsErrs_t errCode);

#if 0
/** libExprsEval - Evaluate an expression
 *
 *  At entry:
 *  @param exprs - pointer to expression parser control as
 *  			 returned from libExprsInit().
 *  @param text - null terminated text of expression to
 *  			evaluate.
 *  @param returnTerm - pointer to place into which to deposit
 *  				  the result.
 *  @param alreadyLocked - set to non-zero to indicate the mutex
 *  					 lock on the ExprsDef_t has already been
 *  					 performed by libExprsLock().
 *
 *  At exit:
 *  @return 0 on success, else error. On success, the place
 *  		pointed to by 'returnTerm' will have been set to the
 *  		result. The result may only be of type integer,
 *  		float or string. It will not return anything more
 *  		complex.
 *
 *  @note At entry to this function all internal variables are
 *  	  cleared. That is, nothing is remembered about any
 *  	  previous expression parse from one call to another.
 *
 *  @note If the return result is of type string, a pointer to
 *  	  that string will be into the internal string pool
 *  	  maintained by lib_exprs. If one wants to keep that
 *  	  string, one must make a copy of it before making
 *  	  another call to libExprsEval() or
 *  	  libExprsDestroy() because the pointer to that string
 *  	  will not survive a subsequent call to either.
 **/
extern ExprsErrs_t libExprsEval(ExprsDef_t *exprs, const char *text, ExprsTerm_t *returnTerm, int alreadyLocked);
#endif

/** libExprsParseToRPN - Parse an expression to RPN
 *
 *  At entry:
 *  @param exprs - pointer to expression parser control as
 *  			 returned from libExprsInit().
 *  @param text - null terminated text of expression to
 *  			parse.
 *  @param alreadyLocked - set to non-zero to indicate the mutex
 *  					 lock on the ExprsDef_t has already been
 *  					 performed by libExprsLock().
 *
 *  At exit:
 *  @return 0 on success, else error. The parsed expression can
 *  		be found in the members mStacks and mNumStacks. The
 *  		member mCurrPtr will point to the place in the text
 *  		where the parser stopped whether or not return code
 *  		is error.
 *
 *  @note At entry to this function all internal variables are
 *  	  cleared. That is, nothing is remembered about any
 *  	  previous expression parse from one call to another.
 *
 **/
extern ExprsErrs_t libExprsParseToRPN(ExprsDef_t *exprs, const char *text, int alreadyLocked);

extern ExprsErrs_t libExprsWalkParsedStack(ExprsDef_t *exprs, ExprsErrs_t (*walkCallback)(ExprsDef_t *exprs, const ExprsTerm_t *term), int alreadyLocked);

/** libExprsXXXPoolTop - get the pointers to the tops of the
 *  various pools.
 *
 *  At entry:
 *  @param exprs - pointer to expression parser control as
 *  			 returned from libExprsInit().
 *
 *  At exit:
 *  @return pointer to top of selected pool.
 **/
extern ExprsTerm_t *libExprsTermPoolTop(ExprsDef_t *exprs, ExprsStack_t *stack);
extern char *libExprsStringPoolTop(ExprsDef_t *exprs);

#if 0
/** libExprsLock - lock the hash table.
 *
 * At entry:
 * @param pTable - pointer to hash table
 *
 * At exit:
 * @return 0 on success else error code. Look in errno for
 *  	   further indication of error.
 **/
extern ExprsErrs_t libExprsLock(ExprsDef_t *pTable);

/** libExprsUnlock - unlock the hash table.
 *
 * At entry:
 * @param pTable - pointer to hash table
 *
 * At exit:
 * @return 0 on success else error code. Look in errno for
 *  	   further indication of error.
 **/
extern ExprsErrs_t  libExprsUnlock(ExprsDef_t *pTable);
#endif
#endif	/* _LIB_EXPRS_H_ */
