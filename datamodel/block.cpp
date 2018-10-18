
#include <map>
#include <sstream>
#include <string>

#include "datamodel/block.h"

using std::map;
using std::stoi;
using std::string;

namespace datamodel {

	Block::Block(const blockID_t blockID, const std::string& name, const layoutPosition_t x, const layoutPosition_t y, const layoutPosition_t z, const layoutItemSize_t width, const layoutRotation_t rotation) :
		LayoutItem(blockID, name, x, y, z, width, Height1, rotation),
		lockState(LockStateFree) /* FIXME */,
		locoID(0) /* FIXME */,
		locoDirection(DirectionLeft) {
	}

	Block::Block(const std::string& serialized) {
		deserialize(serialized);
	}

	std::string Block::serialize() const {
		std::stringstream ss;
		ss << "objectType=Block;" << LayoutItem::serialize() << ";lockState=" << static_cast<int>(lockState) << ";locoID=" << (int)locoID << ";locoDirection=" << static_cast<int>(locoDirection);
		return ss.str();
	}

	bool Block::deserialize(const std::string& serialized) {
		map<string,string> arguments;
		parseArguments(serialized, arguments);
		LayoutItem::deserialize(arguments);
		if (arguments.count("objectType") && arguments.at("objectType").compare("Block") == 0) {
			if (arguments.count("lockState")) lockState = static_cast<lockState_t>(stoi(arguments.at("lockState")));
			if (arguments.count("locoID")) locoID = stoi(arguments.at("locoID"));
			if (arguments.count("locoDirection")) locoDirection = static_cast<direction_t>(stoi(arguments.at("locoDirection")));
			return true;
		}
		return false;
	}

	bool Block::reserve(const locoID_t locoID) {
		std::lock_guard<std::mutex> Guard(updateMutex);
		if (locoID == this->locoID) {
			if (lockState == LockStateFree) {
				lockState = LockStateReserved;
			}
			return true;
		}
		if (lockState != LockStateFree) {
			return false;
		}
		lockState = LockStateReserved;
		this->locoID = locoID;
		return true;
	}

	bool Block::lock(const locoID_t locoID) {
		std::lock_guard<std::mutex> Guard(updateMutex);
		if (lockState != LockStateReserved) {
			return false;
		}
		if (this->locoID != locoID) {
			return false;
		}
		lockState = LockStateHardLocked;
		return true;
	}

	bool Block::release(const locoID_t locoID) {
		std::lock_guard<std::mutex> Guard(updateMutex);
		if (lockState == LockStateFree) {
			return true;
		}
		if (this->locoID != locoID) {
			return false;
		}
		this->locoID = LocoNone;
		lockState = LockStateFree;
		return true;
	}

	bool Block::addStreet(Street* street) {
		std::lock_guard<std::mutex> Guard(updateMutex);
		for(auto s : streets) {
			if (s == street) return false;
		}
		streets.push_back(street);
		return true;
	}

	bool Block::removeStreet(Street* street) {
		std::lock_guard<std::mutex> Guard(updateMutex);
		/* FIXME */
		return false;
	}

	bool Block::getValidStreets(std::vector<Street*>& validStreets) {
		std::lock_guard<std::mutex> Guard(updateMutex);
		for(auto street : streets) {
			if (street->fromBlockDirection(objectID, locoDirection)) {
				validStreets.push_back(street);
			}
		}
		return true;
	}
} // namespace datamodel
