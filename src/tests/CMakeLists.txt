add_subdirectory(demo_konsolepart)

include(ECMMarkAsTest)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

set(KONSOLE_TEST_LIBS Qt::Test konsoleprivate)

add_executable(PartManualTest PartManualTest.cpp)
ecm_mark_as_test(PartManualTest)
target_link_libraries(PartManualTest KF6::XmlGui KF6::Parts ${KONSOLE_TEST_LIBS})

