#include "Engine/NetSocket.h"

#include "Engine/SBSLog.h"

#include <cstring>
#include <sstream>

#if !defined(_WIN32)
#include <netdb.h>
#include <unistd.h>
#endif

namespace sbs {
namespace {
bool gWsaReady = false;

bool ensureSockets() {
#if defined(_WIN32)
    if (gWsaReady) return true;
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return false;
    }
    gWsaReady = true;
    return true;
#else
    (void)gWsaReady;
    return true;
#endif
}

sockaddr_in toNative(const SocketAddr& addr) {
    sockaddr_in out{};
    out.sin_family = AF_INET;
    out.sin_port = htons(addr.port);
    out.sin_addr.s_addr = htonl(addr.ipv4);
    return out;
}
} // namespace

std::string SocketAddr::toString() const {
    std::ostringstream oss;
    oss << ((ipv4 >> 24) & 255) << "." << ((ipv4 >> 16) & 255) << "."
        << ((ipv4 >> 8) & 255) << "." << (ipv4 & 255) << ":" << port;
    return oss.str();
}

SocketAddr SocketAddr::fromIpv4Port(uint32_t ipv4, uint16_t port) {
    SocketAddr a;
    a.ipv4 = ipv4;
    a.port = port;
    return a;
}

SocketAddr SocketAddr::broadcast(uint16_t port) {
    return fromIpv4Port(0xFFFFFFFFu, port);
}

bool UdpSocket::open(uint16_t port, bool enableBroadcast) {
    close();
    if (!ensureSockets()) {
        return false;
    }
    fd_ = static_cast<int>(::socket(AF_INET, SOCK_DGRAM, 0));
    if (fd_ < 0) {
        logError("Failed to create UDP socket");
        return false;
    }

#if defined(_WIN32)
    u_long nonblock = 1;
    ioctlsocket(fd_, FIONBIO, &nonblock);
#else
    const int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
#endif

    int reuse = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
    if (enableBroadcast) {
        int b = 1;
        setsockopt(fd_, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&b), sizeof(b));
    }

    sockaddr_in bindAddr{};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bindAddr.sin_port = htons(port);
    if (::bind(fd_, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) != 0) {
        logError("Failed to bind UDP port " + std::to_string(port));
        close();
        return false;
    }

    sockaddr_in got{};
    socklen_t gotLen = sizeof(got);
    if (getsockname(fd_, reinterpret_cast<sockaddr*>(&got), &gotLen) == 0) {
        boundPort_ = ntohs(got.sin_port);
    } else {
        boundPort_ = port;
    }
    return true;
}

void UdpSocket::close() {
    if (fd_ >= 0) {
#if defined(_WIN32)
        closesocket(fd_);
#else
        ::close(fd_);
#endif
        fd_ = -1;
        boundPort_ = 0;
    }
}

bool UdpSocket::sendTo(const SocketAddr& addr, const uint8_t* data, int len) {
    if (!valid() || !data || len <= 0) return false;
    const sockaddr_in native = toNative(addr);
    const int sent = static_cast<int>(::sendto(fd_, reinterpret_cast<const char*>(data), len, 0,
                                               reinterpret_cast<const sockaddr*>(&native), sizeof(native)));
    return sent == len;
}

int UdpSocket::recvFrom(uint8_t* data, int cap, SocketAddr& from) {
    if (!valid() || !data || cap <= 0) return 0;
    sockaddr_in native{};
    socklen_t nativeLen = sizeof(native);
    const int n = static_cast<int>(::recvfrom(fd_, reinterpret_cast<char*>(data), cap, 0,
                                              reinterpret_cast<sockaddr*>(&native), &nativeLen));
    if (n <= 0) return 0;
    from.ipv4 = ntohl(native.sin_addr.s_addr);
    from.port = ntohs(native.sin_port);
    return n;
}

bool UdpChannel::open(uint16_t port, bool broadcast) {
    return socket_.open(port, broadcast);
}

void UdpChannel::close() {
    socket_.close();
}

bool UdpChannel::send(const SocketAddr& to, const uint8_t* data, int len) {
    return socket_.sendTo(to, data, len);
}

bool UdpChannel::sendBroadcast(uint16_t port, const uint8_t* data, int len) {
    return socket_.sendTo(SocketAddr::broadcast(port), data, len);
}

bool UdpChannel::recv(Packet& packet) {
    uint8_t buf[4096];
    SocketAddr from;
    const int n = socket_.recvFrom(buf, sizeof(buf), from);
    if (n <= 0) return false;
    packet.from = from;
    packet.bytes.assign(buf, buf + n);
    return true;
}

std::string localHostName() {
    char host[256];
    if (gethostname(host, sizeof(host)) != 0) {
        return "SBS-Host";
    }
    host[sizeof(host) - 1] = 0;
    return host;
}

} // namespace sbs
