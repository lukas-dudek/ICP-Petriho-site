/**
 * @file    udp_bridge.h
 * @brief   UDP komunikace mezi interpretem a GUI (monitorování, injekce vstupů)
 * @author  Lukáš
 * @date    2026-05-04
 *
 * Interpret otevře UDP socket a naslouchá na zvoleném portu.
 * GUI se připojí jako klient, posílá vstupní události a přijímá
 * runtime stav + log záznamy serializované do JSON.
 */

#ifndef PETS_UDP_BRIDGE_H
#define PETS_UDP_BRIDGE_H

#include <string>
#include <functional>

namespace pets {

struct RuntimeState;
struct LogEntry;

/**
 * @brief UDP most pro komunikaci s GUI.
 *
 * Používá jednoduchý textový protokol s JSON zprávami.
 * Každá zpráva je jeden řádek (ukončený '\\n').
 */
class UdpBridge {
public:
    /// Callback – volá se, když přijde vstup z GUI.
    using InputCallback = std::function<void(const std::string& name,
                                               const std::string& value)>;

    /// Callback – volá se, když přijde příkaz k ukončení.
    using StopCallback = std::function<void()>;

    UdpBridge() = default;
    ~UdpBridge();

    UdpBridge(const UdpBridge&) = delete;
    UdpBridge& operator=(const UdpBridge&) = delete;

    /**
     * @brief Otevře UDP socket a začne naslouchat.
     * @param port Port pro naslouchání (výchozí 12345).
     * @return true při úspěchu.
     */
    bool start(int port = 12345);

    /// Zavře socket.
    void stop();

    /// Je socket otevřen?
    bool is_open() const;

    /// Odešle runtime stav do GUI.
    void send_state(const RuntimeState& state);

    /// Odešle log záznam do GUI.
    void send_log(const LogEntry& entry);

    /// Nastaví callback pro příchozí vstupy.
    void set_input_callback(InputCallback callback);

    /// Nastaví callback pro požadavek ukončení.
    void set_stop_callback(StopCallback callback);

    /**
     * @brief Zpracuje příchozí data (neblokující).
     *
     * Toto by mělo být voláno periodicky (např. v hlavní smyčce) nebo
     * v samostatném vlákně.
     */
    void poll();

private:
    /// Serializuje RuntimeState do JSON stringu.
    std::string state_to_json(const RuntimeState& state) const;

    /// Serializuje LogEntry do JSON stringu.
    std::string log_to_json(const LogEntry& entry) const;

    /// Zpracuje jeden přijatý řádek.
    void handle_line(const std::string& line);

    int socket_fd_ = -1;
    InputCallback input_callback_;
    StopCallback stop_callback_;
};

} // namespace pets

#endif // PETS_UDP_BRIDGE_H