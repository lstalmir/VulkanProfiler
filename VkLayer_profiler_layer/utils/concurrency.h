// Copyright (c) 2024 Lukasz Stalmirski
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
#include <mutex>
#include <shared_mutex>

template<typename Mutex = std::mutex>
class Lockable
{
public:
    void lock() const { m_Mutex.lock(); }
    void unlock() const { m_Mutex.unlock(); }
    bool try_lock() const { return m_Mutex.try_lock(); }

protected:
    mutable Mutex m_Mutex;
};

template<typename Mutex = std::shared_mutex>
class SharedLockable : public Lockable<Mutex>
{
public:
    void lock_shared() const { m_Mutex.lock_shared(); }
    void unlock_shared() const { m_Mutex.unlock_shared(); }
    bool try_lock_shared() const { return m_Mutex.try_lock_shared(); }
};
