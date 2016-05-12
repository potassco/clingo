// 
// Copyright (c) 2015, Benjamin Kaufmann
// 
// This file is part of Potassco. See http://potassco.sourceforge.net/
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 

#include <potassco/aspif.h>
#include <potassco/convert.h>
#include <program_opts/application.h>
#include <program_opts/typed_value.h>
#include <fstream>
#include <iostream>
#include <cctype>

using namespace ProgramOptions;

class LpConvert : public ProgramOptions::Application {
public:
	virtual const char* getName()       const { return "lpconvert"; }
	virtual const char* getVersion()    const { return "1.0.0"; }
	virtual PosOption   getPositional() const { return &positional; }
	virtual const char* getUsage()      const {
		return
			"[options] [<file>]\n"
			"Convert program in <file> or standard input";
	}
	virtual void initOptions(OptionContext& root);
	virtual void validateOptions(const OptionContext&, const ParsedOptions&, const ParsedValues&) {}
	virtual void setup() {}
	virtual void run();
private:
	static bool positional(const std::string&, std::string& optOut) {
		optOut = "input";
		return true;
	}
	static int error(int line, const char* what) {
		fprintf(stderr, "*** ERROR: In line %d: %s\n", line, what);
		static_cast<LpConvert*>(Application::getInstance())->exit(EXIT_FAILURE);
		return EXIT_FAILURE;
	}
	std::string input_;
	std::string output_;
	bool potassco_;
	bool filter_;
};

void LpConvert::initOptions(OptionContext& root) {
	OptionGroup convert("Conversion Options");
	convert.addOptions()
		("input,i,@2", storeTo(input_),  "Input file")
		("potassco,p", flag(potassco_ = false), "Enable potassco extensions")
		("filter,f", flag(filter_ = false), "Hide converted potassco predicates")
		("output,o", storeTo(output_)->arg("<file>"), "Write output to <file> (default: stdout)")
	;
	root.add(convert);
}
void LpConvert::run() {
	std::ifstream iFile;
	std::ofstream oFile;
	if (!input_.empty() && input_ != "-") {
		iFile.open(input_.c_str());
		if (!iFile.is_open()) { throw std::runtime_error("Could not open input file!"); }
	}
	if (!output_.empty() && output_ != "-") {
		if (input_ == output_) { throw std::runtime_error("Input and output must be different!"); }
		oFile.open(output_.c_str());
		if (!oFile.is_open()) { throw std::runtime_error("Could not open output file!"); }
	}
	std::istream& in = iFile.is_open() ? iFile : std::cin;
	std::ostream& os = oFile.is_open() ? oFile : std::cout;
	if (in.peek() == 'a') {
		Potassco::SmodelsOutput writer(os, potassco_);
		Potassco::SmodelsConvert converter(writer, potassco_);
		Potassco::readAspif(in, converter, &error);
	}
	else if (std::isdigit(in.peek())) {
		Potassco::AspifOutput out(os);
		Potassco::SmodelsInput::Options opts;
		if (potassco_) {
			opts.enableClaspExt().convertEdges().convertHeuristic();
			if (filter_) { opts.dropConverted(); }
		}
		Potassco::readSmodels(in, out, &error, opts);
	}
	else {
		throw std::runtime_error("Unrecognized input format!");
	}
	iFile.close();
	oFile.close();
}

int main(int argc, char** argv) {
	LpConvert app;
	return app.main(argc, argv);
}
