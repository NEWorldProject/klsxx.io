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

namespace {
    using namespace kls;
    using namespace kls::io;
    using namespace kls::io::detail;
    using namespace kls::essential;

    class TcpImpl : public SocketTCP {
        template<Uring::Ops Op>
        IOAwait<IOResult> simple(Span<> buffer) {
            auto lk = m_core->lock();
            return m_core->create<IOResult, Op>(mFd, buffer.data(), buffer.size(), 0);
        }

        template<Uring::Ops Op>
        VecAwait aggregated(Span<iovec> vec) {
            msghdr message{
                    .msg_name = nullptr, .msg_namelen = 0,
                    .msg_iov = vec.data(), .msg_iovlen = vec.size(),
                    .msg_control = nullptr, .msg_controllen = 0, .msg_flags = 0
            };
            auto lk = m_core->lock();
            return m_core->create_vec<Op>(mFd, message, 0);
        }

    public:
        TcpImpl(int socket, std::shared_ptr<Uring> core) noexcept: mFd{socket}, m_core{std::move(core)} {}

        IOAwait<IOResult> read(Span<> buffer) noexcept override { return simple<Uring::Recv>(buffer); }

        IOAwait<IOResult> write(Span<> buffer) noexcept override { return simple<Uring::Send>(buffer); }

        VecAwait readv(Span<IoVec> vec) noexcept override {
            return aggregated<Uring::RecvMsg>(reinterpret_span_cast<iovec>(vec));
        }

        VecAwait writev(Span<IoVec> vec) noexcept override {
            return aggregated<Uring::SendMsg>(reinterpret_span_cast<iovec>(vec));
        }

        IOAwait<Status> close() noexcept override {
            auto lk = m_core->lock();
            return m_core->create<Status, Uring::Close>(mFd);
        }

    private:
        const int mFd;
        std::shared_ptr<Uring> m_core;
    };

    class AcceptImpl : public AcceptorTCP {
    public:
        explicit AcceptImpl(int socket) noexcept: mFd(socket) {}

        IOAwait<Status> close() noexcept override {
            auto lk = m_core->lock();
            return m_core->create<Status, Uring::Close>(mFd);
        }

    protected:
        const int mFd;
        std::shared_ptr<Uring> m_core = Uring::get();

        IOAwait<IOResult> accept(sockaddr *address, socklen_t len) noexcept {
            auto lk = m_core->lock();
            return m_core->create<IOResult, Uring::Accept>(mFd, address, &len, 0);
        }
    };

    using PSAddr = sockaddr*;

    class AcceptImpl4 : public AcceptImpl {
    public:
        using AcceptImpl::AcceptImpl;

        coroutine::ValueAsync<Result> once() override {
            sockaddr_in peer{};
            const auto res = (co_await accept(PSAddr(&peer), sizeof(peer))).get_result();
            co_return Result{
                    .peer = from_os_ip(peer),
                    .handle = std::make_unique<TcpImpl>(res, m_core)
            };
        }
    };

    class AcceptImpl6 : public AcceptImpl {
    public:
        using AcceptImpl::AcceptImpl;

        coroutine::ValueAsync<Result> once() override {
            sockaddr_in6 peer{};
            const auto res = (co_await accept(PSAddr(&peer), sizeof(peer))).get_result();
            co_return Result{
                    .peer = from_os_ip(peer),
                    .handle = std::make_unique<TcpImpl>(res, m_core)
            };
        }
    };

    IOAwait<IOResult> connect(Uring &core, int socket, sockaddr *address, socklen_t len) noexcept {
        auto lk = core.lock();
        return core.create<IOResult, Uring::Connect>(socket, address, len);
    }

    coroutine::ValueAsync<std::unique_ptr<SocketTCP>> connect4(Address address, int port) {
        std::shared_ptr<Uring> core = Uring::get();
        const auto sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock != -1) {
            sockaddr_in in = to_os_ipv4(address, port);
            if (const auto res = co_await connect(*core, sock, PSAddr(&in), sizeof(in)); res.success())
                co_return std::make_unique<TcpImpl>(sock, std::move(core));
            close(sock);
        }
        throw exception_errc(detail::map_error(errno));
    }

    coroutine::ValueAsync<std::unique_ptr<SocketTCP>> connect6(Address address, int port) {
        std::shared_ptr<Uring> core = Uring::get();
        const auto sock = socket(AF_INET6, SOCK_STREAM, 0);
        if (sock != -1) {
            sockaddr_in6 in = to_os_ipv6(address, port);
            if (const auto res = co_await connect(*core, sock, PSAddr(&in), sizeof(in)); res.success())
                co_return std::make_unique<TcpImpl>(sock, std::move(core));
            close(sock);
        }
        throw exception_errc(detail::map_error(errno));
    }

    std::unique_ptr<AcceptorTCP> acceptor4(Address address, int port, int backlog) {
        const auto sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock != -1) {
            sockaddr_in target = to_os_ipv4(address, port);
            if (bind(sock, PSAddr(&target), sizeof(target)) == -1) goto error;
            if (listen(sock, backlog) != -1) return std::make_unique<AcceptImpl4>(sock);
            error:
            close(sock);
        }
        throw exception_errc(detail::map_error(errno));
    }

    std::unique_ptr<AcceptorTCP> acceptor6(Address address, int port, int backlog) {
        const auto sock = socket(AF_INET6, SOCK_STREAM, 0);
        if (sock != -1) {
            sockaddr_in6 target = to_os_ipv6(address, port);
            if (bind(sock, PSAddr(&target), sizeof(target)) == -1) goto error;
            if (listen(sock, backlog) != -1) return std::make_unique<AcceptImpl6>(sock);
            error:
            close(sock);
        }
        throw exception_errc(detail::map_error(errno));
    }
}

namespace kls::io {
    coroutine::ValueAsync<std::unique_ptr<SocketTCP>> connect(Address address, int port) {
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