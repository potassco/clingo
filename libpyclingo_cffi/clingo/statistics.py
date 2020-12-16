'''
This module contains functions and classes related to statistics.

Examples
--------
The following example shows how to dump the solving statistics in json format:

    >>> import json
    >>> import clingo
    >>> ctl = clingo.Control()
    >>> ctl.add("base", [], "{a}.")
    >>> ctl.ground([("base", [])])
    >>> ctl.solve()
    SAT
    >>> print(json.dumps(ctl.statistics['solving'], sort_keys=True, indent=4,
    ... separators=(',', ': ')))
    {
        "solvers": {
            "choices": 1.0,
            "conflicts": 0.0,
            "conflicts_analyzed": 0.0,
            "restarts": 0.0,
            "restarts_last": 0.0
        }
    }
'''
from typing import (AbstractSet, Any, Iterable, KeysView, Sequence, Tuple, MutableMapping, MutableSequence, Union,
                    ValuesView,
                    cast)
from collections import abc

from ._internal import _c_call, _handle_error, _lib, _to_str

__all__ = [ 'StatisticsArray', 'StatisticsMap', 'StatisticsValue' ]

StatisticsValue = Union['StatisticsArray', 'StatisticsMap', float]

def _statistics_type(stats, key):
    return _c_call('clingo_statistics_type_t', _lib.clingo_statistics_type, stats, key)

def _statistics(stats, key):
    '''
    Transform clingo's statistics into python type.
    '''
    type_ = _statistics_type(stats, key)

    if type_ == _lib.clingo_statistics_type_value:
        return _c_call('double', _lib.clingo_statistics_value_get, stats, key)

    if type_ == _lib.clingo_statistics_type_array:
        ret = []
        for i in range(_c_call('size_t', _lib.clingo_statistics_array_size, stats, key)):
            ret.append(_statistics(stats, _c_call('uint64_t', _lib.clingo_statistics_array_at, stats, key, i)))
        return ret

    assert type_ == _lib.clingo_statistics_type_map
    ret = {}
    for i in range(_c_call('size_t', _lib.clingo_statistics_map_size, stats, key)):
        name = _c_call('char*', _lib.clingo_statistics_map_subkey_name, stats, key, i)
        subkey = _c_call('uint64_t', _lib.clingo_statistics_map_at, stats, key, name)
        ret[_to_str(name)] = _statistics(stats, subkey)
    return ret

def _mutable_statistics_value_type(value):
    if isinstance(value, str):
        raise TypeError('unexpected string')
    if isinstance(value, abc.Sequence):
        return _lib.clingo_statistics_type_array
    if isinstance(value, abc.Mapping):
        return _lib.clingo_statistics_type_map
    return _lib.clingo_statistics_type_value

def _mutable_statistics(stats) -> 'StatisticsMap':
    key = _c_call('uint64_t', _lib.clingo_statistics_root, stats)
    return cast(StatisticsMap, _mutable_statistics_get(stats, key))

def _mutable_statistics_get(stats, key) -> StatisticsValue:
    type_ = _statistics_type(stats, key)

    if type_ == _lib.clingo_statistics_type_array:
        return StatisticsArray(stats, key)

    if type_ == _lib.clingo_statistics_type_map:
        return StatisticsMap(stats, key)

    assert type_ == _lib.clingo_statistics_type_value
    return _c_call('double', _lib.clingo_statistics_value_get, stats, key)

def _mutable_statistics_set(stats, key, type_, value, update):
    if type_ == _lib.clingo_statistics_type_map:
        StatisticsMap(stats, key).update(value)
    elif type_ == _lib.clingo_statistics_type_array:
        StatisticsArray(stats, key).update(value)
    else:
        assert type_ == _lib.clingo_statistics_type_value
        if callable(value):
            if update:
                uval = value(_c_call('double', _lib.clingo_statistics_value_get, stats, key))
            else:
                uval = value(None)
        else:
            uval = value
        _handle_error(_lib.clingo_statistics_value_set(stats, key, uval))

