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

#define NOMINMAX
#include "kls/io/IP.h"
#include <WS2tcpip.h>
#include <utility>
#include <cstring>

namespace kls::io::detail {
    inline sockaddr_in to_os_ipv4(const Address &ip, int port) noexcept {
        sockaddr_in in{.sin_family = AF_INET, .sin_port = htons(port)};
        std::memcpy(&in.sin_addr.s_addr, ip.data(), 4);
        return in;
    }

    inline sockaddr_in6 to_os_ipv6(const Address &ip, int port) noexcept {
        sockaddr_in6 in{.sin6_family = AF_INET6, .sin6_port = htons(port)};
        std::memcpy(&in.sin6_addr.s6_addr, ip.data(), 16);
        return in;
    }

    inline std::pair<Address, int> from_os_ip(const sockaddr_in &in) noexcept {
        return {Address::CreateIPv4((std::byte *) (&(in.sin_addr.s_addr))), ntohs(in.sin_port)}; //NOLINT
    }

    inline std::pair<Address, int> from_os_ip(const sockaddr_in6 &in) noexcept {
        return {Address::CreateIPv6((std::byte *) (&(in.sin6_addr.s6_addr))), ntohs(in.sin6_port)}; //NOLINT
    }

    template <class SockIn>
    bool bind(SOCKET socket, const SockIn& address) noexcept {
        return bind(socket, (SOCKADDR *) &address, sizeof(address)) == 0;
    }

    inline static auto any_v4 = []() noexcept {
        sockaddr_in ret{.sin_family = AF_INET, .sin_port = 0};
        ret.sin_addr.s_addr = htonl(INADDR_ANY);
        return ret;
    }();

    inline static auto any_v6 = []() noexcept {
        sockaddr_in6 ret{.sin6_family = AF_INET6, .sin6_port = 0, .sin6_addr = IN6ADDR_ANY_INIT};
        return ret;
    }();
}