import dv_toolkit as kit
from datetime import timedelta

# Register MonoCameraRead
reader = kit.io.MonoCameraReader("/path/to/aedat4")

# Read offline MonoCameraData
data = reader.loadData()

# Get data resolution
resolution = reader.getResolution()

# Send MonoCameraData to player, modes include:
# hybrid, 3d, 2d
player = kit.plot.OfflineMonoCameraPlayer(resolution, mode="hybrid", core = 'plotly')

# View every 33 millisecond of events
player.viewPerTimeInterval(data, "events", timedelta(milliseconds=33))

# View every 2 elements of frames
player.viewPerNumberInterval(data, "frames", 2)