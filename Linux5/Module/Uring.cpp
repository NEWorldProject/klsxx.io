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

#include "Uring.h"

namespace kls::io::detail {
    static Storage<IoRing> gIoRing;

    IoRing::IoRing() {
        io_uring_queue_init(QUEUE_DEPTH, &m_ring, 0);
        std::thread([this]() { while (wait_one()); }).detach();
    }

    IoRing::~IoRing() { io_uring_queue_exit(&m_ring); }

    IoRing *IoRing::get() noexcept { return &gIoRing.value; }

    io_uring_sqe *IoRing::get_sqe() noexcept {
        thread::SpinWait spin{};
        for (;;) if (const auto sqe = io_uring_get_sqe(&m_ring); sqe) return sqe; else spin.once();
    }

    bool IoRing::wait_one() {
        io_uring_cqe *cqe{};
        if (const auto ret = io_uring_wait_cqe(&m_ring, &cqe); ret == 0) {
            static_cast<AwaitCore *>(io_uring_cqe_get_data(cqe))->release(cqe->res);
            io_uring_cqe_seen(&m_ring, cqe);
            return true;
        } else return (ret != ENXIO); // break if instance shutdown
    }

    SafeHandle<Uring> Uring::get() noexcept {
        static SafeHandle<Uring> instance{Uring{}};
        return instance;
    }

    Uring::Uring() : Handle<IoRing *>([](auto p) noexcept { std::destroy_at(p); }, IoRing::get()) {
        std::construct_at(value());
    }
}