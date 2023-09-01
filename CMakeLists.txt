cmake_minimum_required(VERSION 3.10)
project(xtypes VERSION 2.0.0 DESCRIPTION "X-Rock Types")
include(${CMAKE_SOURCE_DIR}/cmake/xtypes-dependencies.cmake)
xtypes_project()

if(APPLE)
target_link_libraries(${XTYPES_CPP_TARGET} PUBLIC
	PkgConfig::libgit2
  PkgConfig::cpr
)
else(APPLE)
target_link_libraries(${XTYPES_CPP_TARGET} PUBLIC "-Wl,-no-undefined"
  pantor::inja
  PkgConfig::libgit2
  PkgConfig::cpr
  stdc++fs
)
endif()

# Export the library interface
install(EXPORT xtypes-targets
	NAMESPACE xtypes::
	DESTINATION lib/cmake/xtypes
)

# Create and install the version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file("xtypes-config-version.cmake"
	VERSION ${xtypes_VERSION}
	COMPATIBILITY SameMajorVersion
)
configure_file(${CMAKE_SOURCE_DIR}/cmake/xtypes-config.cmake.in  ${CMAKE_BINARY_DIR}/cmake/xtypes-config.cmake @ONLY)
install(
	FILES
  ${CMAKE_BINARY_DIR}/cmake/xtypes-config.cmake
  ${CMAKE_SOURCE_DIR}/cmake/xtypes-dependencies.cmake
	${PROJECT_BINARY_DIR}/xtypes-config-version.cmake
	DESTINATION
		lib/cmake/xtypes
)

# Install pkg-config file
configure_file(pkg-config-file.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig/)


################
# Executables #
##############
FILE(GLOB EXECUTABLE_SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/bin/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bin/*.py
)

foreach(EXECUTABLE_SOURCE ${EXECUTABLE_SOURCE_FILES})
  string(REGEX REPLACE ".*/bin/" "" EXECUTABLE_NAME ${EXECUTABLE_SOURCE})
  if(${EXECUTABLE_SOURCE} MATCHES ".cpp$")
    string(REPLACE ".cpp" "" EXECUTABLE_NAME ${EXECUTABLE_NAME})
    message(STATUS "Building & installing executable ${EXECUTABLE_NAME} from ${EXECUTABLE_SOURCE}")
    add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SOURCE})
    add_dependencies(${EXECUTABLE_NAME} ${XTYPES_CPP_TARGET})
    target_link_libraries(${EXECUTABLE_NAME}
        pthread
        ${PKGCONFIG${PROJECT_NAME}_LIBRARIES}
        ${XTYPES_CPP_TARGET}
    )
    target_link_directories(${EXECUTABLE_NAME} PUBLIC ${PKGCONFIG${PROJECT_NAME}_LIBRARY_DIRS})
    install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)
  elseif(${EXECUTABLE_SOURCE} MATCHES ".py$")
    string(REPLACE ".py" "" EXECUTABLE_NAME ${EXECUTABLE_NAME})
    message(STATUS "Installing python executable ${EXECUTABLE_NAME} from ${EXECUTABLE_SOURCE}")
    configure_file(${EXECUTABLE_SOURCE} ${CMAKE_BINARY_DIR}/${EXECUTABLE_NAME} COPYONLY)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/${EXECUTABLE_NAME} DESTINATION bin/)
  endif()
endforeach()



#########
# Tests #
#########
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test EXCLUDE_FROM_ALL)


#################
# Documentation #
#################
configure_file(${CMAKE_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_BINARY_DIR}/doc/Doxyfile @ONLY)
add_custom_target(doc
  COMMAND doxygen ${CMAKE_BINARY_DIR}/doc/Doxyfile
  DEPENDS ${XTYPES_CPP_TARGET}
)
