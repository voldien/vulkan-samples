FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(ShadowMap ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(ShadowMap ${Vulkan_LIBRARIES} vkcommon)
ADD_DEPENDENCIES(ShadowMap vkcommon)

# Vulkan dependies headers.
TARGET_INCLUDE_DIRECTORIES(ShadowMap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
