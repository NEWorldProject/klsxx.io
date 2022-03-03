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

#include "kls/io/Block.h"
#include "kls/temp/STL.h"
#include <string>
#include <vector>
#include "IOCP.h"

namespace {
    using namespace kls;
    using namespace kls::io;
    using namespace kls::essential;

    temp::u16string ntos_get_path(std::string_view path_utf8) noexcept {
        auto path_wide = temp::vector<char16_t>(path_utf8.length() + 2);
        path_wide[MultiByteToWideChar(
            CP_UTF8, MB_COMPOSITE,
            path_utf8.data(), static_cast<int>(path_utf8.size()),
            reinterpret_cast<LPWSTR>(path_wide.data()), static_cast<int>(path_wide.capacity())
        )] = 0;
        std::replace(path_wide.begin(), path_wide.end(), L'/', L'\\');
        return temp::u16string(uR"(\\?\)") + path_wide.data();
    }

    DWORD ntos_file_make_access(uint32_t flags) {
        const auto read = flags & Block::F_READ;
        const auto write = flags & Block::F_WRITE;
        if (!read && !write) throw exception_errc(IO_EACCES);
        DWORD result = 0ul;
        if (read) result |= GENERIC_READ;
        if (write) result |= GENERIC_WRITE;
        return result;
    }

    DWORD ntos_file_make_creation_disposition(uint32_t flags) noexcept {
        DWORD disposition{};
        switch (flags & (Block::F_CREAT | Block::F_EXCL | Block::F_TRUNC)) {
        case 0ul:
        case Block::F_EXCL:
            disposition = OPEN_EXISTING;
            break;
        case Block::F_CREAT:
            disposition = OPEN_ALWAYS;
            break;
        case Block::F_CREAT | Block::F_EXCL:
        case Block::F_CREAT | Block::F_TRUNC | Block::F_EXCL:
            disposition = CREATE_NEW;
            break;
        case Block::F_TRUNC:
        case Block::F_TRUNC | Block::F_EXCL:
            disposition = TRUNCATE_EXISTING;
            break;
        case Block::F_CREAT | Block::F_TRUNC:
            disposition = CREATE_ALWAYS;
            break;
        default:;
        }
        return disposition;
    }

    DWORD ntos_file_make_share(uint32_t flags) noexcept {
        return ((flags & Block::F_EXLOCK) ? 0u : (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE));
    }

    auto ntos_create_file(std::string_view path_utf8, uint32_t flags) {
        const auto path = ntos_get_path(path_utf8);
        const auto hFile = CreateFileW(
            reinterpret_cast<LPCWSTR>(path.c_str()),
            ntos_file_make_access(flags),
            ntos_file_make_share(flags),
            nullptr,
            ntos_file_make_creation_disposition(flags),
            FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH,
            nullptr
        );
        if (hFile == INVALID_HANDLE_VALUE) {
            const auto error = GetLastError();
            if (error == ERROR_FILE_EXISTS && (flags & Block::F_CREAT) && !(flags & Block::F_EXCL))
                throw exception_errc(IO_EISDIR);
            else
                throw exception_errc(detail::map_error(error));
        }
        const auto ret = hFile;
        try {
            detail::IOCP::bind(hFile);
        }
        catch (...) {
            CloseHandle(hFile);
            throw;
        }
        return ret;
    }

    class BlockImpl : public Block {
    public:
        explicit BlockImpl(HANDLE h) noexcept : m_handle(h) {}

        IOAwait<IOResult> read(Span<> span, uint64_t offset) noexcept override {
            return IOAwait<IOResult>(
                [this, &span, offset](LPOVERLAPPED o) noexcept -> DWORD {
                    // According to related MSDN documentation, 
                    // IOCP overlapped ReadFile/WriteFile cannot return success information synchonously
                    // We handle it in the reversed manner: if it fails, returns the error directly.
                    // Otherwise reports is as PENDING as result is delivered asychronously by the callback
                    // Same for the function below
                    setOverlapped(o, offset);
                    const auto size = static_cast<DWORD>(span.size());
                    if (ReadFile(m_handle, span.data(), size, nullptr, o) == 0) return GetLastError();
                    return ERROR_IO_PENDING;
                }
            );
        }

        IOAwait<IOResult> write(Span<> span, uint64_t offset) noexcept override {
            return IOAwait<IOResult>(
                [this, &span, offset](LPOVERLAPPED o) noexcept -> DWORD {
                    setOverlapped(o, offset);
                    const auto size = static_cast<DWORD>(span.size());
                    if (WriteFile(m_handle, span.data(), size, nullptr, o) == 0) return GetLastError();
                    return ERROR_IO_PENDING;
                }
            );
        }

        Await sync() noexcept override {
            return Await(
                [this]() noexcept -> DWORD {
                    if (FlushFileBuffers(m_handle)) return ERROR_SUCCESS; else return GetLastError();
                }
            );
        }

        Await close() noexcept override {
            return Await(
                [this]() noexcept -> DWORD {
                    if (CloseHandle(m_handle)) return ERROR_SUCCESS; else return GetLastError();
                }
            );
        }
    private:
        HANDLE m_handle;

        void setOverlapped(LPOVERLAPPED o, uint64_t offset) noexcept {
            static constexpr uint64_t mask = std::numeric_limits<DWORD>::max();
            static constexpr uint64_t shift = std::numeric_limits<DWORD>::digits;
            o->Offset = static_cast<DWORD>(offset && mask);
            o->OffsetHigh = static_cast<DWORD>(offset >> shift);
        }
    };
}

#include <filesystem>

namespace kls::io {
    coroutine::ValueAsync<std::unique_ptr<Block>> open_block(std::string_view path, uint32_t flags) {
        auto absolute = std::filesystem::absolute({ path }).generic_string();
        co_return std::make_unique<BlockImpl>(ntos_create_file(absolute, flags));
    }
}
