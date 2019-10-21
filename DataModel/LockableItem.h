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

#pragma once

#include <map>
#include <mutex>

#include "DataTypes.h"

namespace DataModel
{
	class LockableItem
	{
		public:
			enum lockState_t : unsigned char
			{
				LockStateFree = 0,
				LockStateReserved,
				LockStateSoftLocked,
				LockStateHardLocked
			};

			LockableItem()
			:	lockState(LockStateFree),
			 	locoID(LocoNone)
			{
			}

			std::string Serialize() const;
			bool Deserialize(const std::map<std::string,std::string> arguments);


			locoID_t GetLoco() const { return locoID; }
			lockState_t GetLockState() const { return lockState; }
			virtual bool Reserve(const locoID_t locoID);
			virtual bool Lock(const locoID_t locoID);
			virtual bool Release(const locoID_t locoID);

			bool IsInUse() const { return lockState != LockStateFree || locoID != LocoNone; }


		private:
			std::mutex lockMutex;
			lockState_t lockState;
			locoID_t locoID;
	};
} // namespace DataModel
