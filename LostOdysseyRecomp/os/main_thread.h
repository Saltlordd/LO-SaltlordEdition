#pragma once

#include <functional>

// Host main-thread services. macOS (AppKit) accepts window and event work only
// on the process main thread. Other platforms have no such rule: there every
// call runs inline on the calling thread and behavior is unchanged.
namespace os::main_thread
{
// Runs body on a new thread with the main thread's stack size while the calling
// (main) thread serves Run() requests, calling idle between them. Returns
// body's result. Without a main-thread rule, runs body inline.
int RunServing(const std::function<int()>& body, const std::function<void()>& idle);

// Runs task on the main thread and waits for it to finish. Runs inline when
// already on the main thread or when no RunServing loop is active.
void Run(const std::function<void()>& task);
}
