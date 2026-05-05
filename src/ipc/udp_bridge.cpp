/**
 * @file    udp_bridge.cpp
 * @brief   Implementace UDP mostu pro komunikaci s GUI.
 * @author  Lukáš
 * @date    2026-05-04
 *
 * Používá BSD sockety (POSIX) s platformními rozdíly pro Windows.
 * Protokol: jedna JSON zpráva na řádek, ukončená '\\n'.
 */

#include "udp_bridge.h"
#include "../interpreter/runtime_state.h"
#include "../interpreter/log_entry.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using ssize_t = int;
  #define CLOSE_SOCKET closesocket
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #define CLOSE_SOCKET close
#endif

#include <cstring>
#include <cstdio>
#include <sstream>
#include <chrono>
#include <ctime>

namespace pets {

UdpBridge::~UdpBridge()
{
    stop();
}

bool UdpBridge::start(int port)
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return false;
    }
#endif

    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        return false;
    }

    // Nastav neblokující režim
#ifndef _WIN32
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);
#else
    u_long mode = 1;
    ioctlsocket(socket_fd_, FIONBIO, &mode);
#endif

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        CLOSE_SOCKET(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    return true;
}

void UdpBridge::stop()
{
    if (socket_fd_ >= 0) {
        CLOSE_SOCKET(socket_fd_);
        socket_fd_ = -1;
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

bool UdpBridge::is_open() const
{
    return socket_fd_ >= 0;
}

void UdpBridge::send_state(const RuntimeState& state)
{
    std::string json = state_to_json(state);
    // PLACEHOLDER: odeslat na známou adresu GUI (localhost:PORT)
    // Pro teď jen výpis
    (void)json;
}

void UdpBridge::send_log(const LogEntry& entry)
{
    std::string json = log_to_json(entry);
    (void)json;
}

void UdpBridge::set_input_callback(InputCallback callback)
{
    input_callback_ = std::move(callback);
}

void UdpBridge::set_stop_callback(StopCallback callback)
{
    stop_callback_ = std::move(callback);
}

void UdpBridge::poll()
{
    if (socket_fd_ < 0) return;

    char buffer[4096];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    ssize_t n = recvfrom(socket_fd_, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr*)&sender_addr, &sender_len);
    if (n > 0) {
        buffer[n] = '\0';
        // Může obsahovat více řádků – zjednodušeně zpracuj první
        std::string line(buffer);
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        handle_line(line);
    }
}

void UdpBridge::handle_line(const std::string& line)
{
    // Jednoduchý protokol:
    //   INPUT:<name>:<value>   -> injektovat vstup
    //   STOP                   -> ukončit interpret

    if (line == "STOP") {
        if (stop_callback_) {
            stop_callback_();
        }
        return;
    }

    if (line.rfind("INPUT:", 0) == 0) {
        size_t first_colon = 6;  // za "INPUT:"
        size_t second_colon = line.find(':', first_colon);
        if (second_colon != std::string::npos && second_colon > first_colon) {
            std::string name = line.substr(first_colon, second_colon - first_colon);
            std::string value = line.substr(second_colon + 1);
            if (input_callback_) {
                input_callback_(name, value);
            }
        }
        return;
    }

    // Neznámý příkaz – ignoruj
}

std::string UdpBridge::state_to_json(const RuntimeState& state) const
{
    std::ostringstream ss;
    ss << "{\"type\":\"state\",\"places\":{";
    for (size_t i = 0; i < state.places_tokens.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "\"" << state.places_tokens[i].name << "\":"
           << state.places_tokens[i].value;
    }
    ss << "},\"enabled\":[";
    for (size_t i = 0; i < state.enabled_transitions.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "\"" << state.enabled_transitions[i] << "\"";
    }
    ss << "]}";
    return ss.str();
}

std::string UdpBridge::log_to_json(const LogEntry& entry) const
{
    // Převeď timestamp na UNIX timestamp (ms)
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()
    ).count();

    std::ostringstream ss;
    ss << "{\"type\":\"log\",\"ts\":" << ts
       << ",\"msg\":\"" << entry.message << "\"";
    if (!entry.detail.empty()) {
        ss << ",\"detail\":\"" << entry.detail << "\"";
    }
    ss << "}";
    return ss.str();
}

} // namespace pets