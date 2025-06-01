/*
    le_itfc.c - Part of macxx, a cross assembler family for various micro-processors
    Copyright (C) 2025 David Shepperd

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
#include "exproper.h"
#include "listctrl.h"
#include "memmgt.h"
#include <stdlib.h>
#if defined(MAC_8080)
#include "le_itfc.h"
#endif
