add_library(imgui
        imgui.cpp
        imgui_widgets.cpp
        imgui_draw.cpp
        imgui_tables.cpp

        imgui_demo.cpp

        implot_items.cpp
        implot.cpp implot.h implot_internal.h

        imfilebrowser.h

        imgui_impl_vulkan.cpp imgui_impl_vulkan.h
        imgui_impl_sdl.cpp imgui_impl_sdl.h

        imstb_rectpack.h
        imstb_textedit.h
        imstb_truetype.h
        imgui.h
        imgui_internal.h)

target_include_directories(imgui PUBLIC
        ../parallel-rdp/parallel-rdp-standalone/volk
        ../parallel-rdp/parallel-rdp-standalone/vulkan-headers/include)
