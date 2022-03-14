FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(Skybox ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(Skybox ${Vulkan_LIBRARIES} vk-sample-common vk-sample-common-asset-importer)
ADD_DEPENDENCIES(Skybox vk-sample-common vk-sample-common-asset-importer)

# Vulkan dependies headers.
TARGET_INCLUDE_DIRECTORIES(Skybox PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
