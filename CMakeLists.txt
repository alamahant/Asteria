cmake_minimum_required(VERSION 3.16)
project(Asteria VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build for Flathub
option(FLATHUB_BUILD "Build for Flathub" OFF)
option(GENTOO_BUILD "Build for Gentoo" OFF)

# Define FLATHUB_BUILD for conditional compilation
if(FLATHUB_BUILD)
    add_definitions(-DFLATHUB_BUILD)
endif()
if(GENTOO_BUILD)
    add_definitions(-DGENTOO_BUILD)
endif()

##### Swiss Ephemeris setup
if(FLATHUB_BUILD)
    # For Flatpak builds, build Swiss Ephemeris from source
    set(SWISSEPH_DATA_DIR "/app/share/swisseph")  # Runtime path in Flatpak
    set(SWISSEPH_SRC_DIR "${CMAKE_SOURCE_DIR}/swisseph_src")
    
    # Check if source directory exists
    if(NOT EXISTS ${SWISSEPH_SRC_DIR})
        message(FATAL_ERROR "Swiss Ephemeris source directory not found at ${SWISSEPH_SRC_DIR}")
    endif()
    
    # Build Swiss Ephemeris
    execute_process(
        COMMAND make
        WORKING_DIRECTORY ${SWISSEPH_SRC_DIR}
        RESULT_VARIABLE MAKE_RESULT
    )
    
    if(NOT MAKE_RESULT EQUAL "0")
        message(FATAL_ERROR "Failed to build Swiss Ephemeris")
    endif()
    
    # Set paths for finding Swiss Ephemeris in the build environment
    set(SWISSEPH_INCLUDE_DIR "${SWISSEPH_SRC_DIR}")
    set(SWISSEPH_LIBRARY "${SWISSEPH_SRC_DIR}/libswe.a")
    
    # Get header files
    file(GLOB SWISSEPH_HEADERS "${SWISSEPH_SRC_DIR}/*.h")
    
    # Install the files to the Flatpak
    install(FILES ${SWISSEPH_LIBRARY}
            DESTINATION share/swisseph)
    
    install(FILES ${SWISSEPH_HEADERS}
            DESTINATION share/swisseph)
    
    install(DIRECTORY ${SWISSEPH_SRC_DIR}/ephe/
            DESTINATION share/swisseph)
    
    include_directories(${SWISSEPH_INCLUDE_DIR})

elseif(GENTOO_BUILD)
    # Path inside the Gentoo build environment
    set(SWISSEPH_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/asteria/ephemeris")
    set(SWISSEPH_SRC_DIR "${CMAKE_SOURCE_DIR}/swisseph_src")

    if(NOT EXISTS ${SWISSEPH_SRC_DIR})
        message(FATAL_ERROR "Swiss Ephemeris source directory not found at ${SWISSEPH_SRC_DIR}")
    endif()

    # Build Swiss Ephemeris
    execute_process(
        COMMAND make
        WORKING_DIRECTORY ${SWISSEPH_SRC_DIR}
        RESULT_VARIABLE MAKE_RESULT
    )

    if(NOT MAKE_RESULT EQUAL "0")
        message(FATAL_ERROR "Failed to build Swiss Ephemeris")
    endif()

    # Set paths for CMake to find the library
    set(SWISSEPH_INCLUDE_DIR "${SWISSEPH_SRC_DIR}")
    set(SWISSEPH_LIBRARY "${SWISSEPH_SRC_DIR}/libswe.a")

    # Header files
    file(GLOB SWISSEPH_HEADERS "${SWISSEPH_SRC_DIR}/*.h")

    # Install the files to Gentoo prefix
    install(FILES ${SWISSEPH_LIBRARY}
            DESTINATION share/asteria/ephemeris)

    install(FILES ${SWISSEPH_HEADERS}
            DESTINATION share/asteria/ephemeris)

    install(DIRECTORY ${SWISSEPH_SRC_DIR}/ephe/
            DESTINATION share/asteria/ephemeris)

    include_directories(${SWISSEPH_INCLUDE_DIR})

else()
    # For local builds, use the local Swiss Ephemeris
    set(SWISSEPH_DIR "/home/dharma/ssd/cpp/swisseph")
    set(SWISSEPH_INCLUDE_DIR "${SWISSEPH_DIR}")
    set(SWISSEPH_DATA_DIR "${SWISSEPH_DIR}/ephe")
    include_directories(${SWISSEPH_INCLUDE_DIR})
    
    # Swiss Ephemeris source files
    set(SWISSEPH_SOURCES
        ${SWISSEPH_DIR}/swecl.c
        ${SWISSEPH_DIR}/swedate.c
        ${SWISSEPH_DIR}/swehouse.c
        ${SWISSEPH_DIR}/swejpl.c
        ${SWISSEPH_DIR}/swemmoon.c
        ${SWISSEPH_DIR}/swemplan.c
        ${SWISSEPH_DIR}/sweph.c
        ${SWISSEPH_DIR}/swephlib.c
    )
    
    # Create a static library for Swiss Ephemeris
    add_library(sweph STATIC ${SWISSEPH_SOURCES})
    target_include_directories(sweph PUBLIC ${SWISSEPH_INCLUDE_DIR})
endif()



# Find required Qt components
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)


if(FLATHUB_BUILD)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
        Widgets Network Svg QuickWidgets Positioning Location Charts)
