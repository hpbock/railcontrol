#include <algorithm>
#include <cstring>		//memset
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "datatypes.h"
#include "railcontrol.h"
#include "text/converters.h"
#include "util.h"
#include "webserver/webclient.h"
#include "webserver/webserver.h"

using std::map;
using std::thread;
using std::string;
using std::stringstream;
using std::vector;

namespace webserver {

	WebServer::WebServer(Manager& manager, const unsigned short port)
	:	CommandInterface(ControlTypeWebserver),
		Network::TcpServer(port),
		run(false),
		lastClientID(0),
		manager(manager),
		updateID(1)
	{
		updates[updateID] = "data: status=Railcontrol started";

		run = true;
	}

	WebServer::~WebServer()
	{
		if (run == false)
		{
			return;
		}
		xlog("Stopping webserver");
		{
			std::lock_guard<std::mutex> lock(updateMutex);
			updates[++updateID] = "data: status=Stopping Railcontrol";
		}
		sleep(1);
		run = false;

		// stopping all clients
		for (auto client : clients)
		{
			client->stop();
		}

		// delete all client memory
		while (clients.size())
		{
			WebClient* client = clients.back();
			clients.pop_back();
			delete client;
		}
	}

	void WebServer::Work(Network::TcpConnection* connection)
	{
		clients.push_back(new WebClient(++lastClientID, connection, *this, manager));
	}

	void WebServer::booster(const controlType_t managerID, const boosterStatus_t status)
	{
		if (status)
		{
			addUpdate("booster;on=true", "Booster is on");
		}
		else
		{
			addUpdate("booster;on=false", "Booster is off");
		}
	}

	void WebServer::locoSpeed(const controlType_t managerID, const locoID_t locoID, const LocoSpeed speed)
	{
		stringstream command;
		stringstream status;
		command << "locospeed;loco=" << locoID << ";speed=" << speed;
		status << manager.getLocoName(locoID) << " speed is " << speed;
		addUpdate(command.str(), status.str());
	}

	void WebServer::locoDirection(const controlType_t managerID, const locoID_t locoID, const direction_t direction)
	{
		stringstream command;
		stringstream status;
		command << "locodirection;loco=" << locoID << ";direction=" << (direction ? "true" : "false");
		status << manager.getLocoName(locoID) << " direction is " << (direction ? "right" : "left");
		addUpdate(command.str(), status.str());
	}

	void WebServer::locoFunction(const controlType_t managerID, const locoID_t locoID, const function_t function, const bool state)
	{
		stringstream command;
		stringstream status;
		command << "locofunction;loco=" << locoID << ";function=" << (unsigned int) function << ";on=" << (state ? "true" : "false");
		status << manager.getLocoName(locoID) << " f" << (unsigned int) function << " is " << (state ? "on" : "off");
		addUpdate(command.str(), status.str());
	}

	void WebServer::accessory(const controlType_t managerID, const accessoryID_t accessoryID, const accessoryState_t state, const bool on)
	{
		if (on == false)
		{
			return;
		}
		stringstream command;
		stringstream status;
		string stateText;
		text::Converters::accessoryStatus(state, stateText);
		command << "accessory;accessory=" << accessoryID << ";state=" << stateText;
		status << manager.getAccessoryName(accessoryID) << " is " << stateText;
		addUpdate(command.str(), status.str());
	}

	void WebServer::accessorySettings(const accessoryID_t accessoryID, const std::string& name, const layoutPosition_t posX, const layoutPosition_t posY, const layoutPosition_t posZ)
	{
		stringstream command;
		stringstream status;
		command << "accessorysettings;accessory=" << accessoryID;
		status << name << " updated";
		addUpdate(command.str(), status.str());
	}

	void WebServer::accessoryDelete(const accessoryID_t accessoryID, const std::string& name)
	{
		stringstream command;
		stringstream status;
		command << "accessorydelete;accessory=" << accessoryID;
		status << name << " deleted";
		addUpdate(command.str(), status.str());
	}

	void WebServer::feedback(const controlType_t managerID, const feedbackPin_t pin, const feedbackState_t state)
	{
		stringstream command;
		stringstream status;
		command << "feedback;pin=" << pin << ";state=" << (state ? "on" : "off");
		status << "Feedback " << pin << " is " << (state ? "on" : "off");
		addUpdate(command.str(), status.str());
	}

	void WebServer::track(const controlType_t managerID, const trackID_t trackID, const lockState_t state)
	{
		stringstream command;
		stringstream status;
		string stateText;
		text::Converters::lockStatus(state, stateText);
		command << "track;track=" << trackID << ";state=" << stateText;
		status << manager.getTrackName(trackID) << " is " << stateText;
		addUpdate(command.str(), status.str());
	}

