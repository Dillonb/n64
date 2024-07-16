#ifndef N64_PARALLEL_RDP_WRAPPER_SDL_H
#define N64_PARALLEL_RDP_WRAPPER_SDL_H

#include <wsi.hpp>
#include <SDL_video.h>
#include <SDL_vulkan.h>
#include <frontend/render.h>
#include <frontend/frontend.h>

#include "parallel_rdp_wrapper.h"

extern "C" {
extern SDL_Window* window;
}

class SDLWSIPlatform : public Vulkan::WSIPlatform {
        public:
        SDLWSIPlatform() = default;

        std::vector<const char *> get_instance_extensions() override {
            const char* extensions[64];
            unsigned int num_extensions = 64;

            if (!SDL_Vulkan_GetInstanceExtensions(window, &num_extensions, extensions)) {
                logfatal("SDL_Vulkan_GetInstanceExtensions failed: %s", SDL_GetError());
            }
            auto vec = std::vector<const char*>();

            for (unsigned int i = 0; i < num_extensions; i++) {
                vec.push_back(extensions[i]);
            }

            return vec;
        }

        VkSurfaceKHR create_surface(VkInstance instance, VkPhysicalDevice gpu) override {
            VkSurfaceKHR vk_surface;
            if (!SDL_Vulkan_CreateSurface(window, instance, &vk_surface)) {
                logfatal("Failed to create Vulkan window surface: %s", SDL_GetError());
            }
            return vk_surface;
        }

        uint32_t get_surface_width() override {
            return N64_SCREEN_X * SCREEN_SCALE;
        }

        uint32_t get_surface_height() override {
            return N64_SCREEN_Y * SCREEN_SCALE;
        }

        bool alive(Vulkan::WSI &wsi) override {
            return true;
        }

        void poll_input() override {
            n64_poll_input();
        }

        void poll_input_async(Granite::InputTrackerHandler *handler) override {
            logfatal("poll_input_async is unimplemented.");
        }

        void event_frame_tick(double frame, double elapsed) override {
            n64_render_screen();
        }
};

class SDLParallelRdpWindowInfo : public ParallelRdpWindowInfo {
        CoordinatePair get_window_size() {
            int sdlWinWidth, sdlWinHeight;
            SDL_GetWindowSize(window, &sdlWinWidth, &sdlWinHeight);
            return CoordinatePair{ sdlWinWidth, sdlWinHeight };
        }
};

#endif //N64_PARALLEL_RDP_WRAPPER_SDL_H
