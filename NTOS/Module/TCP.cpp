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

#include "kls/io/TCP.h"
#include "WSA.h"
#include "IOCP.h"
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdexcept>

namespace {
    using namespace kls;
    using namespace kls::io;
    using namespace kls::essential;

    GUID g_id_connect_ex = WSAID_CONNECTEX;
    GUID g_id_disconnect_ex = WSAID_DISCONNECTEX;

    template<class T>
    DWORD WSAO(T ret, T exp) noexcept {
        if (ret == exp) return WSA_IO_PENDING; else return WSAGetLastError();
    }

    template <class T>
    int WSAGetExtFn(SOCKET socket, const GUID& name, T& fn) noexcept {
        DWORD dwBytes{};
        return WSAIoctl(
            socket,
            SIO_GET_EXTENSION_FUNCTION_POINTER,
            const_cast<GUID*>(&name), sizeof(GUID), &fn, sizeof(T),
            &dwBytes, nullptr, nullptr
        );
    }

    IOAwait<Status> closeAsync(SOCKET socket) noexcept {
        return IOAwait<Status>(
            [socket](LPOVERLAPPED o) noexcept -> DWORD {
                LPFN_DISCONNECTEX func{};
                if (WSAGetExtFn(socket, g_id_disconnect_ex, func) == SOCKET_ERROR) return WSAGetLastError();
                return WSAO(func(socket, o, 0, 0), TRUE);
            }
        );
    }

    class SocketTcpImpl : public SocketTCP {
    public:
        explicit SocketTcpImpl(SOCKET s) noexcept : m_socket(s) {}

        IOAwait<IOResult> read(essential::Span<> buffer) noexcept override {
            auto iov = IoVec(buffer);
            auto v = reinterpret_cast<LPWSABUF>(&iov);
            return IOAwait<IOResult>(
                [this, v](LPOVERLAPPED o) noexcept -> DWORD {
                    DWORD flags{ 0 };
                    return WSAO(WSARecv(m_socket, v, 1, nullptr, &flags, o, nullptr), 0);
                }
            );
        }

        IOAwait<IOResult> write(essential::Span<> buffer) noexcept override {
            auto iov = IoVec(buffer);
            auto v = reinterpret_cast<LPWSABUF>(&iov);
            return IOAwait<IOResult>(
                [this, v](LPOVERLAPPED o) noexcept -> DWORD {
                    return WSAO(WSASend(m_socket, v, 1, nullptr, 0, o, nullptr), 0);
                }
            );
        }

        IOAwait<IOResult> readv(essential::Span<IoVec> vec) noexcept override {
            auto v = reinterpret_span_cast<WSABUF>(vec);
            return IOAwait<IOResult>(
                [this, &v](LPOVERLAPPED o) noexcept -> DWORD {
                    DWORD flags{0};
                    return WSAO(WSARecv(m_socket, v.data(), v.size(), nullptr, &flags, o, nullptr), 0);
                }
            );
        }

        IOAwait<IOResult> writev(essential::Span<IoVec> vec) noexcept  override {
            auto v = reinterpret_span_cast<WSABUF>(vec);
            return IOAwait<IOResult>(
                [this, &v](LPOVERLAPPED o) noexcept -> DWORD {
                    return WSAO(WSASend(m_socket, v.data(), v.size(), nullptr, 0, o, nullptr), 0);
                }
            );
        }

        IOAwait<Status> close() noexcept override { return closeAsync(m_socket); }
    private:
        SOCKET m_socket;
        const std::shared_ptr<detail::WSA> m_wsa = detail::WSA::get();
    };

    auto createSocket(Address::Family af) {
        auto socket = WSASocketW(
            af == Address::AF_IPv4 ? AF_INET : AF_INET6,
            SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED
        );
        if (socket == INVALID_SOCKET) throw exception_errc(detail::map_error(WSAGetLastError()));
        return RAII(socket, [](auto s) noexcept { if (s != INVALID_SOCKET) closesocket(s); });
    }

    IOAwait<Status> connectAsync(SOCKET socket, const sockaddr* name, int len) noexcept {
        return IOAwait<Status>(
            [=](LPOVERLAPPED o) noexcept -> DWORD {
                LPFN_CONNECTEX func{};
                if (WSAGetExtFn(socket, g_id_connect_ex, func) == SOCKET_ERROR) return WSAGetLastError();
                return WSAO(func(socket, name, len, nullptr, 0, nullptr, o), TRUE);
            }
        );
    }

    IOAwait<Status> connect(SOCKET socket, const Address& address, int port) noexcept {
        if (address.family() == Address::AF_IPv4) {
            sockaddr_in in{ .sin_family = AF_INET, .sin_port = htons(port) };
            std::memcpy(&in.sin_addr.s_addr, address.data(), 4);
            sockaddr_in local{ .sin_family = AF_INET, .sin_port = 0, .sin_addr = INADDR_ANY };
            if (bind(socket, (SOCKADDR*)&local, sizeof(local)) != 0)
                return { [](LPOVERLAPPED) noexcept -> DWORD { return WSAGetLastError(); } };
            return connectAsync(socket, reinterpret_cast<sockaddr*>(&in), sizeof(in));
        }
        else {
            sockaddr_in6 in{ .sin6_family = AF_INET6, .sin6_port = htons(port) };
            std::memcpy(&in.sin6_addr.s6_addr, address.data(), 16);
            sockaddr_in6 local{ .sin6_family = AF_INET6, .sin6_port = 0, .sin6_addr = IN6ADDR_ANY_INIT };
            if (bind(socket, (SOCKADDR*)&local, sizeof(local)) != 0)
                return { [](LPOVERLAPPED) noexcept -> DWORD { return WSAGetLastError(); } };
            return connectAsync(socket, reinterpret_cast<sockaddr*>(&in), sizeof(in));
        }
    }