	void WebServer::handleSwitch(const controlType_t managerID, const switchID_t switchID, const switchState_t state, const bool on)
	{
		if (on == false)
		{
			return;
		}
		stringstream command;
		stringstream status;
		string stateText;
		text::Converters::switchStatus(state, stateText);
		command << "switch;switch=" << switchID << ";state=" << stateText;
		status << manager.getSwitchName(switchID) << " is " << stateText;
		addUpdate(command.str(), status.str());
	}

	void WebServer::switchSettings(const switchID_t switchID, const std::string& name, const layoutPosition_t posX, const layoutPosition_t posY, const layoutPosition_t posZ, const string rotation)
	{
		stringstream command;
		stringstream status;
		command << "switchsettings;switch=" << switchID;
		status << name << " updated";
		addUpdate(command.str(), status.str());
	}

	void WebServer::switchDelete(const switchID_t switchID, const std::string& name)
	{
		stringstream command;
		stringstream status;
		command << "switchdelete;switch=" << switchID;
		status << name << " deleted";
		addUpdate(command.str(), status.str());
	}

	void WebServer::locoIntoTrack(const locoID_t locoID, const trackID_t trackID)
	{
		stringstream command;
		stringstream status;
		command << "locoIntoTrack;loco=" << locoID << ";track=" << trackID;
		status << manager.getLocoName(locoID) << " is on track " << manager.getTrackName(trackID);
		addUpdate(command.str(), status.str());
	}

	void WebServer::locoRelease(const locoID_t locoID)
	{
		stringstream command;
		stringstream status;
		command << "locoRelease;loco=" << locoID;
		status << manager.getLocoName(locoID) << " is not on a track anymore";
		addUpdate(command.str(), status.str());
	}
	;

	void WebServer::trackRelease(const trackID_t trackID)
	{
		stringstream command;
		stringstream status;
		command << "trackRelease;track=" << trackID;
		status << manager.getTrackName(trackID) << " is released";
		addUpdate(command.str(), status.str());
	}
	;

	void WebServer::streetRelease(const streetID_t streetID)
	{
		stringstream command;
		stringstream status;
		command << "streetRelease;street=" << streetID;
		status << manager.getStreetName(streetID) << " is  released";
		addUpdate(command.str(), status.str());
	}
	;

	void WebServer::locoStreet(const locoID_t locoID, const streetID_t streetID, const trackID_t trackID)
	{
		stringstream command;
		stringstream status;
		command << "locoStreet;loco=" << locoID << ";street=" << streetID << ";track=" << trackID;
		status << manager.getLocoName(locoID) << " runs on street " << manager.getStreetName(streetID) << " with destination track " << manager.getTrackName(trackID);
		addUpdate(command.str(), status.str());
	}

	void WebServer::locoDestinationReached(const locoID_t locoID, const streetID_t streetID, const trackID_t trackID)
	{
		stringstream command;
		stringstream status;
		command << "locoDestinationReached;loco=" << locoID << ";street=" << streetID << ";track=" << trackID;
		status << manager.getLocoName(locoID) << " has reached the destination track " << manager.getTrackName(trackID) << " on street " << manager.getStreetName(streetID);
		addUpdate(command.str(), status.str());
	}

	void WebServer::locoStart(const locoID_t locoID)
	{
		stringstream command;
		stringstream status;
		command << "locoStart;loco=" << locoID;
		status << manager.getLocoName(locoID) << " is in auto mode";
		addUpdate(command.str(), status.str());
	}

	void WebServer::locoStop(const locoID_t locoID)
	{
		stringstream command;
		stringstream status;
		command << "locoStop;loco=" << locoID;
		status << manager.getLocoName(locoID) << " is in manual mode";
		addUpdate(command.str(), status.str());
	}

	void WebServer::addUpdate(const string& command, const string& status)
	{
		stringstream ss;
		ss << "data: command=" << command << ";status=" << status << "\r\n\r\n";
		std::lock_guard<std::mutex> lock(updateMutex);
		updates[++updateID] = ss.str();
		updates.erase(updateID - MaxUpdates);
	}

	bool WebServer::nextUpdate(unsigned int& updateIDClient, string& s)
	{
		std::lock_guard<std::mutex> lock(updateMutex);

		if (updateIDClient + MaxUpdates <= updateID)
		{
			updateIDClient = updateID - MaxUpdates + 1;
		}

		if (updates.count(updateIDClient) == 1)
		{
			s = updates.at(updateIDClient);
			return true;
		}

		return false;
	}

}; // namespace webserver
