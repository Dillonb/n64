#ifndef N64_PARALLEL_RDP_WRAPPER_H
#define N64_PARALLEL_RDP_WRAPPER_H

#include <system/n64system.h>

#ifdef __cplusplus
#include <wsi.hpp>
VkQueue get_graphics_queue();
VkInstance get_vk_instance();
VkPhysicalDevice get_vk_physical_device();
VkDevice get_vk_device();
uint32_t get_vk_graphics_queue_family();
VkFormat get_vk_format();
VkCommandBuffer get_vk_command_buffer();
void submit_requested_vk_command_buffer();
extern "C" {
#endif
    void load_parallel_rdp();
    void update_screen_parallel_rdp();
    void parallel_rdp_enqueue_command(int command_length, word* buffer);
    void parallel_rdp_on_full_sync();
    void update_screen_parallel_rdp_no_game();
    bool is_framerate_unlocked();
    void set_framerate_unlocked(bool unlocked);
#ifdef __cplusplus
};
#endif

#endif //N64_PARALLEL_RDP_WRAPPER_H
