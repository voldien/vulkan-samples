FILE(GLOB VKS_CORE_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Core/*.cpp)
FILE(GLOB VKS_CORE_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Core/*.h )

FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

#
IF(PKG_CONFIG_FOUND)
	MESSAGE(STATUS "Found pkg-config: ${PKG_CONFIG_EXECUTABLE} -- version ${PKG_CONFIG_VERSION_STRING}.")
	PKG_CHECK_MODULES(SDL2 QUIET sdl2)
	PKG_CHECK_MODULES(GLFW3 QUIET glfw3)
ELSE()
	# TODO add logic for handling if pkg-config not available.
ENDIF()

IF(SDL2_FOUND)
	MESSAGE(STATUS "Found sdl2: ${SDL2_LIBRARIES} -- version ${SDL2_VERSION}.")
ENDIF()

IF(GLFW3_FOUND)
	MESSAGE(STATUS "Found glfw3: ${GLFW3_LIBRARIES} -- version ${GLFW3_VERSION}.")
ENDIF()


ADD_LIBRARY(vk-sample-common-window-backend-sdl2 OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/WindowBackend/SDLWindow.cpp)
ADD_LIBRARY(vk-sample-common-window-backend-glfw3 OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/WindowBackend/GLFW3Window.cpp)
TARGET_LINK_LIBRARIES(vk-sample-common-window-backend-sdl2  PUBLIC fragcore-rvk ${SDL2_LIBRARIES} )
TARGET_LINK_LIBRARIES(vk-sample-common-window-backend-glfw3  PUBLIC fragcore-rvk ${GLFW3_LIBRARIES} )



ADD_LIBRARY(vk-sample-common ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(vk-sample-common PUBLIC fmt fragcore-rvk cxxexcept cxxopts vk-sample-common-window-backend-sdl2 spirv-cross-c)
ADD_DEPENDENCIES(vk-sample-common spirv-cross-c fragcore-rvk cxxopts Shaders)

IF (BUILD_SHARED_LIBS AND CMAKE_SIZEOF_VOID_P EQUAL 8) # -fPIC is only required
	SET_PROPERTY(TARGET vk-sample-common PROPERTY POSITION_INDEPENDENT_CODE ON)
ENDIF()

TARGET_INCLUDE_DIRECTORIES(vk-sample-common PRIVATE ${Vulkan_INCLUDE_DIRS}
    PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:vk-sample-common>
)

INSTALL(TARGETS vk-sample-common fragcore-rvk DESTINATION lib)
