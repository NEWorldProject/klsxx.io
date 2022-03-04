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
#include "kls/io/Await.h"
#include "kls/thread/SpinLock.h"
#include "kls/essential/Memory.h"

namespace kls::io::detail {
    class Uring {
        static constexpr int QUEUE_DEPTH = 8192;
    public:
        enum Ops {
            Open, Read, Write, Sync, Close, Send, Recv, SendMsg, RecvMsg, Accept, Connect
        };

        Uring() {
            io_uring_queue_init(QUEUE_DEPTH, &m_ring, 0);
            std::thread([this]() { while (wait_one()); }).detach();
        }

        ~Uring() { io_uring_queue_exit(&m_ring); }

        template<class Ret, Ops Op, class ...Args>
        IOAwait<Ret> create(Args &&... args) noexcept {
            auto *sqe = get_sqe();
            if constexpr(Op == Ops::Open) io_uring_prep_openat(sqe, std::forward<Args>(args)...);
            else if constexpr(Op == Ops::Read) io_uring_prep_read(sqe, std::forward<Args>(args)...);
            else if constexpr(Op == Ops::Write) io_uring_prep_write(sqe, std::forward<Args>(args)...);
            else if constexpr(Op == Ops::Sync) io_uring_prep_fsync(sqe, std::forward<Args>(args)...);
            else if constexpr(Op == Ops::Close) io_uring_prep_close(sqe, std::forward<Args>(args)...);
            else if constexpr(Op == Ops::Send) io_uring_prep_send(sqe, std::forward<Args>(args)...);
            else if constexpr(Op == Ops::Recv) io_uring_prep_recv(sqe, std::forward<Args>(args)...);
            else if constexpr(Op == Ops::SendMsg) io_uring_prep_sendmsg(sqe, std::forward<Args>(args)...);
            else if constexpr(Op == Ops::RecvMsg) io_uring_prep_recvmsg(sqe, std::forward<Args>(args)...);
            else if constexpr(Op == Ops::Accept) io_uring_prep_accept(sqe, std::forward<Args>(args)...);
            else if constexpr(Op == Ops::Connect) io_uring_prep_connect(sqe, std::forward<Args>(args)...);
            return IOAwait<Ret>{
                    [this, sqe](IOAwait<Ret> *ths) noexcept {
                        io_uring_sqe_set_data(sqe, ths);
                        io_uring_submit(&m_ring);
                    }
            };
        }

        template<Ops Op>
        VecAwait create_vec(int fd, const msghdr& msg, unsigned flags) noexcept {
            auto *sqe = get_sqe();
            return VecAwait{
                    [this, sqe, fd, &msg, flags](VecAwait *ths, msghdr* m) noexcept {
                        *m = msg;
                        if constexpr(Op == Ops::SendMsg) io_uring_prep_sendmsg(sqe, fd, m, flags);
                        else if constexpr(Op == Ops::RecvMsg) io_uring_prep_recvmsg(sqe, fd, m, flags);
                        io_uring_sqe_set_data(sqe, ths);
                        io_uring_submit(&m_ring);
                    }
            };
        }

        static std::shared_ptr<Uring> get() {
            static auto ins = std::make_shared<Uring>();
            return ins;
        }

        auto lock() noexcept { return std::lock_guard<thread::SpinLock>{m_lock}; }

    private:
        io_uring m_ring{};
        // We need the lock as we are not able to gather wait CQEs
        // Which made it not practical to use multiple rings to submit.
        // Since the ring itself is not constructed with thread safe ring,
        //     we need a large lock around the ring for safe submission
        thread::SpinLock m_lock{};

        bool wait_one() {
            io_uring_cqe *cqe{};
            if (const auto ret = io_uring_wait_cqe(&m_ring, &cqe); ret == 0) {
                static_cast<AwaitCore *>(io_uring_cqe_get_data(cqe))->release(cqe->res);
                io_uring_cqe_seen(&m_ring, cqe);
                return true;
            } else return (ret != ENXIO); // break if instance shutdown
        }

        io_uring_sqe *get_sqe() noexcept {
            thread::SpinWait spin{};
            for (;;) if (const auto sqe = io_uring_get_sqe(&m_ring); sqe) return sqe; else spin.once();
        }
    };
}