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

#ifndef _GRINGO_DOMAIN_HH
#define _GRINGO_DOMAIN_HH

#include <cassert>
#include <gringo/base.hh>
#include <deque>
#include <gringo/unique_list.hh>

// Note: this file could as well be migrated to output

namespace Gringo {

// {{{ declaration of BinderType

enum class BinderType { NEW, OLD, ALL };

std::ostream &operator<<(std::ostream &out, BinderType x);

// }}}
// {{{ declaration of Exports

template <class Element>
struct Exports {
    using element_type           = Element;
    using element_vec            = std::vector<std::reference_wrapper<Element>>;
    using element_iterator       = typename element_vec::iterator;
    using const_element_iterator = typename element_vec::const_iterator;

    bool nextGeneration();
    void init();
    void append(element_type &elem);
    void clear();
    void incNext();
    void showNext();
    unsigned size() const;
    element_iterator begin();
    element_iterator end();
    const_element_iterator begin() const;
    const_element_iterator end() const;
    element_type &operator[](unsigned x);

    element_vec exports;
    unsigned    generation_     = 0; //!< starting point of the current generation
    unsigned    nextGeneration_ = 0; //!< starting point of the next generation
    //! The incOffset divides elements added at the current and previous incremental steps.
    //! It is used to only output newly inserted atoms, for projection, and classical negation.
    unsigned    incOffset      = 0;
    //! Used to decouple symbol table generation from grounding
    unsigned    showOffset     = 0;
};

// }}}
// {{{ declaration of IndexUpdater

struct IndexUpdater {
    virtual bool update() = 0;
    virtual ~IndexUpdater() { }
};

using SValVec = std::vector<Term::SVal>;

// }}}
// {{{ declaration of BindIndex

template <class Element>
struct BindIndex : IndexUpdater {
    using element_type     = Element;
    using exports_type     = Exports<element_type>;
    using element_vec      = typename exports_type::element_vec;
    using element_iterator = typename exports_type::element_iterator;
    using index_map        = std::unordered_map<FWValVec, element_vec>;

    struct element_range {
        element_type *next(Term const &repr, BindIndex &);
        element_iterator current;
        element_iterator end;
    };
    BindIndex(exports_type &import, SValVec &&bound, UTerm &&repr);
    virtual bool update();
    element_range lookup(SValVec const &bound, BinderType type);
    bool operator==(BindIndex const &x) const;
    size_t hash() const;
    virtual ~BindIndex();
    UTerm const   repr;
    exports_type &import;
    SValVec       bound;
    ValVec        boundVals;
    index_map     data;
    unsigned      imported  = 0;
};

// }}}
// {{{ declaration of FullIndex

template <class Element>
struct FullIndex : IndexUpdater {
    using exports_type      = Exports<Element>;
    using element_type      = typename exports_type::element_type;
    using interval_vec      = std::vector<std::pair<unsigned, unsigned>>;
    using interval_iterator = typename interval_vec::iterator;

    struct element_range {
        element_type *next(Term const &repr, FullIndex &idx);
        interval_iterator range;
        unsigned          current;
        unsigned          end;
    };
    FullIndex(exports_type &exports, UTerm &&repr, unsigned imported);
    element_range lookup(BinderType type);
    virtual bool update();
    bool operator==(FullIndex const &x) const;
    size_t hash() const;
    virtual ~FullIndex();

    UTerm         repr;
    exports_type &exports;
    interval_vec  index;
    unsigned      imported;
    unsigned      initialImport;
};

// }}}
// {{{ declaration of Domain

struct Domain {
    virtual void init() = 0;
    virtual void enqueue() = 0;
    virtual bool dequeue() = 0;
    virtual bool isEnqueued() const = 0;
    virtual void nextGeneration() = 0;
    virtual ~Domain() { }
};

// }}}
// {{{ declaration of AbstractDomain

template <class Element>
struct AbstractDomain : Domain {
    using element_map     = std::unordered_map<Value, Element>;
    using element_type    = typename element_map::value_type;
    using bind_index_type = BindIndex<element_type>;
    using full_index_type = FullIndex<element_type>;
    using exports_type    = typename bind_index_type::exports_type;
    using element_vec     = typename exports_type::element_vec;
    using bind_index_set  = std::unordered_set<bind_index_type, call_hash<bind_index_type>>;
    using full_index_set  = std::unordered_set<full_index_type, call_hash<full_index_type>>;

