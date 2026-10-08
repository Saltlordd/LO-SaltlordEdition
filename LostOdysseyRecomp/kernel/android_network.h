#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Xbox IPv4 socket ABI over Android/POSIX sockets. Sockaddr family and scalar
// option/ioctl words are big endian; port/address bytes retain network order.
namespace kernel::android_network {
constexpr uint32_t InvalidSocket = 0xffffffffu;
uint32_t Startup(uint32_t version, uint8_t* data);
int32_t Cleanup();
uint32_t LastError();
void SetLastError(uint32_t error);
uint32_t Socket(uint32_t family, uint32_t type, uint32_t protocol);
int32_t Close(uint32_t socket);
int32_t Bind(uint32_t socket, const uint8_t* address, uint32_t length);
int32_t Connect(uint32_t socket, const uint8_t* address, uint32_t length);
int32_t GetName(uint32_t socket, uint8_t* address, uint8_t* length);
int32_t SetOption(uint32_t socket, uint32_t level, uint32_t option, const uint8_t* value, uint32_t length);
int32_t GetOption(uint32_t socket, uint32_t level, uint32_t option, uint8_t* value, uint8_t* length);
int32_t Ioctl(uint32_t socket, uint32_t command, uint8_t* value);
int32_t Send(uint32_t socket, const void* data, uint32_t length, uint32_t flags);
int32_t Receive(uint32_t socket, void* data, uint32_t length, uint32_t flags);
int32_t SendTo(uint32_t socket, const void* data, uint32_t length, uint32_t flags, const uint8_t* address, uint32_t addressLength);
int32_t ReceiveFrom(uint32_t socket, void* data, uint32_t length, uint32_t flags, uint8_t* address, uint8_t* addressLength);
uint32_t InetAddress(const char* text);
// Addresses are Xbox numeric/network-order values, ready for be<uint32_t>.
uint32_t Resolve(const char* host, std::vector<uint32_t>& addresses);
}
