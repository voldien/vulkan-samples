CMAKE_MINIMUM_REQUIRED( VERSION 3.8 )
PROJECT(vulkan-samples)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "" FORCE)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE INTERNAL "" FORCE)

FIND_PACKAGE(PkgConfig QUIET)
FIND_PACKAGE(OpenMP QUIET)

OPTION(BUILD_WITH_ASAN "Enable AddressSanitizer." OFF )
OPTION(BUILD_WITH_UBSAN "Enable Undefined Behavior sanitizer." OFF )
OPTION(BUILD_WITH_TSAN "Enable Thread sanitizer" OFF)


IF(CMAKE_BUILD_TYPE MATCHES Release)
	MESSAGE(STATUS "Compile for release.")
	ADD_DEFINITIONS(-O3 -march=native)
	SET(SIMD_FLAGS "-ftree-vectorize -ftree-slp-vectorize")
ENDIF()
IF(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
	MESSAGE(STATUS "Compile for release with Debug.")
	ADD_DEFINITIONS(-O3 -march=native)
	ADD_DEFINITIONS(-O3 -g3)
ENDIF()
IF(CMAKE_BUILD_TYPE MATCHES Debug)
	MESSAGE(STATUS "Compile for debug.")
	ADD_DEFINITIONS(-O3 -march=native)
	ADD_DEFINITIONS(-g3 -O0)
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


# ###########################################
# Backend libraries
# ###########################################
SET(BUILD_WITH_OPENMP ON)
#ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi EXCLUDE_FROM_ALL)
# Fragcore specific libraries
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/image EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/image/imageloader EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/filenotify EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/taskscheduler/TaskSch EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/geometryutil EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/window EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/window/sdl EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/audio EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/audio/openal EXCLUDE_FROM_ALL)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/renderer EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/fragcore/modules/renderer/vulkan EXCLUDE_FROM_ALL)

###################################################
# Backend common logic for all vulkan Samples
####################################################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/common)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/common/Importer)

####################################!
#	Vulkan Samples
####################################!
#	Basic Samples
####################################!
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/StartupWindow)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Triangle)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Cube)


####################################!
#	Vulkan Internal Samples
####################################!
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Memory)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ExternalMemoryfd)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Cachepipeline)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ShaderInfo)


####################################!
# Advanced
####################################!
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Texture)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Instance)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/NormalMap)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/SkyBox)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/PushConstant)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/MultiSampling)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/VectorField2D)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/VideoPlayback)

####################################!
# Compute Shader Based Samples
####################################!
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ReactionDiffusion)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Mandelbrot)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/GameOfLife)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Prime)


####################################
# SPIR-V Compiler
####################################
FIND_PROGRAM(GLSLLANGVALIDATOR glslangValidator)
FIND_PROGRAM(GLSLC glslc)

####################################
# Shader Source Files
####################################
FILE(GLOB_RECURSE GLSL_SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.fs
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vs
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.gs
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.geom
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.tese
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.tesc
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.comp
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.task
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.mesh
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.rgen
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.miss
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.rchit
)

####################################
# SPIR-V Compiler
####################################
FIND_PROGRAM(GLSLLANGVALIDATOR glslangValidator)
FIND_PROGRAM(GLSLC glslc)


# Compile shader source code to its respective spriv files.
FOREACH(GLSL ${GLSL_SOURCE_FILES})
	GET_FILENAME_COMPONENT(FILE_NAME ${GLSL} ABSOLUTE)


	#glslangValidator
	SET(SPIRV "${FILE_NAME}.spv")
	IF(GLSLLANGVALIDATOR)
		IF(CMAKE_BUILD_TYPE STREQUAL "Release")
			SET(SPIRV_BUILD_SETTINGS --invert-y)
		ELSE()
			SET(SPIRV_BUILD_SETTINGS -g -gVS -Od --invert-y)
		ENDIF()
		ADD_CUSTOM_COMMAND(
			OUTPUT ${SPIRV}
			COMMAND ${CMAKE_COMMAND} -E make_directory "${EXECUTABLE_OUTPUT_PATH}/Shaders/"
			COMMAND ${GLSLLANGVALIDATOR} --define-macro gl_InstanceID=gl_InstanceIndex --define-macro gl_VertexID=gl_VertexIndex  -I${CMAKE_CURRENT_SOURCE_DIR}/Shaders/common --target-env vulkan1.1 ${SPIRV_BUILD_SETTINGS} -o ${SPIRV} ${GLSL}
			DEPENDS ${GLSL})
	ELSEIF(GLSLC)
		IF(CMAKE_BUILD_TYPE STREQUAL "Release")
			SET(SPIRV_BUILD_SETTINGS -O -finvert-y)
		ELSE()
			SET(SPIRV_BUILD_SETTINGS -g -Od -finvert-y)
		ENDIF()

		ADD_CUSTOM_COMMAND(
			OUTPUT ${SPIRV}
			COMMAND ${CMAKE_COMMAND} -E make_directory "${EXECUTABLE_OUTPUT_PATH}/Shaders/"
			COMMAND ${GLSLC} -Dgl_InstanceID=gl_InstanceIndex -Dgl_VertexID=gl_VertexIndex  -I${CMAKE_CURRENT_SOURCE_DIR}/Shaders/common --target-spv=spv1.3 -x glsl ${SPIRV_BUILD_SETTINGS} -Werror --target-env=vulkan1.1 -o ${SPIRV} ${GLSL}
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

ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_SOURCE_DIR}/asset/downloaded
	COMMAND ${CMAKE_COMMAND} -E make_directory  ${CMAKE_SOURCE_DIR}/asset/zip
	COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/download_asset.py ${CMAKE_SOURCE_DIR}/asset/
	COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_SOURCE_DIR}/asset/downloaded
)

# All shader files.
FILE(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/Shaders ${CMAKE_CURRENT_BINARY_DIR}/Shaders SYMBOLIC)
FILE(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/asset ${CMAKE_CURRENT_BINARY_DIR}/asset SYMBOLIC)
