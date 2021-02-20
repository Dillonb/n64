#include "parallel_rdp_wrapper.h"
#include "rdp.h"
#include <memory>
#include <SDL_video.h>
#include <rdp_device.hpp>
#include <frontend/render.h>
#include <SDL_vulkan.h>
#include <mem/mem_util.h>
#include <imgui/imgui_ui.h>
#include <imgui_impl_vulkan.h>

using namespace Vulkan;
using std::unique_ptr;
using RDP::CommandProcessor;
using RDP::CommandProcessorFlags;
using RDP::VIRegister;

static CommandProcessor* command_processor;
static WSI* wsi;

std::vector<Semaphore> acquire_semaphore;

VkQueue get_graphics_queue() {
    return wsi->get_context().get_graphics_queue();
}

VkInstance get_vk_instance() {
    return wsi->get_context().get_instance();
}

VkPhysicalDevice get_vk_physical_device() {
    return wsi->get_device().get_physical_device();
}

VkDevice get_vk_device() {
    return wsi->get_device().get_device();
}

uint32_t get_vk_graphics_queue_family() {
    return wsi->get_context().get_graphics_queue_family();
}

VkFormat get_vk_format() {
    return wsi->get_device().get_swapchain_view().get_format();
}

CommandBufferHandle requested_command_buffer;

VkCommandBuffer get_vk_command_buffer() {
    requested_command_buffer = wsi->get_device().request_command_buffer();
    return requested_command_buffer->get_command_buffer();
}

void submit_requested_vk_command_buffer() {
    wsi->get_device().submit(requested_command_buffer);
}

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
        n64_poll_input(global_system);
    }

    void event_frame_tick(double frame, double elapsed) override {
        n64_render_screen(global_system);
    }
};

