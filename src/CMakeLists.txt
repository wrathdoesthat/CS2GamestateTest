set(CMAKE_CXX_STANDARD 23)

add_executable(GamestateTest 
    main.cc

    CS2GamestateData.hh
    CS2GamestateAPI.hh
    GamestateThread.hh
    LogWatchThread.hh
    ConfigInteraction.hh
    ConfigInteraction.cc
    Config.hh
    Config.cc
    UpdateEvents.hh
    UpdateEvents.cc

    json.hpp
    json_fwd.hpp
)

set(SPDLOG_WCHAR_TO_UTF8_SUPPORT ON)
target_link_libraries(GamestateTest wsock32 ws2_32 spdlog::spdlog "${CMAKE_SOURCE_DIR}/ext/lua-5.4.2_Win64_vc17_lib/lua54.lib")

target_include_directories(GamestateTest PRIVATE
    "${CMAKE_SOURCE_DIR}/ext/lua-5.4.2_Win64_vc17_lib/include"
    "${CMAKE_SOURCE_DIR}/ext/sol"
)

set_target_properties(GamestateTest
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

make_directory("${CMAKE_SOURCE_DIR}/bin/Debug")
make_directory("${CMAKE_SOURCE_DIR}/bin/Release")

add_custom_target(copy-defaultconfig ALL
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/config.json
    ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>/
)

add_dependencies(GamestateTest copy-defaultconfig)