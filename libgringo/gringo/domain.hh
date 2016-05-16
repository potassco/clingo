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
#include <gringo/types.hh>
#include <deque>
#include <gringo/hash_set.hh>
#include <gringo/unique_list.hh>

namespace Gringo {

// {{{ declaration of BinderType

enum class BinderType { NEW, OLD, ALL };

inline std::ostream &operator<<(std::ostream &out, BinderType x) {
    switch (x) {
        case BinderType::NEW: { out << "NEW"; break; }
        case BinderType::OLD: { out << "OLD"; break; }
        case BinderType::ALL: { out << "ALL"; break; }
    }
    return out;
}


// }}}
// {{{ declaration of IndexUpdater

class IndexUpdater {
public:
    // Updates the index with fresh atoms from the domain.
    // First traverses the atoms in the domain skipping undefined atoms and
    // afterwards traversing undefined atoms that have been defined later.
    virtual bool update() = 0;
    virtual ~IndexUpdater() { }
};

using SValVec = std::vector<Term::SVal>;

// }}}
// {{{ declaration of BindIndex

template <class Domain>
class BindIndexEntry {
public:
    struct Hash {
        size_t operator()(BindIndexEntry const &e) const {
            return hash_range(e.data_, reinterpret_cast<uint64_t *>(e.begin_));
        };
        size_t operator()(SymVec const &e) const {
            return hash_range(e.begin(), e.end());
        };
    };
    using SizeType  = typename Domain::SizeType;
    using DataVec = std::vector<uint64_t>;
    BindIndexEntry(SymVec const &bound)
    : end_(0)
    , reserved_(1)
    , data_(nullptr)
    , begin_(nullptr) {
        data_ = reinterpret_cast<uint64_t*>(malloc(sizeof(uint64_t) * bound.size() + sizeof(SizeType)));
        if (!data_) { throw std::bad_alloc(); }
        begin_ = reinterpret_cast<uint32_t*>(data_ + bound.size());
        uint64_t *it = data_;
        for (auto &sym : bound) { *it++ = sym.rep; }
    }
    ~BindIndexEntry() { free(data_); }
    SizeType const *begin() const { return begin_; }
    SizeType const *end() const { return begin_ + end_; }
    void push(SizeType x) {
        if (end_ == reserved_) {
            size_t bound = reinterpret_cast<uint64_t const*>(begin_) - data_;
            size_t oldsize = bound + sizeof(SizeType) * end_;
            size_t size = oldsize + sizeof(SizeType) * end_;
            if (size < oldsize) { throw std::runtime_error("size limit exceeded"); }
            void *ret = realloc(data_, size);
            if (!ret) { throw std::bad_alloc(); }
            if (data_ != ret) {
                data_ = reinterpret_cast<uint64_t*>(ret);
                begin_ = reinterpret_cast<uint32_t*>(data_ + bound);
            }
        }
        data_[end_++] = x;
    }
    size_t hash() const {
        return hash_range(data_, reinterpret_cast<uint64_t *>(begin_));
    }
    bool operator==(BindIndexEntry const &x) const {
        return std::equal(x.data_, reinterpret_cast<uint64_t const *>(x.begin_), data_, [](uint64_t a, uint64_t b) { return a == b; });
    }
    bool operator==(SymVec const &vec) const {
        return std::equal(vec.begin(), vec.end(), data_, [](Symbol const &a, uint64_t b) { return a.rep == b; });
    }
private:
    Id_t end_;
    Id_t reserved_;
    uint64_t* data_;
    SizeType* begin_;
};

// An index for a positive literal occurrence
// with at least one variable bound and one variable unbound.
template <class Domain>
class BindIndex : public IndexUpdater {
public:
    using SizeType  = typename Domain::SizeType;
    using OffsetVec = std::vector<SizeType>;
    using Iterator  = typename OffsetVec::iterator;
    using Entry     = BindIndexEntry<Domain>;
    using Index     = UniqueVec<Entry, typename Entry::Hash, EqualTo>;

    struct OffsetRange {
        bool next(Id_t &offset, Term const &repr, BindIndex &idx) {
            if (current != end) {
                offset = *current++;
                repr.match(idx.domain_[offset]);
                return true;
            }
            return false;
        }
        Iterator current;
        Iterator end;
    };

    BindIndex(Domain &domain, SValVec &&bound, UTerm &&repr)
    : repr_(std::move(repr))
    , domain_(domain)
    , bound_(std::move(bound)) {
        assert(!bound_.empty());
    }

    bool update() override {
        return domain_.update([this](SizeType offset) { add(offset); }, *repr_, imported_, importedDelayed_);
    }

