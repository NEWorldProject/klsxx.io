/*
* Copyright (c) 2022 DWVoid and Infinideastudio Team
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "kls/io/Status.h"
#include "kls/hal/System.h"

#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED 0x000002E4
#endif

namespace kls::io::detail {
    Status map_error(DWORD sys) noexcept {
        if (sys <= 0) return (Status)sys;
        switch (sys) {
        case ERROR_NOACCESS:                    return IO_EACCES;
        case WSAEACCES:                         return IO_EACCES;
        case ERROR_ELEVATION_REQUIRED:          return IO_EACCES;
        case ERROR_CANT_ACCESS_FILE:            return IO_EACCES;
        case ERROR_ADDRESS_ALREADY_ASSOCIATED:  return IO_EADDRINUSE;
        case WSAEADDRINUSE:                     return IO_EADDRINUSE;
        case WSAEADDRNOTAVAIL:                  return IO_EADDRNOTAVAIL;
        case WSAEAFNOSUPPORT:                   return IO_EAFNOSUPPORT;
        case WSAEWOULDBLOCK:                    return IO_EAGAIN;
        case WSAEALREADY:                       return IO_EALREADY;
        case ERROR_INVALID_FLAGS:               return IO_EBADF;
        case ERROR_INVALID_HANDLE:              return IO_EBADF;
        case ERROR_LOCK_VIOLATION:              return IO_EBUSY;
        case ERROR_PIPE_BUSY:                   return IO_EBUSY;
        case ERROR_SHARING_VIOLATION:           return IO_EBUSY;
        case ERROR_OPERATION_ABORTED:           return IO_ECANCELED;
        case WSAEINTR:                          return IO_ECANCELED;
        case ERROR_NO_UNICODE_TRANSLATION:      return IO_ECHARSET;
        case ERROR_CONNECTION_ABORTED:          return IO_ECONNABORTED;
        case WSAECONNABORTED:                   return IO_ECONNABORTED;
        case ERROR_CONNECTION_REFUSED:          return IO_ECONNREFUSED;
        case WSAECONNREFUSED:                   return IO_ECONNREFUSED;
        case ERROR_NETNAME_DELETED:             return IO_ECONNRESET;
        case WSAECONNRESET:                     return IO_ECONNRESET;
        case ERROR_ALREADY_EXISTS:              return IO_EEXIST;
        case ERROR_FILE_EXISTS:                 return IO_EEXIST;
        case ERROR_BUFFER_OVERFLOW:             return IO_EFAULT;
        case WSAEFAULT:                         return IO_EFAULT;
        case ERROR_HOST_UNREACHABLE:            return IO_EHOSTUNREACH;
        case WSAEHOSTUNREACH:                   return IO_EHOSTUNREACH;
        case ERROR_INSUFFICIENT_BUFFER:         return IO_EINVAL;
        case ERROR_INVALID_DATA:                return IO_EINVAL;
        case ERROR_INVALID_PARAMETER:           return IO_EINVAL;
        case ERROR_SYMLINK_NOT_SUPPORTED:       return IO_EINVAL;
        case WSAEINVAL:                         return IO_EINVAL;
        case WSAEPFNOSUPPORT:                   return IO_EINVAL;
        case ERROR_BEGINNING_OF_MEDIA:          return IO_EIO;
        case ERROR_BUS_RESET:                   return IO_EIO;
        case ERROR_CRC:                         return IO_EIO;
        case ERROR_DEVICE_DOOR_OPEN:            return IO_EIO;
        case ERROR_DEVICE_REQUIRES_CLEANING:    return IO_EIO;
        case ERROR_DISK_CORRUPT:                return IO_EIO;
        case ERROR_EOM_OVERFLOW:                return IO_EIO;
        case ERROR_FILEMARK_DETECTED:           return IO_EIO;
        case ERROR_GEN_FAILURE:                 return IO_EIO;
        case ERROR_INVALID_BLOCK_LENGTH:        return IO_EIO;
        case ERROR_IO_DEVICE:                   return IO_EIO;
        case ERROR_NO_DATA_DETECTED:            return IO_EIO;
        case ERROR_NO_SIGNAL_SENT:              return IO_EIO;
        case ERROR_OPEN_FAILED:                 return IO_EIO;
        case ERROR_SETMARK_DETECTED:            return IO_EIO;
        case ERROR_SIGNAL_REFUSED:              return IO_EIO;
        case WSAEISCONN:                        return IO_EISCONN;
        case ERROR_CANT_RESOLVE_FILENAME:       return IO_ELOOP;
        case ERROR_TOO_MANY_OPEN_FILES:         return IO_EMFILE;
        case WSAEMFILE:                         return IO_EMFILE;
        case WSAEMSGSIZE:                       return IO_EMSGSIZE;
        case ERROR_FILENAME_EXCED_RANGE:        return IO_ENAMETOOLONG;
        case ERROR_NETWORK_UNREACHABLE:         return IO_ENETUNREACH;
        case WSAENETUNREACH:                    return IO_ENETUNREACH;
        case WSAENOBUFS:                        return IO_ENOBUFS;
        case ERROR_BAD_PATHNAME:                return IO_ENOENT;
        case ERROR_DIRECTORY:                   return IO_ENOENT;
        case ERROR_ENVVAR_NOT_FOUND:            return IO_ENOENT;
        case ERROR_FILE_NOT_FOUND:              return IO_ENOENT;
        case ERROR_INVALID_NAME:                return IO_ENOENT;
        case ERROR_INVALID_DRIVE:               return IO_ENOENT;
        case ERROR_INVALID_REPARSE_DATA:        return IO_ENOENT;
        case ERROR_MOD_NOT_FOUND:               return IO_ENOENT;
        case ERROR_PATH_NOT_FOUND:              return IO_ENOENT;
        case WSAHOST_NOT_FOUND:                 return IO_ENOENT;
        case WSANO_DATA:                        return IO_ENOENT;
        case ERROR_NOT_ENOUGH_MEMORY:           return IO_ENOMEM;
        case ERROR_OUTOFMEMORY:                 return IO_ENOMEM;
        case ERROR_CANNOT_MAKE:                 return IO_ENOSPC;
        case ERROR_DISK_FULL:                   return IO_ENOSPC;
        case ERROR_EA_TABLE_FULL:               return IO_ENOSPC;
        case ERROR_END_OF_MEDIA:                return IO_ENOSPC;
        case ERROR_HANDLE_DISK_FULL:            return IO_ENOSPC;
        case ERROR_NOT_CONNECTED:               return IO_ENOTCONN;
        case WSAENOTCONN:                       return IO_ENOTCONN;
        case ERROR_DIR_NOT_EMPTY:               return IO_ENOTEMPTY;
        case WSAENOTSOCK:                       return IO_ENOTSOCK;
        case ERROR_NOT_SUPPORTED:               return IO_ENOTSUP;
        case ERROR_BROKEN_PIPE:                 return IO_EOF;
        case ERROR_ACCESS_DENIED:               return IO_EPERM;
        case ERROR_PRIVILEGE_NOT_HELD:          return IO_EPERM;
        case ERROR_BAD_PIPE:                    return IO_EPIPE;
        case ERROR_NO_DATA:                     return IO_EPIPE;
        case ERROR_PIPE_NOT_CONNECTED:          return IO_EPIPE;
        case WSAESHUTDOWN:                      return IO_EPIPE;
        case WSAEPROTONOSUPPORT:                return IO_EPROTONOSUPPORT;
        case ERROR_WRITE_PROTECT:               return IO_EROFS;
        case ERROR_SEM_TIMEOUT:                 return IO_ETIMEDOUT;
        case WSAETIMEDOUT:                      return IO_ETIMEDOUT;
        case ERROR_NOT_SAME_DEVICE:             return IO_EXDEV;
        case ERROR_INVALID_FUNCTION:            return IO_EISDIR;
        case ERROR_META_EXPANSION_TOO_LONG:     return IO_E2BIG;
        case WSAESOCKTNOSUPPORT:                return IO_ESOCKTNOSUPPORT;
        default:                                return IO_UNKNOWN;
        }
    }
}
