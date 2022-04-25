FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

ADD_LIBRARY(vk-sample-common-asset-importer ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(vk-sample-common-asset-importer fragcore-imageloader fmt freeimage assimp vk-sample-common )
ADD_DEPENDENCIES(vk-sample-common-asset-importer vk-sample-common )
# Vulkan dependencies headers.
TARGET_INCLUDE_DIRECTORIES(vk-sample-common-asset-importer PRIVATE ${Vulkan_INCLUDE_DIRS} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
