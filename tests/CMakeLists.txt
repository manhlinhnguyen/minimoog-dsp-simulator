add_executable(minimoog_tests
    test_main.cpp
    test_oscillator.cpp
    test_moog_filter.cpp
    test_envelope.cpp
    test_glide.cpp
    test_voice.cpp
    test_arpeggiator.cpp
    test_sequencer.cpp
    test_scale_quantizer.cpp
)
target_link_libraries(minimoog_tests PRIVATE
    dsp_core Catch2::Catch2WithMain)
target_include_directories(minimoog_tests PRIVATE
    ${CMAKE_SOURCE_DIR})

include(CTest)
include(Catch)
catch_discover_tests(minimoog_tests)
