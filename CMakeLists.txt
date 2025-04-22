
cmake_minimum_required(VERSION 3.16)
project(Asteria VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build for Flathub
option(FLATHUB_BUILD "Build for Flathub" OFF)

# Find required Qt components
if(FLATHUB_BUILD)
    # Define FLATHUB_BUILD for conditional compilation
    add_definitions(-DFLATHUB_BUILD)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Qt6Network Qt6PrintSupport Qt6Svg)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network PrintSupport Svg)
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Qt6Network Qt6PrintSupport Qt6Svg Qt6Pdf)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network PrintSupport Svg Pdf)
endif()

file(COPY ${CMAKE_SOURCE_DIR}/python/venv DESTINATION ${CMAKE_BINARY_DIR}/python)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Asteria
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        chartcalculator.h chartcalculator.cpp
        chartdatamanager.h chartdatamanager.cpp
        chartrenderer.h chartrenderer.cpp
        mistralapi.h mistralapi.cpp
        chartwidget.h chartwidget.cpp
        aspectarianwidget.h aspectarianwidget.cpp
        elementmodalitywidget.h elementmodalitywidget.cpp
        planetlistwidget.h planetlistwidget.cpp
        resources.qrc
        symbolsdialog.h symbolsdialog.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Asteria APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Asteria SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Asteria
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Link libraries based on build type
if(FLATHUB_BUILD)
    target_link_libraries(Asteria PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt6::Network Qt6::PrintSupport Qt6::Svg)
else()
    target_link_libraries(Asteria PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt6::Network Qt6::PrintSupport Qt6::Svg Qt6::Pdf)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Asteria)
endif()

set_target_properties(Asteria PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE)

include(GNUInstallDirs)
install(TARGETS Asteria
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Asteria)
endif()

# Install desktop file and metainfo
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Asteria.desktop"
        DESTINATION "share/applications")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Asteria.appdata.xml"
        DESTINATION "share/metainfo")


install(FILES "${CMAKE_CURRENT_SOURCE_DIR}icons/asteria-icon-192.png"
        DESTINATION "share/icons/hicolor/192x192/apps"
        RENAME "io.github.alamahant.Asteria.png")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icons/asteria-icon-256.png"
        DESTINATION "share/icons/hicolor/256x256/apps"
        RENAME "io.github.alamahant.Asteria.png")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icons/asteria-icon-512.png"
        DESTINATION "share/icons/hicolor/512x512/apps"
        RENAME "io.github.alamahant.Asteria.png")

#      if(FLATHUB_BUILD)
#          # Define the path for Python scripts in Flatpak
#          add_definitions(-DFLATHUB_DEFAULT_PYTHON_PATH="/app/python/venv")
#
#          # Install Python scripts to the appropriate location
#          install(DIRECTORY ${CMAKE_SOURCE_DIR}/python/venv/scripts/
#                  DESTINATION python/venv/scripts
#                  FILES_MATCHING PATTERN "*.py"
#                  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
#                              GROUP_READ GROUP_EXECUTE
#                              WORLD_READ WORLD_EXECUTE)
#      endif()


if(FLATHUB_BUILD)
    # Define the path for Python scripts in Flatpak
    add_definitions(-DFLATHUB_DEFAULT_PYTHON_PATH="/app/python/venv")

    # Install Python scripts
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/python/venv/scripts/
        DESTINATION python/venv/scripts
        FILES_MATCHING PATTERN "*.py"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)

    # Install the Python packages
    install(FILES
        ${CMAKE_SOURCE_DIR}/python/venv/flatlib-0.2.3-py3-none-any.whl
        ${CMAKE_SOURCE_DIR}/python/venv/pyswisseph-2.10.3.2.tar.gz
        DESTINATION python/venv
        PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ
                    WORLD_READ)
endif()
