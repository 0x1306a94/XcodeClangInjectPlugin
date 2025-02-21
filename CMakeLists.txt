if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(BUILD_CODE_STYLE_OUTSIDE_LLVM ON)
else()
  # used when build inside the clang tool/extra folder
  set(BUILD_CODE_STYLE_OUTSIDE_LLVM OFF)
  cmake_minimum_required(VERSION 3.20.0)
  project(CheckerCodeStylePlugin)
  message(STATUS "Build out of LLVM project")

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  message(STATUS "LLVM_SOURCE_DIR: ${LLVM_SOURCE_DIR}")
  message(STATUS "LLVM_BINARY_DIR: ${LLVM_BINARY_DIR}")
  message(STATUS "LLVM_EXTERNAL_CLANG_SOURCE_DIR: ${LLVM_EXTERNAL_CLANG_SOURCE_DIR}")

  include_directories(BEFORE
    ${LLVM_SOURCE_DIR}/include
    ${LLVM_EXTERNAL_CLANG_SOURCE_DIR}/include
    ${LLVM_BINARY_DIR}/include
    ${LLVM_BINARY_DIR}/tools/clang/include
  )

endif()

message(STATUS "BUILD_CODE_STYLE_OUTSIDE_LLVM: ${BUILD_CODE_STYLE_OUTSIDE_LLVM}")
add_llvm_library(CheckerCodeStylePlugin MODULE
  plugin/CodeStyle.cpp

  PLUGIN_TOOL
  clang
)

if(NOT BUILD_CODE_STYLE_OUTSIDE_LLVM)
  add_dependencies(CheckerCodeStylePlugin clang-tablegen-targets)
endif()

if(WIN32 OR CYGWIN)
  set(LLVM_LINK_COMPONENTS
    Support
  )
  clang_target_link_libraries(CheckerCodeStylePlugin PRIVATE
    clangAST
    clangBasic
    clangFrontend
    clangLex
    )
endif()

if(APPLE)
  
  add_llvm_library(CheckerCodeStylePluginLoader SHARED
    loader/Loader.cpp
  )

  add_dependencies(CheckerCodeStylePluginLoader CheckerCodeStylePlugin)

  set_target_properties(CheckerCodeStylePluginLoader PROPERTIES
    SKIP_BUILD_RPATH TRUE
    SKIP_INSTALL_RPATH TRUE
    # INSTALL_RPATH_USE_LINK_PATH FALSE
    # BUILD_WITH_INSTALL_RPATH FALSE
  )

  # target_link_options(CheckerCodeStylePlugin PRIVATE -undefined PRIVATE dynamic_lookup PRIVATE -flat_namespace)
  target_link_options(CheckerCodeStylePluginLoader PRIVATE -undefined PRIVATE dynamic_lookup PRIVATE -flat_namespace)
  target_include_directories(CheckerCodeStylePluginLoader PRIVATE ${CMAKE_CURRENT_LIST_DIR}/dobby)
  target_include_directories(CheckerCodeStylePluginLoader PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
  target_link_libraries(CheckerCodeStylePluginLoader ${CMAKE_CURRENT_LIST_DIR}/dobby/libdobby.a)

  add_custom_target(install_CheckerCodeStylePluginLoader ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/lib
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CheckerCodeStylePlugin> ${CMAKE_INSTALL_PREFIX}/lib
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CheckerCodeStylePluginLoader> ${CMAKE_INSTALL_PREFIX}/lib
    COMMENT "Installing CheckerCodeStylePluginLoader to ${CMAKE_INSTALL_PREFIX}/lib"
    DEPENDS CheckerCodeStylePluginLoader
  )
 
else()

endif(APPLE)

if(UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS "-fPIC")
endif()
