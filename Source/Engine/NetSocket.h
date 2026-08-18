#pragma once

#include "Engine/SBSTypes.h"

#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace sbs {

struct SocketAddr {
    uint32_t ipv4 = 0;
    uint16_t port = 0;

    std::string toString() const;
    static SocketAddr fromIpv4Port(uint32_t ipv4, uint16_t port);
    static SocketAddr broadcast(uint16_t port);
};

class UdpSocket {
public:
    UdpSocket() = default;
    ~UdpSocket() { close(); }
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    bool open(uint16_t port, bool enableBroadcast);
    void close();
    bool valid() const { return fd_ >= 0; }
    uint16_t port() const { return boundPort_; }

    bool sendTo(const SocketAddr& addr, const uint8_t* data, int len);
    int recvFrom(uint8_t* data, int cap, SocketAddr& from);

private:
    int fd_ = -1;
    uint16_t boundPort_ = 0;
};

struct Packet {
    SocketAddr from;
    std::vector<uint8_t> bytes;
};

class UdpChannel {
public:
    bool open(uint16_t port, bool broadcast);
    void close();
    bool valid() const { return socket_.valid(); }
    bool send(const SocketAddr& to, const uint8_t* data, int len);
    bool sendBroadcast(uint16_t port, const uint8_t* data, int len);
    bool recv(Packet& packet);
    uint16_t port() const { return socket_.port(); }

private:
    UdpSocket socket_;
};

std::string localHostName();

} // namespace sbs
