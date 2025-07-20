#ifndef _FORMATS_H_
#define _FORMATS_H_ (1)

#include <inttypes.h>

#if __SIZEOF_PTRDIFF_T__ > __SIZEOF_INT__
	#if __SIZEOF_PTRDIFF_T__ > __SIZEOF_LONG__
		#define FMT_PTRDIF_PRFX "ll"
	#else
		#define FMT_PTRDIF_PRFX "l"
	#endif
#else
	#define FMT_PTRDIF_PRFX ""
#endif
#if __SIZEOF_SIZE_T__ > __SIZEOF_INT__
	#if __SIZEOF_SIZE_T__ > __SIZEOF_LONG__
		#define FMT_PRFX "ll"
	#else
		#define FMT_PRFX "l"
	#endif
#else
	#define FMT_PRFX ""
#endif

#define FMT_SZ "%" FMT_PRFX "d"

#endif	/* _FORMATS_H_*/