    // Returns a range of offsets corresponding to atoms that match the given bound variables.
    OffsetRange lookup(SValVec const &bound, BinderType type) {
        boundVals_.clear();
        for (auto &&x : bound) { boundVals_.emplace_back(*x); }
        auto it(data_.find(boundVals_));
        if (it != data_.end()) {
            auto cmp = [this](SizeType a, SizeType gen) { return domain_[a].generation() < gen; };
            switch (type) {
                case BinderType::NEW: { return { std::lower_bound(it->begin(), it->end(), domain_.generation(), cmp), it->end() }; }
                case BinderType::OLD: { return { it->begin(), std::lower_bound(it->begin(), it->end(), domain_.generation(), cmp) }; }
                case BinderType::ALL: { return { it->begin(), it->end() }; }
            }
        }
        static OffsetVec dummy;
        return { dummy.begin(), dummy.end() };
    }

    // The equality and hash functions are used to prevent adding structurally equivalent indices twice.
    bool operator==(BindIndex const &x) const {
        return *repr_ == *x.repr_;
    }

    size_t hash() const {
        return repr_->hash();
    }

    virtual ~BindIndex() noexcept = default;

private:
    // Adds an atom given by its offset to the index.
    // Assumes that the atom matches and has not been added previously.
    void add(Id_t offset) {
        boundVals_.clear();
        for (auto &y : bound_) { boundVals_.emplace_back(*y); }
        auto jt = data_.findPush(boundVals_, boundVals_).first;
        jt->push(offset);
    }

private:
    UTerm const repr_;
    Domain     &domain_;
    SValVec     bound_;
    SymVec      boundVals_;
    Index       data_;
    Id_t        imported_ = 0;
    Id_t        importedDelayed_ = 0;
};

// }}}
// {{{ declaration of FullIndex

// An index for a positive literal occurrence with all literals unbound.
// The matches are represented as ranges.
// This means a literal of form p(X,Y) where both X and Y are unbound has an index with just one range.
template <class Domain>
class FullIndex : public IndexUpdater {
public:
    using SizeType    = typename Domain::SizeType;
    using IntervalVec = std::vector<std::pair<SizeType, SizeType>>;
    using Iterator    = typename IntervalVec::iterator;

    struct OffsetRange {
        bool next(SizeType &offset, Term const &repr, FullIndex &idx) {
            // For Old and All atoms, iterate forward until the atoms in the index are exceeded.
            // For Old atoms stop early if the atoms do not belong to previous generations anymore.
            if (type != BinderType::NEW) {
                if (rangeOffset == idx.index_.size()) {
                    return false;
                }
                if (intervalOffset == idx.index_[rangeOffset].second) {
                    ++rangeOffset;
                    if (rangeOffset == idx.index_.size()) {
                        return false;
                    }
                    intervalOffset = idx.index_[rangeOffset].first;
                }
                offset = intervalOffset++;
                auto &atom = idx.domain_[offset];
                if (type == BinderType::OLD && atom.generation() >= idx.domain_.generation()) {
                    rangeOffset = idx.index_.size();
                    return false;
                }
            }
            // For New atoms iterate backward until the atoms do not belong to the current generation anymore.
            else {
                if (rangeOffset == 0) {
                    return false;
                }
                if (intervalOffset == idx.index_[rangeOffset - 1].first) {
                    --rangeOffset;
                    if (rangeOffset == 0) {
                        return false;
                    }
                    intervalOffset = idx.index_[rangeOffset-1].second;
                }
                offset = --intervalOffset;
                auto &atom = idx.domain_[offset];
                if (atom.generation() < idx.domain_.generation()) {
                    rangeOffset = 0;
                    return false;
                }
            }
            repr.match(idx.domain_[offset]);
            return true;
        }
        BinderType type;
        SizeType rangeOffset;
        SizeType intervalOffset;
    };

    // This index can be initialized to skip some atoms in a domain.
    // The number of skipped atoms are part of the equality and hash comparisons of the index.
    // This is used to implement projection in the incremental case.
    FullIndex(Domain &domain, UTerm &&repr, Id_t imported)
    : repr_(std::move(repr))
    , domain_(domain)
    , imported_(imported)
    , initialImport_(imported) { }

    // Returns a range of offsets corresponding to matching atoms.
    OffsetRange lookup(BinderType type) {
        switch (type) {
            case BinderType::OLD:
            case BinderType::ALL: { return { type, 0, !index_.empty() ? index_.front().first : 0 }; }
            case BinderType::NEW: { return { type, static_cast<SizeType>(index_.size()), !index_.empty() ? index_.back().second : 0 }; }
        }
        throw std::logic_error("cannot happen");
    }

    bool update() override {
        return domain_.update([this](SizeType offset) { add(offset); return true; }, *repr_, imported_, importedDelayed_);
    }

    bool operator==(FullIndex const &x) const {
        return *repr_ == *x.repr_ && initialImport_ == x.initialImport_;
    }

