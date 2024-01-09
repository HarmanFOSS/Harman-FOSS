/*
Copyright (C) 2012 Intel Corporation

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "debugout.h"

using namespace std;

int DebugOut::debugThreshhold = 0;
std::streambuf * DebugOut::buf = cout.rdbuf();
bool DebugOut::throwErr = false;
bool DebugOut::throwWarn = false;

const int DebugOut::Error = 1 << 16;
const int DebugOut::Warning = 1 << 24;

void debugOut(const string &message)
{
	DebugOut()<<"DEBUG: "<<message.c_str()<<endl;
}


void amb::deprecateMethod(const string & methodName, const string &version)
{
	DebugOut(DebugOut::Warning) << methodName << " is deprecated in " << version << endl;

	if(version == PROJECT_SERIES)
	{
		throw std::runtime_error("Using deprecated function.");
	}
}
