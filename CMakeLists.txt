cmake_minimum_required(VERSION 3.16)
project(minimoog_dsp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_TESTS    "Build unit tests"   OFF)
option(BUILD_SIMULATOR "Build PC simulator app" OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Statically link MinGW runtime so the .exe runs without DLLs
    if(MINGW)
        add_link_options(-static-libgcc -static-libstdc++ -static)
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g)
        # ASan/UBSan only on non-MinGW GCC (MinGW lacks libasan/libubsan)
        if(NOT MINGW)
            add_compile_options(-fsanitize=address,undefined)
            add_link_options(-fsanitize=address,undefined)
        endif()
    endif()
endif()

# ── FetchContent ─────────────────────────────────────────
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ── DSP Core (no platform deps) ──────────────────────────
add_library(dsp_core STATIC
    shared/params.cpp
    core/dsp/oscillator.cpp
    core/dsp/moog_filter.cpp
    core/dsp/envelope.cpp
    core/dsp/glide.cpp
    core/dsp/lfo.cpp
    core/engines/moog/voice.cpp
    core/engines/moog/voice_pool.cpp
    core/music/arpeggiator.cpp
    core/music/sequencer.cpp
    core/music/chord_engine.cpp
    core/music/scale_quantizer.cpp
    core/effects/gain_effect.cpp
    core/effects/chorus_effect.cpp
    core/effects/flanger_effect.cpp
    core/effects/phaser_effect.cpp
    core/effects/tremolo_effect.cpp
    core/effects/delay_effect.cpp
    core/effects/reverb_effect.cpp
    core/effects/eq_effect.cpp
    core/effects/effect_chain.cpp
    # V2.2 Multi-Engine + MIDI file player
    core/music/midi_file_player.cpp
    core/engines/engine_manager.cpp
    core/engines/moog/moog_engine.cpp
    core/engines/hammond/hammond_engine.cpp
    core/engines/rhodes/rhodes_engine.cpp
    core/engines/dx7/dx7_engine.cpp
    core/engines/mellotron/mellotron_engine.cpp
    core/engines/drums/drum_engine.cpp
)
target_include_directories(dsp_core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/shared
)

