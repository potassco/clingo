// 
// Copyright (c) 2016, Benjamin Kaufmann
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
#include <clasp/statistics.h>
#include <clasp/util/hash_map.h>
#include <clasp/util/misc_types.h>
#include <potassco/match_basic_types.h>
#include <stdexcept>
#include <cstring>
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// StatisticObject
/////////////////////////////////////////////////////////////////////////////////////////
StatisticObject::RegVec StatisticObject::types_;
StatisticObject::StatisticObject() : handle_(0) {}
StatisticObject::StatisticObject(const void* obj, uint32 type) {
	handle_  = static_cast<uint64>(type) << 48;
	handle_ |= static_cast<uint64>(reinterpret_cast<uintp>(obj));
}
const void* StatisticObject::self() const {
	static const uint64 ptrMask = bit_mask<uint64>(48) - 1;
	return reinterpret_cast<const void*>(static_cast<uintp>(handle_ & ptrMask));
}
const StatisticObject::I* StatisticObject::tid() const {
	return types_.at(static_cast<uint32>(handle_ >> 48));
}
StatisticObject::Type StatisticObject::type() const {
	return !empty() ? tid()->type : Potassco::Statistics_t::Empty;
}
bool StatisticObject::empty() const {
	return handle_ == 0;
}
uint32 StatisticObject::size() const {
	switch (type()) {
		default: throw std::logic_error("invalid object");
		case Potassco::Statistics_t::Value: return 0;
		case Potassco::Statistics_t::Map:   return static_cast<const M*>(tid())->size(self());
		case Potassco::Statistics_t::Array: return static_cast<const A*>(tid())->size(self());
	}
}
const char* StatisticObject::key(uint32 i) const {
	CLASP_FAIL_IF(type() != Potassco::Statistics_t::Map, "type error");
	return static_cast<const M*>(tid())->key(self(), i);
}
static inline StatisticObject check(StatisticObject o) {
	return !o.empty() ? o : throw std::out_of_range("StatisticObject");
}
StatisticObject StatisticObject::at(const char* k) const {
	CLASP_FAIL_IF(type() != Potassco::Statistics_t::Map, "type error");
	return check(static_cast<const M*>(tid())->at(self(), k));
}
StatisticObject StatisticObject::operator[](uint32 i) const {
	CLASP_FAIL_IF(type() != Potassco::Statistics_t::Array, "type error");
	return check(static_cast<const A*>(tid())->at(self(), i));
}
double StatisticObject::value() const {
	CLASP_FAIL_IF(type() != Potassco::Statistics_t::Value, "type error");
	return static_cast<const V*>(tid())->value(self());
}
std::size_t StatisticObject::hash() const {
	typedef Clasp::HashSet_t<uint64>::set_type::hasher Hasher;
	return Hasher()(toRep());
}
uint64 StatisticObject::toRep() const {
	return handle_;
}
StatisticObject StatisticObject::fromRep(uint64 x) {
	if (!x) { return StatisticObject(0, 0); }
	StatisticObject r;
	r.handle_ = x;
	CLASP_FAIL_IF(r.tid() == 0 || (reinterpret_cast<uintp>(r.self()) & 3u) != 0, "invalid key");
	return r;
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspStatistics
/////////////////////////////////////////////////////////////////////////////////////////
struct ClaspStatistics::Impl {
	typedef Clasp::HashMap_t<uint64, uint32>::map_type RegMap;
	Impl() : gc_(0), rem_(0) {}
	Key_t add(const StatisticObject& o) {
		uint64 k = o.toRep();
		map_[k] = gc_;
		return k;
	}
	bool remove(const StatisticObject& o) {
		std::size_t rem = map_.erase(o.toRep());
		return rem != 0;
	}
	StatisticObject get(Key_t k) const {
		RegMap::const_iterator it = map_.find(k);
		return it != map_.end() && it->second == gc_ ? StatisticObject::fromRep(k) : throw std::logic_error("invalid key");
	}
	void update(const StatisticObject& root) {
		++gc_;
		rem_ += (map_.size() - visit(root));
		if (rem_ > (map_.size() >> 1)) {
			for (RegMap::iterator it = map_.begin(), end = map_.end(); it != end;) {
				if (it->second == gc_) { ++it; }
				else { it = map_.erase(it); }
			}
			rem_ = 0;
		}
	}
	uint32 visit(const StatisticObject& obj) {
		uint32 count = 0;
		RegMap::iterator it = map_.find(obj.toRep());
		if (obj.empty() || it == map_.end()) { return count; }
		it->second = gc_;
		++count;
		switch (obj.type()) {
			case Potassco::Statistics_t::Map: 
				for (uint32 i = 0, end = obj.size(); i != end; ++i) { count += visit(obj.at(obj.key(i))); }
				break;
			case Potassco::Statistics_t::Array:
				for (uint32 i = 0, end = obj.size(); i != end; ++i) { count += visit(obj[i]); }
				break;
			default: break;
		}
		return count;
	}
	RegMap map_;
	uint32 gc_;
	uint32 rem_;
};

ClaspStatistics::ClaspStatistics() : root_(0), impl_(new Impl()) {}
ClaspStatistics::~ClaspStatistics() {
	delete impl_;
}
StatisticObject ClaspStatistics::getObject(Key_t k) const {
	return impl_->get(k);
}
ClaspStatistics::Key_t ClaspStatistics::root() const {
	return root_;
}
Potassco::Statistics_t ClaspStatistics::type(Key_t key) const {
	return getObject(key).type();
}
size_t ClaspStatistics::size(Key_t key) const {
	return getObject(key).size();
}
ClaspStatistics::Key_t ClaspStatistics::at(Key_t arrK, size_t index) const {
	return impl_->add(getObject(arrK)[index]);
}
const char* ClaspStatistics::key(Key_t mapK, size_t i) const {
	return getObject(mapK).key(i);
}
ClaspStatistics::Key_t ClaspStatistics::get(Key_t key, const char* path) const {
	return impl_->add(!std::strchr(path, '.')
		? getObject(key).at(path)
		: findObject(key, path));
}
double ClaspStatistics::value(Key_t key) const {
	return getObject(key).value();
}
ClaspStatistics::Key_t ClaspStatistics::setRoot(const StatisticObject& obj) {
	return root_ = impl_->add(obj);
}
bool ClaspStatistics::removeStat(const StatisticObject& obj, bool recurse) {
	bool ret = impl_->remove(obj);
	if (ret && recurse) {
		if (obj.type() == Potassco::Statistics_t::Map) {
			for (uint32 i = 0, end = obj.size(); i != end; ++i) {
				removeStat(obj.at(obj.key(i)), true);
			}
		}
		else if (obj.type() == Potassco::Statistics_t::Array) {
			for (uint32 i = 0, end = obj.size(); i != end; ++i) {
				removeStat(obj[i], true);
			}
		}
	}
	return ret;
}
bool ClaspStatistics::removeStat(Key_t k, bool recurse) {
	try { return removeStat(impl_->get(k), recurse); }
	catch (const std::logic_error&) { return false; }
}
void ClaspStatistics::update() {
	if (root_) { impl_->update(getObject(root_)); }
}
StatisticObject ClaspStatistics::findObject(Key_t root, const char* path, Key_t* res) const {
	StatisticObject o = getObject(root);
	StatisticObject::Type t = o.type();
	char temp[1024]; const char* top, *parent = path;
	for (int pos; path && *path;) {
		top = path;
		if ((path = std::strchr(path, '.')) != 0) {
			std::size_t len = static_cast<std::size_t>(path++ - top);
			CLASP_FAIL_IF(len >= 1024, "invalid key");
			top = (const char*)std::memcpy(temp, top, len);
			temp[len] = 0;
		}
		if      (t == Potassco::Statistics_t::Map) { o = o.at(top); }
		else if (t == Potassco::Statistics_t::Array && Potassco::match(top, pos) && pos >= 0) {
			o = o[uint32(pos)];
		}
		else { 
			throw std::out_of_range(ClaspErrorString("invalid path: '%s' at key '%s'", parent, top).c_str());
		}
		t = o.type();
	}
	if (res) { *res = impl_->add(o); }
	return o;
}

StatisticObject StatsMap::at(const char* k) const {
	if (const StatisticObject* o = find(k)) { return *o; }
	throw std::out_of_range(ClaspErrorString("StatsMap::at with key '%s'", k).c_str());
}
const StatisticObject* StatsMap::find(const char* k) const {
	for (MapType::const_iterator it = keys_.begin(), end = keys_.end(); it != end; ++it) {
		if (std::strcmp(it->first, k) == 0) { return &it->second; }
	}
	return 0;
}
bool StatsMap::add(const char* k, const StatisticObject& o) {
	return !find(k) && (keys_.push_back(MapType::value_type(k, o)), true);
}
/////////////////////////////////////////////////////////////////////////////////////////
// StatsVisitor
/////////////////////////////////////////////////////////////////////////////////////////
StatsVisitor::~StatsVisitor() {}
bool StatsVisitor::visitGenerator(Operation) {
	return true;
}
bool StatsVisitor::visitThreads(Operation) {
	return true;
}
bool StatsVisitor::visitTester(Operation) {
	return true;
}
bool StatsVisitor::visitHccs(Operation) {
	return true;
}
void StatsVisitor::visitHcc(uint32, const ProblemStats& p, const SolverStats& s) {
	visitProblemStats(p);
	visitSolverStats(s);
}
void StatsVisitor::visitThread(uint32, const SolverStats& stats) {
	visitSolverStats(stats);
}

}

