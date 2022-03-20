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

#include <limits>
#include <utility>
#include <concepts>
#include <sys/socket.h>
#include "kls/io/Status.h"
#include "kls/coroutine/Trigger.h"

namespace kls::io::detail {
	class Uring;

	Status map_error(int32_t sys) noexcept;
    IOResult map_result(int32_t sys) noexcept;

    struct AwaitCore: private coroutine::SingleExecutorTrigger, private coroutine::ExecutorAwaitEntry {
        AwaitCore() noexcept = default;

        [[nodiscard]] constexpr bool await_ready() const noexcept { return false; }

        bool await_suspend(std::coroutine_handle<> h) {
            ExecutorAwaitEntry::set_handle(h);
            return SingleExecutorTrigger::trap(*this);
        }
    private:
        int32_t m_result{};

        void release(int32_t status) {
            m_result = status;
            SingleExecutorTrigger::pull();
        }

        friend class detail::Uring;

    protected:
        [[nodiscard]] auto get_result() const noexcept { return m_result; }
    };
}

namespace kls::io {
    template <class T>
    struct IOAwait : detail::AwaitCore {
        template <class Fn> requires std::is_invocable_v<Fn, IOAwait*>
        explicit IOAwait(Fn&& fn) noexcept: AwaitCore() { fn(this); }

        [[nodiscard]] T await_resume() const noexcept {
            if constexpr (std::is_same_v<Status, T>) return detail::map_error(get_result());
            else if constexpr (std::is_same_v<IOResult, T>) return detail::map_result(get_result());
            else static_assert("T is invalid");
        }
    };

    struct VecAwait : detail::AwaitCore {
        template <class Fn> requires std::is_invocable_v<Fn, VecAwait*, msghdr*>
        explicit VecAwait(Fn&& fn) noexcept: AwaitCore() { fn(this, &m_message); }

        [[nodiscard]] IOResult await_resume() const noexcept { return detail::map_result(get_result()); }
    private:
        msghdr m_message {};
    };
}
