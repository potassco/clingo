#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define GRINGO_PLATFORM_WINDOWS 1
#else
#define GRINGO_PLATFORM_WINDOWS 0
#endif

#if !GRINGO_PLATFORM_WINDOWS && (defined(__unix__) || defined(__APPLE__))
#define GRINGO_PLATFORM_POSIX 1
#else
#define GRINGO_PLATFORM_POSIX 0
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if GRINGO_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#elif GRINGO_PLATFORM_POSIX
#include <pthread.h>
#else
#include <shared_mutex>
#endif

namespace Gringo {

#if GRINGO_PLATFORM_POSIX
namespace Detail {

inline void check_pthread_call(char const *fun, int code) noexcept {
    if (code != 0) {
        std::fprintf(stderr, "fatal: %s failed: %s\n", fun, std::strerror(code));
        std::abort();
    }
}

} // namespace Detail
#endif

class SharedMutex {
  public:
    SharedMutex() noexcept {
#if GRINGO_PLATFORM_WINDOWS
        InitializeSRWLock(&srw_);
#elif GRINGO_PLATFORM_POSIX
        Detail::check_pthread_call("pthread_rwlock_init", pthread_rwlock_init(&rw_, nullptr));
#endif
    }

    ~SharedMutex() noexcept {
#if GRINGO_PLATFORM_POSIX
        Detail::check_pthread_call("pthread_rwlock_destroy", pthread_rwlock_destroy(&rw_));
#endif
    }

    SharedMutex(const SharedMutex &) = delete;
    SharedMutex &operator=(const SharedMutex &) = delete;
    SharedMutex(SharedMutex &&) = delete;
    SharedMutex &operator=(SharedMutex &&) = delete;

    void lock_shared() noexcept {
#if GRINGO_PLATFORM_WINDOWS
        AcquireSRWLockShared(&srw_);
#elif GRINGO_PLATFORM_POSIX
        Detail::check_pthread_call("pthread_rwlock_rdlock", pthread_rwlock_rdlock(&rw_));
#else
        mtx_.lock_shared();
#endif
    }

    bool try_lock_shared() noexcept {
#if GRINGO_PLATFORM_WINDOWS
        return TryAcquireSRWLockShared(&srw_) != 0;
#elif GRINGO_PLATFORM_POSIX
        return pthread_rwlock_tryrdlock(&rw_) == 0;
#else
        return mtx_.try_lock_shared();
#endif
    }

    void unlock_shared() noexcept {
#if GRINGO_PLATFORM_WINDOWS
        ReleaseSRWLockShared(&srw_);
#elif GRINGO_PLATFORM_POSIX
        Detail::check_pthread_call("pthread_rwlock_unlock", pthread_rwlock_unlock(&rw_));
#else
        mtx_.unlock_shared();
#endif
    }

    void lock() noexcept {
#if GRINGO_PLATFORM_WINDOWS
        AcquireSRWLockExclusive(&srw_);
#elif GRINGO_PLATFORM_POSIX
        Detail::check_pthread_call("pthread_rwlock_wrlock", pthread_rwlock_wrlock(&rw_));
#else
        mtx_.lock();
#endif
    }

    bool try_lock() noexcept {
#if GRINGO_PLATFORM_WINDOWS
        return TryAcquireSRWLockExclusive(&srw_) != 0;
#elif GRINGO_PLATFORM_POSIX
        return pthread_rwlock_trywrlock(&rw_) == 0;
#else
        return mtx_.try_lock();
#endif
    }

    void unlock() noexcept {
#if GRINGO_PLATFORM_WINDOWS
        ReleaseSRWLockExclusive(&srw_);
#elif GRINGO_PLATFORM_POSIX
        Detail::check_pthread_call("pthread_rwlock_unlock", pthread_rwlock_unlock(&rw_));
#else
        mtx_.unlock();
#endif
    }

  private:
#if GRINGO_PLATFORM_WINDOWS
    SRWLOCK srw_;
#elif GRINGO_PLATFORM_POSIX
    pthread_rwlock_t rw_;
#else
    std::shared_timed_mutex mtx_;
#endif
};

} // namespace Gringo
