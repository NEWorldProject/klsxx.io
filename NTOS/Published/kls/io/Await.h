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
#include "kls/io/Status.h"
#include "kls/hal/System.h"
#include "kls/coroutine/Trigger.h"

namespace kls::io::detail {
    class IOCP;

    Status map_error(DWORD sys) noexcept;
}

namespace kls::io {
    // on windows, non-IOCP io related operation is never async
    class Await : public AddressSensitive {
    public:
        template<class Fn>
        requires requires(Fn f) {{ f() } -> std::same_as<DWORD>; }
        Await(const Fn &fn) noexcept : m_result{fn()} {} //NOLINT

        [[nodiscard]] constexpr bool await_ready() const noexcept { return true; }

        [[nodiscard]] constexpr bool await_suspend(std::coroutine_handle<> h) { return false; }

        [[nodiscard]] Status await_resume() const noexcept { return detail::map_error(m_result); }
    private:
        DWORD m_result{};

        friend class kls::io::detail::IOCP;
    };

    template<class T>
    class IOAwait {
    public:
        template<class Fn>
        requires requires(Fn f, LPOVERLAPPED o) {{ f(o) } -> std::same_as<DWORD>; }
        IOAwait(const Fn &fn) noexcept { //NOLINT
            if (auto code = fn(&m_overlap); code != ERROR_IO_PENDING) {
                m_immediate_completion = true;
                setResult(code, 0);
            }
        }

        [[nodiscard]] bool await_ready() const noexcept { return m_immediate_completion; }

        [[nodiscard]] bool await_suspend(std::coroutine_handle<> h) {
            return (m_entry.set_handle(h), m_trigger.trap(m_entry));
        }

        [[nodiscard]] T await_resume() const noexcept {
            if constexpr (std::is_same_v<Status, T>) return detail::map_error(m_result);
            else if constexpr (std::is_same_v<IOResult, T>) return IOResult(detail::map_error(m_result), m_transferred);
            else static_assert("T is invalid");
        }
    private:
        OVERLAPPED m_overlap{};
        DWORD m_result{}, m_transferred{};
        bool m_immediate_completion{false};
        coroutine::ExecutorAwaitEntry m_entry{};
        coroutine::SingleExecutorTrigger m_trigger{};

        void release(DWORD code, DWORD transferred) noexcept { (setResult(code, transferred), m_trigger.pull()); }
        void setResult(DWORD code, DWORD transferred) noexcept { (m_transferred = transferred, m_result = code); }

        friend class kls::io::detail::IOCP;
    };
}