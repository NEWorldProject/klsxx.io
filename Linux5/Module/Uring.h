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

#include <atomic>
#include <coroutine>
#include <liburing.h>
#include "kls/Handle.h"
#include "kls/io/Await.h"
#include "kls/thread/SpinLock.h"
#include "kls/essential/Memory.h"

namespace kls::io::detail {
    enum class IoOps {
        Open, Read, Write, Sync, Close, Send, Recv, SendMsg, RecvMsg, Accept, Connect
    };

    class IoRing {
        static constexpr int QUEUE_DEPTH = 8192;
    public:
        IoRing();
        ~IoRing();
        static IoRing *get() noexcept;
        [[nodiscard]] io_uring_sqe *get_sqe() noexcept;
        [[nodiscard]] auto &ring() noexcept { return m_ring; }
        [[nodiscard]] auto &lock() noexcept { return m_lock; }
    private:
        io_uring m_ring{};
        // We need the lock as we are not able to gather wait CQEs
        // Which made it not practical to use multiple rings to submit.
        // Since the ring itself is not constructed with thread safe,
        //     we need a large lock around the ring for safe submission
        thread::SpinLock m_lock{};

        bool wait_one();
    };

    template<IoOps Op, class ...Args>
    void io_pack_args(io_uring_sqe *sqe, Args &&... args) noexcept {
        if constexpr(Op == IoOps::Open) io_uring_prep_openat(sqe, std::forward<Args>(args)...);
        else if constexpr(Op == IoOps::Read) io_uring_prep_read(sqe, std::forward<Args>(args)...);
        else if constexpr(Op == IoOps::Write) io_uring_prep_write(sqe, std::forward<Args>(args)...);
        else if constexpr(Op == IoOps::Sync) io_uring_prep_fsync(sqe, std::forward<Args>(args)...);
        else if constexpr(Op == IoOps::Close) io_uring_prep_close(sqe, std::forward<Args>(args)...);
        else if constexpr(Op == IoOps::Send) io_uring_prep_send(sqe, std::forward<Args>(args)...);
        else if constexpr(Op == IoOps::Recv) io_uring_prep_recv(sqe, std::forward<Args>(args)...);
        else if constexpr(Op == IoOps::SendMsg) io_uring_prep_sendmsg(sqe, std::forward<Args>(args)...);
        else if constexpr(Op == IoOps::RecvMsg) io_uring_prep_recvmsg(sqe, std::forward<Args>(args)...);
        else if constexpr(Op == IoOps::Accept) io_uring_prep_accept(sqe, std::forward<Args>(args)...);
        else if constexpr(Op == IoOps::Connect) io_uring_prep_connect(sqe, std::forward<Args>(args)...);
    }

    template<IoOps Op>
    void io_vec_pack_args(io_uring_sqe *sqe, int fd, msghdr *msg, unsigned flags) noexcept {
        if constexpr(Op == IoOps::SendMsg) io_uring_prep_sendmsg(sqe, fd, msg, flags);
        else if constexpr(Op == IoOps::RecvMsg) io_uring_prep_recvmsg(sqe, fd, msg, flags);
    }

    template<class Ret, IoOps Op, class ...Args>
    IOAwait<Ret> io_plain(Args &&... args) noexcept {
        std::lock_guard lk{IoRing::get()->lock()};
        return IOAwait<Ret>{
                [&, value = IoRing::get()](IOAwait<Ret> *ths) noexcept {
                    const auto sqe = value->get_sqe();
                    io_pack_args<Op>(sqe, std::forward<Args>(args)...);
                    io_uring_sqe_set_data(sqe, ths);
                    io_uring_submit(&value->ring());
                }
        };
    }

    template<IoOps Op>
    VecAwait io_message(int fd, const msghdr &msg, unsigned flags) noexcept {
        std::lock_guard lk{IoRing::get()->lock()};
        return VecAwait{
                [&, value = IoRing::get()](VecAwait *ths, msghdr *m) noexcept {
                    *m = msg;
                    const auto sqe = value->get_sqe();
                    io_vec_pack_args<Op>(sqe, fd, m, flags);
                    io_uring_sqe_set_data(sqe, ths);
                    io_uring_submit(&value->ring());
                }
        };
    }

    struct Uring : Handle<IoRing *> {
        static SafeHandle<Uring> get() noexcept;
    private:
        Uring();
    };
}