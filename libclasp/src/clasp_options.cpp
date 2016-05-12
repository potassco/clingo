//
// Copyright (c) 2006-2015, Benjamin Kaufmann
//
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/
//
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#include <clasp/cli/clasp_options.h>
#include <clasp/minimize_constraint.h>
#include <clasp/lookahead.h>
#include <clasp/unfounded_check.h>
#include <program_opts/program_options.h>
#include <program_opts/typed_value.h>
#include <cstring>
#include <cstdarg>
#include <cfloat>
#include <fstream>
#include <cctype>
#ifdef _MSC_VER
#pragma warning (disable : 4996)
#endif
/////////////////////////////////////////////////////////////////////////////////////////
// Helper MACROS
/////////////////////////////////////////////////////////////////////////////////////////
#define SET(x, v)           ( ((x)=(v)) == (v) )
#define SET_LEQ(x, v, m)    ( ((v)<=(m)) && SET((x), (v)) )
#define SET_GEQ(x, v, m)    ( ((v)>=(m)) && SET((x), (v)) )
#define SET_OR_FILL(x, v)   ( SET((x),(v)) || ((x) = 0, (x) = ~(x),true) )
#define SET_OR_ZERO(x,v)    ( SET((x),(v)) || SET((x),uint32(0)) )
#define SET_R(x, v, lo, hi) ( ((lo)<=(v)) && ((v)<=(hi)) && SET((x), (v)) )
/////////////////////////////////////////////////////////////////////////////////////////
// Primitive types/functions for string <-> T conversions
/////////////////////////////////////////////////////////////////////////////////////////
namespace bk_lib {
static const struct OffType {} off = {};
static int xconvert(const char* x, const OffType&, const char** errPos, int) {
	bool temp = true;
	const char* n = x;
	if (xconvert(n, temp, &n, 0) && !temp) { x = n; }
	if (errPos) { *errPos = x; }
	return int(temp == false);
}
static std::string& xconvert(std::string& out, const OffType&) { return out.append("no"); }

template <class T>
static int xconvert(const char* x, pod_vector<T>& out, const char** errPos, int sep) {
	if (sep == 0) { sep = def_sep; }
	std::size_t sz = out.size();
	std::size_t t  = convert_seq<T>(x, out.max_size() - sz, std::back_inserter(out), static_cast<char>(sep), errPos);
	if (!t) { out.resize(sz); }
	return static_cast<int>(t);
}
template <class T>
static std::string& xconvert(std::string& out, const pod_vector<T>& x) { return xconvert(out, x.begin(), x.end()); }

static std::size_t findEnumValImpl(const char* value, int& out, const char* k1, int v1, va_list args) {
	std::size_t kLen = std::strlen(k1);
	std::size_t vLen = std::strlen(value);
	if (const char* x = std::strchr(value, ',')) {
		vLen = x - value;
	}
	if (vLen == kLen && strncasecmp(value, k1, kLen) == 0) { out = v1; return kLen; }
	while (const char* key = va_arg(args, const char *)) {
		int val = va_arg(args, int);
		kLen    = std::strlen(key);
		if (vLen == kLen && strncasecmp(value, key, kLen) == 0) { out = val; return kLen; }
	}
	return 0;
}
template <class T>
static int findEnumVal(const char* value, T& out, const char** errPos, const char* k1, int v1, ...) {
	va_list args;
	va_start(args, v1);
	int temp;
	std::size_t p = bk_lib::findEnumValImpl(value, temp, k1, v1, args);
	va_end(args);
	if (errPos) { *errPos = value + p; }
	if (p)      { out = static_cast<T>(temp); }
	return p != 0;
}
static const char* enumToString(int x, const char* k1, int v1, ...) {
	va_list args;
	va_start(args, v1);
	const char* res = x == v1 ? k1 : 0;
	if (res == 0) {
		while (const char* key = va_arg(args, const char *)) {
			int val = va_arg(args, int);
			if (x == val) { res = key; break; }
		}
	}
	va_end(args);
	return res ? res : "";
}
struct ArgString {
	ArgString(const char* x) : in(x) { }
	~ArgString() { CLASP_FAIL_IF(ok() && *in && !off(), "Unused argument!"); }
	bool ok()       const { return in != 0; }
	bool off()      const { return ok() && stringTo(in, bk_lib::off); }
	bool empty()    const { return ok() && !*in; }
	operator void*()const { return (void*)in; }
	char peek()     const { return ok() ? *in : 0; }
	template <class T>
	ArgString& get(T& x)  {
		if (ok()) {
			const char* next = in + (*in == ',');
			in = xconvert(next, x, &next, 0) != 0 ? next : 0;
		}
		return *this;
	}
	const char* in;
	template <class T>
	struct Opt_t {
		Opt_t(T& x) : obj(&x) {}
		T* obj;
	};
};
template <class T>
inline ArgString::Opt_t<T> opt(T& x) { return ArgString::Opt_t<T>(x); }
template <class T>
inline ArgString& operator>>(ArgString& arg, T& x) { return arg.get(x); }
template <class T>
inline ArgString& operator>>(ArgString& arg, const ArgString::Opt_t<T>& x) { return !arg.empty() ? arg.get(*x.obj) : arg; }

struct StringBuilder {
	StringBuilder(std::string& o) : out(&o) {}
	template <class T>
	StringBuilder& _add(const T& x) {
		if (!out->empty()) { out->append(1, ','); }
		xconvert(*out, x);
		return *this;
	}
	operator bool() const { return out != 0; }
	std::string* out;
};
template <class T>
inline StringBuilder& operator<<(StringBuilder& str, const T& val) { return str ? str._add(val) : str; }

}
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// Enum mappings for clasp types
/////////////////////////////////////////////////////////////////////////////////////////
#define MAP(x, y) static_cast<const char*>(x), static_cast<int>(y)
#define DEFINE_ENUM_MAPPING(X, ...) \
static int xconvert(const char* x, X& out, const char** errPos, int) {\
	return bk_lib::findEnumVal(x, out, errPos, __VA_ARGS__, MAP(0,0)); \
}\
static std::string& xconvert(std::string& out, X x) { \
	return out.append(bk_lib::enumToString(static_cast<int>(x), __VA_ARGS__, MAP(0,0))); \
}
#define OPTION(k, e, a, d, ...) a
#define CLASP_CONTEXT_OPTIONS
#define CLASP_SOLVE_OPTIONS
#define CLASP_ASP_OPTIONS
#define CLASP_SOLVER_OPTIONS
#define CLASP_SEARCH_OPTIONS
#define COMBINE_2(a, b) a b
#define ARG(a, ...) __VA_ARGS__
#define NO_ARG
#include <clasp/cli/clasp_cli_options.inl>
namespace Cli {
DEFINE_ENUM_MAPPING(ConfigKey, \
  MAP("auto",   config_default), MAP("frumpy", config_frumpy), MAP("jumpy",  config_jumpy), \
  MAP("tweety", config_tweety) , MAP("handy" , config_handy) ,\
  MAP("crafty", config_crafty) , MAP("trendy", config_trendy), MAP("many", config_many));
}
#undef MAP
#undef DEFINE_ENUM_MAPPING
#undef COMBINE_2
/////////////////////////////////////////////////////////////////////////////////////////
// Conversion functions for complex clasp types
/////////////////////////////////////////////////////////////////////////////////////////
static int xconvert(const char* x, ScheduleStrategy& out, const char** errPos, int e) {
	using bk_lib::xconvert;
	if (!x) { return 0; }
	const char* next = std::strchr(x, ',');
	uint32      base = 0;
	int         tok  = 1;
	if (errPos) { *errPos = x; }
	if (!next || !xconvert(next+1, base, &next, e) || base == 0)         { return 0; }
	if (strncasecmp(x, "f,", 2) == 0 || strncasecmp(x, "fixed,", 6) == 0){
		out = ScheduleStrategy::fixed(base);
	}
	else if (strncasecmp(x, "l,", 2) == 0 || strncasecmp(x, "luby,", 5) == 0) {
		uint32 lim = 0;
		if (*next == ',' && !xconvert(next+1, lim, &next, e)) { return 0; }
		out = ScheduleStrategy::luby(base, lim);
	}
	else if (strncmp(x, "+,", 2) == 0 || strncasecmp(x, "add,", 4) == 0) {
		std::pair<uint32, uint32> arg(0, 0);
		if (*next != ',' || !xconvert(next+1, arg, &next, e)) { return 0; }
		out = ScheduleStrategy::arith(base, arg.first, arg.second);
	}
	else if (strncmp(x, "x,", 2) == 0 || strncmp(x, "*,", 2) == 0 || strncasecmp(x, "d,", 2) == 0) {
		std::pair<double, uint32> arg(0, 0);
		if (*next != ',' || !xconvert(next+1, arg, &next, e)) { return 0; }
		if      (strncasecmp(x, "d", 1) == 0 && arg.first > 0.0) { out = ScheduleStrategy(ScheduleStrategy::User, base, arg.first, arg.second); }
		else if (strncasecmp(x, "d", 1) != 0 && arg.first >= 1.0){ out = ScheduleStrategy::geom(base, arg.first, arg.second); }
		else { return 0; }
	}
	else { next = x; tok = 0; }
	if (errPos) { *errPos = next; }
	return tok;
}
static std::string& xconvert(std::string& out, const ScheduleStrategy& sched) {
	using bk_lib::xconvert;
	if (sched.defaulted()){ return xconvert(out, ScheduleStrategy()); }
	if (sched.disabled()) { return out.append("0"); }
	std::size_t t = out.size();
	out.append("f,");
	xconvert(out, sched.base);
	switch (sched.type) {
		case ScheduleStrategy::Geometric:
			out[t] = 'x';
			return xconvert(out.append(1, ','), std::make_pair((double)sched.grow, sched.len));
		case ScheduleStrategy::Arithmetic:
			if (sched.grow) { out[t] = '+'; return xconvert(out.append(1, ','), std::make_pair((uint32)sched.grow, sched.len)); }
			else            { out[t] = 'f'; return out; }
		case ScheduleStrategy::Luby:
			out[t] = 'l';
			if (sched.len) { return xconvert(out.append(1, ','), sched.len); }
			else           { return out; }
		case ScheduleStrategy::User:
			out[t] = 'd';
			return xconvert(out.append(1, ','), std::make_pair((double)sched.grow, sched.len));
		default: CLASP_FAIL_IF(true, "xconvert(ScheduleStrategy): unknown type");
	}
}
namespace Asp { using Clasp::xconvert; }
namespace mt  { using Clasp::xconvert; }
namespace Cli {
/////////////////////////////////////////////////////////////////////////////////////////
// Option -> Key mapping
/////////////////////////////////////////////////////////////////////////////////////////
// Valid option keys.
enum OptionKey {
	detail__before_options = -1,
	meta_config = 0,
#define CLASP_CONTEXT_OPTIONS  GRP(option_category_nodes_end,   option_category_context_begin),
#define CLASP_SOLVER_OPTIONS   GRP(option_category_context_end, option_category_solver_begin), 
#define CLASP_SEARCH_OPTIONS   GRP(option_category_solver_end,  option_category_search_begin),
#define CLASP_ASP_OPTIONS      GRP(option_category_search_end,  option_category_asp_begin),
#define CLASP_SOLVE_OPTIONS    GRP(option_category_asp_end,     option_category_solve_begin),
#define OPTION(k,e,...) opt_##k,
#define GROUP_BEGIN(X) X
#define GRP(X, Y) X, Y = X, detail__before_##Y = X - 1
#include <clasp/cli/clasp_cli_options.inl>
#undef GRP
	option_category_solve_end, 
	detail__num_options = option_category_solve_end,
	meta_tester = detail__num_options
};
static inline bool isOption(int k)       { return k >= option_category_nodes_end && k < detail__num_options; }
static inline bool isTesterOption(int k) { return k >= option_category_nodes_end && k < option_category_search_end; }
static inline bool isSolverOption(int k) { return k >= option_category_solver_begin && k < option_category_search_end; }
#if WITH_THREADS
#define MANY_DESC  "        many  : Use default portfolio to configure solver(s)\n"
#define MANY_ARG   "|many"
#else
#define MANY_DESC
#define MANY_ARG   ""
#endif
#define KEY_INIT_DESC(desc) \
desc "      <arg>: {auto|frumpy|jumpy|tweety|handy|crafty|trendy" MANY_ARG "|<file>}\n" \
"        auto  : Select configuration based on problem type\n"                          \
"        frumpy: Use conservative defaults\n"                                           \
"        jumpy : Use aggressive defaults\n"                                             \
"        tweety: Use defaults geared towards asp problems\n"                            \
"        handy : Use defaults geared towards large problems\n"                          \
"        crafty: Use defaults geared towards crafted problems\n"                        \
"        trendy: Use defaults geared towards industrial problems\n"                     \
         MANY_DESC                                                                      \
"        <file>: Use configuration file to configure solver(s)"

struct NodeKey {
	const char* name;
	const char* desc;
	int16       skBegin;
	int16       skEnd;
	uint32      numSubkeys() const { return static_cast<uint32>( skEnd - skBegin ); }
};
enum { key_leaf = 0, key_solver = -1, key_asp = -2, key_solve = -3, key_tester = -4, key_root = -5 };
// nodes_g[-k]: entry for key k
static const NodeKey nodes_g[] = {
/* 0: config */ {"configuration", KEY_INIT_DESC("Initializes this configuration\n"), 0,0},
/* 1: */ {"solver.", "Solver Options", option_category_solver_begin, option_category_search_end},
/* 2: */ {"asp.", "Asp Options", option_category_asp_begin, option_category_asp_end},
/* 3: */ {"solve.", "Solve Options", option_category_solve_begin, option_category_solve_end},
/* 4: */ {"tester.", "Tester Options", key_solver  , option_category_context_end},
/* 5: */ {".", "Options", key_tester, option_category_context_end}
};
static uint32 makeKeyHandle(int16 kId, uint32 mode, uint32 sId) {
	assert(sId <= 255 && mode <= 255);
	return (mode << 24) | (sId << 16) | static_cast<uint16>(kId);
}
static int16 decodeKey(uint32 key)   { return static_cast<int16>(static_cast<uint16>(key)); }
static uint8 decodeMode(uint32 key)  { return static_cast<uint8>( (key >> 24) ); }
static uint8 decodeSolver(uint32 key){ return static_cast<uint8>( (key >> 16) ); }
static bool  isValidId(int16 id)     { return id >= key_root && id < detail__num_options; }
static bool  isLeafId(int16 id)      { return id >= key_leaf && id < detail__num_options; }
const ClaspCliConfig::KeyType ClaspCliConfig::KEY_INVALID = static_cast<ClaspCliConfig::KeyType>(-1);
const ClaspCliConfig::KeyType ClaspCliConfig::KEY_ROOT    = makeKeyHandle(key_root, 0, 0);
const ClaspCliConfig::KeyType ClaspCliConfig::KEY_SOLVER  = makeKeyHandle(key_solver, 0, 0);
const ClaspCliConfig::KeyType ClaspCliConfig::KEY_TESTER  = makeKeyHandle(key_tester, ClaspCliConfig::mode_tester, 0);

struct Name2Id { 
	const char* name; int key;
	bool operator<(const Name2Id& rhs) const { return std::strcmp(name, rhs.name) < 0; }
};
static Name2Id options_g[detail__num_options+1] = {
	{"configuration", meta_config},
#define OPTION(k, e, ...) { #k, opt_##k }, 
#define CLASP_CONTEXT_OPTIONS
#define CLASP_SOLVER_OPTIONS
#define CLASP_SEARCH_OPTIONS
#define CLASP_ASP_OPTIONS     
#define CLASP_SOLVE_OPTIONS   
#include <clasp/cli/clasp_cli_options.inl>
	{"tester"       , meta_tester}
};
struct ClaspCliConfig::OptIndex {
	OptIndex(Name2Id* first, Name2Id* last) { 
		std::sort(begin = first, end = last); 
	}
	Name2Id* find(const char* name) const {
		Name2Id temp = { name, 0 };
		Name2Id* it  = std::lower_bound(begin, end, temp);
		if (it != end && std::strcmp(it->name, name) == 0) { return it; }
		return 0;
	}
	Name2Id* begin;
	Name2Id* end;
};
ClaspCliConfig::OptIndex ClaspCliConfig::index_g(options_g, options_g + detail__num_options+1);
/////////////////////////////////////////////////////////////////////////////////////////
// Interface to ProgramOptions
/////////////////////////////////////////////////////////////////////////////////////////
// Converts option key to command-line option name.
static void keyToCliName(std::string& out, const char* n, const char* ext) {
	out.clear();
	for (const char* x; (x = std::strchr(n, '_')) != 0; n = ++x) {
		out.append(n, x-n);
		out.append(1, '-');
	}
	out.append(n).append(ext);
}
// Converts command-line option name to option key.
static void cliNameToKey(std::string& out, const char* n) {
	out.clear();
	for (const char* x; (x = std::strchr(n, '-')) != 0; n = ++x) {
		out.append(n, x-n);
		out.append(1, '_');
	}
	out.append(n);
}
// Type for storing one command-line option.
class ClaspCliConfig::ProgOption : public ProgramOptions::Value {
public:
	ProgOption(ClaspCliConfig& c, int o) : ProgramOptions::Value(0), config_(&c), option_(o) {}
	bool doParse(const std::string& opt, const std::string& value) {
		int ret = isOption(option_) ? config_->setActive(option_, value.c_str()) : config_->setAppOpt(option_, value.c_str());
		if (ret == -1) { throw ProgramOptions::UnknownOption(config_->isGenerator() ? "<clasp>" : "<tester>", opt); }
		return ret > 0;
	}
	int option() const { return option_; }
private:
	ClaspCliConfig* config_;
	int             option_;
};

// Adapter for parsing a command string.
struct ClaspCliConfig::ParseContext : public ProgramOptions::ParseContext {
	typedef ProgramOptions::SharedOptPtr OptPtr;
	ParseContext(ClaspCliConfig& x, const char* c, const ParsedOpts* ex, bool allowMeta, ParsedOpts* o) 
		: self(&x), config(c), exclude(ex), out(o), meta(allowMeta) { seen[0] = seen[1] = 0;  }
	OptPtr getOption(const char* name, FindType ft);
	OptPtr getOption(int, const char* key) { throw ProgramOptions::UnknownOption(config, key); }
	void   addValue(const OptPtr& key, const std::string& value);
	uint64            seen[2];
	std::string       temp;
	ClaspCliConfig*   self;
	const char*       config;
	const ParsedOpts* exclude;
	ParsedOpts*       out;
	bool              meta;
};
void ClaspCliConfig::ParseContext::addValue(const OptPtr& key, const std::string& value) {
	using namespace ProgramOptions;
	if (exclude->count(key->name()) == 0) {
		ProgOption* v = static_cast<ProgOption*>(key->value());
		Value::State s= v->state();
		int        id = v->option();
		uint64&    xs = seen[id/64];
		uint64      m = static_cast<uint64>(1u) << (id & 63);
		if ((xs & m) != 0 && !v->isComposing()){ throw ValueError(config, ValueError::multiple_occurences, key->name(), value); }
		if (!v->parse(key->name(), value, s))  { throw ValueError(config, ValueError::invalid_value, key->name(), value); }
		if (out) { out->add(key->name()); }
		xs |= m;
	}
}
ProgramOptions::SharedOptPtr ClaspCliConfig::ParseContext::getOption(const char* cmdName, FindType ft) {
	Options::option_iterator end = self->opts_->end(), it = end;
	if (ft == OptionContext::find_alias) { 
		char a = cmdName[*cmdName == '-'];
		for (it = self->opts_->begin(); it != end && it->get()->alias() != a; ++it) { ; }
	}
	else {
		Name2Id  key = { cmdName, -2 };
		if (std::strchr(cmdName, '-') != 0) { cliNameToKey(temp, cmdName); key.name = temp.c_str(); }
		Name2Id* pos = std::lower_bound(self->index_g.begin, self->index_g.end, key);
		if (pos != self->index_g.end) {
			std::size_t len = std::strlen(key.name);
			int  cmp        = std::strncmp(key.name, pos->name, len);
			bool found      = cmp == 0 && !*(pos->name+len);
			if (!found && cmp == 0 && (ft & OptionContext::find_prefix) != 0) {
				Name2Id* next = pos + 1;
				cmp           = next != self->index_g.end ? std::strncmp(key.name, next->name, len) : -1;
				found         = cmp != 0;
				if (!found) { throw ProgramOptions::AmbiguousOption(config, cmdName, ""); }
			}
			if (found) { it = self->opts_->begin() + pos->key; }
		}
		assert(it == end || static_cast<const ProgOption*>(it->get()->value())->option() == pos->key);
	}
	if (it != end && (meta || isOption(static_cast<const ProgOption*>(it->get()->value())->option()))) { 
		return *it;
	}
	throw ProgramOptions::UnknownOption(config, cmdName); 
}
/////////////////////////////////////////////////////////////////////////////////////////
// Default Configs
/////////////////////////////////////////////////////////////////////////////////////////
ConfigIter ClaspCliConfig::getConfig(ConfigKey k) {
	switch(k) {
		#define CONFIG(id, n,c,s,p) case config_##n: return ConfigIter("/[" #n "]\0/\0/" s " " c "\0");
		#define CLASP_CLI_DEFAULT_CONFIGS
		#define CLASP_CLI_AUX_CONFIGS
		#include <clasp/cli/clasp_cli_configs.inl>
		case config_many:
		#define CONFIG(id,n,c,s,p) "/[solver." #id "]\0/\0/" c " " p "\0"
		#define CLASP_CLI_DEFAULT_CONFIGS
		#define CLASP_CLI_AUX_CONFIGS
			return ConfigIter(
				#include <clasp/cli/clasp_cli_configs.inl>
				);
		case config_default: return ConfigIter("/default\0/\0/\0");
		default            : throw std::logic_error(clasp_format_error("Invalid config key '%d'", (int)k));
	}
}
ConfigIter ClaspCliConfig::getConfig(uint8 key, std::string& tempMem) {
	CLASP_FAIL_IF(key > (config_max_value + 1), "Invalid key!");
	if (key < config_max_value) { return getConfig(static_cast<ConfigKey>(key)); }
	tempMem.clear();
	loadConfig(tempMem, config_[key - config_max_value].c_str());
	return ConfigIter(tempMem.data());
}
static inline const char* skipWs(const char* x) {
	while (*x == ' ' || *x == '\t') { ++x; }
	return x;
}
static inline const char* getIdent(const char* x, std::string& to) {
	for (x = skipWs(x); std::strchr(" \t:()[]", *x) == 0; ++x) { to += *x; }
	return x;
}
static inline bool matchSep(const char*& x, char c) {
	if (*(x = skipWs(x)) == c) { ++x; return true; }
	return false;
}

bool ClaspCliConfig::appendConfig(std::string& to, const std::string& line) {
	std::size_t sz = to.size();
	const char*  x = skipWs(line.c_str());
	const bool   p = matchSep(x, '[');
	to.append("/[", 2);
	// match name in optional square brackets
	bool ok = matchSep(x = getIdent(x, to), ']') == p;
	to.append("]\0/", 3);
	// match optional base in parentheses followed by start of option list
	if (ok && (!matchSep(x, '(') || matchSep((x = getIdent(x, to)), ')')) && matchSep(x, ':')) {
		to.append("\0/", 2);
		to.append(skipWs(x));
		to.erase(to.find_last_not_of(" \t") + 1);
		to.append(1, '\0');
		return true;
	}
	to.resize(sz);
	return false;
}
bool ClaspCliConfig::loadConfig(std::string& to, const char* name) {
	std::ifstream file(name);
	CLASP_FAIL_IF(!file, "Could not open config file '%s'", name);
	uint32 lineNum= 0;
	for (std::string line, cont; std::getline(file, line); ) {
		++lineNum;
		line.erase(0, line.find_first_not_of(" \t"));
		if (line.empty() || line[0] == '#') { continue; }
		if (*line.rbegin() == '\\')         { *line.rbegin() = ' '; cont += line; continue; }
		if (!cont.empty()) { cont += line; cont.swap(line); cont.clear(); }
		CLASP_FAIL_IF(!appendConfig(to, line),  "'%s@%u': Invalid configuration", name, lineNum);
	}
	to.append(1, '\0');
	return true;
}
const char* ClaspCliConfig::getDefaults(ProblemType t) {
	if (t == Problem_t::Asp){ return "--configuration=tweety"; }
	else                    { return "--configuration=trendy"; }
}
ConfigIter::ConfigIter(const char* x) : base_(x) {}
const char* ConfigIter::name() const { return base_ + 1; }
const char* ConfigIter::base() const { return base_ + std::strlen(base_) + 2; }
const char* ConfigIter::args() const { const char* x = base(); return x + std::strlen(x) + 2; }
bool        ConfigIter::valid()const { return *base_ != 0; }
bool        ConfigIter::next()       {
	base_ = args();
	base_+= std::strlen(base_) + 1;
	return valid();
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspCliConfig
/////////////////////////////////////////////////////////////////////////////////////////
ClaspCliConfig::ScopedSet::ScopedSet(ClaspCliConfig& s, uint8 mode, uint32 sId) : self(&s) {
	if (sId) { mode |= mode_solver; }
	s.cliId   = static_cast<uint8>(sId);
	s.cliMode = mode;
}
ClaspCliConfig::ScopedSet::~ScopedSet() { self->cliId = self->cliMode = 0; }
ClaspCliConfig::RawConfig::RawConfig(const char* name) {
	raw.append(1, '/').append(name ? name : "").append("\0/\0/", 4);
}
void ClaspCliConfig::RawConfig::addArg(const char* arg) {
	*raw.rbegin() = ' ';
	raw.append(arg ? arg : "").append(1, '\0');
}
void ClaspCliConfig::RawConfig::addArg(const std::string& arg) { addArg(arg.c_str()); }
ClaspCliConfig::ClaspCliConfig()  {
	static_assert(
		(option_category_context_begin< option_category_solver_begin) &&
		(option_category_solver_begin < option_category_search_begin) &&
		(option_category_search_begin < option_category_asp_begin)    &&
		(option_category_asp_begin    < option_category_solve_begin)  &&
		(option_category_solve_begin  < option_category_solve_end), "unexpected option order");
}
ClaspCliConfig::~ClaspCliConfig() {}
void ClaspCliConfig::reset() {
	config_[0] = config_[1] = "";
	ClaspConfig::reset();
}

void ClaspCliConfig::prepare(SharedContext& ctx) {
	ClaspConfig::prepare(ctx);
}
Configuration* ClaspCliConfig::config(const char* n) {
	if (n && std::strcmp(n, "tester") == 0) {
		if (!testerConfig()) {
			setAppOpt(meta_tester, "--config=auto");
		}
		return testerConfig();
	}
	return ClaspConfig::config(n);
}

ClaspCliConfig::ProgOption* ClaspCliConfig::createOption(int o) {  return new ProgOption(*this, o); }

void ClaspCliConfig::createOptions() {
	if (opts_.get()) { return; }
	opts_ = new Options();
	using namespace ProgramOptions;
	opts_->addOptions()("configuration", createOption(meta_config)->defaultsTo("auto")->state(Value::value_defaulted), KEY_INIT_DESC("Configure default configuration [%D]\n"));
	std::string cmdName;
#define CLASP_CONTEXT_OPTIONS
#define CLASP_SOLVE_OPTIONS
#define CLASP_ASP_OPTIONS
#define CLASP_SOLVER_OPTIONS
#define CLASP_SEARCH_OPTIONS
#define OPTION(k, e, a, d, ...) keyToCliName(cmdName, #k, e); opts_->addOptions()(cmdName.c_str(),static_cast<ProgOption*>( createOption(opt_##k)a ), d);
#define ARG(a, ...) ->a
#define NO_ARG
#include <clasp/cli/clasp_cli_options.inl>
	opts_->addOptions()("tester", createOption(meta_tester)->arg("<options>"), "Pass (quoted) string of %A to tester");
}
void ClaspCliConfig::addOptions(OptionContext& root) {
	createOptions();
	using namespace ProgramOptions;
	OptionGroup configOpts("Clasp.Config Options");
	OptionGroup solving("Clasp.Solving Options");
	OptionGroup asp("Clasp.ASP Options");
	OptionGroup search("Clasp.Search Options", ProgramOptions::desc_level_e1);
	OptionGroup lookback("Clasp.Lookback Options", ProgramOptions::desc_level_e1);
	configOpts.addOption(*opts_->begin());
	configOpts.addOption(*(opts_->end()-1));
	for (Options::option_iterator it = opts_->begin() + 1, end = opts_->end() - 1; it != end; ++it) {
		int oId = static_cast<ProgOption*>(it->get()->value())->option();
		if      (oId < option_category_context_end) { configOpts.addOption(*it); }
		else if (oId < opt_no_lookback)             { search.addOption(*it); }
		else if (oId < option_category_solver_end)  { lookback.addOption(*it); }
		else if (oId < opt_restarts)                { search.addOption(*it); }
		else if (oId < option_category_search_end)  { lookback.addOption(*it); }
		else if (oId < option_category_asp_end)     { asp.addOption(*it); }
		else                                        { solving.addOption(*it); } 
	}
	root.add(configOpts).add(solving).add(asp).add(search).add(lookback);
	root.addAlias("number", root.find("models")); // remove on next version
}
bool ClaspCliConfig::assignDefaults(const ProgramOptions::ParsedOptions& exclude) {
	for (Options::option_iterator it = opts_->begin(), end = opts_->end(); it != end; ++it) {
		const ProgramOptions::Option& o = **it;
		CLASP_FAIL_IF(exclude.count(o.name()) == 0 && !o.assignDefault(), "Option '%s': invalid default value '%s'\n", o.name().c_str(), o.value()->defaultsTo());
	}
	return true;
}	
void ClaspCliConfig::releaseOptions() {
	opts_ = 0;
}
bool ClaspCliConfig::match(const char*& path, const char* what, bool matchDot) const {
	const char* t = path;
	while (*t == *what && *what) { ++t; ++what; }
	if (matchDot) {
		t    += (*t == '.');
		what += (!*t && *what == '.');
	}
	return !*what && (path=t) == t;
}
ClaspCliConfig::KeyType ClaspCliConfig::getKey(KeyType k, const char* path) const {
	int16 id = decodeKey(k);
	if (!isValidId(id) || !path || !*path || (match(path, ".") && !*path)) {
		return k;
	}
	if (isLeafId(id)){ return KEY_INVALID; }
	const NodeKey& x = nodes_g[-id];
	for (int16 sk = x.skBegin; sk != x.skEnd && sk < 0; ++sk) {
		NodeKey sub = nodes_g[-sk];
		if (match(path, sub.name)) {
			KeyType ret = makeKeyHandle(sk, (sk == key_tester ? mode_tester : 0) | decodeMode(k), 0);
			if (!*path) { return ret; }
			return getKey(ret, path);
		}
	}
	uint8 mode = decodeMode(k);
	if (id == key_solver) {
		uint32 solverId;
		if ((mode & mode_solver) == 0 && *path != '.' && bk_lib::xconvert(path, solverId, &path, 0) == 1) {
			return getKey(makeKeyHandle(id, mode | mode_solver, std::min(solverId, (uint32)uint8(-1))), path);
		}
		mode |= mode_solver;
	}
	const Name2Id* opt = index_g.find(path);
	// remaining name must be a valid option in our subkey range
	if (!opt || opt->key < x.skBegin || opt->key >= x.skEnd) { 
		return KEY_INVALID; 
	}
	return makeKeyHandle(static_cast<int16>(opt->key), mode, decodeSolver(k));
}

ClaspCliConfig::KeyType ClaspCliConfig::getArrKey(KeyType k, unsigned i) const {
	int16 id = decodeKey(k);
	if (id != key_solver || (decodeMode(k) & mode_solver) != 0 || i >= solve.supportedSolvers()) { return KEY_INVALID; }
	return makeKeyHandle(id, decodeMode(k) | mode_solver, i);
}
int ClaspCliConfig::getKeyInfo(KeyType k, int* nSubkeys, int* arrLen, const char** help, int* nValues) const {
	int16 id = decodeKey(k);
	int ret  = 0;
	if (!isValidId(id)){ return -1; }
	if (isLeafId(id)){
		if (nSubkeys && ++ret) { *nSubkeys = 0;  }
		if (arrLen && ++ret)   { *arrLen   = -1; }
		if (nValues && ++ret)  { *nValues  = static_cast<int>( (decodeMode(k) & mode_tester) == 0 || testerConfig() != 0 ); }
		if (help && ++ret)     { getActive(id, 0, help, 0); }
		return ret;
	}
	const NodeKey& x = nodes_g[-id];
	if (nSubkeys && ++ret) { *nSubkeys = x.numSubkeys(); }
	if (nValues && ++ret)  { *nValues = -1; }
	if (help && ++ret)     { *help = x.desc; }
	if (arrLen && ++ret)   {
		*arrLen = -1; 
		if (id == key_solver && (decodeMode(k) & mode_solver) == 0) {
			const UserConfig* c = (decodeMode(k) & mode_tester) == 0 ? this : testerConfig();
			*arrLen = c ? (int)c->numSolver() : 0;
		}
	}
	return ret;
}
bool ClaspCliConfig::isLeafKey(KeyType k) { return isLeafId(decodeKey(k)); }
const char* ClaspCliConfig::getSubkey(KeyType k, uint32 i) const {
	int16 id = decodeKey(k);
	if (!isValidId(id) || isLeafId(id)) { return 0; }
	const NodeKey& nk = nodes_g[-id];
	if (i >= nk.numSubkeys()) { return 0; }
	int sk = nk.skBegin + static_cast<int16>(i);
	if (sk < key_leaf) { return nodes_g[-sk].name; }
	const char* opt = 0;
	getActive(sk, 0, 0, &opt);
	return opt;
}
int ClaspCliConfig::getValue(KeyType key, std::string& out) const {
	int16 id = decodeKey(key);
	if (!isLeafId(id)) { return -1; }
	try {
		int ret = ScopedSet(const_cast<ClaspCliConfig&>(*this), decodeMode(key), decodeSolver(key))->getActive(id, &out, 0, 0);
		return ret > 0 ? static_cast<int>(out.length()) : ret;
	}
	catch (...) { return -2; }
}
int ClaspCliConfig::getValue(KeyType key, char** value) const {
	if (value) { *value = 0; }
	std::string temp;
	int ret = getValue(key, temp);
	if (ret <= 0 || !value) { return ret; }
	if ((*value = (char*)malloc(temp.length() + 1)) == 0) { return -2; }
	std::strcpy(*value, temp.c_str());
	return static_cast<int>(temp.length());
}
int ClaspCliConfig::getValue(KeyType key, char* buffer, std::size_t bufSize) const {
	std::string temp;
	int ret = getValue(key, temp);
	if (ret <= 0) { return ret; }
	if (buffer && bufSize) {
		std::size_t n = temp.length() >= bufSize ? bufSize - 1 : temp.length();
		std::memcpy(buffer, temp.c_str(), n * sizeof(char));
		buffer[n] = 0;
	}
	return static_cast<int>(temp.length());
}
std::string ClaspCliConfig::getValue(const char* path) const {
	std::string temp;
	CLASP_FAIL_IF(getValue(getKey(KEY_ROOT, path), temp) <= 0, "Invalid key: '%s'", path);
	return temp;
}
bool ClaspCliConfig::hasValue(const char* path) const {
	int nVals;
	return getKeyInfo(getKey(KEY_ROOT, path), 0, 0, 0, &nVals) == 1 && nVals > 0;
}
void ClaspCliConfig::releaseValue(const char* value) const {
	if (value) { free((void*)value); }
}

int ClaspCliConfig::setValue(KeyType key, const char* value) {
	int16 id = decodeKey(key);
	if (!isLeafId(id)) { return -1; }
	if ((decodeMode(key) & mode_tester) != 0) { addTesterConfig(); }
	ScopedSet scope(*this, decodeMode(key), decodeSolver(key));
	try         { return setActive(id, value); }
	catch (...) { return -2; }
}

bool ClaspCliConfig::setValue(const char* path, const char* value) {
	int ret = setValue(getKey(KEY_ROOT, path), value);
	CLASP_FAIL_IF(ret < 0, (ret == -1 ? "Invalid or incomplete key: '%s'" : "Value error in key: '%s'"), path);
	return ret != 0;
}

int ClaspCliConfig::applyActive(int o, const char* _val_, std::string* _val_out_, const char** _desc_out_, const char** _name_out_) {
	UserConfig*    active = this->active();
	uint32         sId    = cliId;
	SolverOpts*    solver = 0;
	SearchOpts*    search = 0;
	ContextParams* ctxOpts= active;
	if (_name_out_) { *_name_out_ = 0; }
	if (_val_ || _val_out_) {
		if (!active || (active == testerConfig() && !isTesterOption(o)) || ((cliMode & mode_solver) != 0 && !isSolverOption(o))) {
			o = (cliMode & mode_relaxed) != 0 ? detail__before_options : detail__num_options;
		}
		else if (isSolverOption(o)) {
			solver = &active->addSolver(sId);
			search = &active->addSearch(sId);
		}
	}
	if (!isOption(o)) {
		return o == detail__before_options ? int(_val_ != 0) : -1;
	}
	// action & helper macros used in get/set
	using bk_lib::xconvert; using bk_lib::off; using bk_lib::opt;
	using bk_lib::stringTo; using bk_lib::toString;
	#define ITE(c, a, b)            (!!(c) ? (a) : (b))
	#define FUN(x)                  for (bk_lib::ArgString x = _val_;;)
	#define STORE(obj)              { return stringTo((_val_), obj); }
	#define STORE_LEQ(x, y)         { unsigned __n; return stringTo(_val_, __n) && SET_LEQ(x, __n, y); }
	#define STORE_FLAG(x)           { bool __b; return stringTo(_val_, __b) && SET(x, static_cast<unsigned>(__b)); }
	#define STORE_OR_FILL(x)        { unsigned __n; return stringTo(_val_, __n) && SET_OR_FILL(x, __n); }
	#define GET_FUN(x)              bk_lib::StringBuilder x(*_val_out_); if (!x);else
	#define GET(...)                *_val_out_ = toString( __VA_ARGS__ )
	#define GET_IF(c, ...)          *_val_out_ = ITE((c), toString(__VA_ARGS__), toString(off))
	switch(static_cast<OptionKey>(o)) {
		default: return -1;
		#define OPTION(k, e, a, d, x, v) \
		case opt_##k:\
			if (_name_out_){ *_name_out_ = #k ; }\
			if (_val_) try { x ; }catch(...) {return 0;}\
			if (_val_out_) { v ; }\
			if (_desc_out_){ *_desc_out_ = d; }\
			return 1;
		#define CLASP_CONTEXT_OPTIONS (*ctxOpts)
		#define CLASP_ASP_OPTIONS asp
		#define CLASP_SOLVE_OPTIONS solve
		#define CLASP_SOLVER_OPTIONS (*solver)
		#define CLASP_SEARCH_OPTIONS (*search)
		#define CLASP_SEARCH_RESTART_OPTIONS search->restart
		#define CLASP_SEARCH_REDUCE_OPTIONS search->reduce
		#include <clasp/cli/clasp_cli_options.inl>
	}
	#undef FUN
	#undef STORE
	#undef STORE_LEQ
	#undef STORE_FLAG
	#undef STORE_OR_FILL
	#undef GET_IF
	#undef GET
	#undef ITE
}

int ClaspCliConfig::setActive(int id, const char* setVal) {
	if      (isOption(id))     { return applyActive(id, setVal ? setVal : "", 0, 0, 0); }
	else if (id == meta_config){
		int sz = setAppOpt(id, setVal);
		if (sz <= 0) { return 0; }
		std::string m;
		UserConfig* act = active();
		ConfigIter it  = getConfig(act->cliConfig, m);
		act->hasConfig = 0;
		cliMode       |= mode_relaxed;
		act->resize(1, 1);
		for (uint32 sId = 0; it.valid(); it.next()) {
			act->addSolver(sId);
			act->addSearch(sId);
			cliId = static_cast<uint8>(sId);
			if (!setConfig(it, false, ParsedOpts(), 0)){ return 0; }
			if (++sId == static_cast<uint32>(sz))      { break; }
			cliMode |= mode_solver;
		}
		if (sz < 65 && static_cast<uint32>(sz) > act->numSolver()) {
			for (uint32 sId = act->numSolver(), mod = sId, end = static_cast<uint32>(sz); sId != end; ++sId) {
				SolverParams& solver = act->addSolver(sId);
				SolveParams&  search = act->addSearch(sId);
				(solver = act->solver(sId % mod)).setId(sId);
				search = act->search(sId % mod);
			}
		}
		return static_cast<int>((act->hasConfig = 1) == 1);
	}
	else { return -1; }
}

int ClaspCliConfig::getActive(int o, std::string* val, const char** desc, const char** opt) const {
	if      (isOption(o))     { return const_cast<ClaspCliConfig&>(*this).applyActive(o, 0, val, desc, opt); }
	else if (!active())       { return -1; }
	else if (o == meta_config){
		const NodeKey& n = nodes_g[-o];
		if (val)  { 
			uint8 k = (ConfigKey)active()->cliConfig;
			if (k < config_max_value) { xconvert(*val, static_cast<ConfigKey>(k)); }
			else                      { val->append(config_[!isGenerator()]); }
		}
		if (desc) { *desc = n.desc; }
		if (opt)  { *opt  = n.name; }
		return 1;
	}
	else { return -1; }
}
int ClaspCliConfig::setAppOpt(int o, const char* _val_) {
	if (o == meta_config) {
		std::pair<ConfigKey, uint32> defC(config_default, INT_MAX);
		if   (bk_lib::stringTo(_val_, defC)){ active()->cliConfig = (uint8)defC.first; }
		else {
			CLASP_FAIL_IF(!std::ifstream(_val_).is_open(), "Could not open config file '%s'", _val_);
			config_[!isGenerator()] = _val_; active()->cliConfig = config_max_value + !isGenerator(); 
		}
		return Range<uint32>(0, INT_MAX).clamp(defC.second);
	}
	else if (o == meta_tester && isGenerator()) {
		addTesterConfig();
		RawConfig config("<tester>");
		config.addArg(_val_);
		ParsedOpts ex;
		bool ret = ScopedSet(*this, mode_tester)->setConfig(config.iterator(), true, ParsedOpts(), &ex);
		return ret && finalizeAppConfig(testerConfig(), finalizeParsed(testerConfig(), ex, ex), Problem_t::Asp, true);
	}
	return -1; // invalid option
}
bool ClaspCliConfig::setAppDefaults(UserConfig* active, uint32 sId, const ParsedOpts& cmdLine, ProblemType t) {
	ScopedSet temp(*this, (active == this ? 0 : mode_tester) | mode_relaxed, sId);
	if (sId == 0 && t != Problem_t::Asp && cmdLine.count("sat-prepro") == 0) {
		setActive(opt_sat_prepro, "2,20,25,120");
	}
	if (active->addSolver(sId).search == SolverParams::no_learning) {
		if (cmdLine.count("heuristic") == 0) { setActive(opt_heuristic, "unit"); }
		if (cmdLine.count("lookahead") == 0) { setActive(opt_lookahead, "atom"); }
		if (cmdLine.count("deletion")  == 0) { setActive(opt_deletion, "no"); }
		if (cmdLine.count("restarts")  == 0) { setActive(opt_restarts, "no"); }
	}
	return true;
}

bool ClaspCliConfig::setConfig(const ConfigIter& config, bool allowMeta, const ParsedOpts& exclude, ParsedOpts* out) {
	createOptions();
	ParseContext ctx(*this, config.name(), &exclude, allowMeta, out);
	ProgramOptions::parseCommandString(config.args(), ctx, ProgramOptions::command_line_allow_flag_value);
	return true;
}

bool ClaspCliConfig::validate() {
	UserConfiguration* arr[3] = { this, testerConfig(), 0 };
	UserConfiguration** c     = arr;
	char ctx[80];
	do {
		for (uint32 i = 0; i != (*c)->numSolver(); ++i) {
			Clasp::Cli::validate(clasp_format(ctx, 80, "<%s>.%u", *c == this ? "<config>":"<tester>", i), (*c)->solver(i), (*c)->search(i));
		}
	} while (*++c);
	return true;
}

bool ClaspCliConfig::finalize(const ParsedOpts& x, ProblemType t, bool defs) {
	ParsedOpts temp;
	return finalizeAppConfig(this, finalizeParsed(this, x, temp), t, defs);
}

void ClaspCliConfig::addDisabled(ParsedOpts& parsed) {
	finalizeParsed(this, parsed, parsed);
}

const ClaspCliConfig::ParsedOpts& ClaspCliConfig::finalizeParsed(UserConfig* active, const ParsedOpts& parsed, ParsedOpts& exclude) const {
	bool copied = &parsed == &exclude;
	if (active->search(0).reduce.fReduce() == 0 && parsed.count("deletion") != 0) {
		if (!copied) { exclude = parsed; copied = true; }
		exclude.add("del-cfl");
		exclude.add("del-max");
		exclude.add("del-grow");
	}
	return !copied ? parsed : exclude;
}

bool ClaspCliConfig::finalizeAppConfig(UserConfig* active, const ParsedOpts& parsed, ProblemType t, bool defs) {
	if (defs && !setAppDefaults(active, 0, parsed, t)) { return false; }
	SolverParams defSolver = active->solver(0);
	SolveParams  defSearch = active->search(0);
	if (active->hasConfig) { return true; }
	uint8 c = active->cliConfig;
	if (c == config_many && solve.numSolver() == 1) { c = config_default; }
	if (c == config_default) {
		if      (defSolver.search == SolverParams::no_learning)       { c = config_nolearn; }
		else if (active == testerConfig())                            { c = config_tester_default; }
		else if (solve.numSolver() == 1 || !solve.defaultPortfolio()) { c = t == Problem_t::Asp ? (uint8)config_asp_default : (uint8)config_sat_default; }
		else                                                          { c = config_many; }
	}
	std::string m;
	ConfigIter conf = getConfig(c, m);
	uint8  mode     = (active == testerConfig() ? mode_tester : 0) | mode_relaxed;
	uint32 portSize = 0;
	const char* ctx = active == testerConfig() ? "<tester>" : "<config>";
	char   buf[80];
	for (uint32 i = 0; i != solve.numSolver() && conf.valid(); ++i) {
		SolverParams& solver = (active->addSolver(i) = defSolver).setId(i);
		SolveParams&  search = (active->addSearch(i) = defSearch);
		ConfigKey     baseK  = config_default;
		CLASP_FAIL_IF(*conf.base() && !bk_lib::stringTo(conf.base(), baseK), "%s.%s: '%s': Invalid base config!", ctx, conf.name(), conf.base());
		if (baseK != config_default && !ScopedSet(*this, mode|mode_solver, i)->setConfig(getConfig(baseK), false, parsed, 0)) {
			return false;
		}
		if (!ScopedSet(*this, mode, i)->setConfig(conf, false, parsed, 0)) {
			return false;
		}
		Clasp::Cli::validate(clasp_format(buf, 80, "%s.%s", ctx, conf.name()), solver, search);
		++portSize;
		conf.next();
		mode |= mode_solver;
	}
	active->hasConfig = 1;
	return true;
}

bool ClaspCliConfig::setAppConfig(const RawConfig& config, ProblemType t) {
	ProgramOptions::ParsedOptions exclude;
	reset();
	return setConfig(config.iterator(), true, exclude, &exclude) && assignDefaults(exclude) && finalize(exclude, t, true);
}

void validate(const char* ctx, const SolverParams& solver, const SolveParams& search) {
	if (!ctx) { ctx = "<clasp>"; }
	const ReduceParams& reduce = search.reduce;
	if (solver.search == SolverParams::no_learning) {
		CLASP_FAIL_IF(Heuristic_t::isLookback(solver.heuId), "'%s': Heuristic requires lookback strategy!", ctx);
		CLASP_FAIL_IF(!search.restart.sched.disabled() && !search.restart.sched.defaulted(), "'%s': 'no-lookback': restart options disabled!", ctx);
		CLASP_FAIL_IF(!reduce.cflSched.disabled() || (!reduce.growSched.disabled() && !reduce.growSched.defaulted()) || search.reduce.fReduce() != 0, "'%s': 'no-lookback': deletion options disabled!", ctx);
	}
	bool  hasSched = !reduce.cflSched.disabled() || !reduce.growSched.disabled() || reduce.maxRange != UINT32_MAX;
	CLASP_FAIL_IF(hasSched  && reduce.fReduce() == 0.0f && !reduce.growSched.defaulted(), "'%s': 'no-deletion': deletion strategies disabled!", ctx);
	CLASP_FAIL_IF(!hasSched && reduce.fReduce() != 0.0f && !reduce.growSched.defaulted(), "'%s': 'deletion': deletion strategy required!", ctx);
}
}}
