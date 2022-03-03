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

#include <filesystem>
#include <gtest/gtest.h>
#include "kls/io/Block.h"
#include "kls/coroutine/Blocking.h"
#include "kls/coroutine/Coroutine.h"

TEST(kls_io, FileEcho) {
    using namespace kls::io;
    using namespace kls::essential;
    using namespace kls::coroutine;

    auto Write = []() -> ValueAsync<void> {
        auto file = co_await open_block("./test.kls.io.file.temp", Block::F_WRITE | Block::F_CREAT);
        char buffer[13] = "Hello World\n";
        auto result = co_await file->write({ buffer, 13 }, 0);
        if (result.success()) {
            printf("num %d\n", result.result());
        }
        else {
            printf("err %d\n", result.error());
        }
        co_await file->close();
    };

    auto Read = []() -> ValueAsync<void> {
        auto file = co_await open_block("./test.kls.io.file.temp", Block::F_READ);
        char buffer[1000];
        auto result = co_await file->read({ buffer, 1000 }, 0);
        if (result.success()) {
            puts(buffer);
        }
        else {
            printf("err %d\n", result.error());
        }
        co_await file->close();
    };

    auto Network = [&]() -> ValueAsync<void> {
        try {
            co_await Write();
            co_await Read();
            std::filesystem::remove_all("./test.kls.io.file.temp");
        }
        catch (std::exception& e) {
            puts(e.what());
        }
    };

    Blocking().Await(Network());
}