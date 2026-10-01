#include <apu/audio_callback.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <stdexcept>
#include <thread>

namespace
{
    void Check(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
}

int main()
{
    using namespace std::chrono_literals;
    apu::detail::AudioCallback client;
    std::atomic<uint32_t> calls = 0;
    const auto invoke = [&](uint32_t callback, uint32_t param)
    {
        Check(param == (callback ^ 0x5A5A5A5Au), "callback and parameter came from different registrations");
        ++calls;
    };
    Check(!client.Dispatch(invoke), "an unregistered client was invoked");

    // Replacing the registration must not replace an in-flight call's param.
    client.Register(1, 1 ^ 0x5A5A5A5Au);
    std::promise<void> entered, release;
    auto released = release.get_future().share();
    std::thread driver([&]
    {
        client.Dispatch([&](uint32_t callback, uint32_t param)
        {
            entered.set_value();
            released.wait();
            Check(callback == 1, "an acquired callback changed during registration");
            invoke(callback, param);
        });
    });
    entered.get_future().wait();
    client.Register(2, 2 ^ 0x5A5A5A5Au);
    release.set_value();
    driver.join();
    Check(client.Dispatch(invoke), "replacement registration was lost");

    // Unregister cannot return while a previously acquired call still runs,
    // even when a replacement was registered after that call was acquired.
    std::promise<void> blocked, unblock;
    auto unblocked = unblock.get_future().share();
    driver = std::thread([&]
    {
        client.Dispatch([&](uint32_t callback, uint32_t param)
        {
            blocked.set_value();
            unblocked.wait();
            invoke(callback, param);
        });
    });
    blocked.get_future().wait();
    client.Register(3, 3 ^ 0x5A5A5A5Au);
    std::promise<void> unregisterEntered;
    auto reset = std::async(std::launch::async, [&]
    {
        unregisterEntered.set_value();
        client.Unregister();
    });
    unregisterEntered.get_future().wait();
    const bool returnedEarly = reset.wait_for(50ms) == std::future_status::ready;
    unblock.set_value();
    driver.join();
    reset.get();
    Check(!returnedEarly, "unregister returned before an acquired callback completed");
    Check(!client.Dispatch(invoke), "unregister left a stale callback callable");

    // Self-unregister and self-replacement must not hold a lock across guest
    // code or wait for their own invocation to complete.
    client.Register(4, 4 ^ 0x5A5A5A5Au);
    Check(client.Dispatch([&](uint32_t callback, uint32_t param)
    {
        invoke(callback, param);
        client.Unregister();
    }), "self-unregister callback was not invoked");
    Check(!client.Dispatch(invoke), "self-unregister allowed another invocation");
    client.Register(5, 5 ^ 0x5A5A5A5Au);
    client.Dispatch([&](uint32_t callback, uint32_t param)
    {
        invoke(callback, param);
        client.Unregister();
        client.Register(6, 6 ^ 0x5A5A5A5Au);
    });
    Check(client.Dispatch(invoke), "self-replacement registration was lost");

    // Exceptions must release the in-flight lease as well.
    try
    {
        client.Dispatch([](uint32_t, uint32_t) { throw 42; });
        Check(false, "the dispatch exception was swallowed");
    }
    catch (int) {}
    client.Unregister();

    std::atomic<bool> stop = false;
    driver = std::thread([&]
    {
        while (!stop.load(std::memory_order_relaxed))
            client.Dispatch(invoke);
    });
    for (uint32_t i = 1; i <= 50000; ++i)
    {
        client.Register(i, i ^ 0x5A5A5A5Au);
        if (i % 13 == 0) client.Unregister();
        if (i % 31 == 0) std::this_thread::yield();
    }
    client.Unregister();
    const auto drainedCalls = calls.load();
    stop = true;
    driver.join();
    Check(calls == drainedCalls, "a callback started after unregister returned");
    std::printf("PASS audio callback: pairing, replacement, draining, self-unregister, exceptions, 50000 concurrent registrations (%u calls)\n", calls.load());
}
