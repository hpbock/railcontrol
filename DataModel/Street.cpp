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
#include <sstream>

#include "DataModel/Loco.h"
#include "DataModel/Relation.h"
#include "DataModel/Street.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::map;
using std::stringstream;
using std::string;

namespace DataModel
{
	Street::Street(Manager* manager, const std::string& serialized)
	:	LockableItem(),
	 	manager(manager)
	{
		Deserialize(serialized);
		Track* track = manager->GetTrack(fromTrack);
		if (track == nullptr)
		{
			return;
		}
		track->AddStreet(this);
	}

	std::string Street::Serialize() const
	{
		stringstream ss;
		ss << "objectType=Street;"
			<< LayoutItem::Serialize()
			<< ";" << LockableItem::Serialize()
			<< ";delay=" << static_cast<int>(delay)
			<< ";pushpull=" << static_cast<int>(pushpull)
			<< ";mintrainlength=" << static_cast<int>(minTrainLength)
			<< ";maxtrainlength=" << static_cast<int>(maxTrainLength)
			<< ";lastused=" << lastUsed
			<< ";counter=" << counter
			<< ";automode=" << static_cast<int>(automode)
			<< ";fromTrack=" << static_cast<int>(fromTrack)
			<< ";fromDirection=" << static_cast<int>(fromDirection)
			<< ";toTrack=" << static_cast<int>(toTrack)
			<< ";toDirection=" << static_cast<int>(toDirection)
			<< ";feedbackIdReduced=" << static_cast<int>(feedbackIdReduced)
			<< ";feedbackIdCreep=" << static_cast<int>(feedbackIdCreep)
			<< ";feedbackIdStop=" << static_cast<int>(feedbackIdStop)
			<< ";feedbackIdOver=" << static_cast<int>(feedbackIdOver)
			<< ";waitafterrelease=" << static_cast<int>(waitAfterRelease);
		return ss.str();
	}

	bool Street::Deserialize(const std::string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("Street") != 0)
		{
			return false;
		}

		LayoutItem::Deserialize(arguments);
		LockableItem::Deserialize(arguments);

