# Python bindings

## Introduction

Binding dv::toolkit to Python helps us use more convenient third-party libraries
to do data processing and visualization. Therefore, we are committed to achieving 
interoperability between the dv::toolkit library in Python and C++, and unify 
their usage as much as possible.

## Getting Started

### Standard type

+ `MonoCameraData()` help access and store types through `__getitem__` and `__setitem__`.

    ```python
    import dv_toolkit as kit

    # Initialize MonoCameraData
    data = kit.MonoCameraData()

    # Access immutable variables through functions, support types are:
    # events, frames, imus and triggers
    print(data.events())
    # "Storage is empty!"

    # Emplace back event elements, the function arguments are:
    # timestamp, x, y, polarity
    store = kit.EventStorage()
    store.emplace_back(dv::now(), 0, 0, false)
    store.emplace_back(dv::now(), 1, 1, true)
    store.emplace_back(dv::now(), 2, 2, false)
    store.emplace_back(dv::now(), 3, 3, true)

    # Assignment value to MonoCameraData
    data["events"] = store;

    # Access mutable variables through __getitem__
    print(data["events"])
    # "Storage containing 4 elements within ..."

    return 0;
    ```

### I/O Operations

+ `MonoCameraReader()` load aedat4 file as offline data.

    ```python
    import dv_toolkit as kit

    # Initialize reader
    reader = kit.io.MonoCameraReader("/path/to/file.aedat4")

    # Get offline MonoCameraData
    data = reader.loadData()

    # Check all basic types
    print(data.events())
    print(data.frames()) 
    print(data.imus()) 
    print(data.triggers())

    # Get camera resolution
    # Can also use reader.getEventResolution()
    print(reader.getResolution("events"))
    ```

+ `MonoCameraWriter()` write offline data to aedat4 file.

    ```python
    import dv_toolkit as kit

    # Initialize MonoCameraData
    data = kit.MonoCameraData()

    # Initialize resolution
    resolution = (346, 260)

    # Create sample events
    data["events"] = kit.simulation.generateSampleEvents(resolution)

    # Initialize MonoCameraWriter
    writer = kit.io.MonoCameraWriter("/path/to/file.aedat4", resolution)

    # Write MonoCameraData
    writer.writeData(data)
    ```

### Unified stream slicing

+ `MonoCameraSlicer()` helps uniformly cut data.

    ```python
    import dv_toolkit as kit
    from datetime import timedelta

    # Initialize reader
    reader = kit.io.MonoCameraReader("/path/to/file.aedat4")

    # Get offline MonoCameraData
    data = reader.loadData()

    # Initialize slicer, it will have no jobs at this time
    slicer = kit.MonoCameraSlicer()

    # Print events
    def print_event_info(data):
        print(data["events"])

    # Register this method to be called every 33 millisecond of events
    slicer.doEveryTimeInterval("events", timedelta(milliseconds=33), print_event_info)

    # Register this method to be called every 2 elements of frames
    slicer.doEveryNumberOfElements("frames", 2, print_event_info)

    # Now push the store into the slicer, the data contents within the store
    # can be arbitrary, the slicer implementation takes care of correct slicing
    # algorithm and calls the previously registered callbacks accordingly.
    slicer.accept(data)
    ```

### Utilities

+ `OfflineMonoCameraPlayer()` gives a glance at events stream. It can change 
core between matplotlib and plotly.

    ```python
    import dv_toolkit as kit
    from datetime import timedelta

    # Register MonoCameraRead
    reader = kit.io.MonoCameraReader("/path/to/aedat4")

    # Read offline MonoCameraData
    data = reader.loadData()

    # Get data resolution
    # Can also use reader.getEventResolution()
    resolution = reader.getResolution("events")

    # Send MonoCameraData to player, modes include:
    # hybrid, 3d, 2d
    player = kit.plot.OfflineMonoCameraPlayer(resolution, mode="hybrid", core = 'plotly')

    # View every 33 millisecond of events
    player.viewPerTimeInterval(data, "events", timedelta(milliseconds=33))

    # View every 2 elements of frames
    player.viewPerNumberInterval(data, "frames", 2)
    ```

---