FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(PushConstant ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(PushConstant ${Vulkan_LIBRARIES} vkcommon )
ADD_DEPENDENCIES(PushConstant vkcommon)

# Vulkan dependies headers.
TARGET_INCLUDE_DIRECTORIES(PushConstant PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
