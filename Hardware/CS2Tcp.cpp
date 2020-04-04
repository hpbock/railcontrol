/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2020 Dominik (Teddy) Mahrer - www.railcontrol.org

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

#include "Hardware/CS2Tcp.h"
#include "Utils/Utils.h"

namespace Hardware
{
	extern "C" CS2Tcp* create_CS2Tcp(const HardwareParams* params)
	{
		return new CS2Tcp(params);
	}

	extern "C" void destroy_CS2Tcp(CS2Tcp* cs2Tcp)
	{
		delete(cs2Tcp);
	}

	CS2Tcp::CS2Tcp(const HardwareParams* params)
	:	MaerklinCAN(params->GetManager(),
			params->GetControlID(),
			Logger::Logger::GetLogger("CS2TCP " + params->GetName() + " " + params->GetArg1()),
			"Maerklin Central Station 2 (CS2) TCP / " + params->GetName() + " at IP " + params->GetArg1()),
	 	run(false),
	 	connection(Network::TcpClient::GetTcpClientConnection(logger, params->GetArg1(), CS2Port))
	{
		logger->Info(Languages::TextStarting, name);

		if (connection.IsConnected() == false)
		{
			logger->Error(Languages::TextUnableToCreateTcpSocket);
			return;
		}
		receiverThread = std::thread(&Hardware::CS2Tcp::Receiver, this);
	}

	CS2Tcp::~CS2Tcp()
	{
		if (run == false)
		{
			return;
		}
		run = false;
		receiverThread.join();
		logger->Info(Languages::TextTerminatingSenderSocket);
	}

	void CS2Tcp::Send(const unsigned char* buffer)
	{
		logger->Hex(buffer, CANCommandBufferLength);
		if (connection.Send(buffer, CANCommandBufferLength) == -1)
		{
			logger->Error(Languages::TextUnableToSendDataToControl);
		}
	}

	void CS2Tcp::Receiver()
	{
		run = true;
		Utils::Utils::SetThreadName("CS2Tcp");
		logger->Info(Languages::TextReceiverThreadStarted);
		if (connection.IsConnected() == false)
		{
			logger->Error(Languages::TextUnableToCreateUdpSocketForReceivingData);
			return;
		}

		unsigned char buffer[CANCommandBufferLength];
		while(run)
		{
			ssize_t datalen = connection.Receive(buffer, sizeof(buffer));
			if (run == false)
			{
				break;
			}

			if (datalen < 0)
			{
				if (errno == ETIMEDOUT)
				{
					continue;
				}
				logger->Error(Languages::TextUnableToReceiveData);
				break;
			}

			if (datalen != 13)
			{
				logger->Error(Languages::TextInvalidDataReceived);
				continue;
			}
			logger->Hex(buffer, sizeof(buffer));
			Parse(buffer);
		}
		connection.Terminate();
		logger->Info(Languages::TextTerminatingReceiverThread);
	}
} // namespace