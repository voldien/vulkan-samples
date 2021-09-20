FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(SignedDistanceFieldTexture ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(SignedDistanceFieldTexture ${Vulkan_LIBRARIES} vkcommon )
ADD_DEPENDENCIES(SignedDistanceFieldTexture vkcommon)

# Vulkan dependies headers.
TARGET_INCLUDE_DIRECTORIES(SignedDistanceFieldTexture PRIVATE ${Vulkan_INCLUDE_DIRS} )

## Generate the SPV object
