/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2019 Dominik (Teddy) Mahrer - www.railcontrol.org

RailControl is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

RailControl is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RailControl; see the file LICENCE. If not see
<http://www.gnu.org/licenses/>.
*/

#include <map>

#include "DataModel/Signal.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::map;
using std::stringstream;
using std::string;

namespace DataModel
{
	std::string Signal::Serialize() const
	{
		return "objectType=Signal;" + Accessory::SerializeWithoutType();
	}

	bool Signal::Deserialize(const std::string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("Signal") != 0)
		{
			return false;
		}

		return Accessory::Deserialize(arguments);
	}

	bool Signal::Release(const locoID_t locoID)
	{
		bool ret = LockableItem::Release(locoID);
		if (ret == false)
		{
			return false;
		}
		manager->SignalState(ControlTypeInternal, this, SignalStateRed, true);
		return true;
	}
} // namespace DataModel
