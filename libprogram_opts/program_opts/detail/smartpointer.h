// scaled down version of Jean Gressmann's smartpointer.h 
// (see copyright notice at end of file)

#ifndef PROGRAM_OPTIONS_DETAIL_SMARTPOINTER_H
#define PROGRAM_OPTIONS_DETAIL_SMARTPOINTER_H

#if defined(_MSC_VER)
# define for if(0);else for
# pragma warning(disable:4521)
# pragma warning(disable:4786)
# pragma warning(disable:4522)
# pragma warning(disable:4355)
# pragma warning(disable:4291)
#endif

#include <algorithm>
#include <cassert>

namespace ProgramOptions {

// Thanks to Benjamin Kaufmann (kaufmann@cs.uni-potsdam.de)
// who provided for compile time assertions for void pointers
template <bool> struct StaticAssert;
template <> struct StaticAssert<true> {};
template <class T> struct is_void {enum {result = 0};};
template <> struct is_void<void> {enum {result = 1};};

// These freeResource() functions are required to handle their
// specific Null indifferently, say delete 0 is
// to do nothing.
template<class T>
struct PointerTraits_
{
	StaticAssert<!is_void<T>::result> Error_Smart_Pointer_Does_Not_Support_Void_Ptr;
	typedef T ValueType;
	typedef T& ReferenceType;
	typedef const T& ConstReferenceType;
	typedef T* PointerType;
	typedef const T* ConstPointerType;
	static const PointerType Null;

	static void freeResource(PointerType p)
	{
		enum { no_incomplete_types_please = sizeof(*p) };
		delete p;
	}
};

template<class T>
const typename PointerTraits_<T>::PointerType PointerTraits_<T>::Null = 0;

// reference counting based on a shared counter
template<class Traits>
class ReferenceCountedOwner_
{
public: 
	typedef typename Traits::PointerType PT;    
	typedef typename Traits::ConstPointerType CPT;    
	static CPT null() { return Traits::Null; }
private:
	class RefCount_
	{
		PT managed_;
		size_t count_;    
		RefCount_();
		RefCount_(const RefCount_&);
		RefCount_& operator=(const RefCount_&);
	public: 
		explicit
		RefCount_(PT p)
			: managed_(p)
			, count_(1)
		{}
		inline
		void inc()
		{
			++count_;
		}
		inline
		size_t count() const { return count_; }
		inline
		bool unique() const { return count_ == 1; }
		void dec()
		{
			--count_;
			if(!count_)
			{
				PT deleteme = managed_;
				delete this;
				Traits::freeResource(deleteme);       
			}
				
		}
		void weakDec()
		{
			--count_;
			if(!count_)
			{
				delete this;        
			}
		}
		inline
		PT get() { return managed_; }
		inline
		CPT get() const { return managed_; }
	};
	RefCount_* refCount_;
	
	void decIfValidRefCount()
	{
		if(refCount_)
		{
			RefCount_* temp = 0;
			std::swap(temp, refCount_);
			temp->dec();
		}
			
	}
	inline
	void incIfValidRefCount()  // nothrow guarantee
	{
		if(refCount_)
			refCount_->inc();
	}
	inline
	static RefCount_* createRefCount(PT p)
	{
		return p == Traits::Null ? 0 : new RefCount_(p);
	} 
public:
	~ReferenceCountedOwner_()
	{
		decIfValidRefCount();
	}
	explicit
	ReferenceCountedOwner_()
		: refCount_(0)
	{}  
	ReferenceCountedOwner_(PT p)
		: refCount_(createRefCount(p))
	{}
	ReferenceCountedOwner_(const ReferenceCountedOwner_& o)
		: refCount_(o.refCount_)
	{
		incIfValidRefCount();
	}
	inline
	ReferenceCountedOwner_& operator=(const ReferenceCountedOwner_& o)    
	{
		ReferenceCountedOwner_(o).swap(*this);
		return *this;
	}
	inline
	ReferenceCountedOwner_& operator=(PT p)   
	{
		ReferenceCountedOwner_(p).swap(*this);
		return *this;
	}
	inline
	bool unique() const { return refCount_ ? refCount_->unique() : true; }  // nothrow guarantee
	inline
	size_t count() const { return refCount_ ? refCount_->count() : 1; }  // nothrow guarantee
	inline
	PT get() { return refCount_ ? refCount_->get() : Traits::Null; }  
	inline
	CPT get() const { return refCount_ ? refCount_->get() : Traits::Null; } 
	inline
	void swap(ReferenceCountedOwner_& o)  // nothrow guarantee
	{
		std::swap(refCount_, o.refCount_);
	}
	inline
	void reset()
	{
		ReferenceCountedOwner_().swap(*this);   
	}
	inline
	void release()
	{
		if(refCount_)
			refCount_->weakDec();
		refCount_ = 0;
	}
};
template
<
	class T,
	class Traits = PointerTraits_<T>,
	class RefCountPolicy = ReferenceCountedOwner_<Traits>
>
class SmartPointer : public RefCountPolicy
{ 
public:
	typedef typename Traits::PointerType PT;
	typedef typename Traits::ConstPointerType CPT;
	typedef typename Traits::ReferenceType RT;
	typedef typename Traits::ConstReferenceType CRT;

	explicit
	SmartPointer() 
	{}
	explicit
	SmartPointer(PT p) 
		: RefCountPolicy(p)
	{}

	SmartPointer(const SmartPointer& o)
		: RefCountPolicy(o)
	{}
	SmartPointer(SmartPointer& o)
		: RefCountPolicy(o)
	{}
	inline
	SmartPointer& operator=(SmartPointer& o)
	{
		RefCountPolicy::operator=(o);
		return *this;
	}
	inline
	SmartPointer& operator=(const SmartPointer& o)
	{
		RefCountPolicy::operator=(o);
		return *this;
	}
	inline
	SmartPointer& operator=(PT p)
	{
		RefCountPolicy::operator=(p);
		return *this;
	}
	inline
	bool operator==(const SmartPointer& o) const
	{
		return this->get() == o.get();
	}
	inline
	bool operator!=(const SmartPointer& o) const
	{
		return this->get() != o.get();
	}
	inline
	operator bool() const { return this->get() != Traits::Null; }
	inline
	PT operator->() { return this->get(); }
	inline
	CPT operator->() const { return this->get(); }
	inline
	RT operator*() { return *this->get(); }
	inline
	CRT operator*() const { return *this->get(); }
};

template<class T, class Traits = PointerTraits_<T> >
class SharedPtr : public SmartPointer<T, Traits, ReferenceCountedOwner_<Traits> >
{
	typedef SmartPointer<T, Traits, ReferenceCountedOwner_<Traits> > Base;
	typedef typename Base::PT PT;
public:
	explicit
	SharedPtr()
		: Base()
	{}
	
	SharedPtr(PT p)
		: Base(p)
	{}
	SharedPtr(const SharedPtr& o)
		: Base(o)
	{}
	inline
	SharedPtr& operator=(PT p)
	{
		Base::operator=(p);
		return *this;
	}
	inline
	SharedPtr& operator=(const SharedPtr& o)
	{
		Base::operator=(o);
		return *this;
	}
};


}
#endif

/*  Copyright (c) October 2004 Jean Gressmann (jsg@rz.uni-potsdam.de)
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. 
 * 
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this file; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

