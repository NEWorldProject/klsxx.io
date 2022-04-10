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

#include "kls/io/TCPUtil.h"

namespace kls::io {
    coroutine::ValueAsync<IOResult> read_fully(SocketTCP& s, Span<> buffer) noexcept {
        auto bytes = static_span_cast<char>(buffer);
        if (buffer.size() == 0) co_return IOResult(IO_OK, 0);
        for (;;) {
            if (auto read_op = co_await s.read(bytes); read_op.success()) {
                auto done = read_op.result();
                if (done == 0) co_return IOResult(IO_EOF);
                if (done == bytes.size()) co_return IOResult(IO_OK, int32_t(buffer.size()));
                bytes = bytes.trim_front(done);
            } else co_return read_op;
        }
    }

    coroutine::ValueAsync<IOResult> write_fully(SocketTCP& s, Span<> buffer) noexcept {
        auto bytes = static_span_cast<char>(buffer);
        if (buffer.size() == 0) co_return IOResult(IO_OK, 0);
        for (;;) {
            if (auto write_op = co_await s.write(bytes); write_op.success()) {
                auto done = write_op.result();
                if (done == 0) co_return IOResult(IO_EOF);
                if (done == bytes.size()) co_return IOResult(IO_OK, int32_t(buffer.size()));
                bytes = bytes.trim_front(done);
            } else co_return write_op;
        }
    }
}