    size_t hash() const {
        return get_value_hash(repr_, initialImport_);
    }

    virtual ~FullIndex() noexcept = default;

private:
    // Adds an atom offset to the index.
    // The offset is merged into the last interval if possible.
    // Maintains the generation order by always inserting at the end.
    void add(SizeType offset) {
        if (!index_.empty() && index_.back().second == offset) {
            index_.back().second++;
        }
        else {
            index_.emplace_back(offset, offset + 1);
        }
    }

private:
    UTerm       repr_;
    Domain     &domain_;
    IntervalVec index_;
    Id_t        imported_;
    Id_t        importedDelayed_ = 0;
    Id_t        initialImport_;
};

// }}}
// {{{ declaration of Domain

class Domain {
public:
    virtual void init() = 0;
    virtual void enqueue() = 0;
    virtual bool dequeue() = 0;
    virtual bool isEnqueued() const = 0;
    // Advances the domain to the next generation.
    // Returns true if there are fresh atoms that have to be incorporated into the indices.
    virtual void nextGeneration() = 0;
    virtual void setDomainOffset(Id_t offset) = 0;
    virtual Id_t domainOffset() const = 0;

    virtual ~Domain() { }
};
using UDom = std::unique_ptr<Domain>;
using UDomVec = std::vector<UDom>;

// }}}
// {{{ declaration of AbstractDomain

template <class T>
class AbstractDomain : public Domain {
public:
    using Atom            = T;
    using Atoms           = UniqueVec<Atom, HashKey<Symbol>, EqualToKey<Symbol>>;
    using BindIndex       = Gringo::BindIndex<AbstractDomain>;
    using FullIndex       = Gringo::FullIndex<AbstractDomain>;
    using BindIndices     = std::unordered_set<BindIndex, call_hash<BindIndex>>;
    using FullIndices     = std::unordered_set<FullIndex, call_hash<FullIndex>>;
    using AtomVec         = typename Atoms::Vec;
    using Iterator        = typename AtomVec::iterator;
    using ConstIterator   = typename AtomVec::const_iterator;
    using SizeType        = typename Atoms::SizeType;
    using OffsetVec       = std::vector<SizeType>;

    AbstractDomain() = default;
    AbstractDomain(AbstractDomain const &) = delete;
    AbstractDomain(AbstractDomain &&) = delete;

    // All indices that use a domain have to be registered with it.
    BindIndex &add(SValVec &&bound, UTerm &&repr) {
        auto ret(indices_.emplace(*this, std::move(bound), std::move(repr)));
        auto &idx = const_cast<BindIndex&>(*ret.first);
        idx.update();
        return idx;
    }

    FullIndex &add(UTerm &&repr, Id_t imported) {
        auto ret(fullIndices_.emplace(*this, std::move(repr), imported));
        auto &idx = const_cast<FullIndex&>(*ret.first);
        idx.update();
        return idx;
    }

    // Function to lookup negative literals or non-recursive atoms.
    bool lookup(SizeType &offset, Term const &repr, RECNAF naf) {
        bool undefined = false;
        switch (naf) {
            case RECNAF::POS: {
                // Note: intended for non-recursive case only
                auto it = atoms_.find(repr.eval(undefined));
                if (!undefined && it != atoms_.end() && it->defined()) {
                    offset = it - begin();
                    return true;
                }
                break;
            }
            case RECNAF::NOT: {
                auto it = atoms_.find(repr.eval(undefined));
                if (!undefined && it != atoms_.end()) {
                    if (!it->fact()) {
                        offset = it - begin();
                        return true;
                    }
                }
                else if (!undefined) {
                    // This can only happen if literals are used negatively and non-recursively.
                    // Example: a :- not b.
                    offset = std::numeric_limits<SizeType>::max();
                    return true;
                }
                break;
            }
            case RECNAF::RECNOT: {
                auto it = reserve(repr.eval(undefined));
                if (!undefined && !it->fact()) {
                    offset = it - begin();
                    return true;
                }
                break;
            }
            case RECNAF::NOTNOT: {
                // Note: intended for recursive case only
                auto it = reserve(repr.eval(undefined));
                if (!undefined) {
                    offset = it - begin();
                    return true;
                }
                break;
            }
        }
        offset = std::numeric_limits<SizeType>::max();
        return false;
    }

    // Function to lookup recursive atoms.
    bool lookup(SizeType &offset, Term const &repr, BinderType type) {
        // Note: intended for recursive case only
        bool undefined = false;
        auto it = atoms_.find(repr.eval(undefined));
        if (!undefined && it != atoms_.end() && it->defined()) {
            switch (type) {
                case BinderType::OLD: {
                    if (it->generation() <  generation_) {
                        offset = it - begin();
                        return true;
                    }
                    break;
                }
                case BinderType::ALL: {
                    if (it->generation() <=  generation_) {
                        offset = it - begin();
                        return true;
                    }
                    break;
                }
                case BinderType::NEW: {
                    if (it->generation() == generation_) {
                        offset = it - begin();
                        return true;
                    }
                    break;
                }
            }
        }
        offset = std::numeric_limits<SizeType>::max();
        return false;
    }

