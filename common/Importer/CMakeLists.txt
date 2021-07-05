FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

ADD_LIBRARY(vkcommon-asset-importer ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(vkcommon-asset-importer ${Vulkan_LIBRARIES} fmt freeimage assimp vkcommon ${SDL2_LIBRARIES})
ADD_DEPENDENCIES(vkcommon-asset-importer vkcommon )
# Vulkan dependencies headers.
TARGET_INCLUDE_DIRECTORIES(vkcommon-asset-importer PRIVATE ${Vulkan_INCLUDE_DIRS} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)