		delay = static_cast<delay_t>(Utils::Utils::GetIntegerMapEntry(arguments, "delay", DefaultDelay));
		lastUsed = Utils::Utils::GetIntegerMapEntry(arguments, "lastused", 0);
		counter = Utils::Utils::GetIntegerMapEntry(arguments, "counter", 0);
		automode = static_cast<automode_t>(Utils::Utils::GetBoolMapEntry(arguments, "automode", AutomodeNo));
		if (automode == AutomodeNo)
		{
			return true;
		}
		fromTrack = Utils::Utils::GetIntegerMapEntry(arguments, "fromTrack", TrackNone);
		fromDirection = static_cast<direction_t>(Utils::Utils::GetBoolMapEntry(arguments, "fromDirection", DirectionRight));
		toTrack = Utils::Utils::GetIntegerMapEntry(arguments, "toTrack", TrackNone);
		toDirection = static_cast<direction_t>(Utils::Utils::GetBoolMapEntry(arguments, "toDirection", DirectionLeft));
		feedbackIdReduced = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackIdReduced", FeedbackNone);
		feedbackIdCreep = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackIdCreep", FeedbackNone);
		feedbackIdStop = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackIdStop", FeedbackNone);
		feedbackIdOver = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackIdOver", FeedbackNone);
		pushpull = static_cast<pushpullType_t>(Utils::Utils::GetIntegerMapEntry(arguments, "commuter", PushpullTypeBoth)); // FIXME: remove later
		pushpull = static_cast<pushpullType_t>(Utils::Utils::GetIntegerMapEntry(arguments, "pushpull", pushpull));
		minTrainLength = static_cast<length_t>(Utils::Utils::GetIntegerMapEntry(arguments, "mintrainlength", 0));
		maxTrainLength = static_cast<length_t>(Utils::Utils::GetIntegerMapEntry(arguments, "maxtrainlength", 0));
		waitAfterRelease = Utils::Utils::GetIntegerMapEntry(arguments, "waitafterrelease", 0);
		return true;
	}

	void Street::DeleteRelations()
	{
		while (!relations.empty())
		{
			delete relations.back();
			relations.pop_back();
		}
	}

	bool Street::AssignRelations(const std::vector<DataModel::Relation*>& newRelations)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		if (GetLockState() != LockStateFree)
		{
			return false;
		}
		DeleteRelations();
		relations = newRelations;
		return true;
	}

	bool Street::FromTrackDirection(Logger::Logger* logger, const trackID_t trackID, const direction_t trackDirection, const Loco* loco, const bool allowLocoTurn)
	{
		if (automode == false)
		{
			return false;
		}

		if (fromTrack != trackID)
		{
			return false;
		}

		const length_t locoLength = loco->GetLength();
		if (locoLength < minTrainLength)
		{
			logger->Debug(Languages::TextTrainIsToShort, GetName());
			return false;
		}
		if (maxTrainLength > 0 && locoLength > maxTrainLength)
		{
			logger->Debug(Languages::TextTrainIsToLong, GetName());
			return false;
		}

		const bool locoPushpull = loco->GetPushpull();
		if (pushpull != locoPushpull && pushpull != PushpullTypeBoth)
		{
			logger->Debug(Languages::TextDifferentPushpullTypes, GetName());
			return false;
		}

		if (allowLocoTurn == true && locoPushpull == true)
		{
			return true;
		}

		bool equalDirection = (fromDirection == trackDirection);
		if (equalDirection)
		{
			return true;
		}
		else
		{
			logger->Debug(Languages::TextDifferentDirections, GetName());
			return false;
		}
	}


	bool Street::Execute()
	{
		if (manager->Booster() == BoosterStop)
		{
			return false;
		}

		std::lock_guard<std::mutex> Guard(updateMutex);
		for (auto relation : relations)
		{
			bool retRelation = relation->Execute(delay);
			if (retRelation == false)
			{
				Logger::Logger* logger = Logger::Logger::GetLogger("street");
				logger->Debug("Unable to execute {0}", GetName());
				return false;
			}
		}
		lastUsed = time(nullptr);
		++counter;
		return true;
	}

	bool Street::Reserve(const locoID_t locoID)
	{
		if (manager->Booster() == BoosterStop)
		{
			return false;
		}

		Logger::Logger* logger = Logger::Logger::GetLogger("street");
		std::lock_guard<std::mutex> Guard(updateMutex);
		bool ret = LockableItem::Reserve(locoID);
		if (ret == false)
		{
			logger->Debug("Unable to reserve {0}", GetName());
			return false;
		}

		if (automode == AutomodeYes)
		{
			Track* track = manager->GetTrack(toTrack);
			if (track == nullptr || track->Reserve(locoID) == false)
			{
				logger->Debug("Unable to reserve {0}", track ? track->GetName() : "missing to track");
				ReleaseInternal(locoID);
				return false;
			}
		}

		for (auto relation : relations)
		{
			bool retRelation = relation->Reserve(locoID);
			if (retRelation == false)
			{
				logger->Debug("Unable to reserve relation");
				ReleaseInternal(locoID);
				return false;
			}
		}
		return true;
	}

	bool Street::Lock(const locoID_t locoID)
	{
		if (manager->Booster() == BoosterStop)
		{
			return false;
		}

		Logger::Logger* logger = Logger::Logger::GetLogger("street");
		std::lock_guard<std::mutex> Guard(updateMutex);
		bool ret = LockableItem::Lock(locoID);
		if (ret == false)
		{
			logger->Debug("Unable to lock {0}", GetName());
			return false;
		}

		if (automode == AutomodeYes)
		{
			Track* track = manager->GetTrack(toTrack);
			if (track == nullptr || track->Lock(locoID) == false)
			{
				logger->Debug("Unable to lock {0}", track ? track->GetName() : "missing to track");
				ReleaseInternal(locoID);
				return false;
			}
		}

		for (auto relation : relations)
		{
			bool retRelation = relation->Lock(locoID);
			if (retRelation == false)
			{
				logger->Debug("Unable to lock relation");
				ReleaseInternalWithToTrack(locoID);
				return false;
			}
		}
		return true;
	}

	bool Street::Release(const locoID_t locoID)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		return ReleaseInternal(locoID);
	}

	bool Street::ReleaseInternal(const locoID_t locoID)
	{
		for (auto relation : relations)
		{
			relation->Release(locoID);
		}
		return LockableItem::Release(locoID);
	}

	void Street::ReleaseInternalWithToTrack(const locoID_t locoID)
	{
		Track* track = manager->GetTrack(toTrack);
		if (track != nullptr)
		{
			track->Release(locoID);
		}
		ReleaseInternal(locoID);
	}
} // namespace DataModel

