/*
    qksort.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "token.h"

#define copy(a,b,siz) (*a = *b)
#define QKCMP(a,b) (strcmp((*a)->ss_string,(*b)->ss_string))
#define qswap(a,b) { SS_struct *tmp = *a; *a = *b; *b = tmp; }
#define ELEMENT SS_struct *

#define LOW_BOUND 13	/* Smaller segments will be done differently */
#define BYTE char
#define LOC_SIZ 10	/* size of a local pivot area */

void qksort (ELEMENT array[],        /* the array to sort */
            unsigned int num_elements)  /* the number of elements in the array */
{
    register ELEMENT *i;
    register ELEMENT *j;
    ELEMENT *i1;
    ELEMENT *lp;
    ELEMENT *rp;
    ELEMENT *p;          /* pointer to middle elt of a segment */
    ELEMENT *lv[18];     /* stack for non-recurs */
    ELEMENT *uv[18];
    int sp;          /* stack pointer */
    ELEMENT **lvsp;      /* pointers for lv[sp] & lv[sp+1] */
    ELEMENT **lvsp1;
    ELEMENT **uvsp;      /* pointers for uv[sp] & uv[sp+1] */
    ELEMENT **uvsp1;
    unsigned int ptr_diff;   /* the difference of 2 pointers */
    unsigned int ptr_limit;  /* difference limit for seqment size */
    register ELEMENT *jx;    /* inner index during insertionsort */
    ELEMENT *pivot;      /* addr of a scratch area */
    ELEMENT loc_pivot[LOC_SIZ];  /* local pivot area for small elements */

    if (num_elements < 2) return;  /* The array is already sorted! */

#define ELT_SIZE 1	/* force element size to 1 for LLF */
    /* because pointer arithmetic will be */
    /* done automatically by the compiler */
    pivot = loc_pivot;
    sp=0;
    lvsp = &lv[0]; lvsp1 = lvsp + 1;
    uvsp = &uv[0]; uvsp1 = uvsp + 1;
    *lvsp = array;
    *uvsp = array + ELT_SIZE * (num_elements - 1);
    ptr_limit = LOW_BOUND * ELT_SIZE;
    while (sp >= 0)
    {
        lp = i = *lvsp;    rp =   j = *uvsp;
        ptr_diff = j-i;
        if (ptr_diff < ptr_limit)
        { /*small segment will be handled later */
            sp--; lvsp--; lvsp1--; uvsp--; uvsp1--; /* pop it off */
        }
        else
        {
            /* select a pivot element & move it around. */
            p = (ptr_diff / 2) + i;    /* middle element */
            /* mung 1st, last, middle elements around */
            qswap(p, i);
            i1 = i+ELT_SIZE;
            if (QKCMP(i1, j) > 0) qswap(i1, j);
            if (QKCMP(i, j) > 0) qswap(i, j);
            if (QKCMP(i1, i) > 0) qswap(i1, i);
            i = i1;
/* Get things on the proper side of the pivot.
 * The above munging around has cleverly set up the boundary conditions
 * so that we don't need to check the pointers against the partition
 * limits. So the inner loop is very fast.
*/
            while (1)
            {
                do (i += ELT_SIZE);  while (QKCMP(i, lp) < 0);
                do (j -= ELT_SIZE);  while (QKCMP(lp, j) < 0);
                if (j < i) break;
                qswap(i, j);
            }
            qswap(lp, j);
/* done with this partition */
            if ( (j-lp) < (rp-j))
            { /* stack so shorter is done first */
                *lvsp1 = *lvsp;
                *uvsp1 = j-ELT_SIZE;
                *lvsp = j+ELT_SIZE;
            }
            else
            {
                *lvsp1 = j+ELT_SIZE;
                *uvsp1 = *uvsp;
                *uvsp = j-ELT_SIZE;
            }
            sp++;  lvsp++;  lvsp1++;   uvsp++;   uvsp1++; /* push stack */
        }
    }
/* Now do an insertionsort to get the small segments (which were
 * bypassed earlier) sorted.
 */
    p = array + ELT_SIZE * (num_elements - 1);   /* last element of array */
    for (i=array, j=i+ELT_SIZE; i<p; i=j, j+=ELT_SIZE)
    {
        if (QKCMP(i,j) > 0)
        { /*we have hit an out-of-sort area */
            copy(pivot, j, ELT_SIZE);
            for (jx=j; (i >= array) && (QKCMP(i,pivot) > 0)
                ; jx=i, i-=ELT_SIZE)
            {
                copy(jx, i, ELT_SIZE);
            }
            copy(jx, pivot, ELT_SIZE);
        }
    }
}
