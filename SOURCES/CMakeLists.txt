cmake_minimum_required(VERSION 3.14)

# Make PROJECT_SOURCE_DIR, PROJECT_BINARY_DIR, and PROJECT_NAME available.
set(PROJECT_NAME vertica-udfs)
project(${PROJECT_NAME})

#####################
##     OPTIONS     ##
#####################

option(VEC_REPORT "Display compilation vectorization reports." OFF)
option(DEBUG_BUILD "Attach debug information to the executables." OFF)
option(BUILD_VERTICA_TEST_DRIVER "Build a test program to show basic functionality of the underlying HLL algorithm" OFF)
option(BUILD_TESTS "Build all tests." OFF)

# Here we say where g++ should look for include files
set(SDK_HOME /opt/vertica/sdk CACHE FILEPATH "Path to the Vertica SDK, by default /opt/vertica/sdk")
if(NOT EXISTS ${SDK_HOME})
  message(FATAL_ERROR "Could not build. No SDK found at ${SDK_HOME} (maybe retry with -DSDK_HOME=<sdk_path>).")
endif()
set(VERTICA_INCLUDE ${SDK_HOME}/include)

## When building for Vertica 12, you need to set specific defines
file(READ "${VERTICA_INCLUDE}/BuildInfo.h" buildinfosdk)
string(REGEX MATCH "#define[ ]+VERTICA_BUILD_ID_SDK_Version_Major[ ]+([0-9]+)" _ ${buildinfosdk})
set(ver_major ${CMAKE_MATCH_1})
message("Version", ${ver_major})
if (${ver_major} LESS_EQUAL 12)
  add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
  message("Using CXX11 ABI because Vertica <= 12:", ${ver_major})
else()
  message("Not using CXX11 ABI because newer SDK than Vertica 12", ${ver_major})
endif()

## For Vertica 24, you don't need it.


#####################
##  COMPILE FLAGS  ##
#####################

set(CMAKE_CXX_FLAGS "-Wall -O3 -std=c++11 -Wno-unused-value")

if(DEBUG_BUILD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

if(VEC_REPORT)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopt-info-vec-all=vecinfo.txt -ftree-vectorizer-verbose=7")
endif()

#####################
##  BUILD TARGETS  ##
#####################
include_directories(${VERTICA_INCLUDE} include src)

# Here we add all source files to appear in libhll.so
set(SOURCES src/hll-druid/hll_druid.cpp ${VERTICA_INCLUDE}/Vertica.h ${VERTICA_INCLUDE}/BuildInfo.h ${VERTICA_INCLUDE}/Vertica.cpp)

add_library(hlldruid SHARED ${SOURCES} src/hll-druid/HllDruidCombine.cpp src/hll-druid/HllDruidCreateSynopsis.cpp src/hll-druid/HllDruidDistinctCount.cpp src/hll-druid/murmur3_hash.cpp)

#set_target_properties(hlldruid PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
# do not use -march=native UNLESS you are building on your target machine itself
set_target_properties(hlldruid PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS}")
# Installation process just copies the binary HLL to Vertica lib folder
if (INSTALL_PREFIX_PATH)
  set(LIB_DIR ${INSTALL_PREFIX_PATH}/opt/vertica/lib)
else()
  set(LIB_DIR /opt/vertica/lib)
endif()

install(
  TARGETS hlldruid
  DESTINATION ${LIB_DIR}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

if (BUILD_VERTICA_TEST_DRIVER)
  add_executable(hll_druid_driver tests/hll-druid/hll_druid_driver.cpp src/hll-druid/hll_druid.cpp src/hll-druid/murmur3_hash.cpp)
endif()

add_custom_target(check COMMAND ctest -V)
add_test(hll_test hll_test)

if (BUILD_TESTS)
  # This adds another subdirectory, which has 'project(gtest)'.
  # Thanks to EXCLUDE_FROM_ALL google test will not be installed
  # when running "make install"
  add_subdirectory(lib/googletest-1.7.0 EXCLUDE_FROM_ALL)

  enable_testing()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSOURCE_PATH='\"${CMAKE_CURRENT_LIST_DIR}\"'")
  # Include the gtest library. gtest_SOURCE_DIR is available due to
  # inclusion of googletest subdirectory with another cmake file
  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} src)

  # Linking to Hll.cpp. We can't link to libhll.so, because there would
  # be some symbols missing, e.g. Vertica::dummy()
  add_executable(hll_test tests/hll-druid/hll_druid_test.cpp tests/hll_utils_test.cpp src/hll-druid/hll_druid.cpp src/hll-druid/murmur3_hash.cpp)
  add_dependencies(check hll_test)
  # Standard linking to googletest stuff.
  target_link_libraries(hll_test gtest gtest_main)

  # Thanks to this one can run `make test' to run all the tests.
  # Every test to be run has to be added here
  # add_test(NAME that-test-I-made COMMAND runUnitTests)
endif()

