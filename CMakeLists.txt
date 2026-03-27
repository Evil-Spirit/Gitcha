cmake_minimum_required(VERSION 3.16)

project(Gitcha VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Network Qml QuickControls2 Multimedia)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/gitlabclient.cpp
    src/messagestore.cpp
    src/contactmanager.cpp
    src/messagemodel.cpp
    src/contactmodel.cpp
    src/settingsmanager.cpp
)

set(HEADERS
    src/gitlabclient.h
    src/messagestore.h
    src/contactmanager.h
    src/messagemodel.h
    src/contactmodel.h
    src/settingsmanager.h
)

qt_add_executable(Gitcha
    ${SOURCES}
    ${HEADERS}
)

qt_add_qml_module(Gitcha
    URI Gitcha
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/LoginPage.qml
        qml/ContactsPage.qml
        qml/ChatPage.qml
        qml/AddContactDialog.qml
        qml/SettingsPage.qml
    RESOURCES
        assets/logo.png
        assets/message_notification.wav
)

target_link_libraries(Gitcha PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Network
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Multimedia
)

target_include_directories(Gitcha PRIVATE src)

# Android-specific configuration
if(ANDROID)
    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL      https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(Gitcha)

    set_target_properties(Gitcha PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
endif()

# iOS-specific configuration
if(IOS)
    set_target_properties(Gitcha PROPERTIES
        MACOSX_BUNDLE TRUE
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
    )
endif()

install(TARGETS Gitcha
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ── Deployment: copy required Qt runtime libraries and plugins ────────────────
# qt_generate_deploy_qml_app_script (Qt 6.3+) calls windeployqt on Windows,
# macdeployqt on macOS, and copies Qt .so files on Linux.  It also resolves
# all QML module plugins referenced by the qml/ directory.
if(NOT ANDROID AND NOT IOS)
    if(QT_VERSION VERSION_GREATER_EQUAL "6.3")
        qt_generate_deploy_qml_app_script(
            TARGET Gitcha
            OUTPUT_SCRIPT GITCHA_DEPLOY_SCRIPT
            MACOS_BUNDLE_POST_BUILD
            NO_UNSUPPORTED_PLATFORM_ERROR
            NO_TRANSLATIONS
        )
        install(SCRIPT ${GITCHA_DEPLOY_SCRIPT})
    else()
        # Fallback for Qt < 6.3: invoke platform deploy tools directly.
        if(WIN32)
            find_program(WINDEPLOYQT_EXECUTABLE windeployqt
                HINTS "${QT_DIR}/../../../bin"
                DOC "Path to windeployqt")
            if(WINDEPLOYQT_EXECUTABLE)
                install(CODE "
                    execute_process(COMMAND
                        \"${WINDEPLOYQT_EXECUTABLE}\"
                        --qmldir \"${CMAKE_SOURCE_DIR}/qml\"
                        --no-translations
                        \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/Gitcha${CMAKE_EXECUTABLE_SUFFIX}\"
                    COMMAND_ERROR_IS_FATAL ANY)
                ")
            endif()
        elseif(APPLE)
            find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
                HINTS "${QT_DIR}/../../../bin"
                DOC "Path to macdeployqt")
            if(MACDEPLOYQT_EXECUTABLE)
                install(CODE "
                    execute_process(COMMAND
                        \"${MACDEPLOYQT_EXECUTABLE}\"
                        \"\${CMAKE_INSTALL_PREFIX}/Gitcha.app\"
                        -qmldir=${CMAKE_SOURCE_DIR}/qml
                    COMMAND_ERROR_IS_FATAL ANY)
                ")
            endif()
        endif()
    endif()
endif()
# Android: Qt .so libs are bundled automatically by cmake --build … --target apk
# iOS:     Qt frameworks are embedded automatically by Xcode when archiving
