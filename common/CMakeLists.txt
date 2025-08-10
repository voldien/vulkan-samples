FILE(GLOB SOURCE_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/Util/*.cpp)
FILE(GLOB HEADER_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
						${CMAKE_CURRENT_SOURCE_DIR}/Util/*.h)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
INCLUDE(assimp)
INCLUDE(libcxxopts)

ADD_LIBRARY(vk-sample-common-window-backend-sdl2 OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/WindowBackend/SDLWindow.cpp)
TARGET_LINK_LIBRARIES(vk-sample-common-window-backend-sdl2  PUBLIC fragcore-rvk ${SDL2_LIBRARIES} )


ADD_LIBRARY(vk-sample-common ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(vk-sample-common PUBLIC fmt fragcore-rvk cxxexcept cxxopts vk-sample-common-window-backend-sdl2 spirv-cross-c fragcore-scheduler-tasksch fragcore-geometry-util fragcore-imageloader fragcore-shader-compiler)
ADD_DEPENDENCIES(vk-sample-common spirv-cross-c fragcore-rvk cxxopts Shaders fragcore-imageloader fragcore-shader-compiler)

TARGET_COMPILE_DEFINITIONS(vk-sample-common PRIVATE FLYTHROUGH_CAMERA_IMPLEMENTATION=1)

IF(OpenMP_FOUND AND OpenMP_CXX_FOUND)
	TARGET_LINK_LIBRARIES(vk-sample-common PRIVATE m pthread PUBLIC OpenMP::OpenMP_CXX)
	TARGET_COMPILE_OPTIONS(vk-sample-common PUBLIC ${OpenMP_CXX_FLAGS})
ENDIF()

IF (BUILD_SHARED_LIBS AND CMAKE_SIZEOF_VOID_P EQUAL 8) # -fPIC is only required
	SET_PROPERTY(TARGET vk-sample-common PROPERTY POSITION_INDEPENDENT_CODE ON)
ENDIF()

TARGET_INCLUDE_DIRECTORIES(vk-sample-common
    PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:vk-sample-common>
	PRIVATE  ${CMAKE_SOURCE_DIR}/extern
)

INSTALL(TARGETS vk-sample-common fragcore-rvk fragcore-imageloader fragcore-shader-compiler DESTINATION lib)
