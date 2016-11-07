set(SOURCE_FILES
  "PlanarUnwrapAll.h"
  "PlanarUnwrapAll.cpp"
  )
find_package(Maya REQUIRED)
set(CMAKE_PREFIX_PATH "D:/Qt/Qt5.6.1/5.6/msvc2015_64")
find_package(Qt5Widgets REQUIRED)

include_directories(${MAYA_INCLUDE_DIR})
link_directories(${MAYA_LIBRARY_DIR})

#include_directories(${MAYA_QT_INCLUDE_DIR})
#link_directories(${MAYA_QT_LIBRARY_DIR})

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${MOC_FILES})
target_link_libraries(${PROJECT_NAME} ${MAYA_LIBRARIES})
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)
#target_link_libraries(${PROJECT_NAME} ${MAYA_QT_LIBRARIES})

MAYA_PLUGIN(${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME} ${MAYA_TARGET_TYPE} DESTINATION plug-ins)
