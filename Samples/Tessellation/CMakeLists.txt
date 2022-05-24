FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(Tessellation ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(Tessellation vk-sample-common vk-sample-common-asset-importer)
ADD_DEPENDENCIES(Tessellation vk-sample-common vk-sample-common-asset-importer)

# Vulkan dependies headers.
TARGET_INCLUDE_DIRECTORIES(Tessellation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