/*
#version 450
layout(location = 0) in vec2 Position;
layout(location = 0) out highp vec2 vUV;

void main()
{
    gl_Position = vec4(Position, 0.0, 1.0);
    vUV = 0.5 * Position + 0.5;
}
*/
uint32_t fullscreen_quad_vert[] = {0x07230203,0x00010000,0x000d000a,0x00000022,
                                   0x00000000,0x00020011,0x00000001,0x0006000b,
                                   0x00000001,0x4c534c47,0x6474732e,0x3035342e,
                                   0x00000000,0x0003000e,0x00000000,0x00000001,
                                   0x0008000f,0x00000000,0x00000004,0x6e69616d,
                                   0x00000000,0x0000000d,0x00000012,0x0000001c,
                                   0x00030003,0x00000002,0x000001c2,0x000a0004,
                                   0x475f4c47,0x4c474f4f,0x70635f45,0x74735f70,
                                   0x5f656c79,0x656e696c,0x7269645f,0x69746365,
                                   0x00006576,0x00080004,0x475f4c47,0x4c474f4f,
                                   0x6e695f45,0x64756c63,0x69645f65,0x74636572,
                                   0x00657669,0x00040005,0x00000004,0x6e69616d,
                                   0x00000000,0x00060005,0x0000000b,0x505f6c67,
                                   0x65567265,0x78657472,0x00000000,0x00060006,
                                   0x0000000b,0x00000000,0x505f6c67,0x7469736f,
                                   0x006e6f69,0x00070006,0x0000000b,0x00000001,
                                   0x505f6c67,0x746e696f,0x657a6953,0x00000000,
                                   0x00070006,0x0000000b,0x00000002,0x435f6c67,
                                   0x4470696c,0x61747369,0x0065636e,0x00070006,
                                   0x0000000b,0x00000003,0x435f6c67,0x446c6c75,
                                   0x61747369,0x0065636e,0x00030005,0x0000000d,
                                   0x00000000,0x00050005,0x00000012,0x69736f50,
                                   0x6e6f6974,0x00000000,0x00030005,0x0000001c,
                                   0x00565576,0x00050048,0x0000000b,0x00000000,
                                   0x0000000b,0x00000000,0x00050048,0x0000000b,
                                   0x00000001,0x0000000b,0x00000001,0x00050048,
                                   0x0000000b,0x00000002,0x0000000b,0x00000003,
                                   0x00050048,0x0000000b,0x00000003,0x0000000b,
                                   0x00000004,0x00030047,0x0000000b,0x00000002,
                                   0x00040047,0x00000012,0x0000001e,0x00000000,
                                   0x00040047,0x0000001c,0x0000001e,0x00000000,
                                   0x00020013,0x00000002,0x00030021,0x00000003,
                                   0x00000002,0x00030016,0x00000006,0x00000020,
                                   0x00040017,0x00000007,0x00000006,0x00000004,
                                   0x00040015,0x00000008,0x00000020,0x00000000,
                                   0x0004002b,0x00000008,0x00000009,0x00000001,
                                   0x0004001c,0x0000000a,0x00000006,0x00000009,
                                   0x0006001e,0x0000000b,0x00000007,0x00000006,
                                   0x0000000a,0x0000000a,0x00040020,0x0000000c,
                                   0x00000003,0x0000000b,0x0004003b,0x0000000c,
                                   0x0000000d,0x00000003,0x00040015,0x0000000e,
                                   0x00000020,0x00000001,0x0004002b,0x0000000e,
                                   0x0000000f,0x00000000,0x00040017,0x00000010,
                                   0x00000006,0x00000002,0x00040020,0x00000011,
                                   0x00000001,0x00000010,0x0004003b,0x00000011,
                                   0x00000012,0x00000001,0x0004002b,0x00000006,
                                   0x00000014,0x00000000,0x0004002b,0x00000006,
                                   0x00000015,0x3f800000,0x00040020,0x00000019,
                                   0x00000003,0x00000007,0x00040020,0x0000001b,
                                   0x00000003,0x00000010,0x0004003b,0x0000001b,
                                   0x0000001c,0x00000003,0x0004002b,0x00000006,
                                   0x0000001d,0x3f000000,0x00050036,0x00000002,
                                   0x00000004,0x00000000,0x00000003,0x000200f8,
                                   0x00000005,0x0004003d,0x00000010,0x00000013,
                                   0x00000012,0x00050051,0x00000006,0x00000016,
                                   0x00000013,0x00000000,0x00050051,0x00000006,
                                   0x00000017,0x00000013,0x00000001,0x00070050,
                                   0x00000007,0x00000018,0x00000016,0x00000017,
                                   0x00000014,0x00000015,0x00050041,0x00000019,
                                   0x0000001a,0x0000000d,0x0000000f,0x0003003e,
                                   0x0000001a,0x00000018,0x0004003d,0x00000010,
                                   0x0000001e,0x00000012,0x0005008e,0x00000010,
                                   0x0000001f,0x0000001e,0x0000001d,0x00050050,
                                   0x00000010,0x00000020,0x0000001d,0x0000001d,
                                   0x00050081,0x00000010,0x00000021,0x0000001f,
                                   0x00000020,0x0003003e,0x0000001c,0x00000021,
                                   0x000100fd,0x00010038};


