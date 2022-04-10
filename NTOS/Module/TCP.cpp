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

#include "IP.h"
#include "WSA.h"
#include "IOCP.h"
#include <MSWSock.h>
#include <stdexcept>
#include "kls/io/TCP.h"
#include "kls/essential/Final.h"

namespace {
    using namespace kls;
    using namespace kls::io;
    using namespace kls::io::detail;
    using namespace kls::essential;

    GUID g_id_connect_ex = WSAID_CONNECTEX;
    GUID g_id_disconnect_ex = WSAID_DISCONNECTEX;

    template<class T>
    DWORD WSAO(T ret, T exp) noexcept {
        if (ret == exp) return WSA_IO_PENDING; else return WSAGetLastError();
    }

    template<class T>
    int WSAGetExtFn(SOCKET socket, const GUID &name, T &fn) noexcept {
        DWORD dwBytes{};
        return WSAIoctl(
                socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                const_cast<GUID *>(&name), sizeof(GUID), &fn, sizeof(T),
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
        explicit SocketTcpImpl(SOCKET s) noexcept: m_socket(s) {}

        IOAwait<IOResult> read(Span<> buffer) noexcept override {
            auto iov = IoVec(buffer);
            auto v = reinterpret_cast<LPWSABUF>(&iov);
            return {
                    [this, v](LPOVERLAPPED o) noexcept -> DWORD {
                        DWORD flags{0};
                        return WSAO(WSARecv(m_socket, v, 1, nullptr, &flags, o, nullptr), 0);
                    }
            };
        }

        IOAwait<IOResult> write(Span<> buffer) noexcept override {
            auto iov = IoVec(buffer);
            auto v = reinterpret_cast<LPWSABUF>(&iov);
            return {
                    [this, v](LPOVERLAPPED o) noexcept -> DWORD {
                        return WSAO(WSASend(m_socket, v, 1, nullptr, 0, o, nullptr), 0);
                    }
            };
        }

        IOAwait<IOResult> readv(Span<IoVec> vec) noexcept override {
            auto v = reinterpret_span_cast<WSABUF>(vec);
            return {
                    [this, &v](LPOVERLAPPED o) noexcept -> DWORD {
                        DWORD flags{0};
                        return WSAO(WSARecv(m_socket, v.data(), v.size(), nullptr, &flags, o, nullptr), 0);
                    }
            };
        }

        IOAwait<IOResult> writev(Span<IoVec> vec) noexcept override {
            auto v = reinterpret_span_cast<WSABUF>(vec);
            return {
                    [this, &v](LPOVERLAPPED o) noexcept -> DWORD {
                        return WSAO(WSASend(m_socket, v.data(), v.size(), nullptr, 0, o, nullptr), 0);
                    }
            };
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
        return RAII(socket, [](auto s) noexcept { closesocket(s); });
    }

    IOAwait<Status> connectOS(SOCKET socket, const sockaddr *name, int len) noexcept {
        return IOAwait<Status>(
                [=](LPOVERLAPPED o) noexcept -> DWORD {
                    LPFN_CONNECTEX func{};
                    if (WSAGetExtFn(socket, g_id_connect_ex, func) == SOCKET_ERROR) return WSAGetLastError();
                    return WSAO(func(socket, name, len, nullptr, 0, nullptr, o), TRUE);
                }
        );
    }

    template<class SockIn>
    IOAwait<Status> connectAdapt(SOCKET socket, const SockIn &local, const SockIn &remote) noexcept {
        if (!bind(socket, local)) return {[](auto) noexcept -> DWORD { return WSAGetLastError(); }};
        return connectOS(socket, (sockaddr *) (&remote), sizeof(SockIn));
    }

    IOAwait<Status> connectAsync(SOCKET socket, const Address &address, int port) noexcept {
        if (address.family() == Address::AF_IPv4)
            return connectAdapt(socket, any_v4, to_os_ipv4(address, port));
        else
            return connectAdapt(socket, any_v6, to_os_ipv6(address, port));
    }

    class AcceptImpl : public AcceptorTCP {
    public:
        explicit AcceptImpl(SOCKET socket) noexcept: m_socket(socket) {}

        void initialize() {
            GUID g_idAcceptEx = WSAID_ACCEPTEX;
            GUID g_idGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
            if (WSAGetExtFn(m_socket, g_idAcceptEx, m_pfnAcceptEx) == SOCKET_ERROR) goto error;
            if (WSAGetExtFn(m_socket, g_idGetAcceptExSockAddrs, m_pfnGetAcceptExSockAddrs) == SOCKET_ERROR) goto error;
            return;
            error:
            throw exception_errc(detail::map_error(WSAGetLastError()));
        }

        IOAwait<Status> close() noexcept override { return closeAsync(m_socket); }
    protected:
        const SOCKET m_socket;
        const std::shared_ptr<detail::WSA> m_wsa = detail::WSA::get();

        IOAwait<Status> acceptOS(SOCKET accept, void *data, int length) noexcept {
            return IOAwait<Status>(
                    [=, this](LPOVERLAPPED o) noexcept -> DWORD {
                        DWORD dwBytes{};
                        return WSAO(m_pfnAcceptEx(m_socket, accept, data, 0, length, length, &dwBytes, o), TRUE);
                    }
            );
        }

        void syncAccept(SOCKET socket) {
            if (setsockopt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                           (char *) &(m_socket), sizeof(SOCKET)) == SOCKET_ERROR)
                throw exception_errc(detail::map_error(WSAGetLastError()));
        }

        template<class SockIn>
        coroutine::ValueAsync<Result> accept(auto accept) {
            SockIn remote{};
            {
                static constexpr auto unit = sizeof(SockIn) + 16;
                char buffer[unit * 2];
                auto result = co_await acceptOS(accept.get(), buffer, unit);
                if (result == IO_OK) {
                    sockaddr *localAdr{}, *remoteAdr{};
                    int localLen{}, remoteLen{};
                    m_pfnGetAcceptExSockAddrs(buffer, 0, unit, unit, &localAdr, &localLen, &remoteAdr, &remoteLen);
                    std::memcpy(&remote, remoteAdr, sizeof(SockIn));
                } else throw exception_errc(result);
            }
            syncAccept(accept.get());
            co_return Result{.peer = from_os_ip(remote), .handle = std::make_unique<SocketTcpImpl>(accept.reset())};
        }
    private:
        LPFN_ACCEPTEX m_pfnAcceptEx{nullptr};
        LPFN_GETACCEPTEXSOCKADDRS m_pfnGetAcceptExSockAddrs{nullptr};
    };

    class AcceptImpl4 : public AcceptImpl {
    public:
        using AcceptImpl::AcceptImpl;

        coroutine::ValueAsync<Result> once() override {
            auto socket = createSocket(Address::AF_IPv4);
            detail::IOCP::bind(HANDLE(socket.get()));
            return accept < sockaddr_in > (std::move(socket));
        }
    };

    class AcceptImpl6 : public AcceptImpl {
    public:
        using AcceptImpl::AcceptImpl;

        coroutine::ValueAsync<Result> once() override {
            auto socket = createSocket(Address::AF_IPv6);
            detail::IOCP::bind(HANDLE(socket.get()));
            return accept < sockaddr_in6 > (std::move(socket));
        }
    };

    std::unique_ptr<AcceptImpl> acceptor4(Address address, int port, int backlog) {
        auto socket = createSocket(Address::AF_IPv4);
        detail::IOCP::bind(HANDLE(socket.get()));
        if (!bind(socket.get(), to_os_ipv4(address, port))) goto error;
        if (listen(socket.get(), backlog) != -1) return std::make_unique<AcceptImpl4>(socket.reset());
        error:
        throw exception_errc(detail::map_error(WSAGetLastError()));
    }

    std::unique_ptr<AcceptImpl> acceptor6(Address address, int port, int backlog) {
        auto socket = createSocket(Address::AF_IPv6);
        detail::IOCP::bind(HANDLE(socket.get()));
        if (!bind(socket.get(), to_os_ipv6(address, port))) goto error;
        if (listen(socket.get(), backlog) != -1) return std::make_unique<AcceptImpl6>(socket.reset());
        error:
        throw exception_errc(detail::map_error(WSAGetLastError()));
    }

    std::unique_ptr<AcceptorTCP> initialize(std::unique_ptr<AcceptImpl> acceptor) {
        acceptor->initialize();
        return std::move(acceptor);
    }
}

namespace kls::io {
    coroutine::ValueAsync<std::unique_ptr<SocketTCP>> connect(Address address, int port) {
        auto wsa = detail::WSA::get();
        auto socket = createSocket(address.family());
        detail::IOCP::bind(HANDLE(socket.get()));
        if (auto r = co_await connectAsync(socket.get(), address, port); r != IO_OK) throw exception_errc(r);
        if (setsockopt(socket.get(), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0) != 0)
            throw exception_errc(detail::map_error(WSAGetLastError()));
        co_return std::make_unique<SocketTcpImpl>(socket.reset());
    }

    std::unique_ptr<AcceptorTCP> acceptor_tcp(Address address, int port, int backlog) {
        auto wsa = detail::WSA::get();
        switch (address.family()) {
            case Address::AF_IPv4:
                return initialize(acceptor4(address, port, backlog));
            case Address::AF_IPv6:
                return initialize(acceptor6(address, port, backlog));
            default:
                throw std::runtime_error("Invalid Peer Family");
        }
    }
}
