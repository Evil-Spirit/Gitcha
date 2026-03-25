cmake_minimum_required(VERSION 3.16)

project(Gitcha VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Network Qml QuickControls2)

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
)

target_link_libraries(Gitcha PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Network
    Qt6::Qml
    Qt6::QuickControls2
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
