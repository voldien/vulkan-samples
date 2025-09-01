FILE(GLOB SOURCE_SKYBOX_PANORAMIC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/SkyboxPanoramic.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(SkyboxPanoramic ${SOURCE_SKYBOX_PANORAMIC_FILES} ${HEADER_FILES})
TARGET_LINK_LIBRARIES(SkyboxPanoramic vk-sample-common vk-sample-common-asset-importer)
ADD_DEPENDENCIES(SkyboxPanoramic vk-sample-common vk-sample-common-asset-importer)

# Vulkan dependies headers.
TARGET_INCLUDE_DIRECTORIES(SkyboxPanoramic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(TARGETS SkyboxPanoramic DESTINATION bin)