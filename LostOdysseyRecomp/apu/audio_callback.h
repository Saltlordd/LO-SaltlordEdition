#pragma once

#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <utility>

namespace apu::detail
{
    // One audio driver dispatches calls. Registration may change from any
    // guest thread, including from inside that driver's current callback.
    class AudioCallback
    {
        std::mutex m_mutex;
        std::condition_variable m_completed;
        uint32_t m_callback = 0;
        uint32_t m_param = 0;
        uint64_t m_started = 0;
        uint64_t m_finished = 0;
        std::thread::id m_dispatchThread;

        void Complete()
        {
            {
                std::lock_guard lock(m_mutex);
                m_finished = m_started;
                m_dispatchThread = {};
            }
            m_completed.notify_all();
        }

    public:
        void Register(uint32_t callback, uint32_t param)
        {
            std::lock_guard lock(m_mutex);
            m_callback = callback;
            m_param = param;
        }

        void Unregister()
        {
            std::unique_lock lock(m_mutex);
            m_callback = 0;
            m_param = 0;
            const auto pending = m_started;
            // A callback may unregister itself. It is already executing, so
            // only future dispatches need disabling; waiting for itself would
            // deadlock. Other callers wait for the call acquired before reset.
            if (m_dispatchThread == std::this_thread::get_id())
                return;
            m_completed.wait(lock, [&] { return m_finished >= pending; });
        }

        template <typename Invoke>
        bool Dispatch(Invoke&& invoke)
        {
            uint32_t callback, param;
            {
                std::lock_guard lock(m_mutex);
                if (!m_callback)
                    return false;
                assert(m_started == m_finished && "only one audio driver may dispatch");
                callback = m_callback;
                param = m_param;
                ++m_started;
                m_dispatchThread = std::this_thread::get_id();
            }
            struct Completion
            {
                AudioCallback& owner;
                ~Completion() { owner.Complete(); }
            } completion{ *this };
            std::forward<Invoke>(invoke)(callback, param);
            return true;
        }
    };
}
