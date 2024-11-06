CMAKE_MINIMUM_REQUIRED( VERSION 3.8 )
PROJECT(vulkan-samples)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "" FORCE)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE INTERNAL "" FORCE)


OPTION(BUILD_SHARED_LIBS "Build package with shared libraries." ON)
IF(NOT BUILD_SHARED_LIBS)
	SET(CMAKE_EXE_LINKER_FLAGS "-static")
	SET(LINK_SEARCH_START_STATIC TRUE)
ELSE()
	MESSAGE(STATUS "Shared libraries enabled")
ENDIF(NOT BUILD_SHARED_LIBS)

OPTION(BUILD_WITH_ASAN "Enable AddressSanitizer." OFF )
OPTION(BUILD_WITH_UBSAN "Enable Undefined Behavior sanitizer." OFF )
OPTION(BUILD_WITH_TSAN "Enable Thread sanitizer" OFF)


IF(CMAKE_BUILD_TYPE STREQUAL "Release")
	MESSAGE(STATUS "Compile for release.")
	ADD_DEFINITIONS(-O2)
	SET(CMAKE_RELEASE TRUE)
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s ${OpenMP_CXX_FLAGS}")
	SET(SIMD_FLAGS "-ftree-vectorize -ftree-slp-vectorize -fopt-info-vec-optimized")
ELSE()
	MESSAGE(STATUS "Compile for debug.")
	ADD_DEFINITIONS(-g3 -O0)
	SET(CMAKE_DEBUG TRUE)
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} -s ${OpenMP_CXX_FLAGS}")
	SET(SIMD_FLAGS "-ftree-vectorize -ftree-slp-vectorize -ftree-vectorizer-verbose=10 -fopt-info-vec-optimized")
ENDIF()


IF (BUILD_SHARED_LIBS AND CMAKE_SIZEOF_VOID_P EQUAL 8) # -fPIC is only required
	SET (CMAKE_POSITION_INDEPENDENT_CODE TRUE)
ELSE()
	SET (CMAKE_POSITION_INDEPENDENT_CODE FALSE)
ENDIF()

# AVX If supported
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${AVX_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${AVX_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${AVX_FLAGS}")

# SIMD
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SIMD_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SIMD_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SIMD_FLAGS}")


############################################
# Backend libraries
############################################
SET(MIMI_VULKAN_SUPPORT ON)
SET(MIMI_OPENGL_SUPPORT OFF)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/image/imageloader EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/taskscheduler/TaskSch EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/geometryutil EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/audio EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/audio/openal EXCLUDE_FROM_ALL)



FIND_PACKAGE(PkgConfig QUIET)
FIND_PACKAGE(Vulkan QUIET)
IF(Vulkan_FOUND)
    MESSAGE(STATUS "Vulkan: ${Vulkan_LIBRARY}")
ENDIF()


###################################################
# Backend common logic for all vulkan Samples
####################################################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/common)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/common/Importer)

####################################!
#	Vulkan Samples
####################################!
####################################
#	Basic Samples
####################################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/StartupWindow)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Triangle)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Cube)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Texture)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/NormalMap)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/SkyBox)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/PushConstant)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Instance)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/MultiSampling)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/MultiThreading)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/SubPasses)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Raytracing)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ParticleSystem)

####################################
#	Vulkan Internal Samples
####################################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Memory)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ExternalMemoryfd)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Cachepipeline)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/VideoPlayback)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ShaderInfo)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/VriablerateShading)


#######################################
# Compute Shader Based Samples
#######################################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ReactionDiffusion)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Mandelbrot)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/GameOfLife)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/SubGroup)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Prime)



####################################
# SPIR-V Compiler
####################################
FIND_PROGRAM(GLSLLANGVALIDATOR glslangValidator)
FIND_PROGRAM(GLSLC glslc)


# All shader files.
FILE(GLOB_RECURSE GLSL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.rgen
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.rmiss
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.tese
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.tesc
)

# Compile shader source code to its respective spriv files.
FOREACH(GLSL ${GLSL_SOURCE_FILES})
	GET_FILENAME_COMPONENT(FILE_NAME ${GLSL} ABSOLUTE)

	#glslangValidator
	SET(SPIRV "${FILE_NAME}.spv")
	IF(GLSLLANGVALIDATOR)
		IF(CMAKE_BUILD_TYPE STREQUAL "Release")
			SET(SPIRV_BUILD_SETTINGS "")
		ELSE()
			SET(SPIRV_BUILD_SETTINGS -g -Od)
		ENDIF()
		ADD_CUSTOM_COMMAND(
			OUTPUT ${SPIRV}
			COMMAND ${CMAKE_COMMAND} -E make_directory "${EXECUTABLE_OUTPUT_PATH}/shaders/"
			COMMAND ${GLSLLANGVALIDATOR} -Dgl_InstanceID=gl_InstanceIndex -Dgl_VertexID=gl_VertexIndex  -I${CMAKE_CURRENT_SOURCE_DIR}/shaders/common --target-env vulkan1.1 ${SPIRV_BUILD_SETTINGS} -o ${SPIRV} ${GLSL} 
			DEPENDS ${GLSL})
	ELSEIF(GLSLC)
		IF(CMAKE_BUILD_TYPE STREQUAL "Release")
			SET(SPIRV_BUILD_SETTINGS -O)
		ELSE()
			SET(SPIRV_BUILD_SETTINGS -g -Od)
		ENDIF()

		ADD_CUSTOM_COMMAND(
			OUTPUT ${SPIRV}
			COMMAND ${CMAKE_COMMAND} -E make_directory "${EXECUTABLE_OUTPUT_PATH}/shaders/"
			COMMAND ${GLSLC} -Dgl_InstanceID=gl_InstanceIndex -Dgl_VertexID=gl_VertexIndex  -I${CMAKE_CURRENT_SOURCE_DIR}/shaders/common --target-spv=spv1.3 -x glsl ${SPIRV_BUILD_SETTINGS} -Werror --target-env=vulkan1.1 -o ${SPIRV} ${GLSL} 
			DEPENDS ${GLSL})
	ENDIF()

	LIST(APPEND SPIRV_BINARY_FILES ${SPIRV})
ENDFOREACH(GLSL)

ADD_CUSTOM_TARGET(
	Shaders ALL
	DEPENDS ${SPIRV_BINARY_FILES}
)

## External Asset
ADD_CUSTOM_TARGET(
	DownloadAsset
	DEPENDS ${CMAKE_SOURCE_DIR}/asset/downloaded
)

# All shader files.
FILE(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders SYMBOLIC)
FILE(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/asset ${CMAKE_CURRENT_BINARY_DIR}/asset SYMBOLIC)

IF(BUILD_WTIH_TEST)
	ENABLE_TESTING()
	ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/test)
ENDIF()
