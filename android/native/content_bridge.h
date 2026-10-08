#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include <functional>
using AndroidContentProgress=std::function<void(uint64_t,uint64_t,std::string_view)>;
using AndroidContentCancelled=std::function<bool()>;
std::string ApplyPendingAndroidContent(const AndroidContentProgress& progress={},const AndroidContentCancelled& cancelled={});
