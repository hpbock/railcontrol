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

#include <algorithm>
#include <cstring>		//memset
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "DataTypes.h"
#include "Languages.h"
#include "RailControl.h"
#include "WebServer/WebClient.h"
#include "WebServer/WebServer.h"

using std::map;
using std::thread;
using std::string;
using std::stringstream;
using std::vector;

namespace WebServer {

	WebServer::WebServer(Manager& manager, const unsigned short port)
	:	ControlInterface(ControlTypeWebserver),
		Network::TcpServer(port, "WebServer"),
		run(false),
		lastClientID(0),
		manager(manager),
		updateID(1)
	{
		Logger::Logger::GetLogger("Webserver")->Info(Languages::TextWebServerStarted);
		{
			std::lock_guard<std::mutex> lock(updateMutex);
			updates[updateID] = GetStatus(Languages::TextRailControlStarted);
		}
		run = true;
	}

	WebServer::~WebServer()
	{
		if (run == false)
		{
			return;
		}
		{
			std::lock_guard<std::mutex> lock(updateMutex);
			updates[++updateID] = GetStatus(Languages::TextStoppingRailControl);
		}
		TerminateTcpServer();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		run = false;

		// stopping all clients
		for (auto client : clients)
		{
			client->Stop();
		}

		// delete all client memory
		while (clients.size())
		{
			WebClient* client = clients.back();
			clients.pop_back();
			delete client;
		}
		Logger::Logger::GetLogger("Webserver")->Info(Languages::TextWebServerStopped);
	}

	void WebServer::Work(Network::TcpConnection* connection)
	{
		clients.push_back(new WebClient(++lastClientID, connection, *this, manager));
	}

	void WebServer::Booster(__attribute__((unused)) const controlType_t controlType, const boosterState_t status)
	{
		if (status)
		{
			AddUpdate("booster;on=true", Languages::TextTurningBoosterOn);
		}
		else
		{
			AddUpdate("booster;on=false", Languages::TextTurningBoosterOff);
		}
	}

	void WebServer::LocoSpeed(__attribute__((unused)) const controlType_t controlType, const locoID_t locoID, const locoSpeed_t speed)
	{
		stringstream command;
		command << "locospeed;loco=" << locoID << ";speed=" << speed;
		AddUpdate(command.str(), Languages::TextLocoSpeedIs, manager.GetLocoName(locoID), speed);
	}

	void WebServer::LocoDirection(__attribute__((unused)) const controlType_t controlType, const locoID_t locoID, const direction_t direction)
	{
		stringstream command;
		command << "locodirection;loco=" << locoID << ";direction=" << (direction ? "true" : "false");
		AddUpdate(command.str(), direction ? Languages::TextLocoDirectionIsRight : Languages::TextLocoDirectionIsLeft, manager.GetLocoName(locoID));
	}

	void WebServer::LocoFunction(__attribute__((unused)) const controlType_t controlType, const locoID_t locoID, const function_t function, const bool state)
	{
		stringstream command;
		command << "locofunction;loco=" << locoID << ";function=" << static_cast<unsigned int>(function) << ";on=" << (state ? "true" : "false");
		AddUpdate(command.str(), state ? Languages::TextLocoFunctionIsOn : Languages::TextLocoFunctionIsOff, manager.GetLocoName(locoID), function);
	}

	void WebServer::AccessoryState(__attribute__((unused)) const controlType_t controlType, const accessoryID_t accessoryID, const accessoryState_t state, const bool on)
	{
		if (on == false)
		{
			return;
		}
		stringstream command;
		command << "accessory;accessory=" << accessoryID << ";state=" << (state == DataModel::Accessory::AccessoryStateOn ? "green" : "red");
		AddUpdate(command.str(), state ? Languages::TextAccessoryStateIsGreen : Languages::TextAccessoryStateIsRed, manager.GetAccessoryName(accessoryID));
	}

	void WebServer::AccessorySettings(const accessoryID_t accessoryID, const std::string& name)
	{
		stringstream command;
		command << "accessorysettings;accessory=" << accessoryID;
		AddUpdate(command.str(), Languages::TextAccessoryUpdated, name);
	}

	void WebServer::AccessoryDelete(const accessoryID_t accessoryID, const std::string& name)
	{
		stringstream command;
		command << "accessorydelete;accessory=" << accessoryID;
		AddUpdate(command.str(), Languages::TextAccessoryDeleted, name);
	}

