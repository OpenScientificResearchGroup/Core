# Core

<!-- ![Build Status](https://img.shields.io/badge/build-passing-brightgreen) -->
![License](https://img.shields.io/badge/license-MIT-blue)
![Standard](https://img.shields.io/badge/c%2B%2B-17%2F20-blue)
<!-- ![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey) -->

A Production-Ready, Modular Desktop Application Framework based on Modern C++ & wxWidgets.

**⚠️ Warning: This project is in early development and may be unstable.**

## Introduction

**Core** is a high-performance, industrial-grade C++ software microkernel architecture designed for building modular and scalable desktop applications.

Core decouples core business logic from the underlying infrastructure by implementing a strictly layered architecture. With a built-in comprehensive Manager system, developers can focus on plugin development and business implementation without worrying about the complexity of the underlying framework.

## Core Features

The architecture is centered around a Microkernel that orchestrates the following managers:

*   **🔌 Plugin Manager**: Dynamic loading/unloading of DLL/SO modules, supporting hot-swapping potential.
*   **⚙️ Config Manager**: Centralized configuration management supporting JSON with hot-reload capabilities.
*   **🌍 I18n Manager**: Industrial-grade internationalization support (Dynamic language switching, placeholder formatting).
*   **📝 Log Manager**: High-performance asynchronous logging system (based on spdlog).
*   **📡 Event Manager**: Decoupled event bus for inter-module communication (Publish/Subscribe pattern).
*   **💾 Data Manager**: Abstracted data persistence layer.
*   **🎨 Ui Manager**: Cross-platform UI lifecycle management (powered by wxWidgets).
*   **🛠️ Service / Task / Command / Resource Managers**: Complete infrastructure for background tasks, command patterns, and resource pooling.

<!-- ## Architecture -->

## Dependencies

We stand on the shoulders of giants. This project utilizes the following open-source libraries:

*   **[wxWidgets](https://www.wxwidgets.org/)**: Cross-Platform GUI Library.
*   **[spdlog](https://github.com/gabime/spdlog)**: Fast C++ logging library.
*   **[fmt](https://github.com/fmtlib/fmt)**: A modern formatting library.
*   **[json](https://github.com/nlohmann/json)**: JSON for Modern C++.
*   **[minizip-ng](https://github.com/zlib-ng/minizip-ng)**: Zip manipulation library.

<!-- ## Build Instructions

### Prerequisites
*   C++17 or later compatible compiler (MSVC, GCC, Clang)
*   CMake 3.15+
*   Dependencies installed via vcpkg (Recommended) or system package manager.

### Building
```bash
# Clone the repository
git clone https://github.com/OpenScientificResearchGroup/Core.git
cd Core

# Configure with CMake
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Release
```

## Usage Example

```cpp
#include "Kernel.h"
#include "managers/LogManager.h"

int main() {
    auto& kernel = Core::Kernel::instance();
    kernel.init();

    // Use the Log Manager
    LOG_INFO("Kernel initialized successfully.");

    // Fire an event
    kernel.getEventManager()->publish("app_start", {{"timestamp", 123456}});

    return kernel.run();
}
``` -->

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.