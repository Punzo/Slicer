
set(proj Dawn)

# Set dependency list
set(${proj}_DEPENDENCIES "")

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(Slicer_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling Slicer_USE_SYSTEM_${proj} is not supported !")
endif()

# Sanity checks
if(DEFINED Dawn_DIR AND NOT EXISTS ${Dawn_DIR})
  message(FATAL_ERROR "Dawn_DIR variable is defined but corresponds to nonexistent directory")
endif()
if(DEFINED Dawn_BIN_DIR AND NOT EXISTS ${Dawn_BIN_DIR})
  message(FATAL_ERROR "Dawn_BIN_DIR variable is defined but corresponds to nonexistent directory")
endif()
if(DEFINED Dawn_LIB_DIR AND NOT EXISTS ${Dawn_LIB_DIR})
  message(FATAL_ERROR "Dawn_LIB_DIR variable is defined but corresponds to nonexistent directory")
endif()

if((NOT DEFINED Dawn_DIR
    OR NOT DEFINED Dawn_BIN_DIR
    OR NOT DEFINED Dawn_LIB_DIR)
    AND NOT Slicer_USE_SYSTEM_${proj})

  # Dawn (WebGPU implementation) is used by the VTK::RenderingWebGPU module.
  # The tag must match the Dawn version used by the VTK continuous integration
  # (dawn_version in VTK/.gitlab/ci/download_dawn.cmake): the tag documented
  # in VTK/Rendering/WebGPU/README.md is outdated for VTK 9.6.

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
  set(EP_INSTALL_DIR ${CMAKE_BINARY_DIR}/${proj}-install)

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/google/dawn.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_TAG
    "v20251002.162335" # 01940842b667a7812d0e4ca0ef4367fbec294241
    QUIET
    )

  # Only the native backend of each platform is built (Vulkan on Linux, D3D12
  # on Windows, Metal on macOS): the OpenGL backends are not needed and window
  # system integration (X11, Wayland, GLFW) is disabled because VTK is only
  # used here for offscreen work (e.g. compute shaders).
  set(_dawn_platform_args)
  if(UNIX AND NOT APPLE)
    list(APPEND _dawn_platform_args
      -DDAWN_USE_X11:BOOL=OFF
      -DDAWN_USE_WAYLAND:BOOL=OFF
      )
  endif()

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${Slicer_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${Slicer_${proj}_GIT_TAG}"
    GIT_SHALLOW TRUE
    # The git submodules (SwiftShader, LLVM, Vulkan validation layers, ...)
    # are not needed: DAWN_FETCH_DEPENDENCIES fetches only the required ones.
    GIT_SUBMODULES ""
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    INSTALL_DIR ${EP_INSTALL_DIR}
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DCMAKE_INSTALL_PREFIX:PATH=${EP_INSTALL_DIR}
      -DCMAKE_INSTALL_LIBDIR:STRING=lib  # Override value set in GNUInstallDirs CMake module
      -DCMAKE_INSTALL_BINDIR:STRING=bin
      -DDAWN_FETCH_DEPENDENCIES:BOOL=ON
      -DDAWN_ENABLE_INSTALL:BOOL=ON
      -DDAWN_BUILD_MONOLITHIC_LIBRARY:STRING=SHARED
      -DDAWN_ENABLE_PIC:BOOL=ON
      -DDAWN_USE_GLFW:BOOL=OFF
      -DDAWN_ENABLE_DESKTOP_GL:BOOL=OFF
      -DDAWN_ENABLE_OPENGLES:BOOL=OFF
      -DDAWN_BUILD_SAMPLES:BOOL=OFF
      -DDAWN_BUILD_TESTS:BOOL=OFF
      -DTINT_BUILD_TESTS:BOOL=OFF
      -DTINT_BUILD_CMD_TOOLS:BOOL=OFF
      ${_dawn_platform_args}
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

  set(Dawn_DIR ${EP_INSTALL_DIR}/lib/cmake/Dawn)
  set(Dawn_BIN_DIR ${EP_INSTALL_DIR}/bin)
  set(Dawn_LIB_DIR ${EP_INSTALL_DIR}/lib)

  #-----------------------------------------------------------------------------
  # Launcher setting specific to build tree

  if(WIN32)
    set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD "${Dawn_BIN_DIR}")
  else()
    set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD "${Dawn_LIB_DIR}")
  endif()
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    LABELS "LIBRARY_PATHS_LAUNCHER_BUILD"
    )

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

mark_as_superbuild(
  VARS
    Dawn_BIN_DIR:PATH
    Dawn_LIB_DIR:PATH
  )

ExternalProject_Message(${proj} "Dawn_DIR:${Dawn_DIR}")
mark_as_superbuild(
  VARS Dawn_DIR:PATH
  LABELS "FIND_PACKAGE"
  )
