#include <arpa/inet.h>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

#define SUCCESS     0
#define NOT_FOUND   1
#define BAD_INPUT   2

#define FIELD_SL            0
#define FIELD_LOCAL_IP      1
#define FIELD_LOCAL_PORT    2
#define FIELD_REMOTE_IP     3
#define FIELD_REMOTE_PORT   4
#define FIELD_STATE         5
#define FIELD_TX_QUEUE      6
#define FIELD_RX_QUEUE      7
#define FIELD_TR            8
#define FIELD_TM_WHEN       9
#define FIELD_RETRNS        10
#define FIELD_UID           11
#define FIELD_TIMEOUT       12
#define FIELD_INODE         13
#define FIELD_REF_COUNT     14
#define FIELD_SOCKET_PTR    15
#define FIELD_RETRY_TIMEOUT 16
#define FIELD_PRED_FLAGS    17
#define FIELD_SND_WSCALE    18
#define FIELD_RCV_WSCALE    19
#define FIELD_OPTIONS       20

#define STATE_LISTENING 0x0A

#ifndef NET_STAT_FILE_PATH
#define NET_STAT_FILE_PATH "/proc/net/tcp"
#endif

namespace fs = std::filesystem;

static bool logDebugEnabled = false;
static bool logQuietEnabled = false;

static void info(const std::string &message) {
    if (!logQuietEnabled) {
        std::cout << message << std::endl;
    }
}

static void error(const std::string &message) {
    if (!logQuietEnabled) {
        std::cerr << "[ERROR] " << message << std::endl;
    }
}

static void debug(const std::string &message) {
    if (logDebugEnabled) {
        std::cout << "[DEBUG] " << message << std::endl;
    }
}

static unsigned int hexToInt(const std::string &hexStr) {
    unsigned int value;
    std::stringstream ss;

    ss << std::hex << hexStr;
    ss >> value;

    return value;
}

#ifdef USE_IPV6

static std::string ipToHex(const std::string &ip) {

    if (ip.empty())
        return "";

    struct in6_addr addr;
    if (inet_pton(AF_INET6, ip.c_str(), &addr) != 1) {
        return "";
    }

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    for (int i = 0; i < 4; ++i) {
        ss << std::setw(8) << reinterpret_cast<uint32_t *>(addr.s6_addr)[i];
    }

    return ss.str();
}

#else

static std::string ipToHex(const std::string &ip) {

    if (ip.empty())
        return "";

    struct in_addr addr;
    if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
        return "";
    }

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    ss << std::setw(8) << static_cast<uint32_t>(addr.s_addr);

    return ss.str();
}

#endif

static std::vector<std::string> normalizeLine(const std::string &line,
                                              const std::string &delimiters = " \t:") {
    std::vector<std::string> fields;
    std::string currentField;

    for (char c : line) {
        if (delimiters.find(c) != std::string::npos) {
            if (!currentField.empty()) {
                fields.push_back(currentField);
                currentField.clear();
            }
        } else {
            currentField += c;
        }
    }

    std::transform(currentField.begin(), currentField.end(), currentField.begin(),
                   ::toupper);

    if (!currentField.empty()) {
        fields.push_back(currentField);
    }

    return fields;
}

static bool isPortOpen(unsigned int targetPort, const std::string &targetIp = "") {

    std::ifstream file(NET_STAT_FILE_PATH);
    if (!file.is_open()) {
        error("Cannot open " NET_STAT_FILE_PATH);
        return false;
    }

    std::string targetIpHex;
    if (!targetIp.empty()) {
        targetIpHex = ipToHex(targetIp);
        if (targetIpHex.empty()) {
            error("Invalid IP address format");
            return false;
        }
        debug("Target IP hex: " + targetIpHex);
    }

    std::string line;
    std::getline(file, line); // Skip header

    while (std::getline(file, line)) {
        std::vector<std::string> fields = normalizeLine(line);

        if (fields.size() <= FIELD_STATE) {
            continue;
        }

        std::string sl = fields[FIELD_SL];
        std::string localAddrHex = fields[FIELD_LOCAL_IP];
        std::string localPortHex = fields[FIELD_LOCAL_PORT];
        std::string state = fields[FIELD_STATE];

        debug("=== SL " + sl + " ===");
        debug("  Local address (hex): " + localAddrHex);
        debug("  Local port (hex): " + localPortHex);
        debug("  State: " + state);

        if (hexToInt(state) == STATE_LISTENING) {
            unsigned int port = hexToInt(localPortHex);

            if (port == targetPort) {
                debug("  Port matches target port!");

                if (targetIpHex.empty()) {
                    debug("  No specific IP requested - port found!");
                    return true;
                }

                if (targetIpHex == localAddrHex) {
                    debug("  IP matches target address!");
                    return true;
                }
            } else {
                debug("  Port does not match target");
            }
        } else {
            debug("  Not in LISTENING state");
        }
    }

    return false;
}

int main(int argc, char *argv[]) {

    if (argc < 2 || argc > 5) {

        fs::path fullPath = fs::path(argv[0]);
        std::string fileName = fullPath.filename();

        info("Usage: " + fileName + " <port_number> [<ip_address>] [--debug] [--quiet]");
        info("Examples:");
        info("  " + fileName + " 8080");
#ifdef USE_IPV6
        info("  " + fileName + " 80 ::");
        info("  " + fileName + " 8080 ::1 --debug");
        info("  " + fileName + " 8080 ::1 --quiet");
#else
        info("  " + fileName + " 80 0.0.0.0");
        info("  " + fileName + " 8080 127.0.0.1 --debug");
        info("  " + fileName + " 8080 127.0.0.1 --quiet");
#endif
        return BAD_INPUT;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            logDebugEnabled = true;
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
            }
            argc--;
            i--;
        } else if (arg == "--quiet") {
            logQuietEnabled = true;
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
            }
            argc--;
            i--;
        }
    }

    if (logDebugEnabled) {
        std::ifstream debugFile(NET_STAT_FILE_PATH);
        if (debugFile.is_open()) {
            debug("=== Contents of " NET_STAT_FILE_PATH " ===");
            std::string line;
            while (std::getline(debugFile, line)) {
                debug(line);
            }
            debug("=== End of file contents ===");
        } else {
            debug("Failed to open " NET_STAT_FILE_PATH);
        }
    }

    try {
        unsigned int targetPort = std::stoi(argv[1]);
        std::string targetIp = (argc >= 3) ? argv[2] : "";

        debug("Starting search for port " + std::to_string(targetPort) +
              (targetIp.empty() ? "" : " on IP " + targetIp));

        if (isPortOpen(targetPort, targetIp)) {
            if (targetIp.empty()) {
                info("Port " + std::to_string(targetPort) + " is open and listening");
            } else {
                info("Port " + std::to_string(targetPort) + " is open and listening on " + targetIp);
            }
            return SUCCESS;
        } else {
            if (targetIp.empty()) {
                info("Port " + std::to_string(targetPort) + " is not open or not listening");
            } else {
                info("Port " + std::to_string(targetPort) + " is not open or not listening on " + targetIp);
            }
            return NOT_FOUND;
        }

    } catch (const std::exception &e) {
        error("Invalid port number - " + std::string(e.what()));
        return NOT_FOUND;
    }
}