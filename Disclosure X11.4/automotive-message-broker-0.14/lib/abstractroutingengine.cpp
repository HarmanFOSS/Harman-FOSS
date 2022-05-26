/*
  Copyright (C) 2012  Intel Corporation

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


#include "abstractroutingengine.h"



AbstractRoutingEngine::~AbstractRoutingEngine()
{
}

AsyncPropertyReply::AsyncPropertyReply()
	:AsyncPropertyRequest(), value(nullptr), success(false), timedout(nullptr), timeoutSource(nullptr)
{
	setTimeout();
}

AsyncPropertyReply::AsyncPropertyReply(const AsyncPropertyRequest &request)
	:AsyncPropertyRequest(request), value(NULL), success(false), timedout(nullptr), timeoutSource(nullptr)
{
	setTimeout();
}

AsyncPropertyReply::AsyncPropertyReply(const AsyncSetPropertyRequest &request)
	:AsyncPropertyRequest(request), value(request.value), success(false), timedout(nullptr), timeoutSource(nullptr)
{
	setTimeout();
	if(value)
		value->zone = request.zoneFilter;
}

AsyncPropertyReply::~AsyncPropertyReply()
{
	if(timeoutSource)
	{
		g_source_destroy(timeoutSource);
		g_source_unref(timeoutSource);
	}
}

void AsyncPropertyReply::setTimeout()
{
	auto timeoutfunc = [](gpointer userData) {
		AsyncPropertyReply* thisReply = static_cast<AsyncPropertyReply*>(userData);
		if(thisReply->success == false)
		{
			thisReply->error = Timeout;
			if(thisReply->timedout)
				thisReply->timedout(thisReply);
			if(thisReply->completed)
				thisReply->completed(thisReply);
		}
		return 0;
	};

	if(timeout)
	{
		timeoutSource = g_timeout_source_new(timeout);
		g_source_set_callback(timeoutSource,(GSourceFunc) timeoutfunc, this, nullptr);
		g_source_attach(timeoutSource, nullptr);
	}
}