/*
#version 450
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 0) uniform sampler2D uImage;

layout(constant_id = 0) const float Scale = 1.0;

void main()
{
    FragColor = Scale * textureLod(uImage, vUV, 0.0);
}
*/
static uint32_t fullscreen_quad_blit[] = {0x07230203,0x00010000,0x000d000a,0x00000017,
                                          0x00000000,0x00020011,0x00000001,0x0006000b,
                                          0x00000001,0x4c534c47,0x6474732e,0x3035342e,
                                          0x00000000,0x0003000e,0x00000000,0x00000001,
                                          0x0007000f,0x00000004,0x00000004,0x6e69616d,
                                          0x00000000,0x00000009,0x00000012,0x00030010,
                                          0x00000004,0x00000007,0x00030003,0x00000002,
                                          0x000001c2,0x000a0004,0x475f4c47,0x4c474f4f,
                                          0x70635f45,0x74735f70,0x5f656c79,0x656e696c,
                                          0x7269645f,0x69746365,0x00006576,0x00080004,
                                          0x475f4c47,0x4c474f4f,0x6e695f45,0x64756c63,
                                          0x69645f65,0x74636572,0x00657669,0x00040005,
                                          0x00000004,0x6e69616d,0x00000000,0x00050005,
                                          0x00000009,0x67617246,0x6f6c6f43,0x00000072,
                                          0x00040005,0x0000000a,0x6c616353,0x00000065,
                                          0x00040005,0x0000000e,0x616d4975,0x00006567,
                                          0x00030005,0x00000012,0x00565576,0x00040047,
                                          0x00000009,0x0000001e,0x00000000,0x00040047,
                                          0x0000000a,0x00000001,0x00000000,0x00040047,
                                          0x0000000e,0x00000022,0x00000000,0x00040047,
                                          0x0000000e,0x00000021,0x00000000,0x00040047,
                                          0x00000012,0x0000001e,0x00000000,0x00020013,
                                          0x00000002,0x00030021,0x00000003,0x00000002,
                                          0x00030016,0x00000006,0x00000020,0x00040017,
                                          0x00000007,0x00000006,0x00000004,0x00040020,
                                          0x00000008,0x00000003,0x00000007,0x0004003b,
                                          0x00000008,0x00000009,0x00000003,0x00040032,
                                          0x00000006,0x0000000a,0x3f800000,0x00090019,
                                          0x0000000b,0x00000006,0x00000001,0x00000000,
                                          0x00000000,0x00000000,0x00000001,0x00000000,
                                          0x0003001b,0x0000000c,0x0000000b,0x00040020,
                                          0x0000000d,0x00000000,0x0000000c,0x0004003b,
                                          0x0000000d,0x0000000e,0x00000000,0x00040017,
                                          0x00000010,0x00000006,0x00000002,0x00040020,
                                          0x00000011,0x00000001,0x00000010,0x0004003b,
                                          0x00000011,0x00000012,0x00000001,0x0004002b,
                                          0x00000006,0x00000014,0x00000000,0x00050036,
                                          0x00000002,0x00000004,0x00000000,0x00000003,
                                          0x000200f8,0x00000005,0x0004003d,0x0000000c,
                                          0x0000000f,0x0000000e,0x0004003d,0x00000010,
                                          0x00000013,0x00000012,0x00070058,0x00000007,
                                          0x00000015,0x0000000f,0x00000013,0x00000002,
                                          0x00000014,0x0005008e,0x00000007,0x00000016,
                                          0x00000015,0x0000000a,0x0003003e,0x00000009,
                                          0x00000016,0x000100fd,0x00010038};

Program* fullscreen_quad_program;

void load_parallel_rdp(n64_system_t* system) {
    wsi = new WSI();
    wsi->set_backbuffer_srgb(false);
    wsi->set_platform(new SDLWSIPlatform());
    if (!wsi->init(1, nullptr)) {
        logfatal("Failed to initialize WSI!");
    }

    fullscreen_quad_program = wsi->get_device().request_program(fullscreen_quad_vert, sizeof(fullscreen_quad_vert), fullscreen_quad_blit, sizeof(fullscreen_quad_blit));

    auto aligned_rdram = reinterpret_cast<uintptr_t>(system->mem.rdram);
    uintptr_t offset = 0;

    if (wsi->get_device().get_device_features().supports_external_memory_host)
    {
        size_t align = wsi->get_device().get_device_features().host_memory_properties.minImportedHostPointerAlignment;
        offset = aligned_rdram & (align - 1);
        aligned_rdram -= offset;
    } else {
        logwarn("VK_EXT_external_memory_host is not supported by this device. Application might run slower because of this.");
    }

    CommandProcessorFlags flags = 1 << 1; // TODO configurable scaling

    command_processor = new CommandProcessor(wsi->get_device(), reinterpret_cast<void *>(aligned_rdram),
                                        offset, 8 * 1024 * 1024, 4 * 1024 * 1024, flags);

    if (!command_processor->device_is_supported()) {
        logfatal("This device probably does not support 8/16-bit storage. Make sure you're using up-to-date drivers!");
    }
}