elseif(GENTOO_BUILD)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
        Widgets Network Svg QuickWidgets Positioning Location Charts)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
        Widgets Network PrintSupport Svg Pdf QuickWidgets Positioning Location Charts)
endif()

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    chartcalculator.h chartcalculator.cpp
    chartdatamanager.h chartdatamanager.cpp
    chartrenderer.h chartrenderer.cpp
    mistralapi.h mistralapi.cpp
    chartwidget.h chartwidget.cpp
    aspectarianwidget.h aspectarianwidget.cpp
    elementmodalitywidget.h elementmodalitywidget.cpp
    planetlistwidget.h planetlistwidget.cpp
    symbolsdialog.h symbolsdialog.cpp
    osmmapdialog.h osmmapdialog.cpp
    Globals.h
    resources.qrc)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    qt_add_executable(Asteria
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        map.qml
        aspectsettingsdialog.h aspectsettingsdialog.cpp

        transitsearchdialog.h transitsearchdialog.cpp
        Globals.cpp
        donationdialog.h donationdialog.cpp
    )
else()
    if(ANDROID)
        add_library(Asteria SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(Asteria
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Link libraries based on build type
if(FLATHUB_BUILD)
    # Create imported target for Swiss Ephemeris
    add_library(sweph STATIC IMPORTED)
    set_target_properties(sweph PROPERTIES
        IMPORTED_LOCATION ${SWISSEPH_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${SWISSEPH_INCLUDE_DIR}
    )
    
    target_link_libraries(Asteria PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::QuickWidgets
        Qt${QT_VERSION_MAJOR}::Location
        Qt${QT_VERSION_MAJOR}::Positioning
        Qt${QT_VERSION_MAJOR}::Charts

        sweph
    )
elseif(GENTOO_BUILD)
    # Create imported target for Swiss Ephemeris
    add_library(sweph STATIC IMPORTED)
    set_target_properties(sweph PROPERTIES
        IMPORTED_LOCATION ${SWISSEPH_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${SWISSEPH_INCLUDE_DIR}
    )

    target_link_libraries(Asteria PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::QuickWidgets
        Qt${QT_VERSION_MAJOR}::Location
        Qt${QT_VERSION_MAJOR}::Positioning
        Qt${QT_VERSION_MAJOR}::Charts

        sweph
    )

else()
    target_link_libraries(Asteria PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::PrintSupport
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Pdf
        Qt${QT_VERSION_MAJOR}::QuickWidgets
        Qt${QT_VERSION_MAJOR}::Location
        Qt${QT_VERSION_MAJOR}::Positioning
        Qt${QT_VERSION_MAJOR}::Charts

        sweph
    )
target_link_libraries(Asteria PRIVATE Qt6::Widgets)
endif()

# Set data directory definition
if(FLATHUB_BUILD)
    # For Flatpak builds, use the absolute path
    target_compile_definitions(Asteria PRIVATE SWISSEPH_DATA_DIR="/app/share/swisseph")
else()
    # For local builds, use the install path
    target_compile_definitions(Asteria PRIVATE SWISSEPH_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/Asteria/ephemeris")
endif()

# Bundle settings for macOS
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER io.github.alamahant.Asteria)
endif()

set_target_properties(Asteria PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE)

include(GNUInstallDirs)

# Handle ephemeris files for non-Flatpak builds
if(NOT FLATHUB_BUILD)
    # Create ephemeris directory in build directory
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/ephemeris)
    
    # Copy ephemeris files from local directory
    file(GLOB EPHEMERIS_FILES "${SWISSEPH_DATA_DIR}/*.se1" "${SWISSEPH_DATA_DIR}/*.txt")
    foreach(EPHEMERIS_FILE ${EPHEMERIS_FILES})
        get_filename_component(FILENAME ${EPHEMERIS_FILE} NAME)
        configure_file(${EPHEMERIS_FILE} ${CMAKE_BINARY_DIR}/ephemeris/${FILENAME} COPYONLY)
    endforeach()
    
    # Install ephemeris files
    install(DIRECTORY ${CMAKE_BINARY_DIR}/ephemeris/
            DESTINATION "share/Asteria/ephemeris")
    
    # Install additional files if they exist
    if(EXISTS "${SWISSEPH_DATA_DIR}/transits_updated")
        install(FILES "${SWISSEPH_DATA_DIR}/transits_updated"
                DESTINATION "share/Asteria/ephemeris"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                            GROUP_READ GROUP_EXECUTE
                            WORLD_READ WORLD_EXECUTE)
    endif()
endif()

# Install the application
install(TARGETS Asteria
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install desktop file and metainfo
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Asteria.desktop"
        DESTINATION "share/applications")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Asteria.metainfo.xml"
        DESTINATION "share/metainfo")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icons/asteria-icon-192.png"
        DESTINATION "share/icons/hicolor/192x192/apps"
        RENAME "io.github.alamahant.Asteria.png")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icons/asteria-icon-256.png"
        DESTINATION "share/icons/hicolor/256x256/apps"
        RENAME "io.github.alamahant.Asteria.png")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icons/asteria-icon-512.png"
        DESTINATION "share/icons/hicolor/512x512/apps"
        RENAME "io.github.alamahant.Asteria.png")

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Asteria)
endif()












