#include "http_api.h"

// TODO: support as much of this as possible: https://github.com/skylersaleh/SkyEmu/blob/dev/docs/HTTP_CONTROL_SERVER.md

#include <cstdlib>
#include <format>

extern "C" {
    #include <common/settings.h>
    #include <mem/n64bus.h>
}
#include <debugger/debugger.hpp>

#include <httplib.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

httplib::Server svr;

void http_server_thread() {
  svr.listen(n64_settings.http_api_host, n64_settings.http_api_port);
}
std::thread t;

#define HTTP_ERROR(message, code) do { \
    res.status = httplib::StatusCode::code; \
    res.set_content(message, "text/plain"); \
    return; \
} while(0)

#define HTTP_OK do { res.set_content("OK", "application/json"); } while (0)

template <typename T>
T debugger_read(u32 address) {
    T result = 0;

    for (int i = 0; i < sizeof(T); i++) {
        // silence a compiler warning
        if (sizeof(T) > 1) {
            result <<= 8;
        }
        u8 b;
        if (debugger_read_physical_byte(address + i, &b)) {
            result |= b;
        } else {
            logwarn("Failed to read byte at %08X", address + i);
        }
    }

    return result;
}

u64 parse_address(std::string s_address) {
    u64 address = std::stoull(s_address, nullptr, 16);
    // If it's a 32 bit address
    if (address <= 0xFFFFFFFF) {
        // And it wasn't explicitly specified to be not sign extended
        if (!s_address.starts_with("00000000") && !s_address.starts_with("0x00000000")) {
            // Sign extend it
            address = (s64)((s32)address);
        }
    }
    return address;
}

void http_api_init() {
    logalways("http_api_init listening on: %s:%d\n", n64_settings.http_api_host, n64_settings.http_api_port);

    svr.Get("/read/:size/:address", [&](const httplib::Request& req, httplib::Response& res) {
        std::string s_size = req.path_params.at("size");
        std::string s_address = req.path_params.at("address");

        u64 vaddr = parse_address(s_address);
        u64 size = 0;
        try {
            size = std::stoull(s_size, nullptr, 0); // autodetect base
        } catch (std::exception&) {}


        u32 physical;
        bool cached;


        if (size != 0) {
            std::string result = "";
            for (int i = 0; i < size; i++) {
                if (!resolve_virtual_address(vaddr + i, BUS_LOAD, &cached, &physical)) {
                    res.status = httplib::StatusCode::BadRequest_400;
                    res.set_content("Failed to resolve virtual address", "text/plain");
                    return;
                } else {
                    result += std::format("{:02X}", debugger_read<u8>(physical));
                }
            }
            res.set_content(result, "text/plain");
        } else if (!resolve_virtual_address(vaddr, BUS_LOAD, &cached, &physical)) {
            res.status = httplib::StatusCode::BadRequest_400;
            res.set_content("Failed to resolve virtual address", "text/plain");
        } else {
            if (s_size == "byte") {
                res.set_content(std::format("{:02X}", debugger_read<u8>(physical)), "text/plain");
            } else if (s_size == "half") {
                if (physical & 1) {
                    HTTP_ERROR("Unaligned halfword read", BadRequest_400);
                } else {
                    res.set_content(std::format("{:04X}", debugger_read<u16>(physical)), "text/plain");
                }
            } else if (s_size == "word") {
                if (physical & 3) {
                    HTTP_ERROR("Unaligned word read", BadRequest_400);
                } else {
                    res.set_content(std::format("{:08X}", debugger_read<u32>(physical)), "text/plain");
                }
            } else if (s_size == "dword") {
                if (physical & 7) {
                    HTTP_ERROR("Unaligned dword read", BadRequest_400);
                } else {
                    res.set_content(std::format("{:016X}", debugger_read<u64>(physical)), "text/plain");
                }
            } else {
                HTTP_ERROR("Invalid size", BadRequest_400);
            }
        }
    });

    svr.Get("/registers", [&](const httplib::Request& req, httplib::Response& res) {
        json result;
        result["pc"] = N64CPU.pc;
        result["hi"] = N64CPU.mult_hi;
        result["lo"] = N64CPU.mult_lo;
        for (int i = 0; i < 32; i++) {
            result[register_names[i]] = N64CPU.gpr[i];
        }
        res.set_content(result.dump(), "application/json");
    });

    auto get_breakpoints = [&](const httplib::Request& req, httplib::Response& res) {
        json result = json::array({});
        for (const auto& [ address, breakpoint ] : breakpoints) {
            if (!breakpoint.temporary) {
                json bp;
                bp["muted"] = false; // TODO
                bp["address"] = std::format("{:016X}", address);
                result.push_back(bp);
            }
        }
        res.set_content(result.dump(), "application/json");
    };

    svr.Get("/breakpoints", get_breakpoints);

    svr.Get("/breakpoints/:operation/:address", [&](const httplib::Request& req, httplib::Response& res) {
        std::string operation = req.path_params.at("operation");
        u64 address = parse_address(req.path_params.at("address"));

        if (operation == "set") {
            logalways("%s", std::format("Set breakpoint at {:016X}", address).c_str());
            n64_debug_set_breakpoint(address);
            get_breakpoints(req, res);
        } else if (operation == "clear") {
            n64_debug_clear_breakpoint(address);
            logalways("%s", std::format("Cleared breakpoint at {:016X}", address).c_str());
            get_breakpoints(req, res);
        } else if (operation == "mute") {
            HTTP_ERROR(std::string("Unimplemented operation: ") + operation, BadRequest_400);
        } else if (operation == "unmute") {
            HTTP_ERROR(std::string("Unimplemented operation: ") + operation, BadRequest_400);
        } else {
            HTTP_ERROR(std::string("Invalid operation: ") + operation, BadRequest_400);
        }
    });

    svr.Get("/control/break", [&](const httplib::Request& req, httplib::Response& res) {
        n64sys.debugger_state.broken = true;
        HTTP_OK;
    });

    svr.Get("/control/step", [&](const httplib::Request& req, httplib::Response& res) {
        debugger_step();
        HTTP_OK;
    });

    svr.Get("/control/continue", [&](const httplib::Request& req, httplib::Response& res) {
        n64sys.debugger_state.broken = false;
        HTTP_OK;
    });

    svr.Get("/control/state", [&](const httplib::Request& req, httplib::Response& res) {
        json result;
        result["running"] = !n64sys.debugger_state.broken;
        res.set_content(result.dump(), "application/json");
    });

    svr.Get("/control/quit", [&](const httplib::Request& req, httplib::Response& res) {
        n64sys.debugger_state.broken = false; // if we're paused, we need to unpause for the quit to work
        n64_request_quit();
        HTTP_OK;
    });

    t = std::thread(http_server_thread);
}

void http_api_stop() {
    svr.stop();
    if (t.joinable()) {
        t.join();
    }
}