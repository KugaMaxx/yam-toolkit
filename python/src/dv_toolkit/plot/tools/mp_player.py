import itertools
import numpy as np
import dv_processing as dv
from datetime import datetime

import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import matplotlib.colors as mcolors
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button, Slider, TextBox
from mpl_toolkits.axes_grid1 import make_axes_locatable

from .func import _reformat_layout, _ravel_multi_index, _visualize_events, _convert_to_rgba


class Animator(FuncAnimation):
    """
    Animated interactive plot using matplotlib
    https://stackoverflow.com/questions/46325447/animated-interactive-plot-using-matplotlib
    """

    def __init__(self, canvas, update, ticks, fps=25) -> None:
        self._canvas = canvas
        self._frames = update
        self._ticks = ticks
        self._len = len(ticks) - 1
        self._pos = -1
        self._min = 0
        self._max = self._len if self._len > 0 else 1
        self._run = True
        self._forward = True
        self._interval = int(1E3 / fps)
        self._init_widgets()
        super().__init__(self._canvas, self._frames, frames=self._play(), interval=self._interval, save_count=self._len)

    def _init_widgets(self):
        # add another axes
        playerax = self._canvas.add_axes([0.13, 0.05, 0.74, 0.04])
        divider = make_axes_locatable(playerax)

        # create button on axes
        bax = divider.append_axes("right", size="100%", pad="15%")
        sax = divider.append_axes("right", size="100%", pad="15%")
        fax = divider.append_axes("right", size="100%", pad="15%")
        ofax = divider.append_axes("right", size="100%", pad="15%")
        sliderax = divider.append_axes("right", size="700%", pad="30%")
        textax = divider.append_axes("right", size="200%", pad="30%")
        self.button_oneback = Button(playerax, label='$\u29CF$')
        self.button_backward = Button(bax, label='$\u25C0$')
        self.button_pauseplay = Button(sax, label='$\u25A0$')
        self.button_forward = Button(fax, label='$\u25B6$')
        self.button_onenext = Button(ofax, label='$\u29D0$')
        self.slider = Slider(sliderax, '', self._min, self._max, valstep=1)
        self.text = TextBox(textax, '', color='1.', textalignment='center')

        # create button functions
        self.button_oneback.on_clicked(self._play_back_frame)
        self.button_backward.on_clicked(self._play_backwards)
        self.button_pauseplay.on_clicked(self._set_stop)
        self.button_forward.on_clicked(self._play_forwards)
        self.button_onenext.on_clicked(self._play_next_frame)
        self.slider.on_changed(self._set_position)
        self.slider.valtext.set_visible(False)

    def run(self, path=None, fps=30):
        if path is None:
            plt.show()
        else:
            self.save(path, writer='imagemagick', fps=fps)

    # ---------------- call back function ----------------
    def _play(self):
        while self._run:
            self._one_step()
            yield self._pos

    def _one_step(self):
        if self._len == 0:
            self._pos = 0
            self.event_source.stop()
        elif 0 < self._pos < self._len:
            self._pos += self._forward - (not self._forward)
        elif self._pos <= 0:
            self._pos += self._forward
        elif self._pos >= self._len and not self._forward:
            self._pos -= not self._forward
        self.slider.set_val(self._pos)
        self.text.set_val(f"{self._ticks[self._pos]}")
        self._frames(self._pos)
        self._canvas.canvas.draw_idle()

    def _play_back_frame(self, event=None):
        self._run, self._forward = False, False
        self.event_source.stop()
        self._one_step()

    def _play_backwards(self, event=None):
        self._run, self._forward = True, False
        self.event_source.start()

    def _set_stop(self, event=None):
        if not self._run:
            self._run = True
            self.event_source.start()
        elif self._run:
            self._run = False
            self.event_source.stop()

    def _play_forwards(self, event=None):
        self._run, self._forward = True, True
        self.event_source.start()

    def _play_next_frame(self, event=None):
        self._run, self._forward = False, True
        self.event_source.stop()
        self._one_step()

    def _set_position(self, i):
        self._pos = int(self.slider.val)
        self.text.set_val(f"{self._ticks[self._pos]}")
        self._frames(self._pos)


