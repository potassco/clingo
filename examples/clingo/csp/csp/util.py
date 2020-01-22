"""
Utility functions.
"""

import math

try:
    # Note: Can be installed to play with realistic domain sizes.
    import sortedcontainers
    _HAS_SC = True
except ImportError:
    import bisect as bi
    _HAS_SC = False


if hasattr(math, "gcd"):
    # pylint: disable=invalid-name
    gcd = math.gcd
else:
    def gcd(x, y):
        """
        Calculate the gcd of the given integers.
        """
        x, y = abs(x), abs(y)
        while y:
            x, y = y, x % y
        return x


def lerp(x, y):
    """
    Linear interpolation between integers `x` and `y` with a factor of `.5`.
    """
    # NOTE: integer division with floor
    return x + (y - x) // 2


def remove_if(rng, pred):
    """
    Remove each element from `rng` for which `pred` evaluates to true by
    swapping them to the end of the sequence.

    The function returns the number of elements retained.
    """
    j = 0
    for i, x in enumerate(rng):
        if pred(x):
            continue
        if i != j:
            rng[i], rng[j] = rng[j], rng[i]
        j += 1
    return j


class TodoList(object):
    """
    Simple class implementing something like an OrderedSet, which is missing
    from pythons collections module.

    The container is similar to Python's set but maintains insertion order.
    """
    def __init__(self):
        """
        Construct an empty container.
        """
        self._seen = set()
        self._list = []

    def __len__(self):
        return len(self._list)

    def __contains__(self, x):
        return x in self._seen

    def __iter__(self):
        return iter(self._list)

    def __getitem__(self, val):
        return self._list[val]

    def add(self, x):
        """
        Add `x` to the container if it is not yet contained in it.

        Returns true if the element has been inserted.
        """
        if x not in self:
            self._seen.add(x)
            self._list.append(x)
            return True
        return False

    def extend(self, i):
        """
        Calls `add` for each element in sequence `i`.
        """
        for x in i:
            self.add(x)

    def clear(self):
        """
        Clears the container.
        """
        self._seen.clear()
        del self._list[:]

    def __str__(self):
        return str(self._list)


if _HAS_SC:
    SortedDict = sortedcontainers.SortedDict
else:
    class SortedDict(object):
        """
        Inefficient substitution for `sortedcontainers.SortedDict`. Lookups are
        fast; insertions are slow.
        """
        def __init__(self):
            self._sorted = []
            self._map = {}

        def __contains__(self, key):
            return key in self._map

        def __getitem__(self, key):
            return self._map[key]

        def __setitem__(self, key, value):
            if key not in self._map:
                bi.insort(self._sorted, key)
            self._map[key] = value

        def __delitem__(self, key):
            del self._map[key]
            del self._sorted[self.bisect_left(key)]

        def __len__(self):
            return len(self._map)

        def clear(self):
            """
            Clear the dict.
            """
            self._sorted = []
            self._map = {}

        def bisect_left(self, key):
            """
            See `bisect.bisect_left`.
            """
            return bi.bisect_left(self._sorted, key)

        def bisect_right(self, key):
            """
            See `bisect.bisect_right`.
            """
            return bi.bisect_right(self._sorted, key)

        def peekitem(self, i):
            """
            Return the key value pair in the sorted dict at the given index.
            """
            key = self._sorted[i]
            return key, self._map[key]

        def items(self):
            """
            Yield key value pairs in sorted order.
            """
            for key in self._sorted:
                yield key, self._map[key]


class IntervalSet(object):
    """
    Simplistic interval set class restricted to methods needed to implement
    `&dom` statements.
    """
    def __init__(self, seq=()):
        self._items = SortedDict()
        for x, y in seq:
            self.add(x, y)

    def add(self, x1, y1):
        """
        Add an interval to the set.
        """
        if y1 <= x1:
            return
        i = self._items.bisect_left(x1)
        while i < len(self._items):
            y2, x2 = self._items.peekitem(i)
            if y1 < x2:
                break
            x1 = min(x1, x2)
            y1 = max(y1, y2)
            del self._items[y2]
        self._items[y1] = x1

    def items(self):
        """
        Return the intervals in the set.
        """
        return ((x, y) for y, x in self._items.items())

    def __len__(self):
        return len(self._items)

    def __repr__(self):
        return " ".join("[{},{})".format(x, y) for x, y in self.items())