    AbstractDomain();
    AbstractDomain(AbstractDomain const &) = delete;
    AbstractDomain(AbstractDomain &&)      = delete;
    bind_index_type &add(SValVec &&bound, UTerm &&repr);
    full_index_type &add(UTerm &&repr, unsigned imported);
    element_type *lookup(Term const &repr, RECNAF naf, bool &undefined);
    element_type *lookup(Term const &repr, BinderType type, bool &undefined);
    bool check(Term const &repr, unsigned &imported);
    void clear();
    virtual void init();
    virtual element_type &reserve(Value x);
    virtual void enqueue() { enqueued = 2; }
    virtual bool dequeue() { 
        --enqueued;
        assert(0 <= enqueued && enqueued <= 2);
        return enqueued;
    }
    virtual bool isEnqueued() const     { return enqueued; }
    virtual void nextGeneration() { exports.nextGeneration(); }
    virtual ~AbstractDomain();

    bind_index_set indices;
    full_index_set fullIndices;
    element_map    domain;
    exports_type   exports;
    int            enqueued = 0;
};

// }}}

// TODO: move to Output
// {{{ declaration of AtomState

struct AtomState {
    AtomState();
    AtomState(std::nullptr_t);
    AtomState(bool fact, unsigned generation);
    bool hasUid() const;
    void uid(unsigned x);
    unsigned uid() const;
    bool fact(bool recursive) const;
    void setFact(bool x);
    bool defined() const;
    unsigned generation() const;
    void generation(unsigned x);
    bool isFalse() const;
    bool isExternal() const { return _generation < 0; }
    void setExternal(bool x) { 
        if (x != isExternal()) { _generation = -_generation; }
    }
    static std::pair<Value const, AtomState> &ignore();

private:
    //! A unique id for the atom that additionally encodes the fact bit.
    //! 0     - undefined atom
    //! > 0   - non-fact
    //! < 0   - fact
    //! 1, -1 - unique identifier not set yet
    int _uid;
    //! The generation of the atom. This value is used by indices to determine
    //! what is new and old, respectively. Value zero is used to encode
    //! reserved atoms, which will not be used for matching in indices.
    //! 0     - must not happen
    //! > 0   - non-external
    //! < 0   - external
    //! 1, -1 - no generation yet
    int _generation;
};

// }}}
// {{{ declaration of PredicateDomain

struct PredicateDomain : AbstractDomain<AtomState> {
    std::tuple<element_type*, bool, bool> insert(Value x, bool fact);
    void insert(element_type &x);
    virtual ~PredicateDomain();
};
using PredDomMap = unique_list<std::pair<FWSignature, PredicateDomain>, extract_first<FWSignature>>;

inline PredicateDomain &add(PredDomMap &domains, FWSignature const &sig) {
    auto it(domains.find(sig));
    return (it != domains.end() ? 
        it->second : 
        domains.emplace_back(std::piecewise_construct, std::forward_as_tuple(sig), std::forward_as_tuple()).first->second);
}

// }}}

// {{{ definition of BinderType

inline std::ostream &operator<<(std::ostream &out, BinderType x) {
    switch (x) {
        case BinderType::NEW: { out << "NEW"; break; }
        case BinderType::OLD: { out << "OLD"; break; }
        case BinderType::ALL: { out << "ALL"; break; }
    }
    return out;
}

// }}}
// {{{ definition of Exports

template <class Element>
bool Exports<Element>::nextGeneration() { 
    generation_     = nextGeneration_;
    nextGeneration_ = exports.size();
    return generation_ != nextGeneration_;
}
template <class Element>
void Exports<Element>::incNext() {
    incOffset = exports.size();
}
template <class Element>
void Exports<Element>::showNext() {
    showOffset = exports.size();
}
template <class Element>
void Exports<Element>::init() { generation_ = 0; }
template <class Element>
void Exports<Element>::append(element_type &elem) { exports.emplace_back(elem); }
template <class Element>
unsigned Exports<Element>::size() const { return exports.size(); }
template <class Element>
typename Exports<Element>::element_iterator Exports<Element>::begin() { return exports.begin(); }
template <class Element>
typename Exports<Element>::element_iterator Exports<Element>::end() { return exports.end(); }
template <class Element>
typename Exports<Element>::const_element_iterator Exports<Element>::begin() const { return exports.begin(); }
template <class Element>
typename Exports<Element>::const_element_iterator Exports<Element>::end() const { return exports.end(); }
template <class Element>
typename Exports<Element>::element_type &Exports<Element>::operator[](unsigned x) { return exports[x]; }
template <class Element>
void Exports<Element>::clear() { 
    exports.clear();
    generation_     = 0;
    nextGeneration_ = 0;
    incOffset      = 0;
    showOffset     = 0;
}

// }}}
// {{{ definition of BindIndex

template <class Element>
typename BindIndex<Element>::element_type *BindIndex<Element>::element_range::next(Term const &repr, BindIndex &) {
    if (current != end) {
        repr.match(current->get().first);
        return &current++->get();
    }
    return nullptr;
}
template <class Element>
BindIndex<Element>::BindIndex(exports_type &import, SValVec &&bound, UTerm &&repr)
    : repr(std::move(repr))
    , import(import)
    , bound(std::move(bound)) { assert(!this->bound.empty()); }
template <class Element>
bool BindIndex<Element>::update() {
    bool updated = false;
    for (auto it(import.begin() + imported), ie(import.end()); it < ie; ++it) {
        if (repr->match(it->get().first)) {
            boundVals.clear();
            for (auto &y : bound) { boundVals.emplace_back(*y); }
            data[boundVals].emplace_back(*it);
            updated = true;
        }
    }
    imported = std::max(imported, import.size());
    return updated;
}
template <class Element>
typename BindIndex<Element>::element_range BindIndex<Element>::lookup(SValVec const &bound, BinderType type) {
    boundVals.clear();
    for (auto &x : bound) { boundVals.emplace_back(*x); }
    auto it(data.find(boundVals));
    if (it != data.end()) {
        auto cmp = [](element_type const &a, unsigned gen) { return a.second.generation() < gen; };
        switch (type) {
            case BinderType::NEW: { return { std::lower_bound(it->second.begin(), it->second.end(), import.generation_, cmp), it->second.end() }; }
            case BinderType::OLD: { return { it->second.begin(), std::lower_bound(it->second.begin(), it->second.end(), import.generation_, cmp) }; }
            case BinderType::ALL: { return { it->second.begin(), it->second.end() }; }
        }
    }
    static element_vec dummy;
    return { dummy.end(), dummy.end() };

}
template <class Element>
bool BindIndex<Element>::operator==(BindIndex const &x) const { return *repr == *x.repr; }
template <class Element>
size_t BindIndex<Element>::hash() const { return repr->hash(); }
template <class Element>
BindIndex<Element>::~BindIndex() { }

// }}}
// {{{ definition of FullIndex

template <class Element>
typename FullIndex<Element>::element_type *FullIndex<Element>::element_range::next(Term const &repr, FullIndex &idx) {
    if (current < end && current >= range->second) {
        current = ++range != idx.index.end() ? range->first : end;
    }
    if (current < end) {
        auto &ret(idx.exports[current++]);
        repr.match(ret.first);
        return &ret;
    }
    return nullptr;
}

template <class Element>
FullIndex<Element>::FullIndex(exports_type &exports, UTerm &&repr, unsigned imported) 
    : repr(std::move(repr))
    , exports(exports) 
    , imported(imported)
    , initialImport(imported) { }
template <class Element>
typename FullIndex<Element>::element_range FullIndex<Element>::lookup(BinderType type) {
    switch (type) {
        case BinderType::ALL: {
            auto range(index.begin());
            auto end(exports.nextGeneration_);
            return { range, range != index.end() ? range->first : end, end };
        }
        case BinderType::NEW: {
            auto cmp([](typename interval_vec::value_type const &x, unsigned y) { return x.second < y;  });
            auto current(exports.generation_);
            auto range(std::lower_bound(index.begin(), index.end(), current, cmp));
            auto end(exports.nextGeneration_);
            return { range, range != index.end() ? std::max(range->first, current) : end, end };
        }
        case BinderType::OLD: {
            auto range(index.begin());
            auto end(exports.generation_);
            return { range, range != index.end() ? range->first : end, end };
        }
    }
    throw std::logic_error("cannot happen");
}
template <class Element>
bool FullIndex<Element>::update() {
    bool ret = false;
    for (auto it(exports.begin() + imported), ie(exports.end()); it < ie; ++it, ++imported) {
        if (repr->match(it->get().first)) {
            if (!index.empty() && index.back().second == imported) { index.back().second++; }
            else { index.emplace_back(imported, imported+1); }
            ret = true;
        }
    }
    return ret;
}
template <class Element>
bool FullIndex<Element>::operator==(FullIndex const &x) const { return *repr == *x.repr && initialImport == x.initialImport; }
template <class Element>
size_t FullIndex<Element>::hash() const                       { return get_value_hash(repr, initialImport); }
template <class Element>
FullIndex<Element>::~FullIndex() { }

// }}}
// {{{ definition of AbstractDomain

template <class Element>
AbstractDomain<Element>::AbstractDomain() { }
template <class Element>
void AbstractDomain<Element>::init()   { exports.init(); }
template <class Element>
typename AbstractDomain<Element>::element_type &AbstractDomain<Element>::reserve(Value x) {
    return *domain.emplace(std::piecewise_construct, std::forward_as_tuple(x), std::forward_as_tuple()).first;
}
template <class Element>
typename AbstractDomain<Element>::bind_index_type &AbstractDomain<Element>::add(SValVec &&bound, UTerm &&repr) {
    auto ret(indices.emplace(exports, std::move(bound), std::move(repr)));
    auto &idx = const_cast<bind_index_type&>(*ret.first);
    idx.update();
    return idx;
}
template <class Element>
typename AbstractDomain<Element>::full_index_type &AbstractDomain<Element>::add(UTerm &&repr, unsigned imported) {
    auto ret(fullIndices.emplace(exports, std::move(repr), imported));
    auto &idx = const_cast<full_index_type&>(*ret.first);
    idx.update();
    return idx;
}
template <class Element>
typename AbstractDomain<Element>::element_type *AbstractDomain<Element>::lookup(Term const &repr, RECNAF naf, bool &undefined) {
    switch (naf) {
        case RECNAF::POS: {
            // Note: intended for non-recursive case only
            auto it = domain.find(repr.eval(undefined));
            return it != domain.end() && it->second.defined() ? &*it : nullptr;
        }
        case RECNAF::NOT: {
            auto it = domain.find(repr.eval(undefined));
            if (it != domain.end()) { return !it->second.fact(false) ? &*it : nullptr; }
            else                    { return &Element::ignore(); }
        }
        case RECNAF::RECNOT: {
            auto result = &reserve(repr.eval(undefined));
            return !result->second.fact(true) ? result : nullptr;
        }
        case RECNAF::NOTNOT: {
            // Note: intended for recursive case only
            return &reserve(repr.eval(undefined));
        }
    }
    return nullptr;
}
template <class Element>
typename AbstractDomain<Element>::element_type *AbstractDomain<Element>::lookup(Term const &repr, BinderType type, bool &undefined) {
    // Note: intended for recursive case only
    auto it = domain.find(repr.eval(undefined));
    if (it != domain.end() && it->second.defined()) {
        auto result = &*it;
        switch (type) {
            case BinderType::OLD: { return it->second.generation() < exports.generation_     ? result : nullptr; }
            case BinderType::ALL: { return it->second.generation() < exports.nextGeneration_ ? result : nullptr; }
            case BinderType::NEW: {
                return
                    it->second.generation() >= exports.generation_ &&
                    it->second.generation() <  exports.nextGeneration_
                    ? result : nullptr;
            }
        }
    }
    return nullptr;

}
template <class Element>
bool AbstractDomain<Element>::check(Term const &repr, unsigned &imported) {
    bool ret = false;
    for (auto it(exports.begin() + imported), ie(exports.end()); it < ie; ++it) {
        if (repr.match(it->get().first)) { 
            ret = true;
            break;
        }
    }
    imported = std::max(imported, exports.size());
    return ret;
}
template <class Element>
void AbstractDomain<Element>::clear() {
    exports.clear();
    domain.clear();
    indices.clear();
    fullIndices.clear();
}
template <class Element>
AbstractDomain<Element>::~AbstractDomain() { }

// }}}

// {{{ definition of AtomState

inline AtomState::AtomState(std::nullptr_t) 
    : _uid(0)
    , _generation(1) { }
inline AtomState::AtomState() 
    : _uid(1)
    , _generation(1) { }
inline AtomState::AtomState(bool fact, unsigned generation)
    : _uid(!fact ? 1 : -1)
    , _generation(generation + 2) { }
inline unsigned AtomState::uid() const        { assert(_uid != 0); return std::abs(_uid) - 1; }
inline bool AtomState::hasUid() const         { assert(_uid != 0); return _uid > 1 || _uid < -1; }
inline void AtomState::uid(unsigned x)        { assert(_uid != 0 && x > 0); _uid = _uid > 0 ? x+1 : -x-1; }
inline bool AtomState::fact(bool) const       { return _uid < 0; }
inline void AtomState::setFact(bool x)        { if (x != fact(false)) { _uid = -_uid; } }
inline bool AtomState::defined() const        { return std::abs(_generation) > 1; }
inline unsigned AtomState::generation() const { return std::abs(_generation) - 2; }
inline void AtomState::generation(unsigned x) { _generation = x + 2; }
inline bool AtomState::isFalse() const        { return _uid == 0; }
inline std::pair<Value const,AtomState> &AtomState::ignore() {
    static AbstractDomain<AtomState>::element_type x{{Value::createId("#false")}, {nullptr}};
    return x;
}

// }}}
// {{{ definition of PredicateDomain

inline std::tuple<PredicateDomain::element_type*, bool, bool> PredicateDomain::insert(Value x, bool fact) {
    auto ret(domain.emplace(x, AtomState{fact, exports.size()}));
    bool wasfact = false;
    if (ret.second) { exports.append(*ret.first); }
    else {
        if (!ret.first->second.defined()) { 
            ret.first->second.generation(exports.size());
            ret.second = true;
            exports.append(*ret.first);
        }
        wasfact = ret.first->second.fact(false);
        if (fact) { 
            ret.first->second.setFact(true);
        }
    }
    return std::forward_as_tuple(&*ret.first, ret.second, wasfact);
}
inline void PredicateDomain::insert(element_type &x) {
    if (!x.second.defined()) {
        x.second.generation(exports.size());
        exports.append(x);
    }
}
inline PredicateDomain::~PredicateDomain() { }

// }}}
 
} // namespace Gringo
 
#endif // _GRINGO_DOMAIN_HH


