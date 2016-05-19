// 
// Copyright (c) 2009, Benjamin Kaufmann
// 
// This file is part of gringo. See http://www.cs.uni-potsdam.de/gringo/ 
// 
// gringo is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with gringo; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#include "gringo_app.h"
#include <iostream>
/////////////////////////////////////////////////////////////////////////////////////////
// main - entry point
/////////////////////////////////////////////////////////////////////////////////////////
// #define CHECK_HEAP
int main(int argc, char** argv) {
	try {
		return gringo::Application::instance().run(argc, argv);
	} 
	catch (...) {
		std::cerr << "\n*** INTERNAL ERROR: please contact the authors of gringo" << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
