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

#pragma once

#include <vector>
#include <cstdint>
#include <sys/uio.h>
#include "Await.h"
#include "kls/io/IP.h"
#include "kls/Handle.h"
#include "kls/coroutine/Async.h"
#include "kls/essential/Memory.h"

namespace kls::io {
    namespace detail {
        struct TCPHelper;
    }

    struct IoVec : private iovec {
        constexpr IoVec() noexcept = default;
        IoVec(Span<> span) noexcept: IoVec(span.data(), span.size()) {}
        IoVec(void* data, size_t size) noexcept: iovec{data, size} {}
    };

    struct SocketTCP: Handle<int> {
        IOAwait<IOResult> read(Span<> buffer) noexcept;
        IOAwait<IOResult> write(Span<> buffer) noexcept;
        VecAwait readv(Span<IoVec> vec) noexcept;
        VecAwait writev(Span<IoVec> vec) noexcept;
        IOAwait<Status> close() noexcept;
    private:
        friend struct ::kls::io::detail::TCPHelper;
        explicit SocketTCP(int h);
    };

    coroutine::ValueAsync<SafeHandle<SocketTCP>> connect(Address address, int port);

    struct AcceptorTCP : PmrBase {
        struct Result {
            Peer peer;
            SafeHandle<SocketTCP> handle;
        };

        virtual coroutine::ValueAsync<Result> once() = 0;
        virtual IOAwait<Status> close() noexcept = 0;
    };

    std::unique_ptr<AcceptorTCP> acceptor_tcp(Address address, int port, int backlog);
}