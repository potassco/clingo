// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright Roland Kaminski

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

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include <gringo/control.hh>
#include <gringo/input/groundtermparser.hh>
#include <clingo.hh>

namespace Gringo {

// {{{1 error handling

namespace {

clingo_solve_result_bitset_t convert(SolveResult r) {
    return static_cast<clingo_solve_result_bitset_t>(r.satisfiable()) |
           static_cast<clingo_solve_result_bitset_t>(r.interrupted()) * static_cast<clingo_solve_result_bitset_t>(clingo_solve_result_interrupted) |
           static_cast<clingo_solve_result_bitset_t>(r.exhausted()) * static_cast<clingo_solve_result_bitset_t>(clingo_solve_result_exhausted);
}

template <class S, class P, class ...Args>
std::string to_string(S size, P print, Args ...args) {
    std::vector<char> ret;
    size_t n;
    handleCError(size(std::forward<Args>(args)..., &n));
    ret.resize(n);
    handleCError(print(std::forward<Args>(args)..., ret.data(), n));
    return std::string(ret.begin(), ret.end()-1);
}

template <class F>
size_t print_size(F f) {
    Gringo::CountStream cs;
    f(cs);
    cs.flush();
    return cs.count() + 1;
}

template <class F>
void print(char *ret, size_t n, F f) {
    Gringo::ArrayStream as(ret, n);
    f(as);
    as << '\0';
    as.flush();
}

thread_local std::exception_ptr g_lastException;
thread_local std::string g_lastMessage;

} // namespace

void handleCError(clingo_error_t code, std::exception_ptr *exc) {
    if (code != clingo_error_success) {
        if (exc && *exc) { std::rethrow_exception(*exc); }
        char const *msg = clingo_error_message();
        if (!msg) { msg = "no message"; }
        switch (code) {
            case clingo_error_fatal:     { throw std::runtime_error(msg); }
            case clingo_error_runtime:   { throw std::runtime_error(msg); }
            case clingo_error_logic:     { throw std::logic_error(msg); }
            case clingo_error_bad_alloc: { throw std::bad_alloc(); }
            case clingo_error_unknown:   { throw std::logic_error(msg); }
        }
    }
}

clingo_error_t handleCXXError() {
    try { throw; }
    catch (Gringo::GringoError const &e)       { g_lastException = std::current_exception(); return clingo_error_fatal; }
    // Note: a ClingoError is throw after an exception is set or a user error is thrown so either
    //       - g_lastException is already set, or
    //       - there was a user error (currently not associated to an error message)
    catch (Gringo::ClingoError const &e)       { return e.err; }
    catch (Gringo::MessageLimitError const &e) { g_lastException = std::current_exception(); return clingo_error_fatal; }
    catch (std::bad_alloc const &e)            { g_lastException = std::current_exception(); return clingo_error_bad_alloc; }
    catch (std::runtime_error const &e)        { g_lastException = std::current_exception(); return clingo_error_runtime; }
    catch (std::logic_error const &e)          { g_lastException = std::current_exception(); return clingo_error_logic; }
    return clingo_error_unknown;
}

// }}}1

} // namespace Gringo

using namespace Gringo;

