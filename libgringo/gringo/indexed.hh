// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#ifndef GRINGO_INDEXED_HH
#define GRINGO_INDEXED_HH

#include <vector>

namespace Gringo {

// {{{ declaration of Indexed

template <class T, class R = unsigned>
struct Indexed {
public:
    using ValueType = T;
    using IndexType = R;
    template <class... Args>
    IndexType emplace(Args&&... args);
    IndexType insert(ValueType &&value);
    ValueType erase(IndexType uid);
    ValueType &operator[](IndexType uid);
private:
    std::vector<ValueType> values_;
    std::vector<IndexType> free_;
};

// }}}

// {{{ defintion of Indexed<T>

template <class T, class R>
template <class... Args>
typename Indexed<T, R>::IndexType Indexed<T, R>::emplace(Args&&... args) {
    if (free_.empty()) {
        values_.emplace_back(std::forward<Args>(args)...);
        return IndexType(values_.size() - 1);
    }
    else {
        IndexType uid = free_.back();
        values_[uid] = ValueType(std::forward<Args>(args)...);
        free_.pop_back();
        return uid;
    }
}

template <class T, class R>
typename Indexed<T, R>::IndexType Indexed<T, R>::insert(ValueType &&value) {
    if (free_.empty()) {
        values_.push_back(std::move(value));
        return IndexType(values_.size() - 1);
    }
    else {
        IndexType uid = free_.back();
        values_[uid] = std::move(value);
        free_.pop_back();
        return uid;
    }
}

template <class T, class R>
typename Indexed<T, R>::ValueType Indexed<T, R>::erase(IndexType uid) {
    ValueType val(std::move(values_[uid]));
    if (uid + 1 == values_.size()) { values_.pop_back(); }
    else { free_.push_back(uid); }
    return val;
}

template <class T, class R>
typename Indexed<T, R>::ValueType &Indexed<T, R>::operator[](typename Indexed<T, R>::IndexType uid) {
    return values_[uid];
}

// }}}

} // namespace Gringo

#endif  // GRINGO_INDEXED_HH
