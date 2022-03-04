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

#include "kls/io/IP.h"
#if __has_include(<WS2tcpip.h>)
#include <WS2tcpip.h>
#elif __has_include(<arpa/inet.h>)
#include <arpa/inet.h>
#else
#error "Platform Not Supported"
#endif

#include <cstring>

namespace kls::io {
    Address Address::CreateIPv4(std::byte* data) noexcept {
        Address ret{};
        ret.mFamily = AF_IPv4;
        std::memcpy(ret.mStorage, data, 4);
        return ret;
    }

    Address Address::CreateIPv6(std::byte* data) noexcept {
        Address ret{};
        ret.mFamily = AF_IPv6;
        std::memcpy(ret.mStorage, data, 16);
        return ret;
    }

    std::optional<Address> Address::CreateIPv4(std::string_view text) noexcept {
        if (in_addr p{}; inet_pton(AF_INET, text.data(), &p) == 1)
            return CreateIPv4(reinterpret_cast<std::byte*>(&p.s_addr));
        return std::nullopt;
    }

    std::optional<Address> Address::CreateIPv6(std::string_view text) noexcept {
        if (in6_addr p{}; inet_pton(AF_INET6, text.data(), &p) == 1)
            return CreateIPv6(reinterpret_cast<std::byte*>(&p.s6_addr));
        return std::nullopt;
    }
}
