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
print(reader.getResolution())