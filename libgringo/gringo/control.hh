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

#ifndef _GRINGO_CONTROL_HH
#define _GRINGO_CONTROL_HH

#include <gringo/value.hh>

namespace Gringo {

enum class SolveResult { UNKNOWN=0, SAT=1, UNSAT=2 };

// {{{1 declaration of Any

struct Any {
    struct PlaceHolder {
        virtual ~PlaceHolder() { };
    };
    template <class T>
    struct Holder : public PlaceHolder {
        Holder(T const &value) : value(value) { }
        Holder(T&& value) : value(std::forward<T>(value)) { }
        T value;
    };
    Any() : content(nullptr) { }
    Any(Any &&other) : content(nullptr) { std::swap(content, other.content); }
    template<typename T>
    Any(T &&value) : content(new Holder<T>(std::forward<T>(value))) { }
    template<typename T>
    Any(T const &value) : content(new Holder<T>(value)) { }
    ~Any() { delete content; }

    Any &operator=(Any &&other) {
        std::swap(content, other.content);
        return *this;
    }

    template<typename T>
    T *get() {
        auto x = dynamic_cast<Holder<T>*>(content);
        return x ? &x->value : nullptr;
    }
    template<typename T>
    T const *get() const {
        auto x = dynamic_cast<Holder<T>*>(content);
        return x ? &x->value : nullptr;
    }
    bool empty() const { return !content; }

    PlaceHolder *content = nullptr;
};

// {{{1 declaration of Model

using Int64Vec = std::vector<int64_t>;

struct Model {
    using LitVec = std::vector<std::pair<bool,Gringo::Value>>;
    static const unsigned CSP   = 1;
    static const unsigned SHOWN = 2;
    static const unsigned ATOMS = 4;
    static const unsigned TERMS = 8;
    static const unsigned COMP  = 16;
    virtual bool contains(Value atom) const = 0;
    virtual ValVec atoms(int showset) const = 0;
    virtual Int64Vec optimization() const = 0;
    virtual void addClause(LitVec const &lits) const = 0;
    virtual ~Model() { }
};

// {{{1 declaration of Statistics

struct Statistics {
    enum Error { error_none = 0, error_unknown_quantity = 1, error_ambiguous_quantity = 2, error_not_available = 3 };
    struct Quantity {
        Quantity(double d) : rep(d) { assert(d >= 0.0); }
        Quantity(Error e) : rep(-double(int(e))) { assert(e != error_none); }
        bool     valid()  const { return error() == error_none; }
        Error    error()  const { return rep >= 0.0 ? error_none : static_cast<Error>(int(-rep)); }
        operator bool()   const { return valid(); }
        operator double() const { return valid() ? rep : std::numeric_limits<double>::quiet_NaN(); }
    private:
        double rep;
    };
    virtual Quantity    getStat(char const* key) const = 0;
    virtual char const *getKeys(char const* key) const = 0;
    virtual ~Statistics() { }
};

// {{{1 declaration of SolveFuture

struct SolveFuture {
    virtual SolveResult get() = 0;
    virtual void wait() = 0;
    virtual bool wait(double timeout) = 0;
    virtual void cancel() = 0;
    virtual ~SolveFuture() { }
};

// {{{1 declaration of SolveIter

struct SolveIter {
    virtual Model const *next() = 0;
    virtual void close() = 0;
    virtual SolveResult get() = 0;
    virtual ~SolveIter() { }
};

// {{{1 declaration of ConfigProxy

struct ConfigProxy {
    virtual bool hasSubKey(unsigned key, char const *name, unsigned* subKey = nullptr) = 0;
    virtual unsigned getSubKey(unsigned key, char const *name) = 0;
    virtual unsigned getArrKey(unsigned key, unsigned idx) = 0;
    virtual void getKeyInfo(unsigned key, int* nSubkeys = 0, int* arrLen = 0, const char** help = 0, int* nValues = 0) const = 0;
	virtual const char* getSubKeyName(unsigned key, unsigned idx) const = 0;
    virtual bool getKeyValue(unsigned key, std::string &value) = 0;
    virtual void setKeyValue(unsigned key, const char *val) = 0;
    virtual unsigned getRootKey() = 0;
};

// {{{1 declaration of DomainProxy

struct DomainProxy {
    struct Element;
    using ElementPtr = std::unique_ptr<Element>;
    struct Element {
        virtual Value atom() const = 0;
        virtual bool fact() const = 0;
        virtual bool external() const = 0;
        virtual ElementPtr next() = 0;
        virtual bool valid() const = 0;
        virtual ~Element() { };
    };
    virtual ElementPtr iter(Signature const &sig) const = 0;
    virtual ElementPtr iter() const = 0;
    virtual ElementPtr lookup(Value const &atom) const = 0;
    virtual std::vector<FWSignature> signatures() const = 0;
    virtual size_t length() const = 0;
    virtual ~DomainProxy() { }
};

// {{{1 declaration of Control

using FWStringVec = std::vector<FWString>;

struct Control {
    using ModelHandler    = std::function<bool (Model const &)>;
    using FinishHandler   = std::function<void (SolveResult, bool)>;
    using Assumptions     = std::vector<std::pair<Value, bool>>;
    using GroundVec       = std::vector<std::pair<std::string, FWValVec>>;
    using NewControlFunc  = Control* (*)(int, char const **);
    using FreeControlFunc = void (*)(Control *);

    virtual ConfigProxy &getConf() = 0;
    virtual DomainProxy &getDomain() = 0;

    virtual void ground(GroundVec const &vec, Any &&context) = 0;
    virtual void prepareSolve(Assumptions &&assumptions) = 0;
    virtual SolveResult solve(ModelHandler h) = 0;
    virtual SolveFuture *solveAsync(ModelHandler mh, FinishHandler fh) = 0;
    virtual SolveIter *solveIter() = 0;
    virtual void add(std::string const &name, FWStringVec const &params, std::string const &part) = 0;
    virtual void load(std::string const &filename) = 0;
    virtual Value getConst(std::string const &name) = 0;
    virtual bool blocked() = 0;
    virtual void assignExternal(Value ext, TruthValue val) = 0;
    virtual Statistics *getStats() = 0;
    virtual void useEnumAssumption(bool enable) = 0;
    virtual bool useEnumAssumption() = 0;
    virtual void cleanupDomains() = 0;
    virtual ~Control() { }
};

// {{{1 declaration of Gringo

struct GringoModule {
    virtual Control *newControl(int argc, char const **argv) = 0;
    virtual void freeControl(Control *ctrl) = 0;
    virtual Value parseValue(std::string const &repr) = 0;
    virtual ~GringoModule() { }
};

// }}}1

} // namespace Gringo

#endif // _GRINGO_CONTROL_HH