void draw_fullscreen_textured_quad(Util::IntrusivePtr<Image> image, Util::IntrusivePtr<CommandBuffer> cmd) {
    cmd->set_texture(0, 0, image->get_view(), Vulkan::StockSampler::LinearClamp);
    cmd->set_program(fullscreen_quad_program);
    cmd->set_quad_state();
    auto *data = static_cast<float *>(cmd->allocate_vertex_data(0, 6 * sizeof(float), 2 * sizeof(float)));
    *data++ = -1.0f;
    *data++ = -3.0f;
    *data++ = -1.0f;
    *data++ = +1.0f;
    *data++ = +3.0f;
    *data++ = +1.0f;

    cmd->set_vertex_attrib(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
    cmd->set_depth_test(false, false);
    cmd->set_depth_compare(VK_COMPARE_OP_ALWAYS);
    cmd->set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    cmd->draw(3, 1);
}

void update_screen(Util::IntrusivePtr<Image> image) {
    wsi->begin_frame();

    if (!image) {
        auto info = Vulkan::ImageCreateInfo::immutable_2d_image(N64_SCREEN_X * SCREEN_SCALE, N64_SCREEN_Y * SCREEN_SCALE, VK_FORMAT_R8G8B8A8_UNORM);
        info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.misc = IMAGE_MISC_MUTABLE_SRGB_BIT;
        info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        image = wsi->get_device().create_image(info);

        auto cmd = wsi->get_device().request_command_buffer();

        cmd->image_barrier(*image,
                           VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
        cmd->clear_image(*image, {});
        cmd->image_barrier(*image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
        wsi->get_device().submit(cmd);
    }

    Util::IntrusivePtr<CommandBuffer> cmd = wsi->get_device().request_command_buffer();

    cmd->begin_render_pass(wsi->get_device().get_swapchain_render_pass(SwapchainRenderPass::ColorOnly));
    draw_fullscreen_textured_quad(image, cmd);
    ImGui_ImplVulkan_RenderDrawData(imgui_frame(), cmd->get_command_buffer());
    cmd->end_render_pass();
    wsi->get_device().submit(cmd);
    wsi->end_frame();
}

void update_screen_parallel_rdp(n64_system_t* system) {
    if (unlikely(!command_processor)) {
        logfatal("Update screen without an initialized command processor");
    }

    command_processor->set_vi_register(VIRegister::Control,      system->vi.status.raw);
    command_processor->set_vi_register(VIRegister::Origin,       system->vi.vi_origin);
    command_processor->set_vi_register(VIRegister::Width,        system->vi.vi_width);
    command_processor->set_vi_register(VIRegister::Intr,         system->vi.vi_v_intr);
    command_processor->set_vi_register(VIRegister::VCurrentLine, system->vi.v_current);
    command_processor->set_vi_register(VIRegister::Timing,       system->vi.vi_burst.raw);
    command_processor->set_vi_register(VIRegister::VSync,        system->vi.vsync);
    command_processor->set_vi_register(VIRegister::HSync,        system->vi.hsync);
    command_processor->set_vi_register(VIRegister::Leap,         system->vi.leap);
    command_processor->set_vi_register(VIRegister::HStart,       system->vi.hstart);
    command_processor->set_vi_register(VIRegister::VStart,       system->vi.vstart.raw);
    command_processor->set_vi_register(VIRegister::VBurst,       system->vi.vburst);
    command_processor->set_vi_register(VIRegister::XScale,       system->vi.xscale);
    command_processor->set_vi_register(VIRegister::YScale,       system->vi.yscale);

    RDP::ScanoutOptions opts;
    opts.persist_frame_on_invalid_input = true;
    opts.vi.aa = true;
    opts.vi.scale = true;
    opts.vi.dither_filter = true;
    opts.vi.divot_filter = true;
    opts.vi.gamma_dither = true;
    opts.downscale_steps = true;
    opts.crop_overscan_pixels = true;
    Util::IntrusivePtr<Image> image = command_processor->scanout(opts);
    update_screen(image);
    command_processor->begin_frame_context();
}

void update_screen_parallel_rdp_no_game() {
    update_screen(static_cast<Util::IntrusivePtr<Image>>(nullptr));
}

void parallel_rdp_enqueue_command(int command_length, word* buffer) {
    command_processor->enqueue_command(command_length, buffer);
}

void parallel_rdp_on_full_sync() {
    command_processor->wait_for_timeline(command_processor->signal_timeline());
}
