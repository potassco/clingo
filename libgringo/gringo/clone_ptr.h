// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <boost/ptr_container/clone_allocator.hpp>

template<typename T>
class clone_ptr
{
public:
	clone_ptr();
	explicit clone_ptr(T *ptr);
	clone_ptr(const clone_ptr<T> &ptr);
	void reset(T *ptr);
	T *release();
	T *get() const;
	T *operator->();
	const T *operator->() const;
	T &operator *();
	const T &operator *() const;
	~clone_ptr();
private:
	T *ptr_;
};

template<typename T>
inline clone_ptr<T>::clone_ptr() 
	: ptr_(0)
{
}

template<typename T>
inline clone_ptr<T>::clone_ptr(T *ptr)
	: ptr_(ptr)
{
}

template<typename T>
inline clone_ptr<T>::clone_ptr(const clone_ptr<T> &ptr)
	: ptr_(ptr.get() ? boost::new_clone(*ptr.get()) : 0)
{
}

template<typename T>
inline void clone_ptr<T>::reset(T *ptr)
{
	delete ptr_;
	ptr_ = ptr;
}

template<typename T>
inline T *clone_ptr<T>::release()
{
	T *ptr = ptr_;
	ptr_ = 0;
	return ptr;
}

template<typename T>
inline T *clone_ptr<T>::get() const
{
	return ptr_;
}

template<typename T>
inline T *clone_ptr<T>::operator->()
{
	return ptr_;
}

template<typename T>
inline const T *clone_ptr<T>::operator->() const
{
	return ptr_;
}

template<typename T>
inline T &clone_ptr<T>::operator *()
{
	return *ptr_;
}

template<typename T>
inline const T &clone_ptr<T>::operator *() const
{
	return *ptr_;
}

template<typename T>
inline clone_ptr<T>::~clone_ptr()
{
	delete ptr_;
}

