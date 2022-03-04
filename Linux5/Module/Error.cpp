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

#include <cerrno>
#include <cstdio>
#include <cstdint>
#include "kls/io/Status.h"

namespace kls::io::detail {
    Status map_error(int32_t sys) noexcept {
        switch(sys) {
            case EACCES: return IO_EACCES;
            case EADDRINUSE: return IO_EADDRINUSE;
            case EADDRNOTAVAIL: return IO_EADDRNOTAVAIL;
            case EAFNOSUPPORT: return IO_EAFNOSUPPORT;
            case EAGAIN: return IO_EAGAIN;
            case EALREADY: return IO_EALREADY;
            case EBADF: return IO_EBADF;
            case EBUSY: return IO_EBUSY;
            case ECANCELED: return IO_ECANCELED;
            case ECONNABORTED: return IO_ECONNABORTED;
            case ECONNREFUSED: return IO_ECONNREFUSED;
            case ECONNRESET: return IO_ECONNRESET;
            case EDESTADDRREQ: return IO_EDESTADDRREQ;
            case EEXIST: return IO_EEXIST;
            case EFAULT: return IO_EFAULT;
            case E2BIG: return IO_E2BIG;
            case EHOSTUNREACH: return IO_EHOSTUNREACH;
            case EINTR: return IO_EINTR;
            case EINVAL: return IO_EINVAL;
            case EIO: return IO_EIO;
            case EISCONN: return IO_EISCONN;
            case EISDIR: return IO_EISDIR;
            case ELOOP: return IO_ELOOP;
            case EMFILE: return IO_EMFILE;
            case EMSGSIZE: return IO_EMSGSIZE;
            case ENAMETOOLONG: return IO_ENAMETOOLONG;
            case ENETDOWN: return IO_ENETDOWN;
            case ENETUNREACH: return IO_ENETUNREACH;
            case ENFILE: return IO_ENFILE;
            case ENOBUFS: return IO_ENOBUFS;
            case ENODEV: return IO_ENODEV;
            case ENOENT: return IO_ENOENT;
            case ENOMEM: return IO_ENOMEM;
            case ENONET: return IO_ENONET;
            case ENOPROTOOPT: return IO_ENOPROTOOPT;
            case ENOSPC: return IO_ENOSPC;
            case ENOSYS: return IO_ENOSYS;
            case ENOTCONN: return IO_ENOTCONN;
            case ENOTDIR: return IO_ENOTDIR;
            case ENOTEMPTY: return IO_ENOTEMPTY;
            case ENOTSOCK: return IO_ENOTSOCK;
            case ENOTSUP: return IO_ENOTSUP;
            case EOVERFLOW: return IO_EOVERFLOW;
            case EPERM: return IO_EPERM;
            case EPIPE: return IO_EPIPE;
            case EPROTO: return IO_EPROTO;
            case EPROTONOSUPPORT: return IO_EPROTONOSUPPORT;
            case EPROTOTYPE: return IO_EPROTOTYPE;
            case ERANGE: return IO_ERANGE;
            case EROFS: return IO_EROFS;
            case ESHUTDOWN: return IO_ESHUTDOWN;
            case ESPIPE: return IO_ESPIPE;
            case ESRCH: return IO_ESRCH;
            case ETIMEDOUT: return IO_ETIMEDOUT;
            case ETXTBSY: return IO_ETXTBSY;
            case EXDEV: return IO_EXDEV;
            case EOF: return IO_EOF;
            case ENXIO: return IO_ENXIO;
            case EMLINK: return IO_EMLINK;
            case ENOTTY: return IO_ENOTTY;
            case EILSEQ: return IO_EILSEQ;
            case ESOCKTNOSUPPORT: return IO_ESOCKTNOSUPPORT;
            default: return IO_UNKNOWN;
        }
    }

    IOResult map_result(int32_t ret) noexcept {
        if (ret >= 0) return IOResult(IO_OK, ret); else return IOResult(map_error(ret));
    }
}