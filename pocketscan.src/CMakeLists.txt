
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

FIND_PATH(HYDRA_SRC_PATH CMakeLists.txt DOC "Directory containing the hydra CMakeLists.txt and source code")
IF(NOT HYDRA_SRC_PATH)
  MESSAGE(FATAL_ERROR "HYDRA_SRC_PATH must be set")
ENDIF(NOT HYDRA_SRC_PATH)
INCLUDE(${HYDRA_SRC_PATH}/CMakeLists.txt)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

QT4_WRAP_CPP(POCKETSCAN_MOC
  MainWindow.h TileView.h WizardBar.h TabBar.h
  ImageAddButton.h LevelEditor.h
  DynamicSlot.h)
SET(POCKETSCAN_SOURCES
  Project.cpp
  Main.cpp MainWindow.cpp TileView.cpp WizardBar.cpp TabBar.cpp ImageAddButton.cpp
  AutoClip.cpp ImageAlg.cpp FileNameSeries.cpp
  LevelEditor.cpp
  ImageFileCache.cpp AboutDialog.cpp
  DynamicSlot.cpp)

QT4_ADD_RESOURCES(POCKETSCAN_RES icons/icons.qrc)

INCLUDE_DIRECTORIES(.)

ADD_EXECUTABLE(PocketScan WIN32 ${POCKETSCAN_SOURCES} ${POCKETSCAN_MOC}
    ${UI_HEADERS} ${POCKETSCAN_RES})
TARGET_LINK_LIBRARIES(PocketScan hydra)

