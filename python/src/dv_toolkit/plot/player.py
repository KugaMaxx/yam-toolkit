from typing import Tuple
from datetime import timedelta

from ..lib import _lib_toolkit as kit

from .tools import mp_player
from .tools import go_player


class OfflineMonoCameraPlayer(object):
    def __init__(self, 
                 resolution: Tuple[int, int], 
                 mode: str = "hybrid", 
                 core: str = "matplotlib"):
        if (len(resolution) == 2):
            self.resolution = resolution
        else:
            raise ValueError("Resolution must be a tuple of length 2.")            
        
        if core == "matplotlib":
            self.core = mp_player
        elif core == "plotly":
            self.core = go_player
        else:
            raise ValueError("Invalid player core. use 'matplotlib' or 'plotly'")
        
        if mode in ["hybrid", "2d", "3d"]:
            self.mode = mode
        else:
            raise ValueError("Invalid view mode. use '2d', '3d' or 'hybrid'")

    def viewPerTimeInterval(self, data, 
                            reference: str = "events",
                            interval: timedelta = timedelta(milliseconds=33)):
        # slice data
        slicer, packets = kit.MonoCameraSlicer(), list()
        slicer.doEveryTimeInterval(
            reference, interval, lambda data: packets.append(data)
        )
        slicer.accept(data)

        # run
        self._run(
            self.core.Figure(),
            self.core.Preset(self.resolution, packets)
        )

    def viewPerNumberInterval(self, data, 
                              reference: str = "events",
                              interval: int = 15000):
        # slice data
        slicer, packets = kit.MonoCameraSlicer(), list()
        slicer.doEveryNumberOfElements(
            reference, interval, lambda data: packets.append(data)
        )
        slicer.accept(data)
        
        # run
        self._run(
            self.core.Figure(),
            self.core.Preset(self.resolution, packets)
        )

    def _run(self, figure, preset):
        figure.set_ticks(preset.get_ticks())
        if self.mode == "2d":
            figure.set_subplot(rows=1, cols=1, specs=[[{"type": "2d"}]])
            figure.append_trace(row=1, col=1, plot_func=preset.plot_2d_frame, set_func=preset.set_2d_plot)
            figure.append_trace(row=1, col=1, plot_func=preset.plot_2d_event, set_func=preset.set_2d_plot)
        elif self.mode == "3d":
            figure.set_subplot(rows=1, cols=1, specs=[[{"type": "3d"}]])
            figure.append_trace(row=1, col=1, plot_func=preset.plot_3d_frame, set_func=preset.set_3d_plot)
            figure.append_trace(row=1, col=1, plot_func=preset.plot_3d_event, set_func=preset.set_3d_plot)
        else:
            figure.set_subplot(rows=1, cols=2, specs=[[{"type": "2d"}, {"type": "3d"}]])
            figure.append_trace(row=1, col=1, plot_func=preset.plot_2d_frame, set_func=preset.set_2d_plot)
            figure.append_trace(row=1, col=1, plot_func=preset.plot_2d_event, set_func=preset.set_2d_plot)
            figure.append_trace(row=1, col=2, plot_func=preset.plot_3d_frame, set_func=preset.set_3d_plot)
            figure.append_trace(row=1, col=2, plot_func=preset.plot_3d_event, set_func=preset.set_3d_plot)
        figure.show()
