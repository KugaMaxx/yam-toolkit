import dv_toolkit as kit
from datetime import timedelta

# Initialize reader
reader = kit.io.MonoCameraReader("/home/kuga/Workspace/tmp-dvstoolkit-v2/data/test-01.aedat4")

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