# ── Tests (only need Catch2 + dsp_core) ──────────────────
if(BUILD_TESTS)
    FetchContent_Declare(catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(catch2)

    enable_testing()
    add_subdirectory(tests)
endif()

# ── Simulator (needs GLFW, ImGui, RtAudio, RtMidi, JSON) ─
if(BUILD_SIMULATOR)
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.90.4
        GIT_SHALLOW    TRUE)
    FetchContent_Declare(imgui_knobs
        GIT_REPOSITORY https://github.com/altschuler/imgui-knobs.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE)
    FetchContent_Declare(rtaudio
        GIT_REPOSITORY https://github.com/thestk/rtaudio.git
        GIT_TAG        6.0.1
        GIT_SHALLOW    TRUE)
    FetchContent_Declare(rtmidi
        GIT_REPOSITORY https://github.com/thestk/rtmidi.git
        GIT_TAG        6.0.0
        GIT_SHALLOW    TRUE)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.9
        GIT_SHALLOW    TRUE)
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    # Disable test/example targets to avoid name conflicts between rtaudio & rtmidi
    set(BUILD_TESTING         OFF CACHE BOOL "" FORCE)
    set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    # Force all fetched libs to build as static so no DLLs need to ship alongside .exe
    set(BUILD_SHARED_LIBS     OFF CACHE BOOL "" FORCE)
    # Populate rtmidi manually to avoid add_custom_target(uninstall) conflict
    FetchContent_GetProperties(rtmidi)
    if(NOT rtmidi_POPULATED)
        FetchContent_Populate(rtmidi)
    endif()
    # Build rtmidi as a simple static lib from its source directly
    add_library(rtmidi STATIC ${rtmidi_SOURCE_DIR}/RtMidi.cpp)
    target_include_directories(rtmidi PUBLIC ${rtmidi_SOURCE_DIR})
    if(WIN32)
        target_compile_definitions(rtmidi PRIVATE __WINDOWS_MM__)
        target_link_libraries(rtmidi PUBLIC winmm)
    elseif(APPLE)
        target_compile_definitions(rtmidi PRIVATE __MACOSX_CORE__)
        find_library(COREMIDI_LIB CoreMIDI)
        find_library(COREFOUNDATION_LIB CoreFoundation)
        target_link_libraries(rtmidi PUBLIC ${COREMIDI_LIB} ${COREFOUNDATION_LIB})
    else()
        find_package(ALSA REQUIRED)
        target_compile_definitions(rtmidi PRIVATE __LINUX_ALSA__)
        target_link_libraries(rtmidi PUBLIC ALSA::ALSA)
    endif()

    FetchContent_MakeAvailable(imgui imgui_knobs rtaudio nlohmann_json glfw)

    find_package(OpenGL REQUIRED)

    # ImGui as static lib
    add_library(imgui_lib STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_knobs_SOURCE_DIR}/imgui-knobs.cpp
    )
    target_include_directories(imgui_lib PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${imgui_knobs_SOURCE_DIR}
    )
    target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

    # PC HAL
    add_library(pc_hal STATIC
        hal/pc/rtaudio_backend.cpp
        hal/pc/pc_midi.cpp
        hal/pc/keyboard_input.cpp
        hal/pc/sequencer_pattern_storage.cpp
        hal/pc/effect_preset_storage.cpp
        hal/pc/engine_preset_storage.cpp
        hal/pc/global_preset_storage.cpp
    )
    target_include_directories(pc_hal PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${glfw_SOURCE_DIR}/include
    )
    target_link_libraries(pc_hal PUBLIC
        dsp_core rtaudio rtmidi nlohmann_json::nlohmann_json)

    # UI
    add_library(ui_lib STATIC
        ui/imgui_app.cpp
        ui/panels/engines/panel_engine_selector.cpp
        ui/panels/engines/panel_moog.cpp
        ui/panels/engines/panel_hammond.cpp
        ui/panels/engines/panel_rhodes.cpp
        ui/panels/engines/panel_dx7.cpp
        ui/panels/engines/panel_mellotron.cpp
        ui/panels/engines/panel_drums.cpp
        ui/panels/controls/panel_output.cpp
        ui/panels/controls/panel_music.cpp
        ui/panels/controls/panel_presets.cpp
        ui/panels/controls/panel_effects.cpp
        ui/panels/controls/panel_midi_player.cpp
        ui/panels/analysis/panel_oscilloscope.cpp
        ui/panels/analysis/panel_spectrum.cpp
        ui/panels/analysis/panel_lissajous.cpp
        ui/panels/analysis/panel_vumeter.cpp
        ui/panels/analysis/panel_spectrogram.cpp
        ui/panels/analysis/panel_correlation.cpp
        ui/widgets/knob_widget.cpp
        ui/widgets/adsr_display.cpp
        ui/widgets/keyboard_display.cpp
        ui/widgets/seq_display.cpp
    )
    target_include_directories(ui_lib PUBLIC ${CMAKE_SOURCE_DIR})
    target_link_libraries(ui_lib PUBLIC dsp_core pc_hal imgui_lib)

    # MIDI asset generator tool
    add_executable(gen_midi_assets tools/gen_midi_assets.cpp)
    target_compile_features(gen_midi_assets PRIVATE cxx_std_17)
    set_target_properties(gen_midi_assets PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools)

    # Generate MIDI files into assets/midi/ (runs once, output is source-tracked)
    set(MIDI_ASSET_DIR ${CMAKE_SOURCE_DIR}/assets/midi)
    set(MIDI_STAMP     ${CMAKE_BINARY_DIR}/midi_assets.stamp)
    add_custom_command(
        OUTPUT  ${MIDI_STAMP}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${MIDI_ASSET_DIR}
        COMMAND $<TARGET_FILE:gen_midi_assets> ${MIDI_ASSET_DIR}
        COMMAND ${CMAKE_COMMAND} -E touch ${MIDI_STAMP}
        DEPENDS gen_midi_assets
        COMMENT "Generating classic MIDI assets..."
    )
    add_custom_target(midi_assets ALL DEPENDS ${MIDI_STAMP})

    # Simulator executable
    add_executable(minimoog_sim sim/main.cpp)
    target_link_libraries(minimoog_sim PRIVATE dsp_core pc_hal ui_lib)
    set_target_properties(minimoog_sim PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    add_dependencies(minimoog_sim midi_assets)

    # Copy all assets next to the executable after build
    add_custom_command(TARGET minimoog_sim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/moog_presets
            $<TARGET_FILE_DIR:minimoog_sim>/moog_presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/sequencer_patterns
            $<TARGET_FILE_DIR:minimoog_sim>/sequencer_patterns
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/effect_presets
            $<TARGET_FILE_DIR:minimoog_sim>/effect_presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/hammond_presets
            $<TARGET_FILE_DIR:minimoog_sim>/hammond_presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/rhodes_presets
            $<TARGET_FILE_DIR:minimoog_sim>/rhodes_presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/dx7_presets
            $<TARGET_FILE_DIR:minimoog_sim>/dx7_presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/mellotron_presets
            $<TARGET_FILE_DIR:minimoog_sim>/mellotron_presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/drum_kits
            $<TARGET_FILE_DIR:minimoog_sim>/drum_kits
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/global_presets
            $<TARGET_FILE_DIR:minimoog_sim>/global_presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/midi
            $<TARGET_FILE_DIR:minimoog_sim>/midi
        COMMENT "Copying all presets, patterns and assets to output directory"
    )
endif()
