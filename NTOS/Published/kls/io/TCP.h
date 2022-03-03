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
#include "Await.h"
#include "kls/Object.h"
#include "kls/io/IP.h"
#include "kls/essential/Memory.h"
#include "kls/coroutine/ValueAsync.h"

namespace kls::io {
    struct IoVec : private WSABUF {
        constexpr IoVec() noexcept = default;

        IoVec(essential::Span<> span) noexcept {
            len = static_cast<ULONG>(span.size());
            buf = static_cast<char*>(span.data());
        }

        IoVec(void* data, size_t size) noexcept {
            len = static_cast<ULONG>(size);
            buf = static_cast<char*>(data);
        }
    };

    struct SocketTCP: PmrBase {
        virtual IOAwait<IOResult> read(essential::Span<> buffer) noexcept = 0;

        virtual IOAwait<IOResult> write(essential::Span<> buffer) noexcept = 0;

        virtual IOAwait<IOResult> readv(essential::Span<IoVec> vec) noexcept = 0;

        virtual IOAwait<IOResult> writev(essential::Span<IoVec> vec) noexcept = 0;

        virtual IOAwait<Status> close() noexcept = 0;
    };

    coroutine::ValueAsync<std::unique_ptr<SocketTCP>> Connect(Address address, int port);

    struct AcceptorTCP : PmrBase {
        struct Result {
            Status Stat{IO_OK};
            Address Peer{};
            std::unique_ptr<SocketTCP> Handle{nullptr};
        };

        virtual coroutine::ValueAsync<Result> once() = 0;

        virtual IOAwait<Status> close() noexcept = 0;
    };

    std::unique_ptr<AcceptorTCP> CreateAcceptorTCP(Address address, int port, int backlog);
}