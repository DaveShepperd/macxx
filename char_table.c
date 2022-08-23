/*
    char_table.c - Part of macxx, a cross assembler family for various micro-processors
    Copyright (C) 2008 David Shepperd

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

#define NO_EXTTBL 1
#include "ct.h"			/* get character types */

#define EOL 	CT_EOL		/*  EOL */
#define COM 	CT_COM		/*  COMMA */
#define WS 	CT_WS		/*  White space */
#define PCX 	CT_PCX		/*  PRINTING CHARACTER */
#define NUM 	CT_NUM		/*  NUMERIC */
#define ALP 	CT_ALP		/*  ALPHA, DOT, DOLLAR */
#define LC 	CT_ALP		/*  LOWER CASE ALPHA */
#define SMC 	CT_SMC		/*  SEMI-COLON */
#define EXP	CT_OPER		/*  expression operator */
#define HEX	CT_HEX		/*  hex digit */
#define LHEX	CT_LHEX		/*  lowercase hex digit */
#define BOP	CT_BOP		/*  binary operator */
#define UOP	CT_UOP		/*  unary operator */

unsigned short cttbl[] = {
    EOL, EOL, EOL, EOL, EOL, EOL, EOL, EOL,  /* NUL,SOH,STX,ETX,EOT,ENQ,ACK,BEL */
    EOL,  WS, EOL, EOL, EOL, EOL, EOL, EOL,  /* BS,HT,LF,VT,FF,CR,SO,SI */
    EOL, EOL, EOL, EOL, EOL, EOL, EOL, EOL,  /* DLE,DC1,DC2,DC3,DC4,NAK,SYN,ETB */
    EOL, EOL, EOL, EOL, EOL, EOL, EOL, EOL,  /* CAN,EM,SUB,ESC,FS,GS,RS,US */

    WS , EXP, UOP, PCX, ALP, EXP, BOP, UOP,  /*   ! " # $ % & ' */
    UOP, PCX, BOP, EXP, COM, EXP, ALP, BOP,  /* ( ) * + , - . / */
    NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM,  /* 0 1 2 3 4 5 6 7 */
    NUM, NUM, PCX, SMC, BOP, BOP, BOP, PCX,  /* 8 9 : ; < = > ? */

    PCX, HEX, HEX, HEX, HEX, HEX, HEX, ALP,  /* @ A B C D E F G */
    ALP, ALP, ALP, ALP, ALP, ALP, ALP, ALP,  /* H I J K L M N O */
    ALP, ALP, ALP, ALP, ALP, ALP, ALP, ALP,  /* P Q R S T U V W */
    ALP, ALP, ALP, PCX, PCX, PCX, EXP, ALP,  /* X Y Z [ \ ] ^ _ */

    PCX,LHEX,LHEX,LHEX,LHEX,LHEX,LHEX, LC ,  /* ` a b c d e f g */
    LC , LC , LC , LC , LC , LC , LC , LC ,  /* h i j k l m n o */
    LC , LC , LC , LC , LC , LC , LC , LC ,  /* p q r s t u v w */
    LC , LC , LC , PCX, BOP, PCX, UOP, EOL   /* x y z { | } ~ DEL */
};

#undef EOL
#undef WS
#undef COM
#undef PCX
#undef NUM
#undef ALP
#undef LC
#undef EXP
#undef SMC
#undef HEX
#undef LHEX

#define EOL 	CC_EOL		/*  EOL */
#define WS 	CC_WS		/*  White space */
#define COM	CC_PCX		/*  comma = pcx */
#define PCX 	CC_PCX		/*  PRINTING CHARACTER */
#define NUM 	CC_NUM		/*  NUMERIC */
#define ALP 	CC_ALP		/*  ALPHA, DOT, DOLLAR */
#define LC 	CC_ALP		/*  LOWER CASE ALPHA */
#define EXP	CC_OPER		/*  expression operator */
#define SMC 	CC_PCX		/*  SEMI-COLON */
#define HEX	CC_ALP		/*  hex digit */
#define LHEX	CC_ALP		/*  lowercase hex digit */

