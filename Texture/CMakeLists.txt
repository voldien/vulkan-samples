FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(SingleTexture ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(SingleTexture ${Vulkan_LIBRARIES} vkcommon vkcommon-asset-importer SDL2)
ADD_DEPENDENCIES(SingleTexture vkcommon vkcommon-asset-importer)

# Vulkan dependies headers.
TARGET_INCLUDE_DIRECTORIES(SingleTexture PRIVATE ${Vulkan_INCLUDE_DIRS} )
