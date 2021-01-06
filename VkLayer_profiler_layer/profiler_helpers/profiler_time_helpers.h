// Copyright (c) 2020 Lukasz Stalmirski
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <chrono>

namespace Profiler
{
    // Standard library defines these with long long storage, so we lose precision when
    // storing nanosecond duration as milliseconds (to avoid unnecessary conversions in
    // the runtime). Redefine common time units with floating-point storage to keep the
    // precision.

    using Nanoseconds = std::chrono::duration<float, std::nano>;
    using Microseconds = std::chrono::duration<float, std::micro>;
    using Milliseconds = std::chrono::duration<float, std::milli>;
    using Seconds = std::chrono::duration<float>;
    using Minutes = std::chrono::duration<float, std::ratio<60>>;
    using Hours = std::chrono::duration<float, std::ratio<3600>>;
}