	void WebServer::FeedbackState(const std::string& name, const feedbackID_t feedbackID, const DataModel::Feedback::feedbackState_t state)
	{
		stringstream command;
		command << "feedback;feedback=" << feedbackID << ";state=" << (state ? "on" : "off");
		AddUpdate(command.str(), state ? Languages::TextFeedbackStateIsOn : Languages::TextFeedbackStateIsOff, name);
	}

	void WebServer::FeedbackSettings(const feedbackID_t feedbackID, const std::string& name)
	{
		stringstream command;
		command << "feedbacksettings;feedback=" << feedbackID;
		AddUpdate(command.str(), Languages::TextFeedbackUpdated, name);
	}

	void WebServer::FeedbackDelete(const feedbackID_t feedbackID, const std::string& name)
	{
		stringstream command;
		command << "feedbackdelete;feedback=" << feedbackID;
		AddUpdate(command.str(), Languages::TextFeedbackDeleted, name);
	}

	void WebServer::StreetSettings(const streetID_t streetID, const std::string& name)
	{
		stringstream command;
		command << "streetsettings;street=" << streetID;
		AddUpdate(command.str(), Languages::TextStreetUpdated, name);
	}

	void WebServer::StreetDelete(const streetID_t streetID, const std::string& name)
	{
		stringstream command;
		command << "streetdelete;street=" << streetID;
		AddUpdate(command.str(), Languages::TextStreetDeleted, name);
	}

	void WebServer::SwitchState(__attribute__((unused)) const controlType_t controlType, const switchID_t switchID, const switchState_t state, const bool on)
	{
		if (on == false)
		{
			return;
		}
		stringstream command;
		command << "switch;switch=" << switchID << ";state=" << (state ? "straight" : "turnout");
		AddUpdate(command.str(), state ? Languages::TextSwitchStateIsStraight : Languages::TextSwitchStateIsTurnout, manager.GetSwitchName(switchID));
	}

	void WebServer::SwitchSettings(const switchID_t switchID, const std::string& name)
	{
		stringstream command;
		command << "switchsettings;switch=" << switchID;
		AddUpdate(command.str(), Languages::TextSwitchUpdated, name);
	}

	void WebServer::SwitchDelete(const switchID_t switchID, const std::string& name)
	{
		stringstream command;
		command << "switchdelete;switch=" << switchID;
		AddUpdate(command.str(), Languages::TextSwitchDeleted, name);
	}

	void WebServer::TrackState(const trackID_t trackID, const std::string& name, const bool occupied, const bool blocked, const direction_t direction, const std::string& locoName)
	{
		stringstream command;
		const string occupiedText = (occupied ? "true" : "false");
		const string blockedText = (blocked ? "true" : "false");
		const bool reserved = locoName.length() > 0;
		const string reservedText = (reserved ? "true" : "false");
		const string directionText = (direction ? "true" : "false");
		command << "trackstate;track=" << trackID
			<< ";occupied=" << occupiedText
			<< ";reserved=" << reservedText
			<< ";blocked=" << blockedText
			<< ";direction=" << directionText
			<< ";loconame=" << locoName;

		if (blocked)
		{
			if (occupied)
			{
				AddUpdate(command.str(), Languages::TextTrackStatusIsBlockedAndOccupied, name, locoName);
			}
			else if (reserved)
			{
				AddUpdate(command.str(), Languages::TextTrackStatusIsBlockedAndReserved, name, locoName);;
			}
			else
			{
				AddUpdate(command.str(), Languages::TextTrackStatusIsBlocked, name);
			}
		}
		else
		{
			if (occupied)
			{
				AddUpdate(command.str(), Languages::TextTrackStatusIsOccupied, name, locoName);
			}
			else if (reserved)
			{
				AddUpdate(command.str(), Languages::TextTrackStatusIsReserved, name, locoName);;
			}
			else
			{
				AddUpdate(command.str(), Languages::TextTrackStatusIsFree, name);
			}
		}
	}

	void WebServer::TrackSettings(const trackID_t trackID, const std::string& name)
	{
		stringstream command;
		command << "tracksettings;track=" << trackID;
		AddUpdate(command.str(), Languages::TextTrackUpdated, name);
	}

	void WebServer::TrackDelete(const trackID_t trackID, const std::string& name)
	{
		stringstream command;
		command << "trackdelete;strack=" << trackID;
		AddUpdate(command.str(), Languages::TextTrackDeleted, name);
	}

