import numpy as np
from abc import ABC, abstractmethod


def _unravel_index(index, shape):
    row, col = np.unravel_index(index, shape)
    return row, col


def _ravel_multi_index(row, col, shape):
    ind = np.ravel_multi_index(np.array([row, col]), shape)
    return ind


def _reformat_layout(list_of_dict, mapping: dict):
    for _list in list_of_dict:
        for _dict in _list:
            _dict['type'] = mapping[_dict['type']]

    return list_of_dict


def _visualize_events(events, size, mode="accumulate"):
    counts = np.zeros(size)
    _bins  = [size[0], size[1]]
    _range = [[0, size[0]], [0, size[1]]]

    # w/ polar
    if mode == 'polar':
        counts = np.histogram2d(events.ys(), events.xs(), 
                                weights=(-1) ** (1 + events.polarities()), 
                                bins=_bins, range=_range)[0]
        return counts
    
    # w/o polar
    elif mode == 'monopolar':
        counts = np.histogram2d(events.ys(), events.xs(), 
                                weights=(+1) ** (1 + events.polarities()), 
                                bins=_bins, range=_range)[0]
        return counts
    
    # count before polar assignment
    elif mode == 'accumulate':
        counts = np.histogram2d(events.ys(), events.xs(), 
                                weights=(+1) ** (1 + events.polarities()), 
                                bins=_bins, range=_range)[0]
        weight = np.zeros(size)
        weight[events.ys(), events.xs()] = (-1) ** (1 + events.polarities())
        return counts * weight

    return counts

def _convert_to_rgba(image):
    if len(image.shape) == 2:
        image = np.stack((image,) * 3, axis=-1)
    return np.concatenate((image, 255 * np.ones((*image.shape[:2], 1))), axis=2)
