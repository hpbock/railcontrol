
#include <map>
#include <sstream>
#include <string>

#include <datamodel/track.h>

using std::map;
using std::stoi;
using std::string;

namespace datamodel {

	Track::Track(const trackID_t trackID,
		const std::string& name,
		const layoutPosition_t x,
		const layoutPosition_t y,
		const layoutPosition_t z,
		const layoutItemSize_t width,
		const layoutRotation_t rotation)
	:	LayoutItem(trackID, name, x, y, z, width, Height1, rotation),
		lockState(LockStateFree) /* FIXME */,
	 	locoID(0) /* FIXME */,
	 	locoDirection(DirectionLeft)
	{
	}

	Track::Track(const std::string& serialized)
	{
		deserialize(serialized);
	}

	std::string Track::serialize() const
	{
		std::stringstream ss;
		ss << "objectType=Track;" << LayoutItem::serialize() << ";lockState=" << static_cast<int>(lockState) << ";locoID=" << (int) locoID << ";locoDirection=" << static_cast<int>(locoDirection);
		return ss.str();
	}

	bool Track::deserialize(const std::string& serialized)
	{
		map<string, string> arguments;
		parseArguments(serialized, arguments);
		LayoutItem::deserialize(arguments);
		if (arguments.count("objectType") && arguments.at("objectType").compare("Track") == 0)
		{
			lockState = static_cast<lockState_t>(GetIntegerMapEntry(arguments, "lockState", LockStateFree));
			locoID = GetIntegerMapEntry(arguments, "locoID", LocoNone);
			locoDirection = static_cast<direction_t>(GetBoolMapEntry(arguments, "locoDirection", DirectionLeft));
			return true;
		}
		return false;
	}

	bool Track::reserve(const locoID_t locoID)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		if (locoID == this->locoID)
		{
			if (lockState == LockStateFree)
			{
				lockState = LockStateReserved;
			}
			return true;
		}
		if (lockState != LockStateFree)
		{
			return false;
		}
		lockState = LockStateReserved;
		this->locoID = locoID;
		return true;
	}

	bool Track::lock(const locoID_t locoID)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		if (lockState != LockStateReserved)
		{
			return false;
		}
		if (this->locoID != locoID)
		{
			return false;
		}
		lockState = LockStateHardLocked;
		return true;
	}

	bool Track::release(const locoID_t locoID)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		if (lockState == LockStateFree)
		{
			return true;
		}
		if (this->locoID != locoID)
		{
			return false;
		}
		this->locoID = LocoNone;
		lockState = LockStateFree;
		return true;
	}

	bool Track::addStreet(Street* street)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		for (auto s : streets)
		{
			if (s == street)
			{
				return false;
			}
		}
		streets.push_back(street);
		return true;
	}

	bool Track::removeStreet(Street* street)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		/* FIXME */
		return false;
	}

	bool Track::getValidStreets(std::vector<Street*>& validStreets)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		for (auto street : streets)
		{
			if (street->fromTrackDirection(objectID, locoDirection))
			{
				validStreets.push_back(street);
			}
		}
		return true;
	}
} // namespace datamodel