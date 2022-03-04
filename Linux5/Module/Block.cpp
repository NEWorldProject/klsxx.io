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
#include <vector>
#include <fcntl.h>
#include <filesystem>
#include "kls/io/Block.h"

namespace {
    using namespace kls;
    using namespace kls::io;
    using namespace kls::io::detail;
    using namespace kls::essential;

    uint32_t flag_conv(uint32_t flags) {
        uint32_t result = 0;
        const auto read = flags & Block::Flag::F_READ;
        const auto write = flags & Block::Flag::F_WRITE;
        if (read && write)
            result |= O_RDWR;
        else {
            if (read) result |= O_RDONLY;
            if (write) result |= O_WRONLY;
            if (!read && !write) throw exception_errc(IO_EACCES);
        }
        if (flags & Block::Flag::F_CREAT) result |= O_CREAT;
        if (flags & Block::Flag::F_EXCL) result |= O_EXCL;
        if (flags & Block::Flag::F_TRUNC) result |= O_TRUNC;
        return result;
    }

    IOAwait<IOResult> open(Uring &core, const char *path, uint32_t flags, mode_t mode) {
        auto lk = core.lock();
        return core.create<IOResult, Uring::Open>(0, path, static_cast<int>(flags), mode);
    }

    class Impl final : public Block {
        template<Uring::Ops Op>
        IOAwait<IOResult> simple(Span<> span, uint64_t offset) noexcept {
            auto lk = m_core->lock();
            return m_core->create<IOResult, Op>(mFd, span.data(), span.size(), offset);
        }

    public:
        Impl(int fd, std::shared_ptr<Uring> &&core) noexcept: mFd{fd}, m_core{std::move(core)} {}

        IOAwait<IOResult> read(Span<> span, uint64_t offset) noexcept override {
            return simple<Uring::Read>(span, offset);
        }

        IOAwait<IOResult> write(Span<> span, uint64_t offset) noexcept override {
            return simple<Uring::Write>(span, offset);
        }

        IOAwait<Status> sync() noexcept override {
            auto lk = m_core->lock();
            return m_core->create<Status, Uring::Sync>(mFd, IORING_FSYNC_DATASYNC);
        }

        IOAwait<Status> close() noexcept override {
            auto lk = m_core->lock();
            return m_core->create<Status, Uring::Close>(mFd);
        }

    private:
        const int mFd;
        std::shared_ptr<Uring> m_core;
    };
}

namespace kls::io {
    coroutine::ValueAsync<std::unique_ptr<Block>> open_block(std::string_view path, uint32_t flags) {
        std::shared_ptr<Uring> core = Uring::get();
        const auto absolute = std::filesystem::absolute({path}).generic_string();
        if (const auto res = co_await open(*core, absolute.c_str(), flag_conv(flags), 00600); res.success())
            co_return std::make_unique<Impl>(res.result(), std::move(core));
        else
            throw exception_errc(res.error());
    }
}
