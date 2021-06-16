FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(SignedDistanceFieldVolumem ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(SignedDistanceFieldVolumem ${Vulkan_LIBRARIES} vkcommon SDL2)
ADD_DEPENDENCIES(SignedDistanceFieldVolumem vkcommon)

# Vulkan dependies headers.
TARGET_INCLUDE_DIRECTORIES(SignedDistanceFieldVolumem PRIVATE ${Vulkan_INCLUDE_DIRS} )

## Generate the SPV object
