#ifndef N64_PARALLEL_RDP_WRAPPER_H
#define N64_PARALLEL_RDP_WRAPPER_H

#include <system/n64system.h>

#ifdef __cplusplus
#include <wsi.hpp>
class ParallelRdpWindowInfo {
public:
    struct CoordinatePair {
        int x;
        int y;
    };
    virtual CoordinatePair get_window_size() = 0;
    virtual ~ParallelRdpWindowInfo() = default;
};

VkQueue get_graphics_queue();
VkInstance get_vk_instance();
VkPhysicalDevice get_vk_physical_device();
VkDevice get_vk_device();
uint32_t get_vk_graphics_queue_family();
VkFormat get_vk_format();
VkCommandBuffer get_vk_command_buffer();
void submit_requested_vk_command_buffer();
Vulkan::WSI* init_vulkan_wsi(Vulkan::InstanceFactory* instance_factory, Vulkan::WSIPlatform* wsi_platform, std::unique_ptr<ParallelRdpWindowInfo>&& windowInfo);
void init_parallel_rdp();

extern "C" {
#endif
    void prdp_init_internal_swapchain();
    void prdp_update_screen();
    void prdp_enqueue_command(int command_length, u32* buffer);
    void prdp_on_full_sync();
    void prdp_update_screen_no_game();
    bool prdp_is_framerate_unlocked();
    void prdp_set_framerate_unlocked(bool unlocked);
#ifdef __cplusplus
};
#endif

#endif //N64_PARALLEL_RDP_WRAPPER_H
