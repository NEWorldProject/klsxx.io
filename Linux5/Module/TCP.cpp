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
#include "Uring.h"
#include "kls/io/TCP.h"

namespace kls::io::detail {
    struct TCPHelper {
        static SocketTCP socket(int s) { return SocketTCP{s}; }
    };
}

namespace {
    using namespace kls;
    using namespace kls::io;
    using namespace kls::io::detail;
    using namespace kls::essential;

    class AcceptImpl : public AcceptorTCP {
    public:
        explicit AcceptImpl(int socket) noexcept: mFd(socket) {}
        IOAwait<Status> close() noexcept override { return io_plain<Status, IoOps::Close>(mFd); }
    protected:
        const int mFd;
        SafeHandle<Uring> m_core = Uring::get();

        IOAwait<IOResult> accept(sockaddr *address, socklen_t &len) noexcept {
            return io_plain<IOResult, IoOps::Accept>(mFd, address, &len, 0);
        }
    };

    using PSAddr = sockaddr *;

    class AcceptImpl4 : public AcceptImpl {
    public:
        using AcceptImpl::AcceptImpl;

        coroutine::ValueAsync<Result> once() override {
            sockaddr_in peer{};
            socklen_t len{sizeof(peer)};
            const auto res = (co_await accept(PSAddr(&peer), len)).get_result();
            co_return Result{.peer = from_os_ip(peer), .handle = SafeHandle{TCPHelper::socket(res)}};
        }
    };

    class AcceptImpl6 : public AcceptImpl {
    public:
        using AcceptImpl::AcceptImpl;

        coroutine::ValueAsync<Result> once() override {
            sockaddr_in6 peer{};
            socklen_t len{sizeof(peer)};
            const auto res = (co_await accept(PSAddr(&peer), len)).get_result();
            co_return Result{.peer = from_os_ip(peer), .handle = SafeHandle{TCPHelper::socket(res)}};
        }
    };

    template<class SockAdr>
    IOAwait<IOResult> connect(int socket, const SockAdr &address) noexcept {
        return io_plain<IOResult, IoOps::Connect>(socket, PSAddr(&address), sizeof(SockAdr));
    }

    coroutine::ValueAsync<SafeHandle<SocketTCP>> connect4(Address address, int port) {
        const auto core = Uring::get();
        const auto sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock != -1) {
            sockaddr_in in = to_os_ipv4(address, port);
            if (const auto res = co_await connect(sock, in); res.success())
                co_return SafeHandle{TCPHelper::socket(sock)};
            close(sock);
        }
        throw exception_errc(map_error(errno));
    }

    coroutine::ValueAsync<SafeHandle<SocketTCP>> connect6(Address address, int port) {
        const auto core = Uring::get();
        const auto sock = socket(AF_INET6, SOCK_STREAM, 0);
        if (sock != -1) {
            sockaddr_in6 in = to_os_ipv6(address, port);
            if (const auto res = co_await connect(sock, in); res.success())
                co_return SafeHandle{TCPHelper::socket(sock)};
            close(sock);
        }
        throw exception_errc(map_error(errno));
    }

    std::unique_ptr<AcceptorTCP> acceptor4(Address address, int port, int backlog) {
        const auto sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock != -1) {
            int enable = 1;
            sockaddr_in target = to_os_ipv4(address, port);
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) goto error;
            if (bind(sock, PSAddr(&target), sizeof(target)) == -1) goto error;
            if (listen(sock, backlog) != -1) return std::make_unique<AcceptImpl4>(sock);
            error:
            close(sock);
        }
        throw exception_errc(map_error(errno));
    }

    std::unique_ptr<AcceptorTCP> acceptor6(Address address, int port, int backlog) {
        const auto sock = socket(AF_INET6, SOCK_STREAM, 0);
        if (sock != -1) {
            sockaddr_in6 target = to_os_ipv6(address, port);
            int enable = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) goto error;
            if (bind(sock, PSAddr(&target), sizeof(target)) == -1) goto error;
            if (listen(sock, backlog) != -1) return std::make_unique<AcceptImpl6>(sock);
            error:
            close(sock);
        }
        throw exception_errc(map_error(errno));
    }
}

namespace kls::io {
    template<IoOps Op>
    static IOAwait<IOResult> simple(int fd, Span<> buffer) {
        return io_plain<IOResult, Op>(fd, buffer.data(), buffer.size(), 0);
    }

    template<IoOps Op>
    static VecAwait aggregated(int fd, Span<iovec> vec) {
        msghdr message{
                .msg_name = nullptr, .msg_namelen = 0,
                .msg_iov = vec.data(), .msg_iovlen = vec.size(),
                .msg_control = nullptr, .msg_controllen = 0, .msg_flags = 0
        };
        return io_message<Op>(fd, message, 0);
    }

    SocketTCP::SocketTCP(int h) : Handle<int>([c = Uring::get()](int h) noexcept {}, h) {}

    IOAwait<IOResult> SocketTCP::read(Span<> buffer) noexcept { return simple<IoOps::Recv>(value(), buffer); }

    IOAwait<IOResult> SocketTCP::write(Span<> buffer) noexcept { return simple<IoOps::Send>(value(), buffer); }

    VecAwait SocketTCP::readv(Span<IoVec> vec) noexcept {
        return aggregated<IoOps::RecvMsg>(value(), reinterpret_span_cast<iovec>(vec));
    }

    VecAwait SocketTCP::writev(Span<IoVec> vec) noexcept {
        return aggregated<IoOps::SendMsg>(value(), reinterpret_span_cast<iovec>(vec));
    }

    IOAwait<Status> SocketTCP::close() noexcept { return io_plain<Status, IoOps::Close>(value()); }

    coroutine::ValueAsync<SafeHandle<SocketTCP>> connect(Address address, int port) {
        switch (address.family()) {
            case Address::AF_IPv4:
                return connect4(address, port);
            case Address::AF_IPv6:
                return connect6(address, port);
            default:
                throw std::runtime_error("Invalid Peer Family");
        }
    }

    std::unique_ptr<AcceptorTCP> acceptor_tcp(Address address, int port, int backlog) {
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