cmake_minimum_required(VERSION 3.16)

# ── vcpkg auto-detect (must be before project()) ──────────────────
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# MinGW triplet for vcpkg
if(DEFINED CMAKE_C_COMPILER AND CMAKE_C_COMPILER MATCHES "mingw")
    if(NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "Vcpkg target triplet")
    endif()
endif()

project(LoopGit VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Qt ────────────────────────────────────────────────────────────
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# ── libgit2 ───────────────────────────────────────────────────────
# Strategy: try vcpkg/system first, fallback to FetchContent
find_package(unofficial-libgit2 CONFIG QUIET)

if(unofficial-libgit2_FOUND)
    message(STATUS "libgit2: found via vcpkg")
    set(LIBGIT2_LIBRARIES unofficial::libgit2::libgit2)
else()
    find_package(libgit2 QUIET)
    if(libgit2_FOUND)
        message(STATUS "libgit2: found on system")
        set(LIBGIT2_LIBRARIES libgit2::libgit2)
    else()
        message(STATUS "libgit2: not found — fetching from GitHub via FetchContent...")
        include(FetchContent)
        FetchContent_Declare(
            libgit2
            GIT_REPOSITORY https://github.com/libgit2/libgit2.git
            GIT_TAG        v1.8.4
            GIT_SHALLOW    TRUE
        )
        # Configure libgit2 build options (minimize build scope)
        set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)
        set(BUILD_TESTS        OFF CACHE BOOL "" FORCE)
        set(BUILD_CLI          OFF CACHE BOOL "" FORCE)
        set(BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
        set(USE_SSH            OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(libgit2)
        # libgit2 v1.8.x: actual CMake target is "libgit2package"
        set(LIBGIT2_LIBRARIES libgit2package)
    endif()
endif()

# ── Sources ───────────────────────────────────────────────────────
set(PROJECT_SOURCES
    main.cpp
    mainwindow.h
    mainwindow.cpp
    repowidget.h
    repowidget.cpp
    gitmanager.h
    gitmanager.cpp
    diffviewwidget.h
    diffviewwidget.cpp
    branchdialog.h
    branchdialog.cpp
    commitgraphmodel.h
    commitgraphmodel.cpp
    commitgraphdelegate.h
    commitgraphdelegate.cpp
    resources.qrc
    app.rc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(LoopGit
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(LoopGit SHARED ${PROJECT_SOURCES})
    else()
        add_executable(LoopGit ${PROJECT_SOURCES})
    endif()
endif()

target_link_libraries(LoopGit PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${LIBGIT2_LIBRARIES}
)

# When using FetchContent, we need to add libgit2 include dirs manually
if(NOT unofficial-libgit2_FOUND AND NOT libgit2_FOUND)
    target_include_directories(LoopGit PRIVATE
        ${libgit2_SOURCE_DIR}/include
    )
endif()

set_target_properties(LoopGit PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS LoopGit
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(LoopGit)
endif()
