FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(SignedDistanceFieldTexture ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(SignedDistanceFieldTexture vk-sample-common )
ADD_DEPENDENCIES(SignedDistanceFieldTexture vk-sample-common)

# Vulkan dependies headers.
TARGET_INCLUDE_DIRECTORIES(SignedDistanceFieldTexture PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )

## Generate the SPV object
