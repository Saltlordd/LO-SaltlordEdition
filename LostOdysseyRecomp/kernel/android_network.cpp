#include "android_network.h"
#if defined(__ANDROID__) || defined(LO_ANDROID_NETWORK_TESTING)
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace kernel::android_network {
namespace {
thread_local uint32_t lastError = 0;
struct OwnedSocket {
    int fd;
    bool datagram;
    OwnedSocket(int value, bool udp) : fd(value), datagram(udp) {}
    ~OwnedSocket() { ::close(fd); }
};
std::mutex socketsMutex;
std::unordered_map<uint32_t, std::shared_ptr<OwnedSocket>> sockets;
uint32_t nextHandle = 0x40000000;
uint32_t startupReferences = 0;
uint32_t Read32(const uint8_t* p) { return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3]; }
void Write32(uint8_t* p, uint32_t value) { for (int i = 3; i >= 0; --i) { p[i] = uint8_t(value); value >>= 8; } }
uint32_t SocketError(int error) {
    switch (error) {
    case EINTR: return 10004; case EACCES: case EPERM: return 10013;
    case EFAULT: return 10014; case EINVAL: return 10022; case EMFILE: case ENFILE: return 10024;
    case EAGAIN: return 10035; case EINPROGRESS: return 10036; case EALREADY: return 10037;
    case EBADF: case ENOTSOCK: return 10038; case EDESTADDRREQ: return 10039;
    case EMSGSIZE: return 10040; case EPROTOTYPE: return 10041; case ENOPROTOOPT: return 10042;
    case EPROTONOSUPPORT: return 10043; case EOPNOTSUPP: return 10045; case EAFNOSUPPORT: return 10047;
    case EADDRINUSE: return 10048; case EADDRNOTAVAIL: return 10049; case ENETDOWN: return 10050;
    case ENETUNREACH: return 10051; case ENETRESET: return 10052; case ECONNABORTED: return 10053;
    case ECONNRESET: return 10054; case ENOBUFS: case ENOMEM: return 10055;
    case EISCONN: return 10056; case ENOTCONN: return 10057; case EPIPE: return 10058;
    case ETIMEDOUT: return 10060; case ECONNREFUSED: return 10061; case EHOSTUNREACH: return 10065;
    default: return 10022;
    }
}
int32_t Fail(uint32_t error) { lastError = error; return -1; }
int32_t Result(ssize_t value) { return value < 0 ? Fail(SocketError(errno)) : int32_t(value); }
std::shared_ptr<OwnedSocket> Find(uint32_t handle) {
    std::lock_guard lock(socketsMutex);
    if (!startupReferences) { lastError = 10093; return {}; }
    auto it = sockets.find(handle);
    if (it == sockets.end()) { lastError = 10038; return {}; }
    return it->second;
}
bool DecodeAddress(const uint8_t* bytes, uint32_t length, sockaddr_in& address) {
    if (!bytes || length < 16) { Fail(10014); return false; }
    if ((uint32_t(bytes[0]) << 8 | bytes[1]) != 2) { Fail(10047); return false; }
    address = {}; address.sin_family = AF_INET;
    std::memcpy(&address.sin_port, bytes + 2, 2);
    std::memcpy(&address.sin_addr, bytes + 4, 4);
    return true;
}
void EncodeAddress(uint8_t* bytes, const sockaddr_in& address) {
    std::memset(bytes, 0, 16); bytes[1] = 2;
    std::memcpy(bytes + 2, &address.sin_port, 2);
    std::memcpy(bytes + 4, &address.sin_addr, 4);
}
bool Flags(uint32_t guest, int& native, bool sending) {
    // Only the implemented Winsock flags are exposed: OOB/DONTROUTE for
    // sends, OOB/PEEK for receives. WAITALL/PARTIAL fail explicitly.
    if (guest & ~uint32_t(sending ? (1 | 4) : (1 | 2))) { Fail(10045); return false; }
    native = (guest & 1 ? MSG_OOB : 0) | (guest & 2 ? MSG_PEEK : 0) | (guest & 4 ? MSG_DONTROUTE : 0);
    if (sending) native |= MSG_NOSIGNAL;
    return true;
}
bool Option(uint32_t guestLevel, uint32_t guestOption, int& level, int& option, bool& timeout) {
    timeout = false;
    if (guestLevel == 6 && guestOption == 1) { level = IPPROTO_TCP; option = TCP_NODELAY; return true; }
    if (guestLevel != 0xffff) { Fail(10042); return false; }
    level = SOL_SOCKET;
    switch (guestOption) {
    case 4: option = SO_REUSEADDR; break; case 8: option = SO_KEEPALIVE; break;
    case 0x20: option = SO_BROADCAST; break;
    case 0x1001: option = SO_SNDBUF; break; case 0x1002: option = SO_RCVBUF; break;
    case 0x1005: option = SO_SNDTIMEO; timeout = true; break;
    case 0x1006: option = SO_RCVTIMEO; timeout = true; break;
    case 0x1007: option = SO_ERROR; break; case 0x1008: option = SO_TYPE; break;
    default: Fail(10042); return false;
    }
    return true;
}
}
uint32_t Startup(uint32_t version, uint8_t* data) {
    if ((version & 0xff) != 2) return 10092;
    if (!data) return 10014;
    // X_WSADATA: two 16-bit versions, 257-byte description, 129-byte status,
    // two 16-bit limits, padding and vendor pointer. Preserve vendor pointer.
    const uint16_t negotiated = uint16_t((std::min(version >> 8, 2u) << 8) | 2);
    data[0] = uint8_t(negotiated >> 8); data[1] = uint8_t(negotiated);
    data[2] = 2; data[3] = 2;
    std::memset(data + 4, 0, 390);
    std::memcpy(data + 4, "Android IPv4 sockets", 20);
    data[390] = 0; data[391] = 64; data[392] = 0xff; data[393] = 0xe3;
    std::lock_guard lock(socketsMutex); ++startupReferences; return 0;
}
int32_t Cleanup() {
    std::unordered_map<uint32_t, std::shared_ptr<OwnedSocket>> retired;
    { std::lock_guard lock(socketsMutex);
      if (!startupReferences) return Fail(10093);
      if (--startupReferences == 0) retired.swap(sockets); }
    for (const auto& [handle, socket] : retired) ::shutdown(socket->fd, SHUT_RDWR);
    return 0;
}
uint32_t LastError() { return lastError; }
void SetLastError(uint32_t error) { lastError = error; }
uint32_t Socket(uint32_t family, uint32_t type, uint32_t protocol) {
    if (family != 2) return uint32_t(Fail(10047));
    if (type != 1 && type != 2) return uint32_t(Fail(10044));
    if (protocol != 0 && protocol != 6 && protocol != 17) return uint32_t(Fail(10043));
    std::lock_guard lock(socketsMutex);
    if (!startupReferences) return uint32_t(Fail(10093));
    const int fd = ::socket(AF_INET, type == 1 ? SOCK_STREAM : SOCK_DGRAM, int(protocol));
    if (fd < 0) return uint32_t(Fail(SocketError(errno)));
    auto owned = std::make_shared<OwnedSocket>(fd, type == 2);
    while (!nextHandle || nextHandle == InvalidSocket || sockets.contains(nextHandle)) ++nextHandle;
    const uint32_t handle = nextHandle++;
    sockets.emplace(handle, std::move(owned)); return handle;
}
int32_t Close(uint32_t handle) {
    std::shared_ptr<OwnedSocket> socket;
    { std::lock_guard lock(socketsMutex);
      if (!startupReferences) return Fail(10093);
      auto it = sockets.find(handle);
      if (it == sockets.end()) return Fail(10038);
      socket = std::move(it->second); sockets.erase(it); }
    // Wake a concurrent blocking operation; its shared ownership prevents fd
    // reuse until that operation has returned.
    ::shutdown(socket->fd, SHUT_RDWR); return 0;
}
int32_t Bind(uint32_t handle, const uint8_t* bytes, uint32_t length) {
    auto socket = Find(handle); if (!socket) return -1;
    sockaddr_in address{}; if (!DecodeAddress(bytes, length, address)) return -1;
    return Result(::bind(socket->fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)));
}
int32_t Connect(uint32_t handle, const uint8_t* bytes, uint32_t length) {
    auto socket = Find(handle); if (!socket) return -1;
    sockaddr_in address{}; if (!DecodeAddress(bytes, length, address)) return -1;
    return Result(::connect(socket->fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)));
}
int32_t GetName(uint32_t handle, uint8_t* bytes, uint8_t* length) {
    auto socket = Find(handle); if (!socket) return -1;
    if (!bytes || !length || Read32(length) < 16) return Fail(10014);
    sockaddr_in address{}; socklen_t nativeLength = sizeof(address);
    if (::getsockname(socket->fd, reinterpret_cast<sockaddr*>(&address), &nativeLength) < 0) return Fail(SocketError(errno));
    EncodeAddress(bytes, address); Write32(length, 16); return 0;
}
int32_t SetOption(uint32_t handle, uint32_t guestLevel, uint32_t guestOption, const uint8_t* bytes, uint32_t length) {
    auto socket = Find(handle); if (!socket) return -1;
    if (!bytes || length != 4) return Fail(10014);
    int level, option; bool timeout; if (!Option(guestLevel, guestOption, level, option, timeout)) return -1;
    const uint32_t value = Read32(bytes);
    if (timeout) { timeval time{long(value / 1000), long(value % 1000) * 1000};
        return Result(::setsockopt(socket->fd, level, option, &time, sizeof(time))); }
    int native = int32_t(value); return Result(::setsockopt(socket->fd, level, option, &native, sizeof(native)));
}
int32_t GetOption(uint32_t handle, uint32_t guestLevel, uint32_t guestOption, uint8_t* bytes, uint8_t* length) {
    auto socket = Find(handle); if (!socket) return -1;
    if (!bytes || !length || Read32(length) < 4) return Fail(10014);
    int level, option; bool timeout; if (!Option(guestLevel, guestOption, level, option, timeout)) return -1;
    uint32_t value;
    if (timeout) { timeval time{}; socklen_t size = sizeof(time);
        if (::getsockopt(socket->fd, level, option, &time, &size) < 0) return Fail(SocketError(errno));
        value = uint32_t(time.tv_sec * 1000 + time.tv_usec / 1000);
    } else { int native = 0; socklen_t size = sizeof(native);
        if (::getsockopt(socket->fd, level, option, &native, &size) < 0) return Fail(SocketError(errno));
        value = option == SO_ERROR && native ? SocketError(native) : uint32_t(native);
    }
    Write32(bytes, value); Write32(length, 4); return 0;
}
int32_t Ioctl(uint32_t handle, uint32_t command, uint8_t* bytes) {
    auto socket = Find(handle); if (!socket) return -1;
    if (!bytes) return Fail(10014);
    if (command != 0x8004667e && command != 0x4004667f) return Fail(10045);
    int value = int32_t(Read32(bytes));
    if (::ioctl(socket->fd, command == 0x8004667e ? FIONBIO : FIONREAD, &value) < 0) return Fail(SocketError(errno));
    if (command == 0x4004667f) Write32(bytes, uint32_t(value));
    return 0;
}
int32_t Send(uint32_t handle, const void* bytes, uint32_t length, uint32_t guestFlags) {
    auto socket = Find(handle); if (!socket) return -1;
    if ((!bytes && length) || length > INT_MAX) return Fail(10014);
    int flags; if (!Flags(guestFlags, flags, true)) return -1;
    return Result(::send(socket->fd, bytes, length, flags));
}
int32_t Receive(uint32_t handle, void* bytes, uint32_t length, uint32_t guestFlags) {
    return ReceiveFrom(handle, bytes, length, guestFlags, nullptr, nullptr);
}
int32_t SendTo(uint32_t handle, const void* bytes, uint32_t length, uint32_t guestFlags, const uint8_t* destination, uint32_t destinationLength) {
    auto socket = Find(handle); if (!socket) return -1;
    if ((!bytes && length) || length > INT_MAX) return Fail(10014);
    int flags; if (!Flags(guestFlags, flags, true)) return -1;
    sockaddr_in address{}; if (!DecodeAddress(destination, destinationLength, address)) return -1;
    return Result(::sendto(socket->fd, bytes, length, flags, reinterpret_cast<sockaddr*>(&address), sizeof(address)));
}
int32_t ReceiveFrom(uint32_t handle, void* bytes, uint32_t length, uint32_t guestFlags, uint8_t* source, uint8_t* sourceLength) {
    auto socket = Find(handle); if (!socket) return -1;
    if ((!bytes && length) || length > INT_MAX) return Fail(10014);
    if (source && (!sourceLength || Read32(sourceLength) < 16)) return Fail(10014);
    int flags; if (!Flags(guestFlags, flags, false)) return -1;
    sockaddr_in address{}; socklen_t nativeLength = sizeof(address);
    const auto result = ::recvfrom(socket->fd, bytes, length, flags | (socket->datagram ? MSG_TRUNC : 0),
        source ? reinterpret_cast<sockaddr*>(&address) : nullptr, source ? &nativeLength : nullptr);
    if (result < 0) return Fail(SocketError(errno));
    if (uint64_t(result) > length) return Fail(10040); // WSAEMSGSIZE for truncated datagrams.
    if (source) { EncodeAddress(source, address); Write32(sourceLength, 16); }
    return int32_t(result);
}
uint32_t InetAddress(const char* text) {
    if (!text) return InvalidSocket;
    if (!*text) return 0; // Console-compatible empty-string behavior.
    in_addr address{}; address.s_addr = ::inet_addr(text);
    return ntohl(address.s_addr);
}
uint32_t Resolve(const char* host, std::vector<uint32_t>& addresses) {
    addresses.clear();
    if (!host || !*host) return 10022;
    addrinfo hints{}; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
    addrinfo* list = nullptr; const int error = ::getaddrinfo(host, nullptr, &hints, &list);
    if (error) return error == EAI_AGAIN ? 11002 : error == EAI_MEMORY ? 10055 : error == EAI_SYSTEM ? SocketError(errno) : 11001;
    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> release(list, freeaddrinfo);
    for (auto* item = list; item && addresses.size() < 8; item = item->ai_next) {
        const auto value = ntohl(reinterpret_cast<sockaddr_in*>(item->ai_addr)->sin_addr.s_addr);
        if (std::find(addresses.begin(), addresses.end(), value) == addresses.end()) addresses.push_back(value);
    }
    return addresses.empty() ? 11004 : 0;
}
}
#endif
