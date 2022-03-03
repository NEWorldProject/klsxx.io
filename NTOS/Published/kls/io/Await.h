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
#include <limits>
#include <utility>
#include <concepts>
#include <coroutine>
#include "kls/io/Status.h"
#include "kls/hal/System.h"
#include "kls/coroutine/Executor.h"

namespace kls::io::detail {
	class IOCP;

	Status map_error(DWORD sys) noexcept;
}

namespace kls::io {
	// on windows, non-IOCP io related operation is never async
	class Await {
	public:
		template<class Fn> requires requires(Fn f) { { f() } -> std::same_as<DWORD>; }
		Await(const Fn& fn) noexcept : m_result{ fn() } {}

		// Await-related classes are not supposed to be copied nor moved
		Await(Await&&) = delete;

		Await(const Await&) = delete;

		Await& operator=(Await&&) = delete;

		Await& operator=(const Await&) = delete;

		[[nodiscard]] constexpr bool await_ready() const noexcept { return true; }

		[[nodiscard]] bool await_suspend(std::coroutine_handle<> h) { return false; }

		Status await_resume() const noexcept { return detail::map_error(m_result); }
	private:
		DWORD m_result{};

		friend class kls::io::detail::IOCP;
	};

	template <class T>
	class IOAwait {
		static constexpr uintptr_t INVALID_PTR = std::numeric_limits<uintptr_t>::max();
	public:
		template<class Fn> requires requires(Fn f, LPOVERLAPPED o) { { f(o) } -> std::same_as<DWORD>; }
		IOAwait(const Fn& fn) noexcept {
			if (auto code = fn(&m_overlap); code != ERROR_IO_PENDING) {
				m_immediate_completion = true;
				setResult(code, 0);
			}
		}

		// Await-related classes are not supposed to be copied nor moved
		IOAwait(IOAwait&&) = delete;

		IOAwait(const IOAwait&) = delete;

		IOAwait& operator=(IOAwait&&) = delete;

		IOAwait& operator=(const IOAwait&) = delete;

		[[nodiscard]] bool await_ready() const noexcept { return m_immediate_completion; }

		[[nodiscard]] bool await_suspend(std::coroutine_handle<> h) {
			m_resume = coroutine::CurrentExecutor();
			for (;;) {
				auto handle = m_handle.load();
				// if the state has been finalized, direct dispatch
				if (handle == INVALID_PTR) return false;
				if (handle) std::abort();
				// try to setup dispatch
				if (m_handle.compare_exchange_weak(handle, reinterpret_cast<uintptr_t>(h.address()))) return true;
			}
		}

		T await_resume() const noexcept {
			if constexpr (std::is_same_v<Status, T>) {
				return detail::map_error(m_result);
			}
			else if constexpr (std::is_same_v<IOResult, T>) {
				return IOResult(detail::map_error(m_result), m_transferred);
			}
			else static_assert("T is invalid");
		}
	private:
		OVERLAPPED m_overlap{};
		DWORD m_result{}, m_transferred{};
		bool m_immediate_completion{ false };
		std::atomic<uintptr_t> m_handle{ 0 };
		kls::coroutine::IExecutor* m_resume{ nullptr };

		void release(DWORD code, DWORD transferred) noexcept {
			setResult(code, transferred);
			if (auto ths = m_handle.exchange(INVALID_PTR); ths) {
				auto handle = std::coroutine_handle<>::from_address(reinterpret_cast<void*>(ths));
				if (m_resume) m_resume->Enqueue(handle); else handle.resume();
			}
		}

		void setResult(DWORD code, DWORD transferred) noexcept {
			m_transferred = transferred;
			m_result = code;
		}

		friend class kls::io::detail::IOCP;
	};
}