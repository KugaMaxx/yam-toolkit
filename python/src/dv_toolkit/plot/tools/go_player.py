import numpy as np
import dv_processing as dv
from datetime import datetime
import plotly.graph_objects as go

from .func import _reformat_layout, _visualize_events


class Animator(object):
    def __init__(self, canvas, update, ticks, fps=25):
        self._canvas = canvas
        self._frames = [update(i) for i in range(len(ticks))]
        self._ticks = ticks
        self._interval = int(1E3 / fps)
        self._init_widgets()

    def _init_widgets(self):
        self._buttons = [{
            "type": "buttons",
            "buttons": [
                {
                    "method": "animate", "label": "&#9654;",
                    "args": [None, {"frame": {"duration": self._interval},
                                    "mode": "immediate", "fromcurrent": True}]
                },
                {
                    "method": "animate", "label": "&#9724;",
                    "args": [[None], {"frame": {"duration": self._interval},
                                      "mode": "immediate", "fromcurrent": True}]
                }
            ],
            "x": 0.1, "y": 0.0, "direction": "left", "pad": {"r": 10, "t": 70},
        }]

        self._sliders = [{
            "steps": [
                {
                    "method": "animate", "label": self._ticks[i],
                    "args": [[self._ticks[i]], {"frame": {"duration": self._interval},
                                          "mode": "immediate", "fromcurrent": True}]
                } for i, val in enumerate(self._frames)
            ],
            "x": 0.1, "y": 0.0, "len": 0.9, "pad": {"b": 10, "t": 50},
        }]
        self._canvas.layout.update(
            sliders=self._sliders,
            updatemenus=self._buttons,
            template="plotly_white",
        )

    def run(self):
        self._canvas.update(frames=self._frames)
        self._canvas.show()


class Figure(object):
    def __init__(self):
        self._figure = go.Figure()
        self._traces = {
            "inds": list(),
            "rows": list(),
            "cols": list(),
            "plot_func": list(),
            "set_func": list(),
            "kwargs": list(),
        }

        self._index = 0
        self._nrows = 0
        self._ncols = 0
        self._shape = (0, 0)
        self._layouts = None
        self._ticks = range(1)

    def set_ticks(self, ticks):
        self._ticks = ticks

    def set_subplot(self, rows, cols, specs):
        self._nrows = rows
        self._ncols = cols
        self._shape = (rows, cols)
        self._layouts = _reformat_layout(specs, {'2d': 'xy', '3d': 'scene'})

    def append_trace(self, row, col, plot_func, set_func, **kwargs):
        self._index = self._index + 1
        self._traces["rows"].append(row)
        self._traces["cols"].append(col)
        self._traces["plot_func"].append(plot_func)
        self._traces["set_func"].append(set_func)
        self._traces["kwargs"].append(kwargs)
        self._traces["inds"].append(self._index - 1)

    def show(self):
        self._traces["data"] = list()
        for plot_func, kwargs in zip(self._traces["plot_func"], self._traces["kwargs"]):
            self._traces["data"].append(plot_func(**kwargs))
            
        self._figure.set_subplots(rows=self._nrows, cols=self._ncols, specs=self._layouts)
        self._figure.add_traces(data=self._traces["data"],
                                rows=self._traces["rows"], 
                                cols=self._traces["cols"])

        for set_func, kwargs in zip(self._traces["set_func"], self._traces["kwargs"]):
            self._figure.layout.update(set_func(**kwargs))

        Animator(canvas=self._figure, update=self._update, ticks=self._ticks).run()

    def _update(self, k):
        data = list()
        for obj, plot_func, kwargs in zip(self._traces["data"], self._traces["plot_func"], self._traces["kwargs"]):
            data.append(plot_func(obj, k, **kwargs))
        self._traces["data"] = data

        return go.Frame(data=self._traces["data"],
                        traces=self._traces["inds"], name=self._ticks[k])


