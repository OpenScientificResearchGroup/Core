cmake_minimum_required(VERSION 3.15)
project(Core VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==================== 平台检测 ====================
message(STATUS "Building for: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# ==================== 包含目录设置 ====================
include(GNUInstallDirs)

# ==================== 用户可配置选项 ====================
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" OFF)

# 用户可配置依赖路径
set(THIRD_PARTY_DIR "" CACHE PATH "Path to third party libraries")
set(WXWIDGETS_ROOT "" CACHE PATH "Path to wxWidgets installation")
set(FMT_ROOT "" CACHE PATH "Path to fmt installation")
set(SPDLOG_ROOT "" CACHE PATH "Path to spdlog installation")
set(MINIZIP_NG_ROOT "" CACHE PATH "Path to minizip-ng installation")

# ==================== 平台特定的默认设置 ====================
# 设置默认的构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# ==================== 设置搜索路径 ====================
# 如果用户提供了第三方库目录，添加到搜索路径
if(THIRD_PARTY_DIR)
    list(APPEND CMAKE_PREFIX_PATH ${THIRD_PARTY_DIR})
    list(APPEND CMAKE_LIBRARY_PATH ${THIRD_PARTY_DIR}/lib)
    list(APPEND CMAKE_INCLUDE_PATH ${THIRD_PARTY_DIR}/include)
endif()

# 添加各库的特定搜索路径
if(WXWIDGETS_ROOT)
    list(APPEND CMAKE_PREFIX_PATH ${WXWIDGETS_ROOT})
endif()

if(FMT_ROOT)
    list(APPEND CMAKE_PREFIX_PATH ${FMT_ROOT})
endif()

if(SPDLOG_ROOT)
    list(APPEND CMAKE_PREFIX_PATH ${SPDLOG_ROOT})
endif()

if(MINIZIP_NG_ROOT)
    list(APPEND CMAKE_PREFIX_PATH ${MINIZIP_NG_ROOT})
endif()

# ==================== 平台特定的搜索路径 ====================
if(WIN32)
    # Windows路径
    list(APPEND CMAKE_PREFIX_PATH
        C:/Libraries
        C:/vcpkg/installed/x64-windows
        C:/vcpkg/installed/x86-windows
        $ENV{VCPKG_ROOT}/installed/x64-windows
        $ENV{VCPKG_ROOT}/installed/x86-windows
    )
    
elseif(APPLE)
    # macOS路径 (Homebrew)
    list(APPEND CMAKE_PREFIX_PATH
        /usr/local
        /usr/local/opt
        /opt/homebrew
        $ENV{HOMEBREW_PREFIX}
        $ENV{HOMEBREW_PREFIX}/opt
    )
    
    # 设置Homebrew默认路径（如果未设置）
    if(NOT DEFINED ENV{HOMEBREW_PREFIX})
        if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
            set(ENV{HOMEBREW_PREFIX} "/opt/homebrew")
        else()
            set(ENV{HOMEBREW_PREFIX} "/usr/local")
        endif()
    endif()
    
elseif(UNIX AND NOT APPLE)
    # Linux路径
    list(APPEND CMAKE_PREFIX_PATH
        /usr
        /usr/local
        /opt
        /opt/local
    )
endif()

# ==================== 查找依赖包 ====================
# 1. 首先尝试使用 find_package 的 CONFIG 模式（现代方式）
# 首先设置查找策略
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)  # 优先使用Config模式

# 1. wxWidgets (需要处理平台差异)
if(APPLE)
    # macOS上wxWidgets可能有不同名称
    find_package(wxWidgets CONFIG QUIET NAMES wxWidgets wx)
else()
    find_package(wxWidgets CONFIG QUIET)
endif()

if(NOT wxWidgets_FOUND)
    # 回退到MODULE模式
    if(APPLE)
        # macOS可能需要指定框架
        find_package(wxWidgets QUIET COMPONENTS core base)
        if(wxWidgets_FOUND)
            # macOS上需要链接Cocoa框架
            list(APPEND wxWidgets_LIBRARIES "-framework Cocoa")
            list(APPEND wxWidgets_LIBRARIES "-framework IOKit")
            list(APPEND wxWidgets_LIBRARIES "-framework Carbon")
            list(APPEND wxWidgets_LIBRARIES "-framework System")
        endif()
    else()
        find_package(wxWidgets QUIET COMPONENTS core base)
    endif()
endif()

# 类似的查找策略用于其他库
find_package(fmt CONFIG QUIET)
if(NOT fmt_FOUND)
    find_package(fmt QUIET)
endif()

find_package(spdlog CONFIG QUIET)
if(NOT spdlog_FOUND)
    find_package(spdlog QUIET)
endif()

find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    find_package(nlohmann_json QUIET)
endif()

if(APPLE)
    # macOS可能使用不同的包名
    find_package(minizip-ng CONFIG QUIET)
    if(NOT minizip_ng_FOUND)
        find_package(minizip QUIET)
        if(minizip_FOUND)
            set(minizip_ng_FOUND TRUE)
            set(minizip_ng_LIBRARIES ${minizip_LIBRARIES})
            set(minizip_ng_INCLUDE_DIRS ${minizip_INCLUDE_DIRS})
        else()
            # 尝试通过pkg-config查找
            find_package(PkgConfig QUIET)
            if(PKGCONFIG_FOUND)
                pkg_check_modules(MINIZIP libminizip)
                if(MINIZIP_FOUND)
                    set(minizip_ng_FOUND TRUE)
                    set(minizip_ng_INCLUDE_DIRS ${MINIZIP_INCLUDE_DIRS})
                    set(minizip_ng_LIBRARIES ${MINIZIP_LIBRARIES})
                endif()
            endif()
        endif()
    endif()
else()
    find_package(minizip-ng CONFIG QUIET)
    if(NOT minizip_ng_FOUND)
        find_package(minizip-ng QUIET)
        if(NOT minizip_ng_FOUND)
            find_package(minizip QUIET)
            if(minizip_FOUND)
                set(minizip_ng_FOUND TRUE)
                set(minizip_ng_LIBRARIES ${minizip_LIBRARIES})
                set(minizip_ng_INCLUDE_DIRS ${minizip_INCLUDE_DIRS})
            endif()
        endif()
    endif()
endif()


# ==================== 依赖检查 ====================
# 检查是否所有依赖都找到了
set(MISSING_DEPS)
if(NOT wxWidgets_FOUND)
    list(APPEND MISSING_DEPS "wxWidgets")
endif()
if(NOT fmt_FOUND)
    list(APPEND MISSING_DEPS "fmt")
endif()
if(NOT spdlog_FOUND)
    list(APPEND MISSING_DEPS "spdlog")
endif()
if(NOT nlohmann_json_FOUND)
    list(APPEND MISSING_DEPS "nlohmann_json")
endif()
if(NOT minizip_ng_FOUND)
    list(APPEND MISSING_DEPS "minizip-ng")
endif()

if(MISSING_DEPS)
    message(WARNING "Missing dependencies: ${MISSING_DEPS}")
    
    # 平台特定的安装指导
    if(WIN32)
        message(STATUS "Windows installation options:")
        message(STATUS "  1. Install via vcpkg: vcpkg install wxwidgets fmt spdlog nlohmann-json minizip-ng")
        message(STATUS "  2. Download prebuilt libraries and set THIRD_PARTY_DIR")
        
    elseif(APPLE)
        message(STATUS "macOS installation options:")
        message(STATUS "  1. Install via Homebrew: brew install wxwidgets fmt spdlog nlohmann-json minizip-ng")
        message(STATUS "  2. Install via MacPorts: sudo port install wxWidgets-3.2 fmt spdlog nlohmann-json minizip-ng")
        
    elseif(UNIX AND NOT APPLE)
        message(STATUS "Linux installation options:")
        message(STATUS "  1. Ubuntu/Debian: sudo apt-get install libwxgtk3.2-dev libfmt-dev libspdlog-dev nlohmann-json3-dev libminizip-ng-dev")
        message(STATUS "  2. Fedora/RHEL: sudo dnf install wxGTK3-devel fmt-devel spdlog-devel nlohmann-json-devel minizip-ng-devel")
        message(STATUS "  3. Arch Linux: sudo pacman -S wxgtk3 fmt spdlog nlohmann-json minizip-ng")
    endif()
    
    message(STATUS "")
    message(STATUS "Or set individual *_ROOT variables (e.g., -DWXWIDGETS_ROOT=/path/to/wxwidgets)")
endif()

# ==================== 设置输出目录 ====================
# 设置平台特定的输出目录结构
if(WIN32)
    # Windows通常按配置分开
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else()
    # Unix-like系统通常不分配置
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# ==================== 收集源文件 ====================
# 移除file(GLOB_RECURSE)，显式列出头文件
set(HEADER_FILES
    # Command模块
    include/Command/lgcCommandManager.hpp
    include/Command/lgcCompositeCommand.hpp
    include/Command/virCommandBase.hpp
    
    # Config模块
    include/Config/lgcConfigManager.hpp
    
    # Data模块
    include/Data/lgcDataManager.hpp
    include/Data/lgcPropertyContainerMap.hpp
    include/Data/virContainerBase.hpp
    include/Data/virObjectBase.hpp
    include/Data/lgcProperty.hpp
    include/Data/virPropertyBase.hpp
    
    # 核心
    include/defCoreApi.hpp
    include/lgcCore.hpp
    
    # Event模块
    include/Event/lgcEventManager.hpp
    
    # I18n模块
    include/I18n/lgcI18nManager.hpp
    
    # Log模块
    include/Log/lgcLogManager.hpp
    
    # Plugin模块
    include/Plugin/lgcPluginManager.hpp
    include/Plugin/virPluginBase.hpp
    
    # Resource模块
    include/Resource/lgcResourceManager.hpp
    include/Resource/virResourceBase.hpp
    
    # Service模块
    include/Service/lgcServiceManager.hpp
    include/Service/virServiceBase.hpp
    
    # Task模块
    include/Task/lgcCancellationToken.hpp
    include/Task/lgcTaskManager.hpp
    
    # Ui模块
    include/Ui/lgcUiManager.hpp
    
    # Utility模块
    include/Utility/lgcString.hpp
    include/Utility/lgcUuid.hpp
)

# 源文件列表（保持不变）
set(SOURCE_FILES
    # Core主文件
    src/lgcCore.cpp
    
    # Command模块
    src/Command/lgcCommandManager.cpp
    src/Command/lgcCompositeCommand.cpp
    
    # Config模块
    src/Config/lgcConfigManager.cpp
    
    # Data模块
    src/Data/lgcDataManager.cpp
    src/Data/lgcPropertyContainerMap.cpp
    
    # Event模块
    src/Event/lgcEventManager.cpp
    
    # I18n模块
    src/I18n/lgcI18nManager.cpp
    
    # Log模块
    src/Log/lgcLogManager.cpp
    
    # Plugin模块
    src/Plugin/lgcPluginManager.cpp
    
    # Resource模块
    src/Resource/lgcResourceManager.cpp
    
    # Service模块
    src/Service/lgcServiceManager.cpp
    
    # Task模块
    src/Task/lgcCancellationToken.cpp
    src/Task/lgcTaskManager.cpp
    
    # Ui模块
    src/Ui/lgcUiManager.cpp
    
    # Utility模块
    src/Utility/lgcString.cpp
    src/Utility/lgcUuid.cpp
)

# ==================== 创建库目标 ====================
add_library(Core ${SOURCE_FILES})

# ==================== 目标属性设置 ====================
set_target_properties(Core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "Core"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Windows多配置输出目录
if(WIN32)
    set_target_properties(Core PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/Debug"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/Release"
        ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/lib/RelWithDebInfo"
        ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/lib/MinSizeRel"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/Debug"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/Release"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/lib/RelWithDebInfo"
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/lib/MinSizeRel"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/RelWithDebInfo"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin/MinSizeRel"
    )
endif()

# ==================== 包含目录设置 ====================
target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core>  # 使用变量
)

# ==================== DLL导出宏定义 ====================
if(BUILD_SHARED_LIBS)
    if(WIN32)
        # Windows DLL导出
        target_compile_definitions(Core PRIVATE
            CORE_EXPORTS
            WXUSINGDLL
            _WINDLL
        )
    else()
        # Unix共享库
        target_compile_options(Core PRIVATE
            -fvisibility=hidden
            -fvisibility-inlines-hidden
        )
        target_compile_definitions(Core PRIVATE CORE_EXPORTS)
        
        # macOS框架相关
        if(APPLE)
            target_compile_definitions(Core PRIVATE __APPLE__)
        endif()
    endif()
endif()

# ==================== 编译器设置 ====================
target_compile_definitions(Core PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:RelWithDebInfo>:NDEBUG>
    $<$<CONFIG:MinSizeRel>:NDEBUG>
)

# 如果是Windows，添加MSVC特定定义
if(MSVC)
    target_compile_definitions(Core PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _SCL_SECURE_NO_WARNINGS
        NOMINMAX
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
        WIN32_LEAN_AND_MEAN
    )
endif()

# 编译器选项
if(MSVC)
    target_compile_options(Core PRIVATE
        /W4
        /WX
        /permissive-
        /utf-8
        /Zc:__cplusplus
        /Zc:inline
        /wd4996  # 禁用警告4996（安全函数警告）
    )
    
    # 链接库
    target_link_libraries(Core PRIVATE bcrypt.lib)
    
else()
    # GCC/Clang选项
    target_compile_options(Core PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wno-deprecated-declarations
        -pedantic
    )
    
    # 特定编译器选项
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(Core PRIVATE
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(Core PRIVATE
            -Weverything
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-padded
        )
    endif()
    
    # 共享库需要PIC
    if(BUILD_SHARED_LIBS)
        target_compile_options(Core PRIVATE -fPIC)
    endif()
endif()

# ==================== 链接器设置 ====================
# 平台特定的链接库
if(WIN32)
    target_link_libraries(Core PRIVATE
        bcrypt.lib
        # ws2_32.lib
        # advapi32.lib
    )
    
    # Release配置的LTCG
    set_target_properties(Core PROPERTIES
        $<$<CONFIG:Release>:LINK_FLAGS> "/LTCG"
    )
    
elseif(APPLE)
    # macOS框架和库
    find_library(COCOA_LIB Cocoa)
    find_library(FOUNDATION_LIB Foundation)
    find_library(CORESERVICES_LIB CoreServices)
    find_library(SECURITY_LIB Security)
    
    if(COCOA_LIB)
        target_link_libraries(Core PRIVATE ${COCOA_LIB})
    endif()
    if(FOUNDATION_LIB)
        target_link_libraries(Core PRIVATE ${FOUNDATION_LIB})
    endif()
    if(CORESERVICES_LIB)
        target_link_libraries(Core PRIVATE ${CORESERVICES_LIB})
    endif()
    if(SECURITY_LIB)
        target_link_libraries(Core PRIVATE ${SECURITY_LIB})
    endif()
    
elseif(UNIX AND NOT APPLE)
    # Linux系统库
    target_link_libraries(Core PRIVATE
        pthread
        dl
        m
        rt
    )
    
    # 检查并链接X11（如果使用）
    if(wxWidgets_FOUND)
        find_package(X11 QUIET)
        if(X11_FOUND)
            target_link_libraries(Core PRIVATE ${X11_LIBRARIES})
        endif()
    endif()
endif()

# ==================== 链接依赖库 ====================
# 使用现代目标名称
if(wxWidgets_FOUND)
    if(TARGET wx::wx)
        target_link_libraries(Core PUBLIC wx::wx)
    elseif(TARGET wxWidgets::wxWidgets)
        target_link_libraries(Core PUBLIC wxWidgets::wxWidgets)
    else()
        # 传统方式
        target_include_directories(Core PUBLIC ${wxWidgets_INCLUDE_DIRS})
        target_link_libraries(Core PUBLIC ${wxWidgets_LIBRARIES})
        target_compile_options(Core PUBLIC ${wxWidgets_CXX_FLAGS})
    endif()
endif()

if(fmt_FOUND)
    if(TARGET fmt::fmt)
        target_link_libraries(Core PUBLIC fmt::fmt)
    else()
        target_include_directories(Core PUBLIC ${fmt_INCLUDE_DIRS})
        target_link_libraries(Core PUBLIC ${fmt_LIBRARIES})
    endif()
endif()

if(spdlog_FOUND)
    if(TARGET spdlog::spdlog)
        target_link_libraries(Core PUBLIC spdlog::spdlog)
    else()
        target_include_directories(Core PUBLIC ${spdlog_INCLUDE_DIRS})
        target_link_libraries(Core PUBLIC ${spdlog_LIBRARIES})
    endif()
endif()

if(nlohmann_json_FOUND)
    if(TARGET nlohmann_json::nlohmann_json)
        target_link_libraries(Core PUBLIC nlohmann_json::nlohmann_json)
    else()
        target_include_directories(Core PUBLIC ${nlohmann_json_INCLUDE_DIRS})
    endif()
endif()

if(minizip_ng_FOUND)
    if(TARGET minizip-ng::minizip-ng)
        target_link_libraries(Core PUBLIC minizip-ng::minizip-ng)
    elseif(TARGET minizip::minizip)
        target_link_libraries(Core PUBLIC minizip::minizip)
    else()
        target_include_directories(Core PUBLIC ${minizip_ng_INCLUDE_DIRS})
        target_link_libraries(Core PUBLIC ${minizip_ng_LIBRARIES})
    endif()
endif()

# ==================== 安装配置 ====================
# 平台特定的安装路径
if(APPLE)
    # macOS框架安装路径
    set(CMAKE_INSTALL_FRAMEWORK_PREFIX "Library/Frameworks")
    set(CMAKE_INSTALL_BINDIR "bin")
    set(CMAKE_INSTALL_LIBDIR "lib")
    set(CMAKE_INSTALL_INCLUDEDIR "include")
elseif(WIN32)
    # Windows安装路径
    set(CMAKE_INSTALL_BINDIR "bin")
    set(CMAKE_INSTALL_LIBDIR "lib")
    set(CMAKE_INSTALL_INCLUDEDIR "include")
else()
    # Linux/Unix标准路径
    set(CMAKE_INSTALL_BINDIR "bin")
    set(CMAKE_INSTALL_LIBDIR "lib")
    set(CMAKE_INSTALL_INCLUDEDIR "include")
endif()

# 安装目标（保持不变但使用上述变量）
install(TARGETS Core
    EXPORT CoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Core
)

# 安装头文件
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Core
    FILES_MATCHING 
        PATTERN "*.hpp"
        PATTERN "*.h"
    COMPONENT Development
)

# 安装导出文件
install(
    EXPORT CoreTargets
    FILE CoreTargets.cmake
    NAMESPACE MoldDesigner::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Core
    COMPONENT Development
)

# ==================== 配置总结 ====================
message(STATUS "")
message(STATUS "================ Core Configuration ================")
message(STATUS "Platform:          ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture:      ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS "Project:           ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "C++ standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler:          ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  wxWidgets:       ${wxWidgets_FOUND}")
message(STATUS "  fmt:             ${fmt_FOUND}")
message(STATUS "  spdlog:          ${spdlog_FOUND}")
message(STATUS "  nlohmann/json:   ${nlohmann_json_FOUND}")
message(STATUS "  minizip-ng:      ${minizip_ng_FOUND}")
message(STATUS "")
message(STATUS "Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "====================================================")

# ==================== 创建配置文件 ====================
# 创建CoreConfig.cmake供其他项目使用
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/CoreConfig.cmake.in
    ${CMAKE_BINARY_DIR}/CoreConfig.cmake
    @ONLY
)

# 安装配置文件
install(
    FILES ${CMAKE_BINARY_DIR}/CoreConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Core
    COMPONENT Development
)

# 生成并安装版本文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/CoreConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES ${CMAKE_BINARY_DIR}/CoreConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Core
    COMPONENT Development
)

# 构建示例程序
if(BUILD_EXAMPLES)
    add_executable(core_example
        ${CMAKE_SOURCE_DIR}/examples/core_config_sample.json
        ${CMAKE_SOURCE_DIR}/examples/core_manifest_sample.json
    )
    
    # 如果需要创建使用库的C++示例程序，可以添加更多源文件
    # 这里只是展示了如何处理现有的JSON文件
    
    target_link_libraries(core_example Core)
    set_target_properties(core_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
endif()

# 构建测试
if(BUILD_TESTS)
    enable_testing()
    
    # 添加测试目录（如果tests目录下有CMakeLists.txt）
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
    else()
        # 或者手动添加测试
        add_executable(core_tests
            # 在这里添加测试源文件
        )
        target_link_libraries(core_tests Core)
        add_test(NAME CoreTests COMMAND core_tests)
    endif()
endif()

# 添加文档生成（如果docs目录下有文档）
if(EXISTS ${CMAKE_SOURCE_DIR}/docs)
    # 可以添加doxygen支持
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# 添加卸载目标
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )
    
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()