	void WebServer::LocoIntoTrack(const locoID_t locoID, const trackID_t trackID, const string& locoName, const string& trackName)
	{
		stringstream command;
		command << "locointotrack;loco=" << locoID << ";track=" << trackID;
		AddUpdate(command.str(), Languages::TextLocoIsOnTrack, locoName, trackName);
	}

	void WebServer::SignalState(__attribute__((unused)) const controlType_t controlType, const signalID_t signalID, const signalState_t state, const bool on)
	{
		if (on == false)
		{
			return;
		}
		stringstream command;
		command << "signal;signal=" << signalID << ";state=" << (state ? "green" : "red");
		AddUpdate(command.str(), state ? Languages::TextSignalStateIsGreen : Languages::TextSignalStateIsRed, manager.GetSignalName(signalID));
	}

	void WebServer::SignalSettings(const signalID_t signalID, const std::string& name)
	{
		stringstream command;
		command << "signalsettings;signal=" << signalID;
		AddUpdate(command.str(), Languages::TextSignalUpdated, name);
	}

	void WebServer::SignalDelete(const signalID_t signalID, const std::string& name)
	{
		stringstream command;
		command << "signaldelete;signal=" << signalID;
		AddUpdate(command.str(), Languages::TextSignalDeleted, name);
	}

	void WebServer::LocoRelease(const locoID_t locoID)
	{
		stringstream command;
		command << "locorelease;loco=" << locoID;
		AddUpdate(command.str(), Languages::TextLocoIsReleased, manager.GetLocoName(locoID));
	}

	void WebServer::StreetRelease(const streetID_t streetID)
	{
		stringstream command;
		command << "streetRelease;street=" << streetID;
		AddUpdate(command.str(), Languages::TextStreetIsReleased, manager.GetStreetName(streetID));
	}

	void WebServer::LocoDestinationReached(const locoID_t locoID, const streetID_t streetID, const trackID_t trackID)
	{
		stringstream command;
		stringstream status;
		command << "locoDestinationReached;loco=" << locoID << ";street=" << streetID << ";track=" << trackID;
		status << manager.GetLocoName(locoID) << " has reached the destination track " << manager.GetTrackName(trackID) << " on street " << manager.GetStreetName(streetID);
		AddUpdate(command.str(), status.str());
	}

	void WebServer::LocoStart(const locoID_t locoID)
	{
		stringstream command;
		stringstream status;
		command << "locoStart;loco=" << locoID;
		status << manager.GetLocoName(locoID) << " is in auto mode";
		AddUpdate(command.str(), status.str());
	}

	void WebServer::LocoStop(const locoID_t locoID)
	{
		stringstream command;
		stringstream status;
		command << "locoStop;loco=" << locoID;
		status << manager.GetLocoName(locoID) << " is in manual mode";
		AddUpdate(command.str(), status.str());
	}

	void WebServer::LocoSettings(const locoID_t locoID, const std::string& name)
	{
		stringstream command;
		stringstream status;
		command << "locosettings;loco=" << locoID;
		status << name << " updated";
		AddUpdate(command.str(), status.str());
	}

	void WebServer::LocoDelete(const locoID_t locoID, const std::string& name)
	{
		stringstream command;
		stringstream status;
		command << "locodelete;loco=" << locoID;
		status << name << " updated";
		AddUpdate(command.str(), status.str());
	}

	void WebServer::LayerSettings(const layerID_t layerID, const std::string& name)
	{
		stringstream command;
		stringstream status;
		command << "layersettings;layer=" << layerID;
		status << name << " updated";
		AddUpdate(command.str(), status.str());
	}

	void WebServer::LayerDelete(const layerID_t layerID, const std::string& name)
	{
		stringstream command;
		stringstream status;
		command << "layerdelete;layer=" << layerID;
		status << name << " updated";
		AddUpdate(command.str(), status.str());
	}

	void WebServer::AddUpdate(const string& command, const string& status)
	{
		stringstream ss;
		ss << "data: command=" << command << ";status=" << status << "\r\n\r\n";
		std::lock_guard<std::mutex> lock(updateMutex);
		updates[++updateID] = ss.str();
		updates.erase(updateID - MaxUpdates);
	}

	bool WebServer::NextUpdate(unsigned int& updateIDClient, string& s)
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

}; // namespace WebServer
