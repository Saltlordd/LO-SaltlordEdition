#include <kernel/android_network.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <thread>
using namespace kernel::android_network;
static void Check(bool value, const char* name) { if (!value) { std::fprintf(stderr, "FAIL %s (WSA error %u)\n", name, LastError()); std::abort(); } }
static void Word(uint8_t* p, uint32_t v) { for (int i = 3; i >= 0; --i) { p[i] = uint8_t(v); v >>= 8; } }
static uint32_t Value(const uint8_t* p) { return uint32_t(p[0])<<24 | uint32_t(p[1])<<16 | uint32_t(p[2])<<8 | p[3]; }
static std::array<uint8_t,16> Address(uint16_t port=0) { return {0,2,uint8_t(port>>8),uint8_t(port),127,0,0,1}; }
int main() {
    std::array<uint8_t,400> wsaData{};wsaData.fill(0xa5);
    Check(Startup(0x0101,wsaData.data())==10092 && wsaData[0]==0xa5,"unsupported WSA version leaves output alone");
    Check(Startup(0x0202,nullptr)==10014,"missing WSA data rejected");
    Check(Startup(0x0202,wsaData.data())==0 && wsaData[0]==2 && wsaData[1]==2 && wsaData[396]==0xa5,"WSADATA negotiation and preserved vendor pointer");
    Check(Startup(0x0202,wsaData.data())==0 && Cleanup()==0,"balanced startup references retain transport");
    Check(InetAddress("127.0.0.1")==0x7f000001 && InetAddress("")==0 && InetAddress("invalid")==InvalidSocket, "inet_addr Xbox values");
    auto server = Socket(2,2,17), client = Socket(2,2,17);
    Check(server != InvalidSocket && client != InvalidSocket && server != client, "distinct owned UDP handles");
    auto bindAddress=Address(); Check(Bind(server,bindAddress.data(),16)==0,"UDP bind");
    uint8_t length[4], option[4]; Word(length,16);
    std::array<uint8_t,16> destination{};
    Check(GetName(server,destination.data(),length)==0 && Value(length)==16 && destination[1]==2 && destination[4]==127,"getsockname byte order");
    Word(option,1000);Check(SetOption(server,0xffff,0x1006,option,4)==0,"receive timeout");
    Word(length,4);Check(GetOption(server,0xffff,0x1008,option,length)==0 && Value(option)==2 && Value(length)==4,"socket type");
    Word(option,1);Check(SetOption(server,0xffff,4,option,4)==0,"reuse address set");
    Word(length,4);Check(GetOption(server,0xffff,4,option,length)==0 && Value(option)!=0,"reuse address get");
    Check(SendTo(client,"hello",5,0,destination.data(),16)==5,"real UDP sendto");
    char buffer[16]{};std::array<uint8_t,16> from{};Word(length,16);
    Check(ReceiveFrom(server,buffer,sizeof(buffer),0,from.data(),length)==5 && !std::memcmp(buffer,"hello",5) && from[4]==127 && Value(length)==16,"real UDP recvfrom");
    Check(Connect(client,destination.data(),16)==0,"UDP connect");
    Check(Send(client,"reply",5,0)==5,"connected send");
    Check(Receive(server,buffer,sizeof(buffer),0)==5 && !std::memcmp(buffer,"reply",5),"connected receive");
    Word(option,1);Check(Ioctl(server,0x8004667e,option)==0,"FIONBIO guest word");
    Check(Receive(server,buffer,sizeof(buffer),0)==-1 && LastError()==10035,"nonblocking empty receive");
    Check(SendTo(client,"read",4,0,destination.data(),16)==4,"queued data");
    Word(option,0);Check(Ioctl(server,0x4004667f,option)==0 && Value(option)==4,"FIONREAD guest output");
    Check(Receive(server,buffer,sizeof(buffer),0)==4,"consume queued data");
    Check(SendTo(client,"oversized",9,0,destination.data(),16)==9,"oversized UDP send");
    Check(Receive(server,buffer,2,0)==-1 && LastError()==10040,"UDP truncation reports WSAEMSGSIZE");
    Check(Send(client,"x",1,2)==-1 && LastError()==10045,"receive-only flag rejected by send");
    Word(length,15);Check(GetName(server,destination.data(),length)==-1 && LastError()==10014 && Value(length)==15,"small sockaddr rejected without output mutation");
    Check(SetOption(server,0xffff,0xdead,option,4)==-1 && LastError()==10042,"unsupported option reports failure");
    Check(Send(client,"x",1,8)==-1 && LastError()==10045,"unsupported flags rejected");
    Check(Close(server)==0 && Close(client)==0,"real close");
    Check(Close(server)==-1 && LastError()==10038,"stale handle rejected");
    Check(Receive(server,buffer,sizeof(buffer),0)==-1 && LastError()==10038,"closed handle cannot access fd");
    const auto mainError=LastError();std::thread isolation([] { SetLastError(123);Check(LastError()==123,"worker error value"); });isolation.join();
    Check(LastError()==mainError,"thread-local Winsock error");
    std::vector<uint32_t> resolved;
    Check(Resolve("127.0.0.1",resolved)==0 && resolved.size()==1 && resolved[0]==0x7f000001,"real numeric resolver and byte order");
    Check(Resolve(nullptr,resolved)==10022 && resolved.empty(),"invalid resolver request");
    Check(Socket(23,1,6)==InvalidSocket && LastError()==10047,"unsupported family");
    Check(Socket(2,1,254)==InvalidSocket && LastError()==10043,"unsupported protocol");
    // Real TCP roundtrip; the test owns a POSIX listener, the production backend owns the client.
    int listener=::socket(AF_INET,SOCK_STREAM,0);Check(listener>=0,"host TCP listener");
    sockaddr_in native{};native.sin_family=AF_INET;native.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    Check(::bind(listener,reinterpret_cast<sockaddr*>(&native),sizeof(native))==0 && ::listen(listener,1)==0,"host TCP bind/listen");
    socklen_t nativeSize=sizeof(native);Check(::getsockname(listener,reinterpret_cast<sockaddr*>(&native),&nativeSize)==0,"host TCP endpoint");
    auto tcp=Socket(2,1,6);auto tcpAddress=Address(ntohs(native.sin_port));
    Word(option,1000);Check(SetOption(tcp,0xffff,0x1006,option,4)==0,"TCP timeout");
    Check(Connect(tcp,tcpAddress.data(),16)==0,"real TCP connect");
    int peer=::accept(listener,nullptr,nullptr);Check(peer>=0,"host TCP accept");
    Check(Send(tcp,"tcp",3,0)==3 && ::recv(peer,buffer,sizeof(buffer),0)==3 && !std::memcmp(buffer,"tcp",3),"real TCP send");
    Check(::send(peer,"ack",3,0)==3 && Receive(tcp,buffer,sizeof(buffer),0)==3 && !std::memcmp(buffer,"ack",3),"real TCP receive");
    Check(Close(tcp)==0,"TCP close");::close(peer);::close(listener);
    auto leaked=Socket(2,2,17);Check(leaked!=InvalidSocket,"socket before final cleanup");
    Check(Cleanup()==0,"final WSA cleanup closes sockets");
    Check(Socket(2,2,17)==InvalidSocket && LastError()==10093,"socket after cleanup requires initialization");
    Check(Cleanup()==-1 && LastError()==10093,"unbalanced cleanup rejected");
    Check(Startup(0x0202,wsaData.data())==0,"WSA reinitialize");
    Check(Close(leaked)==-1 && LastError()==10038,"cleanup invalidates previous handles");
    Check(Cleanup()==0,"release reinitialized transport");
    std::puts("PASS: IPv4 ABI, UDP/TCP loopback, options, ioctl, resolver, error translation, owned handles and thread-local errors");
}
