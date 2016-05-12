//
//  Copyright (c) Benjamin Kaufmann 2010
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. 
// 
//  This file is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//
// NOTE: ProgramOptions is inspired by Boost.Program_options
//       see: www.boost.org/libs/program_options
//
#ifndef PROGRAM_OPTIONS_REFCOUNTABLE_H_INCLUDED
#define PROGRAM_OPTIONS_REFCOUNTABLE_H_INCLUDED
namespace ProgramOptions { namespace detail {

class RefCountable {
public:
	RefCountable() : refCount_(1) {}
	int  addRef()         { return ++refCount_; }
	int  release()        { return --refCount_; }
	int  refCount() const { return refCount_;   }
private:
	int refCount_;
};

template <class T>
class IntrusiveSharedPtr {
public:
	typedef T element_type;
	explicit IntrusiveSharedPtr(T* p = 0) throw()
		: ptr_(p) { /* NO add ref */ }
	IntrusiveSharedPtr(const IntrusiveSharedPtr& o) throw() 
		: ptr_(o.ptr_) { addRef(); }
	~IntrusiveSharedPtr() throw () { release(); }
	IntrusiveSharedPtr& operator=(const IntrusiveSharedPtr& other) {
		other.addRef();
		this->release();
		this->ptr_ = other.ptr_;
		return *this;
	}
	T&     operator*() const throw()   { return *ptr_; }
	T*     operator->() const throw()  { return ptr_;  }
	T*     get() const throw()         { return ptr_; }
	void   reset() throw()             { release(); ptr_ = 0; }
	bool   unique() const throw()      { return  !ptr_ || ptr_->refCount() == 1; }
	int    count() const  throw()      { return ptr_ ? ptr_->refCount() : 0; }
	void   swap(IntrusiveSharedPtr& b) {
		T* temp = ptr_;
		ptr_    = b.ptr_;
		b.ptr_  = temp;
	}
private:
	T* ptr_;
	void addRef()  const { if (ptr_) ptr_->addRef(); }
	void release() const {
		if (ptr_ && ptr_->release() == 0) {
			delete ptr_;
		}
	}
};

}}
#endif
