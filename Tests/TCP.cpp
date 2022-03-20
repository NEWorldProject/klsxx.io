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

#include <gtest/gtest.h>
#include "kls/io/TCP.h"
#include "kls/coroutine/Blocking.h"
#include "kls/coroutine/Operation.h"

TEST(kls_io, TcpEcho) {
    using namespace kls::io;
    using namespace kls::essential;
    using namespace kls::coroutine;

    auto ServerOnceEcho = []() -> ValueAsync<void> {
        auto accept = acceptor_tcp(Address::CreateIPv4("0.0.0.0").value(), 30080, 128);
        auto&&[address, stream] = co_await accept->once();
        char buffer[1000];
        auto resultA = co_await stream->read({buffer, 1000});
        co_await stream->write({buffer, static_cast<uint32_t>(resultA.result())});
        co_await stream->close();
        co_await accept->close();
    };

    auto ClientOnce = []() -> ValueAsync<void> {
        auto conn = co_await connect(Address::CreateIPv4("127.0.0.1").value(), 30080);
        char data[13] = "Hello World\n";
        char buffer[1000];
        co_await conn->write({data, 13});
        auto resultA = co_await conn->read({buffer, 1000});
        puts(buffer);
        co_await conn->close();
    };

    run_blocking([&]() -> ValueAsync<void> {
        co_await kls::coroutine::awaits(ServerOnceEcho(), ClientOnce());
    });
}