// Dear ImGui: standalone example application for SDL2 + Vulkan
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important note to the reader who wish to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
// - Common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
//   You will use those if you want to use this rendering backend in your engine/app.
// - Helper ImGui_ImplVulkanH_XXX functions and structures are only used by this example (main.cpp) and by
//   the backend itself (imgui_impl_vulkan.cpp), but should PROBABLY NOT be used by your own engine/app code.
// Read comments in imgui_impl_vulkan.h.

#include "imgui_ui.h"
#include <rdp/parallel_rdp_wrapper.h>
#include <volk.h>
#include <imgui.h>
#include <imfilebrowser.h>
#include <implot.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>
#include <cstdio>          // printf, fprintf
#include <cstdlib>         // abort

#include <frontend/render_internal.h>
#include <metrics.h>
#include <SDL.h>
#include <mem/pif.h>

static bool show_metrics_window = false;
static bool show_imgui_demo_window = false;

#define METRICS_HISTORY_SECONDS 10

#define METRICS_HISTORY_ITEMS ((METRICS_HISTORY_SECONDS) * 60)

template<typename T>
struct RingBuffer {
    int offset;
    T max;
    T data[METRICS_HISTORY_ITEMS];
    RingBuffer() {
        offset = 0;
        max = 0;
        memset(data, 0, sizeof(data));
    }
    void add_point(T point) {
        if (point > max) {
            max = point;
        }
        data[offset++] = point;
        offset %= METRICS_HISTORY_ITEMS;
    }
};

RingBuffer<double> frame_times;
RingBuffer<ImU64> block_complilations;
RingBuffer<ImU64> rsp_steps;

ImGui::FileBrowser fileBrowser;

void render_menubar() {
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load ROM")) {
                fileBrowser.Open();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Show Metrics", nullptr, show_metrics_window)) { show_metrics_window = !show_metrics_window; }
            if (ImGui::MenuItem("Show ImGui Demo Window", nullptr, show_imgui_demo_window)) { show_imgui_demo_window = !show_imgui_demo_window; }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void render_metrics_window() {
    block_complilations.add_point(get_metric(METRIC_BLOCK_COMPILATION));
    rsp_steps.add_point(get_metric(METRIC_RSP_STEPS));
    double frametime = 1000.0f / ImGui::GetIO().Framerate;
    frame_times.add_point(frametime);

    ImGui::Begin("Performance Metrics");
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", frametime, ImGui::GetIO().Framerate);

    ImPlot::SetNextPlotLimitsY(0, frame_times.max, ImGuiCond_Always, 0);
    ImPlot::SetNextPlotLimitsX(0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
    if (ImPlot::BeginPlot("Frame Times")) {
        ImPlot::PlotLine("Frame Time (ms)", frame_times.data, METRICS_HISTORY_ITEMS, 1, 0, frame_times.offset);
        ImPlot::EndPlot();
    }

    ImPlot::SetNextPlotLimitsY(0, rsp_steps.max, ImGuiCond_Always, 0);
    ImPlot::SetNextPlotLimitsX(0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
    if (ImPlot::BeginPlot("RSP Steps Per Frame")) {
        ImPlot::PlotLine("RSP Steps", rsp_steps.data, METRICS_HISTORY_ITEMS, 1, 0, rsp_steps.offset);
        ImPlot::EndPlot();
    }

    ImGui::Text("Block compilations this frame: %ld", get_metric(METRIC_BLOCK_COMPILATION));
    ImPlot::SetNextPlotLimitsY(0, block_complilations.max, ImGuiCond_Always, 0);
    ImPlot::SetNextPlotLimitsX(0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
    if (ImPlot::BeginPlot("Block Compilations Per Frame")) {
        ImPlot::PlotBars("Block compilations", block_complilations.data, METRICS_HISTORY_ITEMS, 1, 0, block_complilations.offset);
        ImPlot::EndPlot();
    }

    ImGui::End();
}

void render_ui() {
    if (SDL_GetMouseFocus() || global_system->mem.rom.rom == nullptr) {
        render_menubar();
    }
    if (show_metrics_window) { render_metrics_window(); }
    if (show_imgui_demo_window) { ImGui::ShowDemoWindow(); }

    fileBrowser.Display();
    if (fileBrowser.HasSelected()) {
        reset_n64system(global_system);
        n64_load_rom(global_system, fileBrowser.GetSelected().string().c_str());
        pif_rom_execute(global_system);
        fileBrowser.ClearSelected();
    }
}

static VkAllocationCallbacks*   g_Allocator = NULL;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = (uint32_t)-1;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_MinImageCount = 2;

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void load_imgui_ui() {
    VkResult err;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    g_Instance = get_vk_instance();
    g_PhysicalDevice = get_vk_physical_device();
    g_Device = get_vk_device();
    g_QueueFamily = get_vk_graphics_queue_family();
    g_Queue = get_graphics_queue();
    g_PipelineCache = nullptr;
    g_DescriptorPool = nullptr;
    g_Allocator = nullptr;
    g_MinImageCount = 2;


    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] =
                {
                        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
        check_vk_result(err);
    }

    // Create the Render Pass
    VkRenderPass renderPass;
    {
        VkAttachmentDescription attachment = {};
        attachment.format = get_vk_format();
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        //attachment.loadOp = wd->ClearEnable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        err = vkCreateRenderPass(g_Device, &info, g_Allocator, &renderPass);
        check_vk_result(err);

        // We do not create a pipeline by default as this is also used by examples' main.cpp,
        // but secondary viewport in multi-viewport mode may want to create one with:
        //ImGui_ImplVulkan_CreatePipeline(device, allocator, VK_NULL_HANDLE, wd->RenderPass, VK_SAMPLE_COUNT_1_BIT, &wd->Pipeline, g_Subpass);
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.Allocator = g_Allocator;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = 2;
    init_info.CheckVkResultFn = check_vk_result;

    ImGui_ImplVulkan_Init(&init_info, renderPass);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Upload Fonts
    {
        // Use any command queue
        VkCommandBuffer command_buffer = get_vk_command_buffer();

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(command_buffer, &begin_info);
        check_vk_result(err);

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;
        err = vkEndCommandBuffer(command_buffer);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE);
        check_vk_result(err);

        err = vkDeviceWaitIdle(g_Device);
        check_vk_result(err);
        ImGui_ImplVulkan_DestroyFontUploadObjects();

        submit_requested_vk_command_buffer();
    }

    fileBrowser.SetTitle("Load ROM");
    fileBrowser.SetTypeFilters({".n64", ".v64", ".z64", ".N64", ".V64", ".Z64"});
}

ImDrawData* imgui_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    render_ui();

    // Rendering
    ImGui::Render();
    return ImGui::GetDrawData();
}