class Preset(object):
    def __init__(self, size, packets):
        """
        Create a Figure() class with default settings
        """
        self._size = size[::-1]
        self._packets = packets
        self._ticks = range(len(packets))

        self._fr_cmap = 'gray'
        self._ev_cmap = [[0.0, "rgba(223,73,63,1)"],
                         [0.5, "rgba(0,0,0,0)"],
                         [1.0, "rgba(46,102,153,1)"]]

    # ---------------- 2d settings ----------------
    def set_2d_plot(self, **kwargs):
        return {
            "xaxis": dict(range=[0, self._size[1]]),
            "yaxis": dict(range=[0, self._size[0]],
                          scaleanchor="x",
                          scaleratio=1.),
        }

    def plot_2d_event(self, obj=None, i=None, **kwargs):
        if i is None:
            obj = go.Heatmap(
                z=[], zmin=-1, zmax=1,
                colorscale=self._ev_cmap,
                showscale=False,
                showlegend=False,
            )
            return obj

        events = self._packets[i].events()
        if not events.isEmpty():
            image = _visualize_events(events, self._size)
            obj = go.Heatmap(
                z=np.flip(image, axis=0),
            )
        else:
            obj = go.Heatmap(
                z=np.zeros(self._size),
            )            

        return obj

    def plot_2d_frame(self, obj=None, i=None, **kwargs):
        if i is None:
            obj = go.Heatmap(
                z=[], zmin=0, zmax=255,
                colorscale=self._fr_cmap,
                showscale=False,
                showlegend=False,
            )
            return obj

        frames = self._packets[i].frames()
        if not frames.isEmpty():
            image = frames.front().image
            obj = go.Heatmap(
                z=np.flip(image, axis=0)
            )
            self._tmp_frame = frames.front().image
        
        return obj

    # ---------------- 3d settings ----------------
    def set_3d_plot(self, **kwargs):
        return {
            "scene": dict(xaxis=dict(range=[0, self._size[0]]),
                          yaxis=dict(range=[0, self._size[1]]),
                          aspectratio=dict(x=self._size[0]/500, y=self._size[1]/500, z=1.),
                          camera=dict(up=dict(x=1.0, y=0.0, z=0.0))),
        }

    def plot_3d_event(self, obj=None, i=None, **kwargs):
        if i is None:
            obj = go.Scatter3d(
                x=[], y=[], z=[],
                mode='markers',
                marker=dict(
                    size=2,
                    cmin=0,
                    cmax=1,
                    colorscale=self._ev_cmap,
                ),
                showlegend=False,
            )
            return obj
        
        events = self._packets[i].events()
        if not events.isEmpty():
            obj = go.Scatter3d(
                x=self._size[0]-events.ys()-1, y=self._size[1]-events.xs()-1, 
                z=[f"{datetime.fromtimestamp(ts * 1E-6).strftime('%H:%M:%S.%f')}"
                   for ts in events.timestamps()],
                marker=dict(color=events.polarities()),
            )
        else:
            obj = go.Scatter3d(
                x=[], y=[], z=[]
            )
        return obj

    def plot_3d_frame(self, obj=None, i=None, **kwargs):
        if i is None:
            obj = go.Surface(
                x=[], y=[], z=[],
                surfacecolor=np.ones((1000, 1000)),
                colorscale=self._fr_cmap,
                cmin=0,
                cmax=255,
                showscale=False,
                showlegend=False,
            )
            return obj

        frames = self._packets[i].frames()
        if not frames.isEmpty():
            for frame in frames:
                xx, yy = np.mgrid[0:self._size[0], 0:self._size[1]]
                obj = go.Surface(
                    x=xx,
                    y=yy,
                    z=np.full(self._size, datetime.fromtimestamp(frame.timestamp * 1E-6).strftime('%H:%M:%S.%f')),
                    surfacecolor=np.flipud(np.fliplr(frame.image))
                )

        return obj

    def get_ticks(self):
        return self._ticks