unsigned char cctbl[] = {
    EOL, EOL, EOL, EOL, EOL, EOL, EOL, EOL,  /* NUL,SOH,STX,ETX,EOT,ENQ,ACK,BEL */
    EOL,  WS, EOL, EOL, EOL, EOL, EOL, EOL,  /* BS,HT,LF,VT,FF,CR,SO,SI */
    EOL, EOL, EOL, EOL, EOL, EOL, EOL, EOL,  /* DLE,DC1,DC2,DC3,DC4,NAK,SYN,ETB */
    EOL, EOL, EOL, EOL, EOL, EOL, EOL, EOL,  /* CAN,EM,SUB,ESC,FS,GS,RS,US */

    WS , EXP, EXP, PCX, ALP, EXP, EXP, EXP,  /*   ! " # $ % & ' */
    EXP, PCX, EXP, EXP, COM, EXP, ALP, EXP,  /* ( ) * + , - . / */
    NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM,  /* 0 1 2 3 4 5 6 7 */
    NUM, NUM, PCX, SMC, EXP, EXP, EXP, EXP,  /* 8 9 : ; < = > ? */

    PCX, HEX, HEX, HEX, HEX, HEX, HEX, ALP,  /* @ A B C D E F G */
    ALP, ALP, ALP, ALP, ALP, ALP, ALP, ALP,  /* H I J K L M N O */
    ALP, ALP, ALP, ALP, ALP, ALP, ALP, ALP,  /* P Q R S T U V W */
    ALP, ALP, ALP, PCX, PCX, PCX, EXP, ALP,  /* X Y Z [ \ ] ^ _ */

    PCX,LHEX,LHEX,LHEX,LHEX,LHEX,LHEX, LC ,  /* ` a b c d e f g */
    LC , LC , LC , LC , LC , LC , LC , LC ,  /* h i j k l m n o */
    LC , LC , LC , LC , LC , LC , LC , LC ,  /* p q r s t u v w */
    LC , LC , LC , PCX, EXP, PCX, EXP, EOL   /* x y z { | } ~ DEL */
};

unsigned char char_toupper[] = {
    000, 001, 002, 003, 004, 005, 006, 007,  /* NUL,SOH,STX,ETX,EOT,ENQ,ACK,BEL */
    010, 011, 012, 013, 014, 015, 016, 017,  /* BS,HT,LF,VT,FF,CR,SO,SI */
    020, 021, 022, 023, 024, 025, 026, 027,  /* DLE,DC1,DC2,DC3,DC4,NAK,SYN,ETB */
    030, 031, 031, 033, 034, 035, 036, 037,  /* CAN,EM,SUB,ESC,FS,GS,RS,US */

    040, 041, 042, 043, 044, 045, 046, 047,  /*   ! " # $ % & ' */
    050, 051, 052, 053, 054, 055, 056, 057,  /* ( ) * + , - . / */
    060, 061, 062, 063, 064, 065, 066, 067,  /* 0 1 2 3 4 5 6 7 */
    070, 071, 072, 073, 074, 075, 076, 077,  /* 8 9 : ; < = > ? */

    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',  /* @ A B C D E F G */
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',  /* H I J K L M N O */
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',  /* P Q R S T U V W */
    'X', 'Y', 'Z', '[', '\\', ']', '^', '_',  /* X Y Z [ \ ] ^ _ */

    '`', 'A', 'B', 'C', 'D', 'E', 'F', 'G',  /* ` a b c d e f g */
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',  /* h i j k l m n o */
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',  /* p q r s t u v w */
    'X', 'Y', 'Z', '{', '|', '}', '~', 0x7F, /* x y z { | } ~ DEL */

    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,

    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
    0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,

    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
    0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,

    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
    0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};