namespace Clingo {

// c++ interface

// {{{1 signature

Signature::Signature(char const *name, uint32_t arity, bool sign) {
    handleCError(clingo_signature_create(name, arity, sign, &sig_));
}

char const *Signature::name() const {
    return clingo_signature_name(sig_);
}

uint32_t Signature::arity() const {
    return clingo_signature_arity(sig_);
}

bool Signature::sign() const {
    return clingo_signature_sign(sig_);
}


size_t Signature::hash() const {
    return clingo_signature_hash(sig_);
}

bool operator==(Signature a, Signature b) { return  clingo_signature_is_equal_to(a.to_c(), b.to_c()); }
bool operator!=(Signature a, Signature b) { return !clingo_signature_is_equal_to(a.to_c(), b.to_c()); }
bool operator< (Signature a, Signature b) { return  clingo_signature_is_less_than(a.to_c(), b.to_c()); }
bool operator<=(Signature a, Signature b) { return !clingo_signature_is_less_than(b.to_c(), a.to_c()); }
bool operator> (Signature a, Signature b) { return  clingo_signature_is_less_than(b.to_c(), a.to_c()); }
bool operator>=(Signature a, Signature b) { return !clingo_signature_is_less_than(a.to_c(), b.to_c()); }

// {{{1 symbol

Symbol::Symbol() {
    clingo_symbol_create_num(0, &sym_);
}

Symbol::Symbol(clingo_symbol_t sym)
: sym_(sym) { }

Symbol Number(int num) {
    clingo_symbol_t sym;
    clingo_symbol_create_num(num, &sym);
    return Symbol(sym);
}

Symbol Supremum() {
    clingo_symbol_t sym;
    clingo_symbol_create_supremum(&sym);
    return Symbol(sym);
}

Symbol Infimum() {
    clingo_symbol_t sym;
    clingo_symbol_create_infimum(&sym);
    return Symbol(sym);
}

Symbol String(char const *str) {
    clingo_symbol_t sym;
    handleCError(clingo_symbol_create_string(str, &sym));
    return Symbol(sym);
}

Symbol Id(char const *id, bool sign) {
    clingo_symbol_t sym;
    handleCError(clingo_symbol_create_id(id, sign, &sym));
    return Symbol(sym);
}

Symbol Function(char const *name, SymbolSpan args, bool sign) {
    clingo_symbol_t sym;
    handleCError(clingo_symbol_create_function(name, reinterpret_cast<clingo_symbol_t const *>(args.begin()), args.size(), sign, &sym));
    return Symbol(sym);
}

int Symbol::num() const {
    int ret;
    handleCError(clingo_symbol_number(sym_, &ret));
    return ret;
}

char const *Symbol::name() const {
    char const *ret;
    handleCError(clingo_symbol_name(sym_, &ret));
    return ret;
}

char const *Symbol::string() const {
    char const *ret;
    handleCError(clingo_symbol_string(sym_, &ret));
    return ret;
}

bool Symbol::sign() const {
    bool ret;
    handleCError(clingo_symbol_sign(sym_, &ret));
    return ret;
}

SymbolSpan Symbol::args() const {
    clingo_symbol_t const *ret;
    size_t n;
    handleCError(clingo_symbol_arguments(sym_, &ret, &n));
    return {reinterpret_cast<Symbol const *>(ret), n};
}

SymbolType Symbol::type() const {
    return static_cast<SymbolType>(clingo_symbol_type(sym_));
}

#define CLINGO_CALLBACK_TRY try
#define CLINGO_CALLBACK_CATCH(ref) catch (...){ (ref) = std::current_exception(); return clingo_error_unknown; } return clingo_error_success

std::string Symbol::to_string() const {
    return ::to_string(clingo_symbol_to_string_size, clingo_symbol_to_string, sym_);
}

size_t Symbol::hash() const {
    return clingo_symbol_hash(sym_);
}

std::ostream &operator<<(std::ostream &out, Symbol sym) {
    out << sym.to_string();
    return out;
}

bool operator==(Symbol a, Symbol b) { return  clingo_symbol_is_equal_to(a.to_c(), b.to_c()); }
bool operator!=(Symbol a, Symbol b) { return !clingo_symbol_is_equal_to(a.to_c(), b.to_c()); }
bool operator< (Symbol a, Symbol b) { return  clingo_symbol_is_less_than(a.to_c(), b.to_c()); }
bool operator<=(Symbol a, Symbol b) { return !clingo_symbol_is_less_than(b.to_c(), a.to_c()); }
bool operator> (Symbol a, Symbol b) { return  clingo_symbol_is_less_than(b.to_c(), a.to_c()); }
bool operator>=(Symbol a, Symbol b) { return !clingo_symbol_is_less_than(a.to_c(), b.to_c()); }

// {{{1 symbolic atoms

Symbol SymbolicAtom::symbol() const {
    clingo_symbol_t ret;
    clingo_symbolic_atoms_symbol(atoms_, range_, &ret);
    return Symbol(ret);
}

clingo_literal_t SymbolicAtom::literal() const {
    clingo_literal_t ret;
    clingo_symbolic_atoms_literal(atoms_, range_, &ret);
    return ret;
}

bool SymbolicAtom::is_fact() const {
    bool ret;
    clingo_symbolic_atoms_is_fact(atoms_, range_, &ret);
    return ret;
}

bool SymbolicAtom::is_external() const {
    bool ret;
    clingo_symbolic_atoms_is_external(atoms_, range_, &ret);
    return ret;
}

SymbolicAtomIterator &SymbolicAtomIterator::operator++() {
    clingo_symbolic_atom_iterator_t range;
    handleCError(clingo_symbolic_atoms_next(atoms_, range_, &range));
    range_ = range;
    return *this;
}

SymbolicAtomIterator::operator bool() const {
    bool ret;
    handleCError(clingo_symbolic_atoms_is_valid(atoms_, range_, &ret));
    return ret;
}

bool SymbolicAtomIterator::operator==(SymbolicAtomIterator it) const {
    bool ret = atoms_ == it.atoms_;
    if (ret) { handleCError(clingo_symbolic_atoms_iterator_is_equal_to(atoms_, range_, it.range_, &ret)); }
    return ret;
}

SymbolicAtomIterator SymbolicAtoms::begin() const {
    clingo_symbolic_atom_iterator_t it;
    handleCError(clingo_symbolic_atoms_begin(atoms_, nullptr, &it));
    return SymbolicAtomIterator{atoms_,  it};
}

SymbolicAtomIterator SymbolicAtoms::begin(Signature sig) const {
    clingo_symbolic_atom_iterator_t it;
    handleCError(clingo_symbolic_atoms_begin(atoms_, &sig.to_c(), &it));
    return SymbolicAtomIterator{atoms_, it};
}

SymbolicAtomIterator SymbolicAtoms::end() const {
    clingo_symbolic_atom_iterator_t it;
    handleCError(clingo_symbolic_atoms_end(atoms_, &it));
    return SymbolicAtomIterator{atoms_, it};
}

SymbolicAtomIterator SymbolicAtoms::find(Symbol atom) const {
    clingo_symbolic_atom_iterator_t it;
    handleCError(clingo_symbolic_atoms_find(atoms_, atom.to_c(), &it));
    return SymbolicAtomIterator{atoms_, it};
}

std::vector<Signature> SymbolicAtoms::signatures() const {
    size_t n;
    clingo_symbolic_atoms_signatures_size(atoms_, &n);
    Signature sig("", 0);
    std::vector<Signature> ret;
    ret.resize(n, sig);
    handleCError(clingo_symbolic_atoms_signatures(atoms_, reinterpret_cast<clingo_signature_t *>(ret.data()), n));
    return ret;
}

size_t SymbolicAtoms::length() const {
    size_t ret;
    handleCError(clingo_symbolic_atoms_size(atoms_, &ret));
    return ret;
}

// {{{1 theory atoms

TheoryTermType TheoryTerm::type() const {
    clingo_theory_term_type_t ret;
    handleCError(clingo_theory_atoms_term_type(atoms_, id_, &ret));
    return static_cast<TheoryTermType>(ret);
}

int TheoryTerm::number() const {
    int ret;
    handleCError(clingo_theory_atoms_term_number(atoms_, id_, &ret));
    return ret;
}

char const *TheoryTerm::name() const {
    char const *ret;
    handleCError(clingo_theory_atoms_term_name(atoms_, id_, &ret));
    return ret;
}

TheoryTermSpan TheoryTerm::arguments() const {
    clingo_id_t const *ret;
    size_t n;
    handleCError(clingo_theory_atoms_term_arguments(atoms_, id_, &ret, &n));
    return {ret, n, ToTheoryIterator<TheoryTermIterator>{atoms_}};
}

std::ostream &operator<<(std::ostream &out, TheoryTerm term) {
    out << term.to_string();
    return out;
}

std::string TheoryTerm::to_string() const {
    return ::to_string(clingo_theory_atoms_term_to_string_size, clingo_theory_atoms_term_to_string, atoms_, id_);
}


TheoryTermSpan TheoryElement::tuple() const {
    clingo_id_t const *ret;
    size_t n;
    handleCError(clingo_theory_atoms_element_tuple(atoms_, id_, &ret, &n));
    return {ret, n, ToTheoryIterator<TheoryTermIterator>{atoms_}};
}

LiteralSpan TheoryElement::condition() const {
    clingo_literal_t const *ret;
    size_t n;
    handleCError(clingo_theory_atoms_element_condition(atoms_, id_, &ret, &n));
    return {ret, n};
}

literal_t TheoryElement::condition_literal() const {
    clingo_literal_t ret;
    handleCError(clingo_theory_atoms_element_condition_literal(atoms_, id_, &ret));
    return ret;
}

std::string TheoryElement::to_string() const {
    return ::to_string(clingo_theory_atoms_element_to_string_size, clingo_theory_atoms_element_to_string, atoms_, id_);
}

std::ostream &operator<<(std::ostream &out, TheoryElement term) {
    out << term.to_string();
    return out;
}

TheoryElementSpan TheoryAtom::elements() const {
    clingo_id_t const *ret;
    size_t n;
    handleCError(clingo_theory_atoms_atom_elements(atoms_, id_, &ret, &n));
    return {ret, n, ToTheoryIterator<TheoryElementIterator>{atoms_}};
}

TheoryTerm TheoryAtom::term() const {
    clingo_id_t ret;
    handleCError(clingo_theory_atoms_atom_term(atoms_, id_, &ret));
    return TheoryTerm{atoms_, ret};
}

bool TheoryAtom::has_guard() const {
    bool ret;
    handleCError(clingo_theory_atoms_atom_has_guard(atoms_, id_, &ret));
    return ret;
}

literal_t TheoryAtom::literal() const {
    clingo_literal_t ret;
    handleCError(clingo_theory_atoms_atom_literal(atoms_, id_, &ret));
    return ret;
}

std::pair<char const *, TheoryTerm> TheoryAtom::guard() const {
    char const *name;
    clingo_id_t term;
    handleCError(clingo_theory_atoms_atom_guard(atoms_, id_, &name, &term));
    return {name, TheoryTerm{atoms_, term}};
}

std::string TheoryAtom::to_string() const {
    return ::to_string(clingo_theory_atoms_atom_to_string_size, clingo_theory_atoms_atom_to_string, atoms_, id_);
}

std::ostream &operator<<(std::ostream &out, TheoryAtom term) {
    out << term.to_string();
    return out;
}

TheoryAtomIterator TheoryAtoms::begin() const {
    return TheoryAtomIterator{atoms_, 0};
}

TheoryAtomIterator TheoryAtoms::end() const {
    return TheoryAtomIterator{atoms_, clingo_id_t(size())};
}

size_t TheoryAtoms::size() const {
    size_t ret;
    handleCError(clingo_theory_atoms_size(atoms_, &ret));
    return ret;
}

// {{{1 assignment

bool Assignment::has_conflict() const {
    return clingo_assignment_has_conflict(ass_);
}

uint32_t Assignment::decision_level() const {
    return clingo_assignment_decision_level(ass_);
}

bool Assignment::has_literal(literal_t lit) const {
    return clingo_assignment_has_literal(ass_, lit);
}

TruthValue Assignment::truth_value(literal_t lit) const {
    clingo_truth_value_t ret;
    handleCError(clingo_assignment_truth_value(ass_, lit, &ret));
    return static_cast<TruthValue>(ret);
}

uint32_t Assignment::level(literal_t lit) const {
    uint32_t ret;
    handleCError(clingo_assignment_level(ass_, lit, &ret));
    return ret;
}

literal_t Assignment::decision(uint32_t level) const {
    literal_t ret;
    handleCError(clingo_assignment_decision(ass_, level, &ret));
    return ret;
}

bool Assignment::is_fixed(literal_t lit) const {
    bool ret;
    handleCError(clingo_assignment_is_fixed(ass_, lit, &ret));
    return ret;
}

bool Assignment::is_true(literal_t lit) const {
    bool ret;
    handleCError(clingo_assignment_is_true(ass_, lit, &ret));
    return ret;
}

bool Assignment::is_false(literal_t lit) const {
    bool ret;
    handleCError(clingo_assignment_is_false(ass_, lit, &ret));
    return ret;
}

// {{{1 propagate control

literal_t PropagateInit::map_literal(literal_t lit) const {
    literal_t ret;
    handleCError(clingo_propagate_init_map_literal(init_, lit, &ret));
    return ret;
}

void PropagateInit::add_watch(literal_t lit) {
    handleCError(clingo_propagate_init_add_watch(init_, lit));
}

int PropagateInit::number_of_threads() const {
    return clingo_propagate_init_number_of_threads(init_);
}

SymbolicAtoms PropagateInit::symbolic_atoms() const {
    clingo_symbolic_atoms_t *ret;
    handleCError(clingo_propagate_init_symbolic_atoms(init_, &ret));
    return SymbolicAtoms{ret};
}

TheoryAtoms PropagateInit::theory_atoms() const {
    clingo_theory_atoms_t *ret;
    handleCError(clingo_propagate_init_theory_atoms(init_, &ret));
    return TheoryAtoms{ret};
}

// {{{1 propagate control

id_t PropagateControl::thread_id() const {
    return clingo_propagate_control_thread_id(ctl_);
}

Assignment PropagateControl::assignment() const {
    return Assignment{clingo_propagate_control_assignment(ctl_)};
}

bool PropagateControl::add_clause(LiteralSpan clause, ClauseType type) {
    bool ret;
    handleCError(clingo_propagate_control_add_clause(ctl_, clause.begin(), clause.size(), static_cast<clingo_clause_type_t>(type), &ret));
    return ret;
}

bool PropagateControl::propagate() {
    bool ret;
    handleCError(clingo_propagate_control_propagate(ctl_, &ret));
    return ret;
}

// {{{1 propagator

void Propagator::init(PropagateInit &) { }
void Propagator::propagate(PropagateControl &, LiteralSpan) { }
void Propagator::undo(PropagateControl const &, LiteralSpan) { }
void Propagator::check(PropagateControl &) { }

// {{{1 solve control

void SolveControl::add_clause(SymbolicLiteralSpan clause) {
    handleCError(clingo_solve_control_add_clause(ctl_, reinterpret_cast<clingo_symbolic_literal_t const *>(clause.begin()), clause.size()));
}

id_t SolveControl::thread_id() const {
    id_t ret;
    handleCError(clingo_solve_control_thread_id(ctl_, &ret));
    return ret;
}

// {{{1 model

Model::Model(clingo_model_t *model)
: model_(model) { }

bool Model::contains(Symbol atom) const {
    bool ret;
    handleCError(clingo_model_contains(model_, atom.to_c(), &ret));
    return ret;
}

CostVector Model::cost() const {
    CostVector ret;
    size_t n;
    handleCError(clingo_model_cost_size(model_, &n));
    ret.resize(n);
    handleCError(clingo_model_cost(model_, ret.data(), n));
    return ret;
}

SymbolVector Model::atoms(ShowType show) const {
    SymbolVector ret;
    size_t n;
    handleCError(clingo_model_atoms_size(model_, show, &n));
    ret.resize(n);
    handleCError(clingo_model_atoms(model_, show, reinterpret_cast<clingo_symbol_t *>(ret.data()), n));
    return ret;
}

uint64_t Model::number() const {
    uint64_t ret;
    handleCError(clingo_model_number(model_, &ret));
    return ret;
}

bool Model::optimality_proven() const {
    bool ret;
    handleCError(clingo_model_optimality_proven(model_, &ret));
    return ret;
}

SolveControl Model::context() const {
    clingo_solve_control_t *ret;
    handleCError(clingo_model_context(model_, &ret));
    return SolveControl{ret};
}

ModelType Model::type() const {
    clingo_model_type_t ret;
    handleCError(clingo_model_type(model_, &ret));
    return static_cast<ModelType>(ret);
}

// {{{1 solve iter

SolveIteratively::SolveIteratively()
: iter_(nullptr) { }

SolveIteratively::SolveIteratively(clingo_solve_iteratively_t *it)
: iter_(it) { }

SolveIteratively::SolveIteratively(SolveIteratively &&it)
: iter_(nullptr) { std::swap(iter_, it.iter_); }

SolveIteratively &SolveIteratively::operator=(SolveIteratively &&it) {
    std::swap(iter_, it.iter_);
    return *this;
}

Model SolveIteratively::next() {
    clingo_model_t *m = nullptr;
    if (iter_) { handleCError(clingo_solve_iteratively_next(iter_, &m)); }
    return Model{m};
}

SolveResult SolveIteratively::get() {
    clingo_solve_result_bitset_t ret = 0;
    if (iter_) { handleCError(clingo_solve_iteratively_get(iter_, &ret)); }
    return SolveResult{ret};
}

void SolveIteratively::close() {
    if (iter_) {
        clingo_solve_iteratively_close(iter_);
        iter_ = nullptr;
    }
}

// {{{1 solve async

void SolveAsync::cancel() {
    handleCError(clingo_solve_async_cancel(async_));
}

SolveResult SolveAsync::get() {
    clingo_solve_result_bitset_t ret;
    handleCError(clingo_solve_async_get(async_, &ret));
    return SolveResult{ret};
}

bool SolveAsync::wait(double timeout) {
    bool ret;
    handleCError(clingo_solve_async_wait(async_, timeout, &ret));
    return ret;
}

// {{{1 backend

void Backend::rule(bool choice, AtomSpan head, LiteralSpan body) {
    handleCError(clingo_backend_rule(backend_, choice, head.begin(), head.size(), body.begin(), body.size()));
}

void Backend::weight_rule(bool choice, AtomSpan head, weight_t lower, WeightedLiteralSpan body) {
    handleCError(clingo_backend_weight_rule(backend_, choice, head.begin(), head.size(), lower, reinterpret_cast<clingo_weighted_literal_t const *>(body.begin()), body.size()));
}

void Backend::minimize(weight_t prio, WeightedLiteralSpan body) {
    handleCError(clingo_backend_minimize(backend_, prio, reinterpret_cast<clingo_weighted_literal_t const *>(body.begin()), body.size()));
}

void Backend::project(AtomSpan atoms) {
    handleCError(clingo_backend_project(backend_, atoms.begin(), atoms.size()));
}

void Backend::external(atom_t atom, ExternalType type) {
    handleCError(clingo_backend_external(backend_, atom, static_cast<clingo_external_type_t>(type)));
}

void Backend::assume(LiteralSpan lits) {
    handleCError(clingo_backend_assume(backend_, lits.begin(), lits.size()));
}

void Backend::heuristic(atom_t atom, HeuristicType type, int bias, unsigned priority, LiteralSpan condition) {
    handleCError(clingo_backend_heuristic(backend_, atom, static_cast<clingo_heuristic_type_t>(type), bias, priority, condition.begin(), condition.size()));
}

void Backend::acyc_edge(int node_u, int node_v, LiteralSpan condition) {
    handleCError(clingo_backend_acyc_edge(backend_, node_u, node_v, condition.begin(), condition.size()));
}

atom_t Backend::add_atom() {
    clingo_atom_t ret;
    handleCError(clingo_backend_add_atom(backend_, &ret));
    return ret;
}

// {{{1 statistics

StatisticsType Statistics::type() const {
    clingo_statistics_type_t ret;
    handleCError(clingo_statistics_type(stats_, key_, &ret));
    return StatisticsType(ret);
}

size_t Statistics::size() const {
    size_t ret;
    handleCError(clingo_statistics_array_size(stats_, key_, &ret));
    return ret;
}

Statistics Statistics::operator[](size_t index) const {
    clingo_id_t ret;
    handleCError(clingo_statistics_array_at(stats_, key_, index, &ret));
    return Statistics{stats_, ret};
}

StatisticsArrayIterator Statistics::begin() const {
    return StatisticsArrayIterator{this, 0};
}

StatisticsArrayIterator Statistics::end() const {
    return StatisticsArrayIterator{this, size()};
}

Statistics Statistics::operator[](char const *name) const {
    clingo_id_t ret;
    handleCError(clingo_statistics_map_at(stats_, key_, name, &ret));
    return Statistics{stats_, ret};
}

StatisticsKeyRange Statistics::keys() const {
    size_t ret;
    handleCError(clingo_statistics_map_size(stats_, key_, &ret));
    return StatisticsKeyRange{ StatisticsKeyIterator{this, 0}, StatisticsKeyIterator{this, ret} };
}

double Statistics::value() const {
    double ret;
    handleCError(clingo_statistics_value_get(stats_, key_, &ret));
    return ret;
}

char const *Statistics::key_name(size_t index) const {
    char const *ret;
    handleCError(clingo_statistics_map_subkey_name(stats_, key_, index, &ret));
    return ret;
}

// {{{1 configuration

Configuration Configuration::operator[](size_t index) {
    unsigned ret;
    handleCError(clingo_configuration_array_at(conf_, key_, index, &ret));
    return Configuration{conf_, ret};
}

ConfigurationArrayIterator Configuration::begin() {
    return ConfigurationArrayIterator{this, 0};
}

ConfigurationArrayIterator Configuration::end() {
    return ConfigurationArrayIterator{this, size()};
}

size_t Configuration::size() const {
    size_t n;
    handleCError(clingo_configuration_array_size(conf_, key_, &n));
    return n;
}

bool Configuration::empty() const {
    return size() == 0;
}

Configuration Configuration::operator[](char const *name) {
    clingo_id_t ret;
    handleCError(clingo_configuration_map_at(conf_, key_, name, &ret));
    return Configuration{conf_, ret};
}

ConfigurationKeyRange Configuration::keys() const {
    size_t n;
    handleCError(clingo_configuration_map_size(conf_, key_, &n));
    return ConfigurationKeyRange{ ConfigurationKeyIterator{this, size_t(0)}, ConfigurationKeyIterator{this, size_t(n)} };
}

bool Configuration::is_value() const {
    clingo_configuration_type_bitset_t type;
    handleCError(clingo_configuration_type(conf_, key_, &type));
    return type & clingo_configuration_type_value;
}

bool Configuration::is_array() const {
    clingo_configuration_type_bitset_t type;
    handleCError(clingo_configuration_type(conf_, key_, &type));
    return type & clingo_configuration_type_array;
}

bool Configuration::is_map() const {
    clingo_configuration_type_bitset_t type;
    handleCError(clingo_configuration_type(conf_, key_, &type));
    return type & clingo_configuration_type_map;
}

bool Configuration::is_assigned() const {
    bool ret;
    handleCError(clingo_configuration_value_is_assigned(conf_, key_, &ret));
    return ret;
}

std::string Configuration::value() const {
    size_t n;
    handleCError(clingo_configuration_value_get_size(conf_, key_, &n));
    std::vector<char> ret(n);
    handleCError(clingo_configuration_value_get(conf_, key_, ret.data(), n));
    return std::string(ret.begin(), ret.end() - 1);
}

Configuration &Configuration::operator=(char const *value) {
    handleCError(clingo_configuration_value_set(conf_, key_, value));
    return *this;
}

char const *Configuration::decription() const {
    char const *ret;
    handleCError(clingo_configuration_description(conf_, key_, &ret));
    return ret;
}

char const *Configuration::key_name(size_t index) const {
    char const *ret;
    handleCError(clingo_configuration_map_subkey_name(conf_, key_, index, &ret));
    return ret;
}

// {{{1 control

struct Control::Impl {
    Impl(Logger logger)
    : ctl(nullptr)
    , logger(logger) { }
    Impl(clingo_control_t *ctl)
    : ctl(ctl) { }
    ~Impl() noexcept {
        if (ctl) { clingo_control_free(ctl); }
    }
    operator clingo_control_t *() { return ctl; }
    clingo_control_t *ctl;
    Logger logger;
    ModelCallback mh;
    FinishCallback fh;
};

Control::Control(clingo_control_t *ctl)
: impl_(gringo_make_unique<Impl>(ctl)) { }

Control::Control(Control &&c)
: impl_(std::move(c.impl_)) { }

Control &Control::operator=(Control &&c) {
    impl_ = std::move(c.impl_);
    return *this;
}

Control::~Control() noexcept = default;

void Control::add(char const *name, StringSpan params, char const *part) {
    handleCError(clingo_control_add(*impl_, name, params.begin(), params.size(), part));
}

void Control::ground(PartSpan parts, GroundCallback cb) {
    using Data = std::pair<GroundCallback&, std::exception_ptr>;
    Data data(cb, nullptr);
    handleCError(clingo_control_ground(*impl_, reinterpret_cast<clingo_part_t const *>(parts.begin()), parts.size(),
        [](clingo_location_t loc, char const *name, clingo_symbol_t const *args, size_t n, void *data, clingo_symbol_callback_t *cb, void *cbdata) -> clingo_error_t {
            auto &d = *static_cast<Data*>(data);
            CLINGO_CALLBACK_TRY {
                if (d.first) {
                    struct Ret { clingo_error_t ret; };
                    try {
                        d.first(loc, name, {reinterpret_cast<Symbol const *>(args), n}, [cb, cbdata](SymbolSpan symret) {
                            clingo_error_t ret = cb(reinterpret_cast<clingo_symbol_t const *>(symret.begin()), symret.size(), cbdata);
                            if (ret != clingo_error_success) { throw Ret { ret }; }
                        });
                    }
                    catch (Ret e) { return e.ret; }
                }
            }
            CLINGO_CALLBACK_CATCH(d.second);
        }, &data), &data.second);
}

clingo_control_t *Control::to_c() const { return *impl_; }

SolveResult Control::solve(ModelCallback mh, SymbolicLiteralSpan assumptions) {
    clingo_solve_result_bitset_t ret;
    using Data = std::pair<ModelCallback&, std::exception_ptr>;
    Data data(mh, nullptr);
    handleCError(clingo_control_solve(*impl_, [](clingo_model_t *m, void *data, bool *ret) -> clingo_error_t {
        auto &d = *static_cast<Data*>(data);
        CLINGO_CALLBACK_TRY { *ret = d.first(Model(m)); }
        CLINGO_CALLBACK_CATCH(d.second);
    }, &data, reinterpret_cast<clingo_symbolic_literal_t const *>(assumptions.begin()), assumptions.size(), &ret));
    return SolveResult{ret};
}

SolveIteratively Control::solve_iteratively(SymbolicLiteralSpan assumptions) {
    clingo_solve_iteratively_t *it;
    handleCError(clingo_control_solve_iteratively(*impl_, reinterpret_cast<clingo_symbolic_literal_t const *>(assumptions.begin()), assumptions.size(), &it));
    return SolveIteratively{it};
}

void Control::assign_external(Symbol atom, TruthValue value) {
    handleCError(clingo_control_assign_external(*impl_, atom.to_c(), static_cast<clingo_truth_value_t>(value)));
}

void Control::release_external(Symbol atom) {
    handleCError(clingo_control_release_external(*impl_, atom.to_c()));
}

SymbolicAtoms Control::symbolic_atoms() const {
    clingo_symbolic_atoms_t *ret;
    handleCError(clingo_control_symbolic_atoms(*impl_, &ret));
    return SymbolicAtoms{ret};
}

TheoryAtoms Control::theory_atoms() const {
    clingo_theory_atoms_t *ret;
    clingo_control_theory_atoms(*impl_, &ret);
    return TheoryAtoms{ret};
}

namespace {

// NOTE: this sets exceptions in the running (propagation) thread(s)
// CAVEATS:
//   exceptions during propagation are not rethrown
//   they are thrown as new exceptions (with the same error message if available)
//   clasp gobbles exceptions in the multithreaded case
static clingo_error_t g_init(clingo_propagate_init_t *ctl, Propagator *p) {
    GRINGO_CLINGO_TRY {
        PropagateInit pi(ctl);
        p->init(pi);
    }
    GRINGO_CLINGO_CATCH;
}

static clingo_error_t g_propagate(clingo_propagate_control_t *ctl, clingo_literal_t const *changes, size_t n, Propagator *p) {
    GRINGO_CLINGO_TRY {
        PropagateControl pc(ctl);
        p->propagate(pc, {changes, n});
    }
    GRINGO_CLINGO_CATCH;
}

static clingo_error_t g_undo(clingo_propagate_control_t *ctl, clingo_literal_t const *changes, size_t n, Propagator *p) {
    GRINGO_CLINGO_TRY {
        PropagateControl pc(ctl);
        p->undo(pc, {changes, n});
    }
    GRINGO_CLINGO_CATCH;
}

static clingo_error_t g_check(clingo_propagate_control_t *ctl, Propagator *p) {
    GRINGO_CLINGO_TRY {
        PropagateControl pc(ctl);
        p->check(pc);
    }
    GRINGO_CLINGO_CATCH;
}

static clingo_propagator_t g_propagator = {
    reinterpret_cast<decltype(clingo_propagator_t::init)>(g_init),
    reinterpret_cast<decltype(clingo_propagator_t::propagate)>(g_propagate),
    reinterpret_cast<decltype(clingo_propagator_t::undo)>(g_undo),
    reinterpret_cast<decltype(clingo_propagator_t::check)>(g_check)
};

}

void Control::register_propagator(Propagator &propagator, bool sequential) {
    handleCError(clingo_control_register_propagator(*impl_, g_propagator, &propagator, sequential));
}

void Control::cleanup() {
    handleCError(clingo_control_cleanup(*impl_));
}

bool Control::has_const(char const *name) const {
    bool ret;
    handleCError(clingo_control_has_const(*impl_, name, &ret));
    return ret;
}

Symbol Control::get_const(char const *name) const {
    clingo_symbol_t ret;
    handleCError(clingo_control_get_const(*impl_, name, &ret));
    return Symbol(ret);
}

void Control::interrupt() noexcept {
    clingo_control_interrupt(*impl_);
}

void Control::load(char const *file) {
    handleCError(clingo_control_load(*impl_, file));
}

SolveAsync Control::solve_async(ModelCallback mh, FinishCallback fh, SymbolicLiteralSpan assumptions) {
    clingo_solve_async_t *ret;
    impl_->mh = std::move(mh);
    impl_->fh = std::move(fh);
    handleCError(clingo_control_solve_async(*impl_, [](clingo_model_t *m, void *data, bool *ret) -> clingo_error_t {
        GRINGO_CLINGO_TRY {
            auto &mh = *static_cast<ModelCallback*>(data);
            *ret = !mh || mh(Model{m});
        }
        GRINGO_CLINGO_CATCH;
    }, &impl_->mh, [](clingo_solve_result_bitset_t res, void *data) -> clingo_error_t {
        GRINGO_CLINGO_TRY {
            auto &fh = *static_cast<FinishCallback*>(data);
            if (fh) { fh(SolveResult{res}); }
        }
        GRINGO_CLINGO_CATCH;
    }, &impl_->fh, reinterpret_cast<clingo_symbolic_literal_t const *>(assumptions.begin()), assumptions.size(), &ret));
    return SolveAsync{ret};
}

void Control::use_enum_assumption(bool value) {
    handleCError(clingo_control_use_enum_assumption(*impl_, value));
}

Backend Control::backend() {
    clingo_backend_t *ret;
    handleCError(clingo_control_backend(*impl_, &ret));
    return Backend{ret};
}

Configuration Control::configuration() {
    clingo_configuration_t *conf;
    handleCError(clingo_control_configuration(*impl_, &conf));
    unsigned key;
    handleCError(clingo_configuration_root(conf, &key));
    return Configuration{conf, key};
}

Statistics Control::statistics() const {
    clingo_statistics_t *stats;
    handleCError(clingo_control_statistics(const_cast<clingo_control_t*>(impl_->ctl), &stats));
    unsigned key;
    handleCError(clingo_statistics_root(stats, &key));
    return Statistics{stats, key};
}

// {{{1 global functions

Symbol parse_term(char const *str, Logger logger, unsigned message_limit) {
    clingo_symbol_t ret;
    handleCError(clingo_parse_term(str, [](clingo_warning_t code, char const *msg, void *data) {
        try { (*static_cast<Logger*>(data))(static_cast<WarningCode>(code), msg); }
        catch (...) { }
    }, &logger, message_limit, &ret));
    return Symbol(ret);
}

// }}}1

} // namespace Clingo