    class AcceptImpl : public AcceptorTCP {
    public:
        explicit AcceptImpl(SOCKET socket) noexcept : m_socket(socket) {}

        IOAwait<Status> close() noexcept override { return closeAsync(m_socket); }

    protected:
        const SOCKET m_socket;
        const std::shared_ptr<detail::WSA> m_wsa = detail::WSA::get();
    };

    IOAwait<Status> acceptAsync(SOCKET listen, SOCKET accept, void* data, int length) noexcept {
        return IOAwait<Status>(
            [=](LPOVERLAPPED o) noexcept -> DWORD {
                DWORD dwBytes{};
                return WSAO(AcceptEx(listen, accept, data, 0, length, length, &dwBytes, o), TRUE);
            }
        );
    }

    class AcceptImpl4 : public AcceptImpl {
    public:
        using AcceptImpl::AcceptImpl;

        coroutine::ValueAsync<Result> once() override {
            auto socket = createSocket(Address::AF_IPv4);
            detail::IOCP::bind(HANDLE(socket.get()));
            struct Recv {
                sockaddr_in address{};
                char padding[16];
            } data[2];
            auto length = sizeof(Recv);
            auto result = co_await acceptAsync(m_socket, socket.get(), data, length);
            if (result != IO_OK) co_return Result{ .Stat = result };
            if (setsockopt(socket.get(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                (char*)&(m_socket), sizeof(SOCKET)) == SOCKET_ERROR)
                throw exception_errc(detail::map_error(WSAGetLastError()));
            co_return Result{
                    .Peer = Address::CreateIPv4(reinterpret_cast<std::byte*>(&(data[0].address.sin_addr.s_addr))),
                    .Handle = std::make_unique<SocketTcpImpl>(socket.reset(INVALID_SOCKET))
            };
        }
    };

    class AcceptImpl6 : public AcceptImpl {
    public:
        using AcceptImpl::AcceptImpl;

        coroutine::ValueAsync<Result> once() override {
            auto socket = createSocket(Address::AF_IPv6);
            detail::IOCP::bind(HANDLE(socket.get()));
            struct Recv {
                sockaddr_in6 address{};
                char padding[16];
            } data[2];
            auto length = sizeof(Recv);
            auto result = co_await acceptAsync(m_socket, socket.get(), &data, length);
            if (result != IO_OK) co_return Result{ .Stat = result };
            if (setsockopt(socket.get(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                (char*)&(m_socket), sizeof(SOCKET)) == SOCKET_ERROR)
                throw exception_errc(detail::map_error(WSAGetLastError()));
            co_return Result{
                    .Peer = Address::CreateIPv6(reinterpret_cast<std::byte*>(&(data[0].address.sin6_addr.s6_addr))),
                    .Handle = std::make_unique<SocketTcpImpl>(socket.reset(INVALID_SOCKET))
            };
        }
    };

    std::unique_ptr<AcceptorTCP> acceptor4(Address address, int port, int backlog) {
        auto socket = createSocket(address.family());
        detail::IOCP::bind(HANDLE(socket.get()));
        sockaddr_in target{ .sin_family = AF_INET, .sin_port = htons(port) };
        if (bind(socket.get(), reinterpret_cast<sockaddr*>(&target), sizeof(target)) == -1) goto error;
        if (listen(socket.get(), backlog) != -1) return std::make_unique<AcceptImpl4>(socket.reset(INVALID_SOCKET));
    error:
        throw exception_errc(detail::map_error(WSAGetLastError()));
    }

    std::unique_ptr<AcceptorTCP> acceptor6(Address address, int port, int backlog) {
        auto socket = createSocket(address.family());
        detail::IOCP::bind(HANDLE(socket.get()));
        sockaddr_in6 target{ .sin6_family = AF_INET6, .sin6_port = htons(port) };
        if (bind(socket.get(), reinterpret_cast<sockaddr*>(&target), sizeof(target)) == -1) goto error;
        if (listen(socket.get(), backlog) != -1) return std::make_unique<AcceptImpl6>(socket.reset(INVALID_SOCKET));
    error:
        throw exception_errc(detail::map_error(WSAGetLastError()));
    }
}

namespace kls::io {
    coroutine::ValueAsync<std::unique_ptr<SocketTCP>> connect(Address address, int port) {
        auto wsa = detail::WSA::get();
        auto socket = createSocket(address.family());
        detail::IOCP::bind(HANDLE(socket.get()));
        if (auto r = co_await connect(socket.get(), address, port); r != IO_OK) throw exception_errc(r);
        if (setsockopt(socket.get(), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0) != 0)
            throw exception_errc(detail::map_error(WSAGetLastError()));
        co_return std::make_unique<SocketTcpImpl>(socket.reset(INVALID_SOCKET));
    }

    std::unique_ptr<AcceptorTCP> acceptor_tcp(Address address, int port, int backlog) {
        auto wsa = detail::WSA::get();
        switch (address.family()) {
        case Address::AF_IPv4:
            return acceptor4(address, port, backlog);
        case Address::AF_IPv6:
            return acceptor6(address, port, backlog);
        default:
            throw std::runtime_error("Invalid Peer Family");
        }
    }
}
