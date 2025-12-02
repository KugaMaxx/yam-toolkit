# DV Toolkit 

![](https://img.shields.io/github/v/tag/KugaMaxx/yam-toolkit?style=flat-square)
![](https://img.shields.io/github/license/KugaMaxx/yam-toolkit?style=flat-square)

DV Toolkit is an extension of [dv-processing](https://dv-processing.inivation.com/), 
providing a series of wrapped modules to help users to do processing and 
analysis on event camera data. Any questions, please contact me with 
[KugaMaxx@outlook.com](mailto:KugaMaxx@outlook.com).

<div align=center><img src="https://github.com/KugaMaxx/yam-toolkit/blob/main/assets/images/demonstrate.gif" alt="demonstrate" width="100%"></div>

## Background

### Why create this extension?

The dv-processing library provides convenience for developers to handle event-based data. However, there is still room for improvement in **data reading, slicing, and visualization** during actual usage. Therefore, this repository has made the following improvements:

+ Enables fast interoperation between C++ and Python.
+ Adds data alignment and slicing for events, frames, imus, and triggers.
+ Adds offline data reading and processing, eliminating the need for online operations.
+ Implements a more convenient visual interactive interface in Python.
+ Provides a unified usage method for C++ and Python interfaces.

## Installation

### Preliminaries

Since our library is an extension of dv-processing, you need to install dv-processing first. The official installation tutorial is available [here](https://dv-processing.inivation.com/rel_1.7/installation.html), or you can follow the steps below to install it on Ubuntu 20.04:

+ Install necessary dependencies that dv-processing required:

    ```bash
    sudo apt-get install libboost-all-dev libeigen3-dev libopencv-dev
    sudo apt-get install pybind11-dev python3-dev python3-numpy
    ```

+ Add repository and install dv-processing:

    ```bash
    sudo add-apt-repository ppa:inivation-ppa/inivation
    sudo apt-get update
    sudo apt-get install libcaer-dev libfmt-dev liblz4-dev libzstd-dev libssl-dev
    sudo apt-get install dv-processing
    ```

### Git submodule usage

Assuming you are working on a project in Git, you can use this repository as a 
submodule. Here is an example of placing this repository as a dependency in the
 `/external` folder.

+ Add the repository as a submodule in your project:

    ```bash
    git submodule add git@github.com:KugaMaxx/yam-toolkit.git external/dv-toolkit
    ```

+ Use in your cmake project:

    ```CMake
    # Find dv-processing supports
    find_package(dv-processing REQUIRED)

    # Install toolkit supports.
    add_subdirectory(external/dv-toolkit)

    # link your targets against the library
    target_link_libraries(your_target
        dv::processing
        dv::toolkit
        ...)
    ```

### Python package usage

This repository also allows for Python integration, which can be used by binding 
it as a Python package before usage. Here is an example in a conda environment 
named `toolkit`. Please refer to the [README](https://github.com/KugaMaxx/yam-toolkit/blob/main/python/README.md) to get more information.

+ Create conda environment and install:

    ```bash
    # recommend python ≥ 3.8
    conda create -n toolkit

    # activate environment
    conda activate toolkit

    # include pybind11 as submodule
    git submodule update --init

    # install as package
    pip install .
    ```

+ Import the library in your script:

    ```python
    # must have
    import dv_processing as dv

    # introduce extension
    import dv_toolkit as kit
    ```

<!-- ### Include as C++ library

+ Compile this project with CMake, including build

```bash
# create folder
mkdir build && cd build

# compile with samples
CC=gcc-10 CXX=g++-10 cmake .. -DENABLE_SAMPLES=ON

# generate library
cmake --build . --config Release
``` -->

## Getting started

### Standard Type

The dv-toolkit library encapsulates `event`, `frame`, `imu`, and `trigger` into 
addressable storage types, which supports add, erase and slice operations.

+ `dv::toolkit::MonoCameraData` stores all basic storage types of event-based data.

    ```C++
    #include <dv-toolkit/core/core.hpp>

    int main() {
        namespace kit = dv::toolkit;

        // Initialize MonoCameraData
        kit::MonoCameraData data;

        // Access immutable variables through functions, support types are:
        // events, frames, imus and triggers
        std::cout << data.events() << std::endl;
        // "Storage is empty!"

        // Emplace back event elements, the function arguments are:
        // timestamp, x, y, polarity
        kit::EventStorage store;
        store.emplace_back(dv::now(), 0, 0, false);
        store.emplace_back(dv::now(), 1, 1, true);
        store.emplace_back(dv::now(), 2, 2, false);
        store.emplace_back(dv::now(), 3, 3, true);

        // Assignment value to MonoCameraData
        data["events"] = store;

        // Access mutable variables through std::get
        std::cout << std::get<kit::EVTS>(data["events"]) << std::endl;
        // "Storage containing 4 elements within ..."

        return 0;
    }
    ```

### I/O Operations

The dv-toolkit library provide convenient method to read and write standard 
aedat4 files offline. It facilitates the repeated operation and processing on 
data, avoiding the issue of reading and writing files over and over again.

+ `dv::toolkit::MonoCameraReader` reads offline data from standard aedat4 
files.

    ```C++
    #include <dv-toolkit/io/reader.hpp>

    int main() {
        namespace kit = dv::toolkit;

        // Initialize reader
        kit::io::MonoCameraReader reader("/path/to/aedat4");
        
        // Get offline MonoCameraData
        kit::MonoCameraData data = reader.loadData();
        
        // Check all basic types
        std::cout << data.events()   << std::endl;
        std::cout << data.frames()   << std::endl;
        std::cout << data.imus()     << std::endl;
        std::cout << data.triggers() << std::endl;

        // Get camera resolution
        // Can also use reader.getEventResolution()
        const auto resolution = reader.getResolution("events");
        if (resolution.has_value()) {
            std::cout << *resolution << std::endl;
        }

        return 0;
    }
    ```

+ `dv::toolkit::MonoCameraWrite` write data back to standard aedat4 files.

    ```C++
    #include <dv-toolkit/core/core.hpp>
    #include <dv-toolkit/io/reader.hpp>
    #include <dv-toolkit/io/writer.hpp>
    #include <dv-toolkit/simulation/generator.hpp>

    int main() {
        namespace kit = dv::toolkit;

        // Enable literal time expression from the chrono library
        using namespace std::chrono_literals;
        
        // Initialize MonoCameraData
        kit::MonoCameraData data;

        // Initialize resolution
        const auto resolution = cv::Size(346, 260);

        // Create sample events
        data["events"] = kit::simulation::generateSampleEvents(resolution);

        // Initialize MonoCameraWriter
        kit::io::MonoCameraWriter writer("/path/to/file.aedat4", resolution);

        // Write MonoCameraData
        writer.writeData(data);

        return 0;
    }
    ```

### Unified stream slicing

Thanks to the redefined standard data structure, it is possible to achieve 
unified slicing of various types of data by using dv-toolkit library.

+ `dv::toolkit::MonoCameraSlicer` allows to slice by time or by number. Before registering each slicer, please ensure that the reference type of the slicer
 is specified.

    ```C++
    #include <dv-toolkit/core/slicer.hpp>
    #include <dv-toolkit/io/reader.hpp>

    int main() {
        namespace kit = dv::toolkit;

        // Initialize reader
        kit::io::MonoCameraReader reader("/path/to/aedat4");
        
        // Get offline MonoCameraData
        kit::MonoCameraData data = reader.loadData();

        // Initialize slicer, it will have no jobs at this time
        kit::MonoCameraSlicer slicer;

        // Use this namespace to enable literal time expression from the chrono library
        using namespace std::chrono_literals;

        // Register this method to be called every 33 millisecond of events
        slicer.doEveryTimeInterval("events", 33ms, [](const kit::MonoCameraData &mono) {
            std::cout << mono.events() << std::endl;
        });

        // Register this method to be called every 2 elements of frames
        slicer.doEveryNumberOfElements("frames", 2, [](const kit::MonoCameraData &mono) {
            std::cout << mono.events() << std::endl;
        });

        // Now push the store into the slicer, the data contents within the store
        // can be arbitrary, the slicer implementation takes care of correct slicing
        // algorithm and calls the previously registered callbacks accordingly.
        slicer.accept(data);

        return 0;
    }
    ```

## Acknowledgement

Special thanks to [Jinze Chen](mailto:chjz@mail.ustc.edu.cn).
