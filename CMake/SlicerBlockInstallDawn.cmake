# -------------------------------------------------------------------------
# Find and install Dawn (WebGPU implementation used by VTK::RenderingWebGPU)
# -------------------------------------------------------------------------

if(NOT EXISTS "${Dawn_BIN_DIR}" AND WIN32)
  message(FATAL_ERROR "CMake variable Dawn_BIN_DIR is set to a nonexistent directory: ${Dawn_BIN_DIR}")
endif()
if(NOT EXISTS "${Dawn_LIB_DIR}")
  message(FATAL_ERROR "CMake variable Dawn_LIB_DIR is set to a nonexistent directory: ${Dawn_LIB_DIR}")
endif()

if(WIN32)
  install(FILES ${Dawn_BIN_DIR}/webgpu_dawn.dll DESTINATION bin COMPONENT Runtime)
elseif(APPLE)
  slicerInstallLibrary(FILE ${Dawn_LIB_DIR}/libwebgpu_dawn.dylib
    DESTINATION ${Slicer_INSTALL_LIB_DIR} COMPONENT Runtime)
elseif(UNIX)
  slicerInstallLibrary(FILE ${Dawn_LIB_DIR}/libwebgpu_dawn.so
    DESTINATION ${Slicer_INSTALL_LIB_DIR} COMPONENT Runtime STRIP)
endif()