// c interface

// {{{1 error handling

extern "C" char const *clingo_error_message() {
    if (g_lastException) {
        try { std::rethrow_exception(g_lastException); }
        catch (std::bad_alloc const &e) { return "bad_alloc"; }
        catch (std::exception const &e) {
            g_lastMessage = e.what();
            return g_lastMessage.c_str();
        }
    }
    return nullptr;
}

extern "C" char const *clingo_error_string(clingo_error_t code) {
    switch (static_cast<clingo_error>(code)) {
        case clingo_error_success:               { return "success"; }
        case clingo_error_runtime:               { return "runtime error"; }
        case clingo_error_bad_alloc:             { return "bad allocation"; }
        case clingo_error_logic:                 { return "logic error"; }
        case clingo_error_fatal:                 { return "fatal error"; }
        case clingo_error_unknown:               { return "unknown error"; }
    }
    return nullptr;
}

extern "C" char const *clingo_warning_string(clingo_warning_t code) {
    switch (static_cast<clingo_warning>(code)) {
        case clingo_warning_operation_undefined: { return "operation_undefined"; }
        case clingo_warning_atom_undefined:      { return "atom undefined"; }
        case clingo_warning_file_included:       { return "file included"; }
        case clingo_warning_variable_unbounded:  { return "variable unbounded"; }
        case clingo_warning_global_variable:     { return "global variable"; }
        case clingo_warning_other:               { return "other"; }
    }
    return "unknown message code";
}

