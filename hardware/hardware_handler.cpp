#include <dlfcn.h>              // dl*
#include <sstream>

#include "control.h"
#include "datatypes.h"
#include "hardware_handler.h"
#include "util.h"

using std::string;

namespace hardware {

HardwareHandler::HardwareHandler(const hardware_id_t hardwareID, const hardwareControlID_t hardwareControlID, const struct HardwareParams& params) :
  Control(CONTROL_ID_HARDWARE),
	hardwareID(hardwareID),
	hardwareControlID(hardwareControlID),
	createHardware(NULL),
	destroyHardware(NULL),
	instance(NULL),
	dlhandle(NULL),
	params(params) {

  // generate symbol and library names
  char* error;
	string symbol = hardwareSymbols[hardwareID];
	std::stringstream ss;
	ss << "hardware/" << symbol << ".so";

  dlhandle = dlopen(ss.str().c_str(), RTLD_LAZY);
  if (!dlhandle) {
    xlog("Can not open library: %s", dlerror());
		return;
  }
  xlog("Hardware library %s loaded", symbol.c_str());

	// look for symbol create_*
  ss.str(std::string());
	ss << "create_" << symbol;
	const char* s = ss.str().c_str();
  createHardware_t* new_create_hardware = (createHardware_t*)dlsym(dlhandle, s);
  error = dlerror();
  if (error) {
    xlog("Unable to find symbol %s", s);
		return;
  }

	// look for symbol destroy_*
  ss.str(std::string());
	ss << "destroy_" << symbol;
	s = ss.str().c_str();
  destroyHardware_t* new_destroy_hardware = (destroyHardware_t*)dlsym(dlhandle, ss.str().c_str());
  error = dlerror();
  if (error) {
    xlog("Unable to find symbol %s", s);
		return;
  }

	// register  valid symbols
	createHardware = new_create_hardware;
	destroyHardware = new_destroy_hardware;

	// start control
	if (createHardware) {
		instance = createHardware(params);
	}

	return;
}

HardwareHandler::~HardwareHandler() {
	// stop control
	if (instance) {
		destroyHardware(instance);
		instance = NULL;
	}
	// close library
	if (dlhandle) {
		dlclose(dlhandle);
		dlhandle = NULL;
	}
  xlog("Hardware library %s unloaded", hardwareSymbols[hardwareID].c_str());
}

std::string HardwareHandler::getName() const {
	if (instance) {
		return instance->getName();
	}
	return "Unknown, not running";
}

void HardwareHandler::go(const controlID_t controlID) {
  if (controlID != CONTROL_ID_HARDWARE) {
		instance->go();
	}
}

void HardwareHandler::stop(const controlID_t controlID) {
  if (controlID != CONTROL_ID_HARDWARE) {
		instance->stop();
	}
}

void HardwareHandler::locoSpeed(const controlID_t controlID, const locoID_t locoID, const speed_t speed) {
  if (controlID != CONTROL_ID_HARDWARE) {
    //hardwareControlID_t hardwareControlID = 0;
    protocol_t protocol = PROTOCOL_DCC;
    address_t address = 1028;
	/*
    manager.getProtocolAddress(locoID, hardwareControlID, protocol, address);
		*/
		instance->locoSpeed(protocol, address, speed);
  }
}

} // namespace hardware