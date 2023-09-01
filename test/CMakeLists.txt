cmake_minimum_required(VERSION 3.5)
add_executable(xtypes_test EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests.cpp
)

target_include_directories(xtypes_test
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(xtypes_test PUBLIC cxx_std_17) # Use C++17
# from cmake3.13 target_link_options(xtypes_test PUBLIC "-Wl,--disable-new-dtags") # used so that we test the file in build not the installed lib
if(APPLE)
  target_link_libraries(xtypes_test ${PKGCONFIGtest_LIBRARIES} ${CMAKE_BINARY_DIR}/lib${XTYPES_CPP_TARGET}.dylib) # Link order matters xtype/.
else(APPLE)
  target_link_libraries(xtypes_test PUBLIC
		nlohmann_json::nlohmann_json
		${XTYPES_CPP_TARGET}
		"-Wl,--disable-new-dtags"
  )
endif()

if(Catch_FOUND)
  catch_discover_tests(xtypes_test)
endif()

add_custom_target(cpp_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/xtypes_test
    DEPENDS xtypes_test
)

### PYTHON Test ###
if("$ENV{PYTHON}" STREQUAL "")
    set(PYTHON "python3")
    message(STATUS "Using default python.")
else()
    set(PYTHON $ENV{PYTHON})
    message(STATUS ${PYTHON})
endif()

add_custom_target(py_test
    COMMAND ${PYTHON} -m unittest "test_*.py"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${XTYPES_PY_TARGET}
)

