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

void http_api_init() {
    logalways("http_api_init listening on: %s:%d\n", n64_settings.http_api_host, n64_settings.http_api_port);

    svr.Get("/read/:size/:address", [&](const httplib::Request& req, httplib::Response& res) {
        std::string s_size = req.path_params.at("size");
        std::string s_address = req.path_params.at("address");

        u64 vaddr = std::stoull(s_address, nullptr, 16);
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
                    result += std::format("{:02X}", n64_read_physical_byte(physical));
                }
            }
            res.set_content(result, "text/plain");
        } else if (!resolve_virtual_address(vaddr, BUS_LOAD, &cached, &physical)) {
            res.status = httplib::StatusCode::BadRequest_400;
            res.set_content("Failed to resolve virtual address", "text/plain");
        } else {
            if (s_size == "byte") {
                res.set_content(std::format("{:02X}", n64_read_physical_byte(physical)), "text/plain");
            } else if (s_size == "half") {
                if (physical & 1) {
                    HTTP_ERROR("Unaligned halfword read", BadRequest_400);
                } else {
                    res.set_content(std::format("{:04X}", n64_read_physical_half(physical)), "text/plain");
                }
            } else if (s_size == "word") {
                if (physical & 3) {
                    HTTP_ERROR("Unaligned word read", BadRequest_400);
                } else {
                    res.set_content(std::format("{:08X}", n64_read_physical_word(physical)), "text/plain");
                }
            } else if (s_size == "dword") {
                if (physical & 7) {
                    HTTP_ERROR("Unaligned dword read", BadRequest_400);
                } else {
                    res.set_content(std::format("{:016X}", n64_read_physical_dword(physical)), "text/plain");
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

    svr.Get("/control/break", [&](const httplib::Request& req, httplib::Response& res) {
        n64sys.debugger_state.broken = true;
    });

    svr.Get("/control/step", [&](const httplib::Request& req, httplib::Response& res) {
        debugger_step();
    });

    svr.Get("/control/continue", [&](const httplib::Request& req, httplib::Response& res) {
        n64sys.debugger_state.broken = false;
    });

    svr.Get("/control/quit", [&](const httplib::Request& req, httplib::Response& res) {
        n64sys.debugger_state.broken = false; // if we're paused, we need to unpause for the quit to work
        n64_request_quit();
    });

    t = std::thread(http_server_thread);
}

void http_api_stop() {
    svr.stop();
    if (t.joinable()) {
        t.join();
    }
}