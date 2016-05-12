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

class Streams
{
private:
	typedef std::set<std::string> StringSet;
	typedef std::pair<boost::shared_ptr<std::istream>, std::string> StreamPair;
	typedef std::list<StreamPair> StreamList;
public:
	typedef std::auto_ptr<std::istream> StreamPtr;

	Streams() { }
	Streams(const StringVec &files, StreamPtr constants = StreamPtr(0));

	void open(const StringVec &files, StreamPtr constants = StreamPtr(0));
	void addFile(const std::string &filename, bool relative = true);
	void appendStream(StreamPtr stream, const std::string &name);
    bool includeFile(const std::string &filename, bool relative);
	std::istream &currentStream();
	const std::string &currentFilename();
	bool next();

private:
	bool unique(const std::string &file);

	StringSet files_;
	StreamList streams_;

	Streams(const Streams&);
	Streams& operator=(const Streams&);
};