// {{{1 signature

extern "C" clingo_error_t clingo_signature_create(char const *name, uint32_t arity, bool sign, clingo_signature_t *ret) {
    GRINGO_CLINGO_TRY {
        *ret = Sig(name, arity, sign).rep();
    } GRINGO_CLINGO_CATCH;
}

extern "C" char const *clingo_signature_name(clingo_signature_t sig) {
    return Sig(sig).name().c_str();
}

extern "C" uint32_t clingo_signature_arity(clingo_signature_t sig) {
    return Sig(sig).arity();
}

extern "C" bool clingo_signature_sign(clingo_signature_t sig) {
    return Sig(sig).sign();
}

extern "C" size_t clingo_signature_hash(clingo_signature_t sig) {
    return Sig(sig).hash();
}

extern "C" bool clingo_signature_is_equal_to(clingo_signature_t a, clingo_signature_t b) {
    return Sig(a) == Sig(b);
}

extern "C" bool clingo_signature_is_less_than(clingo_signature_t a, clingo_signature_t b) {
    return Sig(a) < Sig(b);
}


// {{{1 value

extern "C" void clingo_symbol_create_num(int num, clingo_symbol_t *val) {
    *val = Symbol::createNum(num).rep();
}

extern "C" void clingo_symbol_create_supremum(clingo_symbol_t *val) {
    *val = Symbol::createSup().rep();
}