class StatisticsArray(MutableSequence[StatisticsValue]):
    '''
    Object to modify statistics stored in an array.

    Note that only inplace concatenation and no deletion is supported.

    Implements: `MutableSequence[Union[StatisticsArray,StatisticsMap,float]]`.

    See Also
    --------
    Control.solve

    Notes
    -----
    The `StatisticsArray.update` function provides convenient means to initialize
    and modify a statistics array.
    '''
    def __init__(self, rep, key):
        self._rep = rep
        self._key = key

    def __len__(self):
        return _c_call('size_t', _lib.clingo_statistics_array_size, self._rep, self._key)

    def __getitem__(self, index):
        if index < 0 or index >= len(self):
            raise IndexError('invalid index')
        key = _c_call('uint64_t', _lib.clingo_statistics_array_at, self._rep, self._key, index)
        return _mutable_statistics_get(self._rep, key)

    def __setitem__(self, index, value):
        key = _c_call('uint64_t', _lib.clingo_statistics_array_at, self._rep, self._key, index)
        type_ = _statistics_type(self._rep, key)
        _mutable_statistics_set(self._rep, key, type_, value, True)

    def insert(self, index, value):
        '''
        This method is not supported.
        '''
        raise NotImplementedError('insertion is not supported')

    def __delitem__(self, index):
        raise NotImplementedError('deletion is not supported')

    def __add__(self, other):
        raise NotImplementedError('only inplace concatenation is supported')

    def __iadd__(self, other):
        for x in other:
            self.append(x)
        return self

    def append(self, value: Any) -> None:
        '''
        Append a value.

        Parameters
        ----------
        value : Any
            A nested structure composed of floats, sequences, and mappings.

        Returns
        -------
        None
        '''
        type_ = _mutable_statistics_value_type(value)
        key = _c_call('uint64_t', _lib.clingo_statistics_array_push, self._rep, self._key, type_)
        _mutable_statistics_set(self._rep, key, type_, value, False)

    def extend(self, values: Iterable[Any]) -> None:
        '''
        Extend the statistics array with the given values.

        Paremeters
        ----------
        values : Sequence[Any]
            A sequence of nested structures composed of floats, sequences, and
            mappings.

        Returns
        -------
        None

        See Also
        -----
        append
        '''
        self += values

    def update(self, values: Sequence[Any]) -> None:
        '''
        Update a statistics array.

        Parameters
        ----------
        values : Sequence[Any]
            A sequence of nested structures composed of floats, callable, sequences,
            and mappings. A callable can be used to update an existing value, it
            receives the previous numeric value (or None if absent) as argument and
            must return an updated numeric value.

        Returns
        -------
        None
        '''
        n = len(self)
        for idx, val in enumerate(values):
            if idx < n:
                self[idx] = val
            else:
                self.append(val)

class StatisticsMap(MutableMapping[str, StatisticsValue]):
    '''
    Object to capture statistics stored in a map.

    This class does not support item deletion.

    Implements: `MutableMapping[str,Union[StatisticsArray,StatisticsMap,float]]`.

    See Also
    --------
    Control.solve

    Notes
    -----
    The `StatisticsMap.update` function provides convenient means to initialize
    and modify a statistics map.
    '''
    def __init__(self, rep, key):
        self._rep = rep
        self._key = key

    def __len__(self):
        return _c_call('size_t', _lib.clingo_statistics_map_size, self._rep, self._key)

    def __getitem__(self, name: str):
        if not name in self:
            raise KeyError('key not found')
        key = _c_call('uint64_t', _lib.clingo_statistics_map_at, self._rep, self._key, name.encode())
        return _mutable_statistics_get(self._rep, key)

    def __setitem__(self, name: str, value: Any):
        has_key = name in self
        if has_key:
            key = _c_call('uint64_t', _lib.clingo_statistics_map_at, self._rep, self._key, name.encode())
            type_ = _statistics_type(self._rep, key)
        else:
            type_ = _mutable_statistics_value_type(value)
            key = _c_call('uint64_t', _lib.clingo_statistics_map_add_subkey, self._rep, self._key, name.encode(), type_)
        _mutable_statistics_set(self._rep, key, type_, value, has_key)

    def __delitem__(self, index):
        raise NotImplementedError('deletion is not supported')

    def __contains__(self, name):
        # note: has to be done like this because of python's container model
        if not isinstance(name, str):
            return False
        return _c_call('bool', _lib.clingo_statistics_map_has_subkey, self._rep, self._key, name.encode())

    def __iter__(self):
        return iter(self.keys())

    def items(self) -> AbstractSet[Tuple[str, StatisticsValue]]:
        '''
        Return the items of the map.

        Returns
        -------
        AbstractSet[Tuple[str,StatisticsValue]]
            The items of the map.
        '''
        ret = []
        for i in range(len(self)):
            name = _c_call('char*', _lib.clingo_statistics_map_subkey_name, self._rep, self._key, i)
            key = _c_call('uint64_t', _lib.clingo_statistics_map_at, self._rep, self._key, name)
            ret.append((_to_str(name), _mutable_statistics_get(self._rep, key)))
        # Note: to shut up the type checker; this should work fine in practice
        return cast(AbstractSet[Tuple[str, StatisticsValue]], ret)

    def keys(self) -> KeysView[str]:
        '''
        Return the keys of the map.

        Returns
        -------
        KeysView[str]
            The keys of the map.
        '''
        ret = []
        for i in range(len(self)):
            ret.append(_to_str(_c_call('char*', _lib.clingo_statistics_map_subkey_name, self._rep, self._key, i)))
        # Note: to shut up the type checker; this should work fine in practice
        return cast(KeysView[str], ret)

    def update(self, values):
        '''
        Update the map with the given values.

        Parameters
        ----------
        values : Mapping[Any]
            A mapping of nested structures composed of floats, callable, sequences,
            and mappings. A callable can be used to update an existing value, it
            receives the previous numeric value (or None if absent) as argument and
            must return an updated numeric value.

        Returns
        -------
        None
        '''
        for key, value in values.items():
            self[key] = value

    def values(self) -> ValuesView[StatisticsValue]:
        '''
        Return the values of the map.

        Returns
        -------
        ValuesView[Union[StatisticsArray,StatisticsMap,float]]
            The values of the map.
        '''
        ret = []
        for i in range(len(self)):
            name = _c_call('char*', _lib.clingo_statistics_map_subkey_name, self._rep, self._key, i)
            key = _c_call('uint64_t', _lib.clingo_statistics_map_at, self._rep, self._key, name)
            ret.append(_mutable_statistics_get(self._rep, key))
        # Note: to shut up the type checker; this should work fine in practice
        return cast(ValuesView[StatisticsValue], ret)
