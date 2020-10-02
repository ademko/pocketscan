
CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)

PROJECT(PocketScan)

FIND_PATH(HYDRA_SRC_PATH CMakeLists.txt DOC "Directory containing the hydra CMakeLists.txt and source code")
IF(NOT HYDRA_SRC_PATH)
  MESSAGE(FATAL_ERROR "HYDRA_SRC_PATH must be set")
ENDIF(NOT HYDRA_SRC_PATH)
INCLUDE(${HYDRA_SRC_PATH}/CMakeLists.txt)

FIND_PACKAGE(Qt5 COMPONENTS Widgets Xml PrintSupport REQUIRED)

GET_FILENAME_COMPONENT(THIS_PATH ${CMAKE_CURRENT_LIST_FILE} PATH)

SET(POCKETSCAN_SOURCES
  MainWindow.h TileView.h WizardBar.h TabBar.h
  ImageAddButton.h LevelEditor.h
  DynamicSlot.h
  Project.cpp
  Main.cpp MainWindow.cpp TileView.cpp WizardBar.cpp TabBar.cpp ImageAddButton.cpp
  AutoClip.cpp ImageAlg.cpp FileNameSeries.cpp
  LevelEditor.cpp
  ImageFileCache.cpp AboutDialog.cpp
  DynamicSlot.cpp)

ADD_EXECUTABLE(PocketScan WIN32 ${POCKETSCAN_SOURCES})

TARGET_LINK_LIBRARIES(PocketScan hydra)
TARGET_LINK_LIBRARIES(PocketScan Qt5::PrintSupport)

TARGET_INCLUDE_DIRECTORIES(PocketScan PUBLIC ${THIS_PATH})
