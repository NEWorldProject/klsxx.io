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

    static constexpr auto payload = std::string_view("Hello World\n");
    static constexpr auto payload_size = payload.size() + 1;

    auto ServerOnceEcho = []() -> ValueAsync<void> {
        auto accept = acceptor_tcp(Address::CreateIPv4("0.0.0.0").value(), 30080, 128);
        co_await uses(accept, [](AcceptorTCP& accept) -> ValueAsync<void> {
            auto&&[address, stream] = co_await accept.once();
            co_await uses(stream, [](SocketTCP &conn) -> ValueAsync<void> {
                char buffer[1000];
                auto resultA = (co_await conn.read({buffer, 1000})).get_result();
                (co_await (conn.write({buffer, resultA}))).get_result();
            });
        });
    };

    auto ClientOnce = []() -> ValueAsync<void> {
        auto file = co_await connect(Address::CreateIPv4("127.0.0.1").value(), 30080);
        if (!co_await uses(file, [](SocketTCP& conn) -> ValueAsync<bool> {
            char buffer[1000];
            if ((co_await conn.write({payload.data(), payload_size})).get_result() != payload_size) co_return false;
            if ((co_await conn.read({buffer, 1000})).get_result() != payload_size) co_return false;
            co_return payload.compare(buffer) == 0;
        })) throw std::runtime_error("Tcp Echo Content Check Failure");
    };

    run_blocking([&]() -> ValueAsync<void> {
        co_await kls::coroutine::awaits(ServerOnceEcho(), ClientOnce());
    });
}