    // Loops over all atoms that might have to be imported into an index.
    // Already seen atoms are indicated with the two offset parameters.
    // Returns true if a maching atom was falls.
    // Furthermore, accepts a callback f that receives the offset of a matching atom.
    // If the callback returns false the search for matching atoms is stopped and the function returns true.
    template <typename F>
    bool update(F f, Term const &repr, SizeType &imported, SizeType &importedDelayed) {
        bool ret = false;
        for (auto it(atoms_.begin() + imported), ie(atoms_.end()); it < ie; ++it, ++imported) {
            if (it->defined()) {
                if (!it->delayed() && repr.match(*it)) {
                    ret = true;
                    f(imported);
                }
            }
            else { it->markDelayed(); }
        }
        for (auto it(delayed_.begin() + importedDelayed), ie(delayed_.end()); it < ie; ++it) {
            auto &atom = operator[](*it);
            if (repr.match(atom)) {
                ret = true;
                f(*it);
            }
        }
        importedDelayed = delayed_.size();
        return ret;
    }

    void clear() {
        atoms_.clear();
        indices_.clear();
        fullIndices_.clear();
        generation_ = 0;
    }
    void reset() {
        indices_.clear();
        fullIndices_.clear();
    }

    // Returns the current generation.
    // The generation corresponds to the number of grounding iterations
    // the domain was involved in.
    SizeType generation() const { return generation_; }
    // Resevers an atom for a recursive negative literal.
    // This does not set a generation.
    Iterator reserve(Symbol x) { return atoms_.findPush(x, x).first; }
    // Defines (adds) an atom setting its generation.
    std::pair<Iterator, bool> define(Symbol value) {
        auto ret = atoms_.findPush(value, value);
        if (ret.second) {
            ret.first->setGeneration(generation() + 1);
        }
        else if (!ret.first->defined()) {
            ret.second = true;
            ret.first->setGeneration(generation() + 1);
            if (ret.first->delayed()) {
                delayed_.emplace_back(ret.first - begin());
            }
        }
        return ret;
    }
    void define(SizeType offset) {
        auto &atm = operator[](offset);
        if (!atm.defined()) {
            atm.setGeneration(generation() + 1);
            if (atm.delayed()) {
                delayed_.emplace_back(offset);
            }
        }
    }
    // Sets the generation of the domain and all atoms back to zero.
    void init() override {
        generation_ = 0;
        for (auto it = begin() + initOffset_, ie = end(); it != ie; ++it) {
            if (it->defined()) { it->setGeneration(0); }
            else               { it->markDelayed(); }
        }
        initOffset_ = atoms_.size();
        for (auto it = delayed_.begin() + initDelayedOffset_, ie = delayed_.end(); it != ie; ++it) {
            operator[](*it).setGeneration(0);
        }
        initDelayedOffset_ = delayed_.size();
    }
    // A domain is enqueued for two grounding iterations.
    // This gives atoms a chance to go from state Open -> New -> Old.
    void enqueue() override { enqueued_ = 2; }
    bool dequeue() override {
        --enqueued_;
        assert(0 <= enqueued_ && enqueued_ <= 2);
        return enqueued_;
    }
    bool isEnqueued() const override { return enqueued_; }
    void nextGeneration() override { ++generation_; }
    OffsetVec &delayed() { return delayed_; }
    Iterator find(Symbol x) { return atoms_.find(x); }
    ConstIterator find(Symbol x) const { return atoms_.find(x); }
    Id_t size() const { return atoms_.size(); }
    Iterator begin() { return atoms_.begin(); }
    Iterator end() { return atoms_.end(); }
    ConstIterator begin() const { return atoms_.begin(); }
    ConstIterator end() const { return atoms_.end(); }
    Atom &operator[](Id_t x) { return atoms_[x]; }
    void setDomainOffset(Id_t offset) override { domainOffset_ = offset; }
    Id_t domainOffset() const override { return domainOffset_; }

    virtual ~AbstractDomain() noexcept { }
protected:
    void hide(Iterator it) { atoms_.hide(it); }

protected:
    BindIndices indices_;
    FullIndices fullIndices_;
    Atoms       atoms_;
    OffsetVec   delayed_;
    Id_t        enqueued_ = 0;
    Id_t        generation_ = 0;
    Id_t        initOffset_ = 0;
    Id_t        initDelayedOffset_ = 0;
    Id_t        domainOffset_ = InvalidId;
};

// }}}

} // namespace Gringo

#endif // _GRINGO_DOMAIN_HH


