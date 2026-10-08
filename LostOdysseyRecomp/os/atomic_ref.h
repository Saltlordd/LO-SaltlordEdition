#pragma once
#include <atomic>
#include <cassert>
#include <cstdint>
#include <type_traits>

#if !defined(__cpp_lib_atomic_ref)
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace os {
#if defined(__cpp_lib_atomic_ref)
template<class T> using AtomicRef = std::atomic_ref<T>;
#else
namespace detail {
// Sleepers per hashed futex word. A waiter counts itself before its final
// value check and the notifier stores before reading the count (both seq_cst),
// so notify_one may skip FUTEX_WAKE when the count is zero. Collisions only
// cost a spurious wake syscall.
inline std::atomic<uint32_t> futexWaiters[64]{};
inline std::atomic<uint32_t>& FutexWaiters(const void* address) {
    return futexWaiters[(reinterpret_cast<uintptr_t>(address) >> 2) & 63];
}
inline void SpinPause() {
#if defined(__aarch64__)
    asm volatile("yield");
#elif defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#endif
}
}
// The NDK's libc++ lacks atomic_ref. Guest locks occupy existing 32/64-bit
// words, so they cannot be replaced by separately constructed std::atomics.
template<class T> class AtomicRef {
    static_assert(std::is_integral_v<T> && (sizeof(T) == 4 || sizeof(T) == 8));
    static_assert(__atomic_always_lock_free(sizeof(T), nullptr));
    T* value_;

    static int Order(std::memory_order order) {
        switch (order) {
        case std::memory_order_relaxed: return __ATOMIC_RELAXED;
        case std::memory_order_consume: return __ATOMIC_CONSUME;
        case std::memory_order_acquire: return __ATOMIC_ACQUIRE;
        case std::memory_order_release: return __ATOMIC_RELEASE;
        case std::memory_order_acq_rel: return __ATOMIC_ACQ_REL;
        default: return __ATOMIC_SEQ_CST;
        }
    }
public:
    explicit AtomicRef(T& value) : value_(&value) {
        assert(reinterpret_cast<uintptr_t>(value_) % sizeof(T) == 0);
    }
    T load(std::memory_order order = std::memory_order_seq_cst) const {
        return __atomic_load_n(value_, Order(order));
    }
    void store(T value, std::memory_order order = std::memory_order_seq_cst) const {
        __atomic_store_n(value_, value, Order(order));
    }
    T operator=(T value) const { store(value); return value; }
    bool compare_exchange_weak(T& expected, T desired) const {
        return __atomic_compare_exchange_n(value_, &expected, desired, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    }
    bool compare_exchange_strong(T& expected, T desired) const {
        return __atomic_compare_exchange_n(value_, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    }
    void wait(T old) const {
        static_assert(sizeof(T) == 4, "guest wait words are 32-bit futexes");
        // Guest critical sections are short: spin briefly before sleeping.
        for (int spin = 0; spin < 64; ++spin) {
            if (load() != old) return;
            detail::SpinPause();
        }
        auto& waiters = detail::FutexWaiters(value_);
        waiters.fetch_add(1);
        while (load() == old)
            syscall(SYS_futex, value_, FUTEX_WAIT_PRIVATE, old, nullptr, nullptr, 0);
        waiters.fetch_sub(1);
    }
    // The caller's preceding store must be seq_cst (see detail::futexWaiters).
    void notify_one() const {
        static_assert(sizeof(T) == 4);
        if (detail::FutexWaiters(value_).load())
            syscall(SYS_futex, value_, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }
};
#endif
}
