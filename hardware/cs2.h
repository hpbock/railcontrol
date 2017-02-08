#ifndef HARDWARE_CS2_H
#define HARDWARE_CS2_H

#include <cstring>
#include <string>
#include <thread>

#include "hardware_interface.h"
#include "hardware_params.h"

namespace hardware {

	class CS2: HardwareInterface {
		public:
			CS2(const HardwareParams &params);
			~CS2();
			std::string getName() const override;
			void go() override;
			void stop() override;
			void locoSpeed(protocol_t protocol, address_t address, speed_t speed) override;
		private:
			void createCommandHeader(char* buffer, const unsigned char prio, const unsigned char command, const unsigned char response, const unsigned char length);
			void createLocID(char* buffer, const protocol_t protocol, address_t address);
			void receiver();
			void sender();
			volatile unsigned char run;
			std::string name;
			struct sockaddr_in sockaddr_inSender;
			int senderSocket;
			std::thread senderThread;
			static const unsigned short hash = 0x7337;
	};

} // namespace

#endif // HARDWARE_CS2_H

