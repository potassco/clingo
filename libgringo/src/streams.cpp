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

#include <gringo/streams.h>
#include <gringo/exceptions.h>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

Streams::Streams(const StringVec &files, StreamPtr constants)
{
	open(files, constants);
}

void Streams::open(const StringVec &files, StreamPtr constants)
{
	if(constants.get()) { appendStream(constants, "<constants>"); }
	foreach(const std::string &filename, files) { addFile(filename, false); }
	if (files.empty()) { addFile("-"); }
}

void Streams::addFile(const std::string &filename, bool relative)
{
	// handle stdin
	if(filename == "-")
	{
		appendStream(StreamPtr(new std::istream(std::cin.rdbuf())), "<stdin>");
	}
	// handle files
	else
	{
		std::string path = filename;
		// create path relative to current file
		if(relative)
		{
			boost::filesystem::path relpath(currentFilename()), child(filename);
			relpath.remove_leaf();
			if (!child.is_complete()) { relpath /= child; }
			else                      { relpath  = child; }
			if(boost::filesystem::exists(relpath)) { path = relpath.string(); }
		}
		// create and add stream
		appendStream(StreamPtr(new std::ifstream(path.c_str())), path);
	}
}
bool Streams::includeFile(const std::string &filename, bool relative) {
    std::string path = filename;
    // create path relative to current file
    if(relative)
    {
        boost::filesystem::path relpath(currentFilename()), child(filename);
        relpath.remove_leaf();
        if (!child.is_complete()) { relpath /= child; }
        else                      { relpath  = child; }
        if(boost::filesystem::exists(relpath)) { path = relpath.string(); }
    }
    // create and add stream
	if(!unique(filename)) { return false; }
	streams_.push_front(StreamPair(boost::make_shared<std::ifstream>(path.c_str()), filename));
	if(!streams_.front().first->good())
	{
		throw FileException(streams_.back().second.c_str());
	}
    return true;
}

void Streams::appendStream(StreamPtr stream, const std::string &name)
{
	if(!unique(name)) { return; }
	streams_.push_back(StreamPair(StreamPair::first_type(stream), name));
	if(!streams_.back().first->good())
	{
		throw FileException(streams_.back().second.c_str());
	}
}

bool Streams::next()
{
	streams_.pop_front();
	return !streams_.empty();
}

std::istream &Streams::currentStream()
{
	return *streams_.front().first;
}

const std::string &Streams::currentFilename()
{
	return streams_.front().second;
}

bool Streams::unique(const std::string &file)
{
	if(file.size() > 0 && file[0] == '<') { return files_.insert(file).second; }
	else { return files_.insert(boost::filesystem::system_complete(boost::filesystem::path(file)).string()).second; }
}
