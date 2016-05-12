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

#include <gringo/gringo.h>
#include <gringo/printer.h>

class Output
{
private:
	typedef std::map<Signature, bool> ShowMap;
	typedef std::set<Signature> ExternalSet;
	typedef boost::ptr_vector<boost::nullable<Printer> > PrinterVec;
public:
	Output(std::ostream *out);
	virtual void initialize() { }
	virtual void finalize() { }
	virtual void addDomain(Domain *d) { (void)d; }
	std::ostream &out() const { return *out_; }
	Storage *storage() const { return s_; }
	void storage(Storage *s) { s_ = s; }
	void show(bool s);
	void show(uint32_t nameId, uint32_t arity, bool s);
	virtual void doShow(bool s) { (void) s; }
	virtual void doShow(uint32_t nameId, uint32_t arity, bool s) { (void)nameId; (void)arity; (void) s; }
	bool show(uint32_t nameId, uint32_t arity);
	void external(uint32_t nameId, uint32_t arity);
	template<class P>
	static bool expPrinter();
	template<class T>
	T *printer();
	template<class P, class B, class O>
	static bool regPrinter();
	virtual ~Output() { }
protected:
	template<class T>
	void initPrinters();
private:
	template<class T>
	struct PrinterFactory;
	template<class T>
	static PrinterFactory<T> &printerFactory();
protected:
	std::ostream *out_;
	ShowMap       showMap_;
	ExternalSet   external_;
	Storage      *s_;
	bool          show_;
//private:
public:
	PrinterVec    printers_;
};

template<class T>
inline T *Output::printer()
{
	assert(Printer::printer<T>() < printers_.size() && !printers_.is_null(Printer::printer<T>()) && "no printer registered!");
	return static_cast<T *>(&printers_[Printer::printer<T>()]);
}

template<class T>
struct Output::PrinterFactory
{
	typedef std::pair<uint32_t, Printer *(*)(T*)> Func;
	typedef std::vector<Func> FuncVec;
	FuncVec init;
};

template<class T>
inline typename Output::PrinterFactory<T> &Output::printerFactory()
{
	static PrinterFactory<T> factory;
	return factory;
}

template<class T>
inline void Output::initPrinters()
{
	size_t printers = Printer::printers(false);
	printers_.reserve(printers);
	for(size_t i = 0; i < printers; i++) printers_.push_back(0);
	foreach(typename PrinterFactory<T>::Func &i, printerFactory<T>().init)
		printers_.replace(i.first, i.second(static_cast<T*>(this))).release();
}

template<class P, class B, class O>
inline bool Output::regPrinter()
{
	Output::printerFactory<O>().init.push_back(std::make_pair(Printer::printer<B>(), &Printer::create<P, O>));
	return true;
}

#define GRINGO_REGISTER_PRINTER(P,B,O) \
template<> bool Output::expPrinter<P>() { return Output::regPrinter<P, B, O>(); }

#define GRINGO_EXPORT_PRINTER(P) \
static bool gringo_exported_ ## P = Output::expPrinter<class P>();

