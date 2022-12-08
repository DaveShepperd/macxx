
/*  TG -  edited Dave Shepperd code 12/07/2022 */


#ifndef _PSTTKN68_H_
#define _PSTTKN68_H_ 1



typedef enum {
   ILL_NUM= -1,
   UNDEF_NUM=0,
   I_NUM=1,
   D_NUM,
   Z_NUM=D_NUM,
   X_NUM,
   E_NUM,
   A_NUM=E_NUM,
   S_NUM,
   SPC_NUM,
   ACC_NUM,
   IMP_NUM=ACC_NUM,
   MAX_NUM
} AModes;


#define   I	(1L << I_NUM)
#define   D	(1L << D_NUM)
#define   Z	(1L << Z_NUM)
#define   X	(1L << X_NUM)
#define   E	(1L << E_NUM)
#define   A	(1L << A_NUM)
#define   S	(1L << S_NUM)
#define   SPC	(1L << SPC_NUM)
#define   ACC	(1L << ACC_NUM)
#define   IMP	(1L << IMP_NUM)

#define   DES	(0x8000)

#define   MOST68	(I|D|X|E)



#endif /* _PSTTKN68_H_ */
