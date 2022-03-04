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

#include <cstdint>
#include <string_view>
#include "Await.h"
#include "kls/Object.h"
#include "kls/essential/Memory.h"
#include "kls/coroutine/ValueAsync.h"

namespace kls::io {
	struct Block: PmrBase {
        enum Flag {
            F_READ = 1ul,
            F_WRITE = 2ul,
            F_CREAT = 4ul,
            F_EXCL = 8ul,
            F_TRUNC = 16ul,
            F_EXLOCK = 32ul
        };

		virtual IOAwait<IOResult> read(essential::Span<> span, uint64_t offset) noexcept = 0;
		virtual IOAwait<IOResult> write(essential::Span<> span, uint64_t offset) noexcept = 0;
        virtual IOAwait<Status> sync() noexcept = 0;
        virtual IOAwait<Status> close() noexcept = 0;
	};

	coroutine::ValueAsync<std::unique_ptr<Block>> open_block(std::string_view path, uint32_t flags);
}