extern "C" void clingo_symbol_create_infimum(clingo_symbol_t *val) {
    *val = Symbol::createInf().rep();
}

extern "C" clingo_error_t clingo_symbol_create_string(char const *str, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY {
        *val = Symbol::createStr(str).rep();
    } GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_create_id(char const *id, bool sign, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY {
        *val = Symbol::createId(id, sign).rep();
    } GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_create_function(char const *name, clingo_symbol_t const *args, size_t n, bool sign, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY {
        *val = Symbol::createFun(name, SymSpan{reinterpret_cast<Symbol const *>(args), n}, sign).rep();
    } GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_number(clingo_symbol_t val, int *num) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Num);
        *num = Symbol(val).num();
    } GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_name(clingo_symbol_t val, char const **name) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Fun);
        *name = Symbol(val).name().c_str();
    } GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_string(clingo_symbol_t val, char const **str) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Str);
        *str = Symbol(val).string().c_str();
    } GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_sign(clingo_symbol_t val, bool *sign) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Fun);
        *sign = Symbol(val).sign();
        return clingo_error_success;
    } GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_arguments(clingo_symbol_t val, clingo_symbol_t const **args, size_t *n) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Fun);
        auto ret = Symbol(val).args();
        *args = reinterpret_cast<clingo_symbol_t const *>(ret.first);
        *n = ret.size;
    } GRINGO_CLINGO_CATCH;
}

extern "C" clingo_symbol_type_t clingo_symbol_type(clingo_symbol_t val) {
    return static_cast<clingo_symbol_type_t>(Symbol(val).type());
}

