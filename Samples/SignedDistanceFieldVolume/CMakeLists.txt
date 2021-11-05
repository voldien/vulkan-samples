FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(SignedDistanceFieldVolume ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(SignedDistanceFieldVolume ${Vulkan_LIBRARIES} vkcommon )
ADD_DEPENDENCIES(SignedDistanceFieldVolume vkcommon)

# Vulkan dependies headers.
TARGET_INCLUDE_DIRECTORIES(SignedDistanceFieldVolume PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )

## Generate the SPV object
