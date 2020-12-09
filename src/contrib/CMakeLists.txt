add_library(glad src/glad.c)
if(NOT WIN32)
target_link_libraries(glad dl)
endif(