class Figure(object):
    def __init__(self):
        self._figure = plt.figure(figsize=(16, 9))
        self._traces = {
            "inds": list(),
            "rows": list(),
            "cols": list(),
            "plot_func": list(),
            "set_func": list(),
            "kwargs": list(),
            "AxesSubplots": list(),
            "AxesImages": list(),
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
        self._layouts = _reformat_layout(specs, {'2d': None, '3d': '3d'})

    def append_trace(self, row, col, plot_func, set_func, **kwargs):
        self._index = self._index + 1
        self._traces["rows"].append(row)
        self._traces["cols"].append(col)
        self._traces["plot_func"].append(plot_func)
        self._traces["set_func"].append(set_func)
        self._traces["kwargs"].append(kwargs)
        self._traces["inds"].append(_ravel_multi_index(row-1, col-1, self._shape))

    def show(self):
        ax_list = list()
        for ind, _layout in enumerate(list(itertools.chain(*self._layouts))):
            ax_list.append(self._figure.add_subplot(*self._shape, ind+1, projection=_layout["type"]))

        for ind, plot_func, set_func, kwargs \
            in zip(self._traces["inds"], self._traces["plot_func"], self._traces["set_func"], self._traces["kwargs"]):
            AxesSubplot = set_func(ax_list[ind], **kwargs)
            self._traces["AxesSubplots"].append(AxesSubplot)
            self._traces["AxesImages"].append(plot_func(AxesSubplot, **kwargs))

        Animator(canvas=self._figure, update=self._update, ticks=self._ticks).run()

    def _update(self, k):
        for i in range(self._index):
            plot_func = self._traces["plot_func"][i]
            AxesSubplot, AxesImage = self._traces["AxesSubplots"][i], self._traces["AxesImages"][i]
            self._traces["AxesImages"][i] = plot_func(AxesSubplot, AxesImage, k, **self._traces["kwargs"][i])


class Preset(object):
    def __init__(self, size, packets) -> None:
        """
        Create a Figure() class with default settings
        """
        self._size = size[::-1]
        self._packets = packets
        self._ticks = range(len(packets))

        self._fr_cmap = plt.get_cmap('gray')
        self._ev_cmap = mcolors.LinearSegmentedColormap.from_list("evCmap", [(0.871, 0.286, 0.247, 1.),
                                                                             (0.000, 0.000, 0.000, 0.),
                                                                             (0.180, 0.400, 0.600, 1.)])

    # ---------------- 2d settings ----------------
    def set_2d_plot(self, ax, **kwargs):
        ax.set_xlim(0, self._size[1])
        ax.set_ylim(0, self._size[0])
        ax.set_xticks([])
        ax.set_yticks([])
        return ax

    def plot_2d_event(self, ax, obj=None, i=None, **kwargs):
        if i is None:
            obj = ax.imshow(
                np.zeros(self._size), 
                vmin=-1, vmax=1,
                cmap=self._ev_cmap
            )        
            return obj

        events = self._packets[i].events()
        if not events.isEmpty():
            image = _visualize_events(events, self._size)
            obj.set_data(np.flip(image, axis=0))
        else:
            obj.set_data(np.zeros(self._size))

        return obj

    def plot_2d_frame(self, ax, obj=None, i=None, **kwargs):
        if i is None:
            obj = ax.imshow(
                np.zeros(self._size),
                vmin=0, vmax=255,
                cmap=self._fr_cmap
            )
            return obj
        
        frames = self._packets[i].frames()
        if not frames.isEmpty():
            image = frames.front().image
            obj.set_data(np.flip(image, axis=0))
        
        return obj

    # ---------------- 3d settings ----------------
    def set_3d_plot(self, ax, **kwargs):
        ax.set_ylim3d(0, self._size[0])
        ax.set_zlim3d(0, self._size[1])
        ax.set_box_aspect((500, self._size[0], self._size[1]))
        ax.view_init(elev=35, azim=-10, roll=-95)
        return ax
    
    def plot_3d_event(self, ax, obj=None, i=None, **kwargs):
        if i is None:
            obj = ax.scatter([], [], [], c=[], s=1.0, vmin=0, vmax=1, cmap=self._ev_cmap)
            return obj

        events = self._packets[i].events()
        if not events.isEmpty():
            obj._offsets3d = (events.timestamps(), events.ys(), events.xs())
            obj.set_array(events.polarities())
            ax.set_xlim3d(events.getLowestTime(), events.getHighestTime())
            ax.set_xticks(np.linspace(events.getLowestTime(), events.getHighestTime(), 5))
            ax.set_xticklabels([f"{datetime.fromtimestamp(ts * 1E-6).strftime('%H:%M:%S')}s" 
                                for ts in ax.get_xticks().tolist()])
        else:
            obj._offsets3d = ([], [], [])    
        
        return obj

    def plot_3d_frame(self, ax, obj=None, i=None, **kwargs):
        xx, yy = np.mgrid[0:self._size[0], 0:self._size[1]]
        if i is None:
            obj = ax.plot_surface(np.nan, xx, yy, facecolors=np.full((*self._size, 1), np.nan))
            return obj
        
        frames = self._packets[i].frames()
        if not frames.isEmpty():
            obj.remove()
            for frame in frames:
                image = _convert_to_rgba(frame.image) / 255.0
                obj = ax.plot_surface(frame.timestamp, xx, yy, facecolors=image)

        return obj

    def get_ticks(self):
        return self._ticks
