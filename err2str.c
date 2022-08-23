/*
    err2str.c - Part of macxx, a cross assembler family for various micro-processors
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
#include <stdio.h>
#include <errno.h>
#include <string.h>

char *error2str( int num)        /* modeled after VMS's strerror() */
{
#ifndef HAS_STRERROR
    static char undef_msg[64];
#ifndef __TURBOC__
    switch (num)
    {
    case EPERM: return "not owner";
    case ENOENT: return "no such file or directory";
    case EIO: return "i/o error";
    case ENXIO: return "no such device or address";
    case EBADF: return "bad file number";
    case EACCES: return "permission denied";
    case ENOTBLK: return "block device required";
    case EBUSY: return "mount device busy";
    case EEXIST: return "file exists";
    case EISDIR: return "is a directory";
    case EXDEV: return "cross-device link";
    case ENODEV: return "no such device";
    case ENOTDIR: return "not a directory";
    case ENFILE: return "file table overflow";
    case EMFILE: return "too many open files";
    case EFBIG: return "file too large";
    case ENOSPC: return "no space left on device";
    case ESPIPE: return "illegal seek";
    case EROFS: return "read-only file system";
    case EMLINK: return "too many links";
#ifdef VMS
    case EWOULDBLOCK: "I/O operation would block channel";
#endif
    case ESRCH: return "no such process";
    case EINTR: return "interrupted system call";
    case E2BIG: return "arg list too long";
    case ENOEXEC: return "exec format error";
    case ECHILD: return "no children";
    case EAGAIN: return "no more processes";
    case ENOMEM: return "not enough core";
    case EFAULT: return "bad address";
    case EINVAL: return "invalid argument";
    case ENOTTY: return "not a typewriter";
    case ETXTBSY: return "text file busy";
    case EPIPE: return "broken pipe";
    case EDOM: return "math argument";
    case ERANGE: return "result too large";
#ifdef VMS
    case EVMSERR: {
            sprintf(undef_msg,"Untranslatable VMS error code of: 0x%08X",vaxc$errno);
            return undef_msg;
        }
#endif
    default: {
#endif
            sprintf(undef_msg,"Undefined error code: 0x%0X",num);
            return undef_msg;
#ifndef __TURBOC__
        }
    }
#endif
#else
    return strerror(num);
#endif
}
