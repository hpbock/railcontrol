#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "datatypes.h"
#include "layout_item.h"
#include "serializable.h"
#include "street.h"

namespace datamodel {

	class Block : public LayoutItem {
		public:
			Block(const blockID_t blockID, const std::string& name, const layoutPosition_t x, const layoutPosition_t y, const layoutPosition_t z, const layoutItemSize_t width, const layoutRotation_t rotation);
			Block(const std::string& serialized);

			std::string serialize() const override;
			bool deserialize(const std::string& serialized) override;
			virtual std::string layoutType() const override { return "block"; };

			bool reserve(const locoID_t locoID);
			bool lock(const locoID_t locoID);
			bool release(const locoID_t locoID);
			locoID_t getLoco() const { return locoID; }
			lockState_t getState() const { return lockState; }

			bool addStreet(Street* street);
			bool removeStreet(Street* street);

			bool getValidStreets(std::vector<Street*>& validStreets);

			bool isInUse() const;

		private:
			lockState_t lockState;
			locoID_t locoID;
			direction_t locoDirection;
			std::mutex updateMutex;
			std::vector<Street*> streets;
	};

	inline bool Block::isInUse() const {
		return this->lockState != LockStateFree || this->locoID != LocoNone || this->streets.size() > 0;
	}

} // namespace datamodel

