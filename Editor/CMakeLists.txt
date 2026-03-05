
if(WIN32)
add_executable(Editor WIN32 "Source/main.cpp" Source/App.cpp Source/Editor.cpp Source/SimpleCamera.cpp Source/Window.cpp )
# Add Windows-specific configurations here
elseif(UNIX)
add_executable(Editor "Source/main.cpp" Source/App.cpp Source/Editor.cpp Source/SimpleCamera.cpp Source/Window.cpp )
# Add Unix-specific configurations here
else()
message(STATUS "Unknown Operating System")
endif()

include_directories(${CMAKE_SOURCE_DIR}/Editor/Headers)
target_link_options(Editor PRIVATE ${SUBSYSTEM_LINKER_OPTIONS})
target_link_libraries(Editor DSECommon)