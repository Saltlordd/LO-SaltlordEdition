#pragma once

// Windows: installs last-resort SEH, std::terminate and SIGABRT reports.
// Android: reports fatal signals and std::terminate to the log, then passes
// the signal to the previous handler so the system tombstone is still written.
// Fast-fail, external termination and an unusable process/OS remain outside
// this best-effort diagnostic path. Call once during process initialization.
void InstallCrashHandler();
