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
#include <frontend/frontend.h>

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

bool is_framerate_unlocked() {
    return wsi->get_present_mode() != PresentMode::SyncToVBlank;
}

void set_framerate_unlocked(bool unlocked) {
    if (unlocked) {
        wsi->set_present_mode(PresentMode::UnlockedForceTearing);
    } else {
        wsi->set_present_mode(PresentMode::SyncToVBlank);
    }
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
        n64_poll_input();
    }

    void event_frame_tick(double frame, double elapsed) override {
        n64_render_screen();
    }
};

uint32_t fullscreen_quad_vert[] =
#include <fullscreen_quad.vert.inc>
;
uint32_t fullscreen_quad_frag[] =
#include <fullscreen_quad.frag.inc>
;

Program* fullscreen_quad_program;

void load_parallel_rdp() {
    wsi = new WSI();
    wsi->set_backbuffer_srgb(false);
    wsi->set_platform(new SDLWSIPlatform());
    Context::SystemHandles handles;
    if (!wsi->init(1, handles)) {
        logfatal("Failed to initialize WSI!");
    }

    ResourceLayout vert_layout;
    ResourceLayout frag_layout;

    vert_layout.input_mask = 1;
    vert_layout.output_mask = 1;

    frag_layout.input_mask = 1;
    frag_layout.output_mask = 1;
    frag_layout.spec_constant_mask = 1;

    frag_layout.sets[0].sampled_image_mask = 1;
    frag_layout.sets[0].fp_mask = 1;
    frag_layout.sets[0].array_size[0] = 1;

    fullscreen_quad_program = wsi->get_device().request_program(fullscreen_quad_vert, sizeof(fullscreen_quad_vert), fullscreen_quad_frag, sizeof(fullscreen_quad_frag), &vert_layout, &frag_layout);

    auto aligned_rdram = reinterpret_cast<uintptr_t>(n64sys.mem.rdram);
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

void update_screen_parallel_rdp() {
    if (unlikely(!command_processor)) {
        logfatal("Update screen without an initialized command processor");
    }

    command_processor->set_vi_register(VIRegister::Control,      n64sys.vi.status.raw);
    command_processor->set_vi_register(VIRegister::Origin,       n64sys.vi.vi_origin);
    command_processor->set_vi_register(VIRegister::Width,        n64sys.vi.vi_width);
    command_processor->set_vi_register(VIRegister::Intr,         n64sys.vi.vi_v_intr);
    command_processor->set_vi_register(VIRegister::VCurrentLine, n64sys.vi.v_current);
    command_processor->set_vi_register(VIRegister::Timing,       n64sys.vi.vi_burst.raw);
    command_processor->set_vi_register(VIRegister::VSync,        n64sys.vi.vsync);
    command_processor->set_vi_register(VIRegister::HSync,        n64sys.vi.hsync);
    command_processor->set_vi_register(VIRegister::Leap,         n64sys.vi.leap);
    command_processor->set_vi_register(VIRegister::HStart,       n64sys.vi.hstart.raw);
    command_processor->set_vi_register(VIRegister::VStart,       n64sys.vi.vstart.raw);
    command_processor->set_vi_register(VIRegister::VBurst,       n64sys.vi.vburst);
    command_processor->set_vi_register(VIRegister::XScale,       n64sys.vi.xscale.raw);
    command_processor->set_vi_register(VIRegister::YScale,       n64sys.vi.yscale.raw);

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