extern "C" clingo_error_t clingo_symbol_to_string_size(clingo_symbol_t val, size_t *n) {
    GRINGO_CLINGO_TRY { *n = print_size([&val](std::ostream &out) { Symbol(val).print(out); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_to_string(clingo_symbol_t val, char *ret, size_t n) {
    GRINGO_CLINGO_TRY { print(ret, n, [&val](std::ostream &out) { Symbol(val).print(out); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_is_equal_to(clingo_symbol_t a, clingo_symbol_t b) {
    return Symbol(a) == Symbol(b);
}

extern "C" bool clingo_symbol_is_less_than(clingo_symbol_t a, clingo_symbol_t b) {
    return Symbol(a) < Symbol(b);
}

extern "C" size_t clingo_symbol_hash(clingo_symbol_t sym) {
    return Symbol(sym).hash();
}

// {{{1 symbolic atoms

extern "C" clingo_error_t clingo_symbolic_atoms_begin(clingo_symbolic_atoms_t *dom, clingo_signature_t const *sig, clingo_symbolic_atom_iterator_t *ret) {
    GRINGO_CLINGO_TRY { *ret = sig ? dom->begin(Sig(*sig)) : dom->begin(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_end(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t *ret) {
    GRINGO_CLINGO_TRY { *ret = dom->end(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_find(clingo_symbolic_atoms_t *dom, clingo_symbol_t atom, clingo_symbolic_atom_iterator_t *ret) {
    GRINGO_CLINGO_TRY { *ret = dom->lookup(Symbol(atom)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_iterator_is_equal_to(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t it, clingo_symbolic_atom_iterator_t jt, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = dom->eq(it, jt); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_signatures_size(clingo_symbolic_atoms_t *dom, size_t *n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        auto sigs = dom->signatures();
        *n = sigs.size();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_signatures(clingo_symbolic_atoms_t *dom, clingo_signature_t *ret, size_t n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        auto sigs = dom->signatures();
        if (n < sigs.size()) { throw std::length_error("not enough space"); }
        for (auto &sig : sigs) { *ret++ = sig.rep(); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_size(clingo_symbolic_atoms_t *dom, size_t *size) {
    GRINGO_CLINGO_TRY { *size = dom->length(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_symbol(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, clingo_symbol_t *sym) {
    GRINGO_CLINGO_TRY { *sym = dom->atom(atm).rep(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_literal(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, clingo_literal_t *lit) {
    GRINGO_CLINGO_TRY { *lit = dom->literal(atm); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_is_fact(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, bool *fact) {
    GRINGO_CLINGO_TRY { *fact = dom->fact(atm); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_is_external(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, bool *external) {
    GRINGO_CLINGO_TRY { *external = dom->external(atm); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_next(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, clingo_symbolic_atom_iterator_t *next) {
    GRINGO_CLINGO_TRY { *next = dom->next(atm); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbolic_atoms_is_valid(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, bool *valid) {
    GRINGO_CLINGO_TRY { *valid = dom->valid(atm); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 theory atoms

extern "C" clingo_error_t clingo_theory_atoms_term_type(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_theory_term_type_t *ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_theory_term_type_t>(atoms->termType(value)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_term_number(clingo_theory_atoms_t *atoms, clingo_id_t value, int *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->termNum(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_term_name(clingo_theory_atoms_t *atoms, clingo_id_t value, char const **ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->termName(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_term_arguments(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->termArgs(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_element_tuple(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->elemTuple(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_element_condition(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_literal_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->elemCond(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_element_condition_literal(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_literal_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->elemCondLit(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_atom_elements(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->atomElems(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_atom_term(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->atomTerm(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_atom_has_guard(clingo_theory_atoms_t *atoms, clingo_id_t value, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->atomHasGuard(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_atom_literal(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_literal_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->atomLit(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_atom_guard(clingo_theory_atoms_t *atoms, clingo_id_t value, char const **ret_op, clingo_id_t *ret_term) {
    GRINGO_CLINGO_TRY {
        auto guard = atoms->atomGuard(value);
        *ret_op = guard.first;
        *ret_term = guard.second;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_size(clingo_theory_atoms_t *atoms, size_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->numAtoms(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_term_to_string_size(clingo_theory_atoms_t *atoms, clingo_id_t value, size_t *n) {
    GRINGO_CLINGO_TRY { *n = print_size([atoms, value](std::ostream &out) { out << atoms->termStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_term_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->termStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_element_to_string_size(clingo_theory_atoms_t *atoms, clingo_id_t value, size_t *n) {
    GRINGO_CLINGO_TRY { *n = print_size([atoms, value](std::ostream &out) { out << atoms->elemStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_element_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->elemStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_atom_to_string_size(clingo_theory_atoms_t *atoms, clingo_id_t value, size_t *n) {
    GRINGO_CLINGO_TRY { *n = print_size([atoms, value](std::ostream &out) { out << atoms->atomStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_theory_atoms_atom_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->atomStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 propagate init

extern "C" clingo_error_t clingo_propagate_init_map_literal(clingo_propagate_init_t *init, clingo_literal_t lit, clingo_literal_t *ret) {
    GRINGO_CLINGO_TRY { *ret = init->mapLit(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_propagate_init_add_watch(clingo_propagate_init_t *init, clingo_literal_t lit) {
    GRINGO_CLINGO_TRY { init->addWatch(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" int clingo_propagate_init_number_of_threads(clingo_propagate_init_t *init) {
    return init->threads();
}

extern "C" clingo_error_t clingo_propagate_init_symbolic_atoms(clingo_propagate_init_t *init, clingo_symbolic_atoms_t **ret) {
    GRINGO_CLINGO_TRY { *ret = &init->getDomain(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_propagate_init_theory_atoms(clingo_propagate_init_t *init, clingo_theory_atoms_t **ret) {
    GRINGO_CLINGO_TRY { *ret = const_cast<Gringo::TheoryData*>(&init->theory()); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 assignment

struct clingo_assignment : public Potassco::AbstractAssignment { };

extern "C" bool clingo_assignment_has_conflict(clingo_assignment_t *ass) {
    return ass->hasConflict();
}

extern "C" uint32_t clingo_assignment_decision_level(clingo_assignment_t *ass) {
    return ass->level();
}

extern "C" bool clingo_assignment_has_literal(clingo_assignment_t *ass, clingo_literal_t lit) {
    return ass->hasLit(lit);
}

extern "C" clingo_error_t clingo_assignment_truth_value(clingo_assignment_t *ass, clingo_literal_t lit, clingo_truth_value_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->value(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_assignment_level(clingo_assignment_t *ass, clingo_literal_t lit, uint32_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->level(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_assignment_decision(clingo_assignment_t *ass, uint32_t level, clingo_literal_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->decision(level); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_assignment_is_fixed(clingo_assignment_t *ass, clingo_literal_t lit, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->isFixed(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_assignment_is_true(clingo_assignment_t *ass, clingo_literal_t lit, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->isTrue(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_assignment_is_false(clingo_assignment_t *ass, clingo_literal_t lit, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->isFalse(lit); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 propagate control

struct clingo_propagate_control : Potassco::AbstractSolver { };

extern "C" clingo_id_t clingo_propagate_control_thread_id(clingo_propagate_control_t *ctl) {
    return ctl->id();
}

extern "C" clingo_assignment_t *clingo_propagate_control_assignment(clingo_propagate_control_t *ctl) {
    return const_cast<clingo_assignment *>(static_cast<clingo_assignment const *>(&ctl->assignment()));
}

extern "C" clingo_error_t clingo_propagate_control_add_clause(clingo_propagate_control_t *ctl, clingo_literal_t const *clause, size_t n, clingo_clause_type_t prop, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ctl->addClause({clause, n}, Potassco::Clause_t(prop)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_propagate_control_propagate(clingo_propagate_control_t *ctl, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ctl->propagate(); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 model

struct clingo_solve_control : clingo_model { };

extern "C" clingo_error_t clingo_solve_control_thread_id(clingo_solve_control_t *ctl, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ctl->threadId(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_solve_control_add_clause(clingo_solve_control_t *ctl, clingo_symbolic_literal_t const *clause, size_t n) {
    GRINGO_CLINGO_TRY {
        // TODO: unnecessary copying
        Gringo::Model::LitVec lits;
        for (auto it = clause, ie = it + n; it != ie; ++it) { lits.emplace_back(Symbol(it->atom), it->sign); }
        ctl->addClause(lits); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_model_contains(clingo_model_t *m, clingo_symbol_t atom, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = m->contains(Symbol(atom)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_model_atoms_size(clingo_model_t *m, clingo_show_type_bitset_t show, size_t *n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        SymSpan atoms = m->atoms(show);
        *n = atoms.size;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_model_atoms(clingo_model_t *m, clingo_show_type_bitset_t show, clingo_symbol_t *ret, size_t n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        SymSpan atoms = m->atoms(show);
        if (n < atoms.size) { throw std::length_error("not enough space"); }
        for (auto it = atoms.first, ie = it + atoms.size; it != ie; ++it) { *ret++ = it->rep(); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_model_optimality_proven(clingo_model_t *m, bool *proven) {
    GRINGO_CLINGO_TRY { *proven = m->optimality_proven(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_model_cost_size(clingo_model_t *m, size_t *n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        auto opt = m->optimization();
        *n = opt.size();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_model_cost(clingo_model_t *m, int64_t *ret, size_t n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        auto opt = m->optimization();
        if (n < opt.size()) { throw std::length_error("not enough space"); }
        std::copy(opt.begin(), opt.end(), ret);
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_model_context(clingo_model_t *m, clingo_solve_control_t **ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_solve_control_t*>(m); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_model_number(clingo_model_t *m, uint64_t *n) {
    GRINGO_CLINGO_TRY { *n = m->number(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_model_type(clingo_model_t *m, clingo_model_type_t *ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_model_type_t>(m->type()); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 solve iter

struct clingo_solve_iteratively : SolveIter { };

extern "C" clingo_error_t clingo_solve_iteratively_next(clingo_solve_iteratively_t *it, clingo_model **m) {
    GRINGO_CLINGO_TRY { *m = static_cast<clingo_model*>(const_cast<Model*>(it->next())); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_solve_iteratively_get(clingo_solve_iteratively_t *it, clingo_solve_result_bitset_t *ret) {
    GRINGO_CLINGO_TRY { *ret = convert(it->get().satisfiable()); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_solve_iteratively_close(clingo_solve_iteratively_t *it) {
    GRINGO_CLINGO_TRY { it->close(); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 solve async

struct clingo_solve_async : SolveFuture { };

extern "C" clingo_error_t clingo_solve_async_cancel(clingo_solve_async_t *async) {
    GRINGO_CLINGO_TRY { async->cancel(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_solve_async_get(clingo_solve_async_t *async, clingo_solve_result_bitset_t *ret) {
    GRINGO_CLINGO_TRY { *ret = async->get(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_solve_async_wait(clingo_solve_async_t *async, double timeout, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = async->wait(timeout); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 configuration

struct clingo_configuration : ConfigProxy { };

extern "C" clingo_error_t clingo_configuration_type(clingo_configuration_t *conf, clingo_id_t key, clingo_configuration_type_bitset_t *ret) {
    GRINGO_CLINGO_TRY {
        int map_size, array_size, value_size;
        conf->getKeyInfo(key, &map_size, &array_size, nullptr, &value_size);
        *ret = 0;
        if (map_size >= 0)   { *ret |= clingo_configuration_type_map; }
        if (array_size >= 0) { *ret |= clingo_configuration_type_array; }
        if (value_size >= 0) { *ret |= clingo_configuration_type_value; }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_map_at(clingo_configuration_t *conf, clingo_id_t key, char const *name, clingo_id_t* subkey) {
    GRINGO_CLINGO_TRY { *subkey = conf->getSubKey(key, name); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_map_subkey_name(clingo_configuration_t *conf, clingo_id_t key, size_t index, char const **name) {
    GRINGO_CLINGO_TRY { *name = conf->getSubKeyName(key, index); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_map_size(clingo_configuration_t *conf, clingo_id_t key, size_t* ret) {
    GRINGO_CLINGO_TRY {
        int n;
        conf->getKeyInfo(key, &n, nullptr, nullptr, nullptr);
        if (n < 0) { throw std::runtime_error("not an array"); }
        *ret = n;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_array_at(clingo_configuration_t *conf, clingo_id_t key, size_t idx, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = conf->getArrKey(key, idx); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_array_size(clingo_configuration_t *conf, clingo_id_t key, size_t *ret) {
    GRINGO_CLINGO_TRY {
        int n;
        conf->getKeyInfo(key, nullptr, &n, nullptr, nullptr);
        if (n < 0) { throw std::runtime_error("not an array"); }
        *ret = n;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_root(clingo_configuration_t *conf, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = conf->getRootKey(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_description(clingo_configuration_t *conf, clingo_id_t key, char const **ret) {
    GRINGO_CLINGO_TRY {
        conf->getKeyInfo(key, nullptr, nullptr, ret, nullptr);
        if (!ret) { throw std::runtime_error("no description"); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_value_get_size(clingo_configuration_t *conf, clingo_id_t key, size_t *n) {
    GRINGO_CLINGO_TRY {
        std::string value;
        conf->getKeyValue(key, value);
        *n = value.size() + 1;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_value_get(clingo_configuration_t *conf, clingo_id_t key, char *ret, size_t n) {
    GRINGO_CLINGO_TRY {
        std::string value;
        conf->getKeyValue(key, value);
        if (n < value.size() + 1) { throw std::length_error("not enough space"); }
        std::strcpy(ret, value.c_str());
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_value_set(clingo_configuration_t *conf, clingo_id_t key, const char *val) {
    GRINGO_CLINGO_TRY { conf->setKeyValue(key, val); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_configuration_value_is_assigned(clingo_configuration_t *conf, clingo_id_t key, bool *ret) {
    GRINGO_CLINGO_TRY {
        int n = 0;
        conf->getKeyInfo(key, nullptr, nullptr, nullptr, &n);
        if (n < 0) { throw std::runtime_error("not a value"); }
        *ret = n > 0;
    }
    GRINGO_CLINGO_CATCH;
}

// {{{1 statistics

struct clingo_statistic : public StatisticsNG { };

extern "C" clingo_error_t clingo_statistics_root(clingo_statistics_t *stats, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->root(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_statistics_type(clingo_statistics_t *stats, clingo_id_t key, clingo_statistics_type_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->type(key); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_statistics_array_size(clingo_statistics_t *stats, clingo_id_t key, size_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->size(key); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_statistics_array_at(clingo_statistics_t *stats, clingo_id_t key, size_t index, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->at(key, index); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_statistics_map_size(clingo_statistics_t *stats, clingo_id_t key, size_t *n) {
    GRINGO_CLINGO_TRY { *n = stats->subkeys(key); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_statistics_map_subkey_name(clingo_statistics_t *stats, clingo_id_t key, size_t index, char const **name) {
    GRINGO_CLINGO_TRY { *name = stats->subkey(key, index); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_statistics_map_at(clingo_statistics_t *stats, clingo_id_t key, char const *name, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->lookup(key, name); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_statistics_value_get(clingo_statistics_t *stats, clingo_id_t key, double *value) {
    GRINGO_CLINGO_TRY { *value = stats->value(key); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 global functions

extern "C" clingo_error_t clingo_parse_term(char const *str, clingo_logger_t *logger, void *data, unsigned message_limit, clingo_symbol_t *ret) {
    GRINGO_CLINGO_TRY {
        Gringo::Input::GroundTermParser parser;
        Gringo::Logger::Printer printer;
        if (logger) {
            printer = [logger, data](clingo_warning_t code, char const *msg) { logger(code, msg, data); };
        }
        Gringo::Logger log(printer, message_limit);
        Symbol sym = parser.parse(str, log);
        if (sym.type() == SymbolType::Special) { throw std::runtime_error("parsing failed"); }
        *ret = sym.rep();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" void clingo_version(int *major, int *minor, int *revision) {
    *major = CLINGO_VERSION_MAJOR;
    *minor = CLINGO_VERSION_MINOR;
    *revision = CLINGO_VERSION_REVISION;
}

// {{{1 backend

struct clingo_backend : clingo_control_t { };

extern "C" clingo_error_t clingo_backend_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_n, clingo_literal_t const *body, size_t body_n) {
    GRINGO_CLINGO_TRY { outputRule(*backend->backend(), choice, {head, head_n}, {body, body_n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_backend_weight_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_n, clingo_weight_t lower, clingo_weighted_literal_t const *body, size_t body_n) {
    GRINGO_CLINGO_TRY { outputRule(*backend->backend(), choice, {head, head_n}, lower, {reinterpret_cast<Potassco::WeightLit_t const *>(body), body_n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_backend_minimize(clingo_backend_t *backend, clingo_weight_t prio, clingo_weighted_literal_t const* lits, size_t lits_n) {
    GRINGO_CLINGO_TRY { backend->backend()->minimize(prio, {reinterpret_cast<Potassco::WeightLit_t const *>(lits), lits_n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_backend_project(clingo_backend_t *backend, clingo_atom_t const *atoms, size_t n) {
    GRINGO_CLINGO_TRY { backend->backend()->project({atoms, n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_backend_external(clingo_backend_t *backend, clingo_atom_t atom, clingo_external_type_t v) {
    GRINGO_CLINGO_TRY { backend->backend()->external(atom, Potassco::Value_t(v)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_backend_assume(clingo_backend_t *backend, clingo_literal_t const *literals, size_t n) {
    GRINGO_CLINGO_TRY { backend->backend()->assume({literals, n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_backend_heuristic(clingo_backend_t *backend, clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority, clingo_literal_t const *condition, size_t condition_n) {
    GRINGO_CLINGO_TRY { backend->backend()->heuristic(atom, Potassco::Heuristic_t(type), bias, priority, {condition, condition_n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_backend_acyc_edge(clingo_backend_t *backend, int node_u, int node_v, clingo_literal_t const *condition, size_t condition_n) {
    GRINGO_CLINGO_TRY { backend->backend()->acycEdge(node_u, node_v, {condition, condition_n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_backend_add_atom(clingo_backend_t *backend, clingo_atom_t *ret) {
    GRINGO_CLINGO_TRY { *ret = backend->addProgramAtom(); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 control

extern "C" void clingo_control_free(clingo_control_t *ctl) {
    delete ctl;
}

extern "C" clingo_error_t clingo_control_add(clingo_control_t *ctl, char const *name, char const * const *params, size_t n, char const *part) {
    GRINGO_CLINGO_TRY {
        FWStringVec p;
        for (char const * const *it = params, * const *ie = it + n; it != ie; ++it) {
            p.emplace_back(*it);
        }
        ctl->add(name, p, part);
    }
    GRINGO_CLINGO_CATCH;
}

namespace {

struct ClingoContext : Context {
    ClingoContext(clingo_control_t *ctl, clingo_ground_callback_t *cb, void *data)
    : ctl(ctl)
    , cb(cb)
    , data(data) {}

    bool callable(String) const override {
        return cb;
    }

    SymVec call(Location const &loc, String name, SymSpan args) override {
        assert(cb);
        clingo_location_t loc_c{loc.beginFilename.c_str(), loc.endFilename.c_str(), loc.beginLine, loc.endLine, loc.beginColumn, loc.endColumn};
        auto err = cb(loc_c, name.c_str(), reinterpret_cast<clingo_symbol_t const *>(args.first), args.size, data, [](clingo_symbol_t const * ret_c, size_t n, void *data) -> clingo_error_t {
            auto t = static_cast<ClingoContext*>(data);
            GRINGO_CLINGO_TRY {
                for (auto it = ret_c, ie = it + n; it != ie; ++it) {
                    t->ret.emplace_back(Symbol(*it));
                }
            } GRINGO_CLINGO_CATCH;
        }, static_cast<void*>(this));
        if (err != 0) { throw ClingoError(err); }
        return std::move(ret);
    }
    virtual ~ClingoContext() noexcept = default;

    clingo_control_t *ctl;
    clingo_ground_callback_t *cb;
    void *data;
    SymVec ret;
};

}

extern "C" clingo_error_t clingo_control_ground(clingo_control_t *ctl, clingo_part_t const * vec, size_t n, clingo_ground_callback_t *cb, void *data) {
    GRINGO_CLINGO_TRY {
        Control::GroundVec gv;
        gv.reserve(n);
        for (auto it = vec, ie = it + n; it != ie; ++it) {
            SymVec params;
            params.reserve(it->size);
            for (auto jt = it->params, je = jt + it->size; jt != je; ++jt) {
                params.emplace_back(Symbol(*jt));
            }
            gv.emplace_back(it->name, params);
        }
        ClingoContext cctx(ctl, cb, data);
        ctl->ground(gv, cb ? &cctx : nullptr);
    } GRINGO_CLINGO_CATCH;
}

namespace {

Control::Assumptions toAss(clingo_symbolic_literal_t const * assumptions, size_t n) {
    Control::Assumptions ass;
    for (auto it = assumptions, ie = it + n; it != ie; ++it) {
        ass.emplace_back(static_cast<Symbol const>(it->atom), !it->sign);
    }
    return ass;
}

}

extern "C" clingo_error_t clingo_control_solve(clingo_control_t *ctl, clingo_model_callback_t *model_handler, void *data, clingo_symbolic_literal_t const *assumptions, size_t n, clingo_solve_result_bitset_t *ret) {
    GRINGO_CLINGO_TRY {
        *ret = static_cast<clingo_solve_result_bitset_t>(ctl->solve([model_handler, data](Model const &m) {
            bool ret;
            auto err = model_handler(static_cast<clingo_model*>(const_cast<Model*>(&m)), data, &ret);
            if (err != 0) { throw ClingoError(err); }
            return ret;
        }, toAss(assumptions, n)));
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_solve_iteratively(clingo_control_t *ctl, clingo_symbolic_literal_t const *assumptions, size_t n, clingo_solve_iteratively_t **it) {
    GRINGO_CLINGO_TRY { *it = static_cast<clingo_solve_iteratively_t*>(ctl->solveIter(toAss(assumptions, n))); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_assign_external(clingo_control_t *ctl, clingo_symbol_t atom, clingo_truth_value_t value) {
    GRINGO_CLINGO_TRY { ctl->assignExternal(Symbol(atom), static_cast<Potassco::Value_t>(value)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_release_external(clingo_control_t *ctl, clingo_symbol_t atom) {
    GRINGO_CLINGO_TRY { ctl->assignExternal(Symbol(atom), Potassco::Value_t::Release); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_parse(clingo_control_t *ctl, char const *program, clingo_ast_callback_t *cb, void *data) {
    GRINGO_CLINGO_TRY {
        ctl->parse(program, [data, cb](clingo_ast const &ast) {
            auto ret = cb(&ast, data);
            if (ret != 0) { throw ClingoError(ret); }
        });
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_add_ast(clingo_control_t *ctl, clingo_add_ast_callback_t *cb, void *data) {
    GRINGO_CLINGO_TRY {
        ctl->add([ctl, data, cb](std::function<void (clingo_ast const &)> f) {
            auto ref = std::make_pair(f, ctl);
            using RefType = decltype(ref);
            auto ret = cb(data, [](clingo_ast_t const *ast, void *data) -> clingo_error_t {
                auto &ref = *static_cast<RefType*>(data);
                GRINGO_CLINGO_TRY {
                    ref.first(static_cast<clingo_ast const &>(*ast));
                } GRINGO_CLINGO_CATCH;
            }, static_cast<void*>(&ref));
            if (ret != 0) { throw ClingoError(ret); }
        });
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_symbolic_atoms(clingo_control_t *ctl, clingo_symbolic_atoms_t **ret) {
    GRINGO_CLINGO_TRY { *ret = &ctl->getDomain(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_theory_atoms(clingo_control_t *ctl, clingo_theory_atoms_t **ret) {
    GRINGO_CLINGO_TRY { *ret = const_cast<Gringo::TheoryData*>(&ctl->theory()); }
    GRINGO_CLINGO_CATCH;
}

namespace {

class ClingoPropagator : public Gringo::Propagator {
public:
    ClingoPropagator(clingo_propagator_t prop, void *data)
    : prop_(prop)
    , data_(data) { }
    void init(Gringo::PropagateInit &init) override {
        auto ret = prop_.init(&init, data_);
        if (ret != 0) { throw ClingoError(ret); }
    }

    void propagate(Potassco::AbstractSolver& solver, const ChangeList& changes) override {
        auto ret = prop_.propagate(static_cast<clingo_propagate_control_t*>(&solver), changes.first, changes.size, data_);
        if (ret != 0) { throw ClingoError(ret); }
    }

    void undo(const Potassco::AbstractSolver& solver, const ChangeList& undo) override {
        auto ret = prop_.undo(static_cast<clingo_propagate_control_t*>(&const_cast<Potassco::AbstractSolver&>(solver)), undo.first, undo.size, data_);
        if (ret != 0) { throw ClingoError(ret); }
    }

    void check(Potassco::AbstractSolver& solver) override {
        auto ret = prop_.check(static_cast<clingo_propagate_control_t*>(&solver), data_);
        if (ret != 0) { throw ClingoError(ret); }
    }
private:
    clingo_propagator_t prop_;
    void *data_;
};

} // namespace

extern "C" clingo_error_t clingo_control_register_propagator(clingo_control_t *ctl, clingo_propagator_t propagator, void *data, bool sequential) {
    GRINGO_CLINGO_TRY { ctl->registerPropagator(gringo_make_unique<ClingoPropagator>(propagator, data), sequential); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_cleanup(clingo_control_t *ctl) {
    GRINGO_CLINGO_TRY { ctl->cleanupDomains(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_has_const(clingo_control_t *ctl, char const *name, bool *ret) {
    GRINGO_CLINGO_TRY {
        auto sym = ctl->getConst(name);
        *ret = sym.type() != SymbolType::Special;

    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_get_const(clingo_control_t *ctl, char const *name, clingo_symbol_t *ret) {
    GRINGO_CLINGO_TRY {
        auto sym = ctl->getConst(name);
        *ret = sym.type() != SymbolType::Special ? sym.rep() : Symbol::createId(name).rep();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" void clingo_control_interrupt(clingo_control_t *ctl) {
    ctl->interrupt();
}

extern "C" clingo_error_t clingo_control_load(clingo_control_t *ctl, char const *file) {
    GRINGO_CLINGO_TRY { ctl->load(file); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_solve_async(clingo_control_t *ctl, clingo_model_callback_t *mh, void *mh_data, clingo_finish_callback_t *fh, void *fh_data, clingo_symbolic_literal_t const * assumptions, size_t n, clingo_solve_async_t **ret) {
    GRINGO_CLINGO_TRY {
        clingo_control::Assumptions ass;
        for (auto it = assumptions, ie = assumptions + n; it != ie; ++it) {
            ass.emplace_back(Symbol(it->atom), it->sign);
        }
        *ret = static_cast<clingo_solve_async_t*>(ctl->solveAsync(
            [mh, mh_data](Gringo::Model const &m) {
                bool result;
                auto code = mh(&const_cast<Gringo::Model &>(m), mh_data, &result);
                if (code != 0) { throw ClingoError(code); }
                return result;
            }, [fh, fh_data](Gringo::SolveResult ret) {
                auto code = fh(ret, fh_data);
                if (code != 0) { throw ClingoError(code); }
            }, std::move(ass)));
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_use_enum_assumption(clingo_control_t *ctl, bool value) {
    GRINGO_CLINGO_TRY { ctl->useEnumAssumption(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_backend(clingo_control_t *ctl, clingo_backend_t **ret) {
    GRINGO_CLINGO_TRY {
        if (ctl->backend()) { *ret = static_cast<clingo_backend_t*>(ctl); }
        else { throw std::runtime_error("backend not available"); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_configuration(clingo_control_t *ctl, clingo_configuration_t **conf) {
    GRINGO_CLINGO_TRY { *conf = static_cast<clingo_configuration_t*>(&ctl->getConf()); }
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_statistics(clingo_control_t *ctl, clingo_statistics_t **stats) {
    GRINGO_CLINGO_TRY { *stats = static_cast<clingo_statistics_t*>(ctl->statistics()); }
    GRINGO_CLINGO_CATCH;
}

// }}}1

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

