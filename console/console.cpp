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
#include "util.h"
#include "console.h"

using std::map;
using std::thread;
using std::to_string;
using std::string;
using std::stringstream;
using std::vector;

namespace console {

	Console::Console(Manager& manager, const unsigned short port) :
		ManagerInterface(MANAGER_ID_CONSOLE),
		port(port),
		serverSocket(0),
		clientSocket(-1),
		run(false),
		manager(manager) {

		run = true;
		struct sockaddr_in6 server_addr;

		xlog("Starting console on port %i", port);

		// create server socket
		serverSocket = socket(AF_INET6, SOCK_STREAM, 0);
		if (serverSocket < 0) {
			xlog("Unable to create socket for console. Unable to serve clients.");
			run = false;
			return;
		}

		// bind socket to an address (in6addr_any)
		memset((char *) &server_addr, 0, sizeof(server_addr));
		server_addr.sin6_family = AF_INET6;
		server_addr.sin6_addr = in6addr_any;
		server_addr.sin6_port = htons(port);

		int on = 1;
		if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, sizeof(on)) < 0) {
			xlog("Unable to set console socket option SO_REUSEADDR.");
		}

		if (bind(serverSocket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
			xlog("Unable to bind socket for console to port %i. Unable to serve clients.", port);
			close(serverSocket);
			run = false;
			return;
		}

		// listen on the socket
		if (listen(serverSocket, 5) != 0) {
			xlog("Unable to listen on socket for console server on port %i. Unable to serve clients.", port);
			close(serverSocket);
			run = false;
			return;
		}

		// create seperate thread that handles the client requests
		serverThread = thread([this] { worker(); });
	}

	Console::~Console() {
		if (run) {
			xlog("Stopping console");
			run = false;

			// join server thread
			serverThread.join();
		}
	}


	// worker is a seperate thread listening on the server socket
	void Console::worker() {
		fd_set set;
		struct timeval tv;
		struct sockaddr_in6 client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		while (run) {
			// wait for connection and abort on shutdown
			int ret;
			do {
				FD_ZERO(&set);
				FD_SET(serverSocket, &set);
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				ret = TEMP_FAILURE_RETRY(select(FD_SETSIZE, &set, NULL, NULL, &tv));
			} while (ret == 0 && run);
			if (ret > 0 && run) {
				// accept connection
				clientSocket = accept(serverSocket, (struct sockaddr *) &client_addr, &client_addr_len);
				if (clientSocket < 0) {
					xlog("Unable to accept client connection for console: %i, %i", clientSocket, errno);
				}
				else {
					// handle client and fill into vector
					handleClient();
				}
			}
		}
	}

	void Console::readBlanks(string& s, size_t& i) {
		// read possible blanks
		while (s.length() > i) {
			unsigned char input = s[i];
			if (input != ' ') {
				break;
			}
			++i;
		}
	}

	int Console::readNumber(string& s, size_t& i) {
		int number = 0;
		while (s.length() > i) {
			unsigned char input = s[i];
			if ( input < '0' || input > '9') {
				break;
			}
			number *= 10;
			number += input - '0';
			++i;
		};
		return number;
	}

	string Console::readText(string& s, size_t& i) {
		size_t start = i;
		size_t length = 0;
		bool escape = false;
		while (true) {
			if (s.length() <= i) {
				break;
			}
			if (s[i] == '\n' || s[i] == '\r') {
				i++;
				break;
			}
			if (s[i] == '"' && escape == false) {
				escape = true;
				i++;
				start++;
				continue;
			}
			if (s[i] == '"' && escape == true) {
				i++;
				break;
			}
			if (escape == false && s[i] == 0x20) {
				i++;
				break;
			}
			length++;
			i++;
		}
		string text(s, start, length);
		return text;
	}

	void Console::handleClient() {
		addUpdate("Welcome to railcontrol console!\nType h for help\n");
		char buffer_in[1024];
		memset(buffer_in, 0, sizeof(buffer_in));

		while(run) {
			size_t pos = 0;
			string s;
			while(run && pos < sizeof(buffer_in) - 1 && s.find("\n") == string::npos && s.find("\r") == string::npos) {
				pos += recv_timeout(clientSocket, buffer_in + pos, sizeof(buffer_in) - 1 - pos, 0);
				s = string(buffer_in);
			}


			size_t i = 0;
			readBlanks(s, i);
			char cmd = s[i];
			i++;
			switch (cmd)
			{
				case 'a':
				case 'A': // accessory commands
					{
						readBlanks(s, i);
						char subcmd = s[i];
						i++;
						switch (subcmd) {
							case 'd':
							case 'D': // delete accessory
								{
									readBlanks(s, i);
									accessoryID_t accessoryID = readNumber(s, i);
									if (!manager.accessoryDelete(accessoryID)) {
										addUpdate("Accessory not found or accessory in use");
										break;
									}
									addUpdate("Accessory deleted");
									break;
								}
							case 'l':
							case 'L': // list accessories
								{
									readBlanks(s, i);
									if (s[i] == 'a') { // list all accessories
										std::map<accessoryID_t,datamodel::Accessory*> accessories = manager.accessoryList();
										stringstream status;
										for (auto accessory : accessories) {
											status << accessory.first << " " << accessory.second->name << "\n";
										}
										status << "Total number of accessorys: " << accessories.size();
										addUpdate(status.str());
										break;
									}
									accessoryID_t accessoryID = readNumber(s, i);
									datamodel::Accessory* accessory = manager.getAccessory(accessoryID);
									if (accessory == nullptr) {
										addUpdate("Unknwown accessory");
										break;
									}
									stringstream status;
									status << accessoryID << " " << accessory->name << " (" << static_cast<int>(accessory->posX) << "/" << static_cast<int>(accessory->posY) << "/" << static_cast<int>(accessory->posZ) << ")";
									addUpdate(status.str());
									break;
								}
							case 'n':
							case 'N': // add new accessory
								{
									readBlanks(s, i);
									string name = readText(s, i);
									readBlanks(s, i);
									layoutPosition_t posX = readNumber(s, i);
									readBlanks(s, i);
									layoutPosition_t posY = readNumber(s, i);
									readBlanks(s, i);
									layoutPosition_t posZ = readNumber(s, i);
									readBlanks(s, i);
									controlID_t controlID = readNumber(s, i);
									readBlanks(s, i);
									protocol_t protocol = readNumber(s, i);
									readBlanks(s, i);
									address_t address = readNumber(s, i);
									readBlanks(s, i);
									accessoryType_t type = readNumber(s, i);
									readBlanks(s, i);
									accessoryState_t state = readNumber(s, i);
									readBlanks(s, i);
									accessoryTimeout_t timeout = readNumber(s, i);
									if (!manager.accessorySave(ACCESSORY_NONE, name, posX, posY, posZ, controlID, protocol, address, type, state, timeout)) {
										addUpdate("Unable to add accessory");
										break;
									}
									stringstream status;
									status << "Accessory \"" << name << "\" added";
									addUpdate(status.str());
									break;
								}
							default:
								{
									addUpdate("Unknown accessory command");
								}
						}
						break;
					}
				case 'b':
				case 'B': // block commands
					{
						readBlanks(s, i);
						char subcmd = s[i];
						i++;
						switch (subcmd) {
							case 'd':
							case 'D': // delete block
								{
									readBlanks(s, i);
									blockID_t blockID = readNumber(s, i);
									if (!manager.blockDelete(blockID)) {
										addUpdate("Block not found or block in use");
										break;
									}
									addUpdate("Block deleted");
									break;
								}
							case 'l':
							case 'L': // list blocks
								{
									readBlanks(s, i);
									if (s[i] == 'a') { // list all blocks
										std::map<blockID_t,datamodel::Block*> blocks = manager.blockList();
										stringstream status;
										for (auto block : blocks) {
											status << block.first << " " << block.second->name << "\n";
										}
										status << "Total number of Blocks: " << blocks.size();
										addUpdate(status.str());
										break;
									}
									blockID_t blockID = readNumber(s, i);
									datamodel::Block* block = manager.getBlock(blockID);
									if (block == nullptr) {
										addUpdate("Unknwown block");
										break;
									}
									stringstream status;
									status << blockID << " " << block->name << " (" << static_cast<int>(block->posX) << "/" << static_cast<int>(block->posY) << "/" << static_cast<int>(block->posZ) << ")";
									addUpdate(status.str());
									break;
								}
							case 'n':
							case 'N': // new block
								{
									readBlanks(s, i);
									string name = readText(s, i);
									readBlanks(s, i);
									layoutItemSize_t width = readNumber(s, i);
									readBlanks(s, i);
									layoutRotation_t rotation = readNumber(s, i);
									readBlanks(s, i);
									layoutPosition_t posX = readNumber(s, i);
									readBlanks(s, i);
									layoutPosition_t posY = readNumber(s, i);
									readBlanks(s, i);
									layoutPosition_t posZ = readNumber(s, i);
									if (!manager.blockSave(BLOCK_NONE, name, width, rotation, posX, posY, posZ)) {
										addUpdate("Unable to add block");
										break;
									}
									stringstream status;
									status << "Block \"" << name << "\" added";
									addUpdate(status.str());
									break;
								}
							default:
								{
									addUpdate("Unknown block command");
								}
						}
						break;
					}
				case 'c':
				case 'C': // control commands
					{
						readBlanks(s, i);
						char subcmd = s[i];
						i++;
						switch (subcmd) {
							case 'd':
							case 'D': // delete control
								{
									readBlanks(s, i);
									controlID_t controlID = readNumber(s, i);
									if (!manager.controlDelete(controlID)) {
										addUpdate("Control not found or control in use");
										break;
									}
									addUpdate("Control deleted");
									break;
								}
							case 'l':
							case 'L': // list controls
								{
									readBlanks(s, i);
									if (s[i] == 'a') { // list all controls
										std::map<controlID_t,hardware::HardwareParams*> params = manager.controlList();
										stringstream status;
										for (auto param : params) {
											status << static_cast<int>(param.first) << " " << param.second->name << "\n";
										}
										status << "Total number of controls: " << params.size();
										addUpdate(status.str());
										break;
									}
									controlID_t controlID = readNumber(s, i);
									hardware::HardwareParams* param = manager.getHardware(controlID);
									if (param == nullptr) {
										addUpdate("Unknwown Control");
										break;
									}
									stringstream status;
									status << static_cast<int>(controlID) << " " << param->name;
									addUpdate(status.str());
									break;
								}
							case 'n':
							case 'N': // new control
								{
									readBlanks(s, i);
									string name = readText(s, i);
									readBlanks(s, i);
									string type = readText(s, i);
									readBlanks(s, i);
									string ip = readText(s, i);
									hardwareType_t hardwareType;
									if (type.compare("virt") == 0) {
										hardwareType = HARDWARE_TYPE_VIRT;
									}
									else if (type.compare("cs2") == 0) {
										hardwareType = HARDWARE_TYPE_CS2;
									}
									else {
										addUpdate("Unknown hardware type");
										break;
									}
									if (!manager.controlSave(CONTROL_NONE, hardwareType, name, ip)) {
										addUpdate("Unable to add control");
										break;
									}
									stringstream status;
									status << "Control \"" << name << "\" added";
									addUpdate(status.str());
									break;
								}
							default:
								{
									addUpdate("Unknown control command");
								}
						}
						break;
					}
				case 'f':
				case 'F': // feedback commands
					{
						readBlanks(s, i);
						char subcmd = s[i];
						i++;
						switch (subcmd) {
							case 'd':
							case 'D': // delete feedback
								{
									readBlanks(s, i);
									feedbackID_t feedbackID = readNumber(s, i);
									if (!manager.feedbackDelete(feedbackID)) {
										addUpdate("Feedback not found or feedback in use");
										break;
									}
									addUpdate("Feedback deleted");
									break;
								}
							case 'l':
							case 'L': // list feedbacks
								{
									readBlanks(s, i);
									if (s[i] == 'a') { // list all feedbacks
										std::map<feedbackID_t,datamodel::Feedback*> feedbacks = manager.feedbackList();
										stringstream status;
										for (auto feedback : feedbacks) {
											status << feedback.first << " " << feedback.second->name << "\n";
										}
										status << "Total number of feedbacks: " << feedbacks.size();
										addUpdate(status.str());
										break;
									}
									feedbackID_t feedbackID = readNumber(s, i);
									datamodel::Feedback* feedback = manager.getFeedback(feedbackID);
									if (feedback == nullptr) {
										addUpdate("Unknwown feedback");
										break;
									}
									stringstream status;
									status << feedbackID << " " << feedback->name << " (" << static_cast<int>(feedback->posX) << "/" << static_cast<int>(feedback->posY) << "/" << static_cast<int>(feedback->posZ) << ")";
									addUpdate(status.str());
									break;
								}
							case 'n':
							case 'N': // new feedback
								{
									readBlanks(s, i);
									string name = readText(s, i);
									readBlanks(s, i);
									layoutPosition_t posX = readNumber(s, i);
									readBlanks(s, i);
									layoutPosition_t posY = readNumber(s, i);
									readBlanks(s, i);
									layoutPosition_t posZ = readNumber(s, i);
									readBlanks(s, i);
									controlID_t control = readNumber(s, i);
									readBlanks(s, i);
									feedbackPin_t pin = readNumber(s, i);
									if(!manager.feedbackSave(FEEDBACK_NONE, name, posX, posY, posZ, control, pin)) {
										addUpdate("Unable to add feedback");
										break;
									}
									stringstream status;
									status << "Feedback \"" << name << "\" added";
									addUpdate(status.str());
									break;
								}
							case 's':
							case 'S': // set feedback
								{
									feedbackID_t feedbackID = readNumber(s, i);
									readBlanks(s, i);
									// read state
									unsigned char input = s[i];
									feedbackState_t state;
									char* text;
									if (input == 'X' || input == 'x') {
										state = FEEDBACK_STATE_OCCUPIED;
										text = (char*)"ON";
									}
									else {
										state = FEEDBACK_STATE_FREE;
										text = (char*)"OFF";
									}
									manager.feedback(MANAGER_ID_CONSOLE, feedbackID, state);
									stringstream status;
									status << "Feedback \"" << manager.getFeedbackName(feedbackID) << "\" turned " << text;
									addUpdate(status.str());
									break;
								}
							default:
								{
									addUpdate("Unknown feedback command");
									break;
								}
						}
						break;
					}
				case 'l':
				case 'L': // loco commands
					{
						readBlanks(s, i);
						char subcmd = s[i];
						i++;
						switch (subcmd) {
							case 'a': // set loco to automode
							case 'A': // set loco to automode
								{
									readBlanks(s, i);
									if (s[i] == 'a') { // set all locos to automode
										manager.locoStartAll();
										break;
									}
									// set specific loco to auto mode
									locoID_t locoID = readNumber(s, i);
									if (!manager.locoStart(locoID)) {
										addUpdate("Unknwon loco");
									}
									break;
								}
							case 'b':
							case 'B': // loco into block
								{
									readBlanks(s, i);
									locoID_t locoID = readNumber(s, i);
									readBlanks(s, i);
									blockID_t blockID = readNumber(s, i);
									if (!manager.locoIntoBlock(locoID, blockID)) {
										addUpdate("Unknwon loco or unknown block");
									}
									break;
								}
							case 'd':
							case 'D': // delete loco
								{
									readBlanks(s, i);
									locoID_t locoID = readNumber(s, i);
									if (!manager.locoDelete(locoID)) {
										addUpdate("Loco not found or loco in use");
										break;
									}
									addUpdate("Loco deleted");
									break;
								}
							case 'l':
							case 'L':
								{
									readBlanks(s, i);
									if (s[i] == 'a') { // list all locos
										std::map<locoID_t,datamodel::Loco*> locos = manager.locoList();
										stringstream status;
										for (auto loco : locos) {
											status << loco.first << " " << loco.second->name << "\n";
										}
										status << "Total number of locos: " << locos.size();
										addUpdate(status.str());
										break;
									}
									locoID_t locoID = readNumber(s, i);
									datamodel::Loco* loco = manager.getLoco(locoID);
									if (loco == nullptr) {
										addUpdate("Unknown loco");
										break;
									}
									stringstream status;
									status << locoID << " " << loco->name << " (" << static_cast<int>(loco->controlID) << "/" << static_cast<int>(loco->protocol) << "/" << loco->address << ")";
									addUpdate(status.str());
									break;
								}
							case 'm':
							case 'M': // set loco to manual mode
								{
									readBlanks(s, i);
									if (s[i] == 'a') { // set all locos to manual mode
										manager.locoStopAll();
										break;
									}
									// set specific loco to manual mode
									locoID_t locoID = readNumber(s, i);
									if (!manager.locoStop(locoID)) {
										addUpdate("Unknwon loco");
									}
									break;
								}
							case 'n':
							case 'N': // new loco
								{
									readBlanks(s, i);
									string name = readText(s, i);
									readBlanks(s, i);
									controlID_t control = readNumber(s, i);
									readBlanks(s, i);
									protocol_t protocol = readNumber(s, i);
									readBlanks(s, i);
									address_t address = readNumber(s, i);
									if (!manager.locoSave(LOCO_NONE, name, control, protocol, address)) {
										string status("Unable to add loco");
										addUpdate(status);
										break;
									}
									stringstream status;
									status << "Loco \"" << name << "\" added";
									addUpdate(status.str());
									break;
								}
							case 's':
							case 'S': // set loco speed
								{
									readBlanks(s, i);
									locoID_t locoID = readNumber(s, i);
									readBlanks(s, i);
									speed_t speed = readNumber(s, i);
									if (!manager.locoSpeed(MANAGER_ID_CONSOLE, locoID, speed)) {
										addUpdate("Unknwon loco");
									}
									break;
								}
							default:
								{
									addUpdate("Unknown loco command");
								}
						}
						break;
					}
				case 'h':
				case 'H': // help
					{
						string status("Available console commands:\n"
								"\n"
								"Accessory commands\n"
								"A D accessory#                    Delete accessory\n"
								"A L A                             List all accessories\n"
								"A L accessory#                    List accessory\n"
								"A N Name X Y Z Control Protocol Address Type State Timeout\n"
								"                                  New Accessory\n"
								"\n"
								"Block commands\n"
								"B D block#                        Delete block\n"
								"B L A                             List all blocks\n"
								"B L block#                        List block\n"
								"B N Name Width Rotation X Y Z     New block\n"
								"\n"
								"Control commands\n"
								"C D control#                      Delete control\n"
								"C L A                             List all controls\n"
								"C L control#                      List control\n"
								"C N Name Type IP                  New control\n"
								"\n"
								"Feedback commands\n"
								"F D feedback#                     Delete feedback\n"
								"F L A                             List all feedbacks\n"
								"F L feedback#                     List feedback\n"
								"F S pin# [X]                      Turn feedback on (with X) or of (without X)\n"
								"F N Name X Y Z Control Pin        New feedback\n"
								"\n"
								"Loco commands\n"
								"L A A                             Start all locos into automode\n"
								"L A loco#                         Start loco into automode\n"
								"L B loco# block#                  Set loco into block\n"
								"L D loco#                         Delete loco\n"
								"L L A                             List all locos\n"
								"L L loco#                         List loco\n"
								"L M A                             Stop all locos and go to manual mode\n"
								"L M loco#                         Stop loco and go to manual mode\n"
								"L N Name Control Protocol Address New loco\n"
								"L S loco# speed                   Set loco speed between 0 and 1024\n"
								"\n"
								"Street commands\n"
								"T L A                             List all streets\n"
								"\n"
								"Switch commands\n"
								"W L A                             List all switches\n"
								"\n"
								"Other commands\n"
								"H                                 Show this help\n"
								"Q                                 Quit console\n"
								"S                                 Shut down railcontrol\n");
						addUpdate(status);
						break;
					}
				case 's':
				case 'S': // shut down railcontrol
					{
						string status("Shutting down railcontrol");
						addUpdate(status);
						stopRailControlConsole();
						// no break, fall throught
					}
				case 'q':
				case 'Q': // quit
					{
						addUpdate("Quit railcontrol console");
						close(clientSocket);
						return;
					}
				case 'w':
				case 'W': // Switch commands
					{
						readBlanks(s, i);
						char subcmd = s[i];
						i++;
						switch (subcmd) {
							case 'd':
							case 'D': // delete switch
								{
									readBlanks(s, i);
									switchID_t switchID = readNumber(s, i);
									if (!manager.switchDelete(switchID)) {
										addUpdate("Switch not found or switch in use");
										break;
									}
									addUpdate("Switch deleted");
									break;
								}
							case 'l':
							case 'L': // list switchs
								{
									readBlanks(s, i);
									if (s[i] == 'a') { // list all switches
										std::map<switchID_t,datamodel::Switch*> switches = manager.switchList();
										stringstream status;
										for (auto mySwitch : switches) {
											status << mySwitch.first << " " << mySwitch.second->name << "\n";
										}
										status << "Total number of switches: " << switches.size();
										addUpdate(status.str());
										break;
									}
									switchID_t switchID = readNumber(s, i);
									datamodel::Switch* mySwitch = manager.getSwitch(switchID);
									if (mySwitch == nullptr) {
										addUpdate("Unknwown switch");
										break;
									}
									stringstream status;
									status << switchID << " " << mySwitch->name << " (" << static_cast<int>(mySwitch->posX) << "/" << static_cast<int>(mySwitch->posY) << "/" << static_cast<int>(mySwitch->posZ) << ")";
									addUpdate(status.str());
									break;
								}
							case 'n':
							case 'N': // new switch
								{
									readBlanks(s, i);
									string name = readText(s, i);
									readBlanks(s, i);
									layoutRotation_t rotation = readNumber(s, i);
									readBlanks(s, i);
									layoutPosition_t posX = readNumber(s, i);
									readBlanks(s, i);
									layoutPosition_t posY = readNumber(s, i);
									readBlanks(s, i);
									layoutPosition_t posZ = readNumber(s, i);
									readBlanks(s, i);
									controlID_t controlID = readNumber(s, i);
									readBlanks(s, i);
									protocol_t protocol = readNumber(s, i);
									readBlanks(s, i);
									address_t address = readNumber(s, i);
									readBlanks(s, i);
									accessoryType_t type = readNumber(s, i);
									readBlanks(s, i);
									accessoryState_t state = readNumber(s, i);
									readBlanks(s, i);
									accessoryTimeout_t timeout = readNumber(s, i);
									if (!manager.switchSave(SWITCH_NONE, name, rotation, posX, posY, posZ, controlID, protocol, address, type, state, timeout)) {
										addUpdate("Unable to add switch");
										break;
									}
									stringstream status;
									status << "Switch \"" << name << "\" added";
									addUpdate(status.str());
									break;
								}
							default:
								{
									addUpdate("Unknown switch command");
								}
						}
						break;
					}
				default:
					{
						addUpdate("Unknown command");
					}
			}
		}
	}

	void Console::addUpdate(const string& status) {
		if (clientSocket < 0) {
			return;
		}
		string s(status);
		s.append("\n> ");
		send_timeout(clientSocket, s.c_str(), s.length(), 0);
	}

	void Console::booster(const managerID_t managerID, const boosterStatus_t status) {
		if (status) {
			addUpdate("Booster is on");
		}
		else {
			addUpdate("Booster is off");
		}
	}

	void Console::locoSpeed(const managerID_t managerID, const locoID_t locoID, const speed_t speed) {
		std::stringstream status;
		status << manager.getLocoName(locoID) << " speed is " << speed;
		addUpdate(status.str());
	}

	void Console::locoDirection(const managerID_t managerID, const locoID_t locoID, const direction_t direction) {
		std::stringstream status;
		const char* directionText = (direction ? "forward" : "reverse");
		status << manager.getLocoName(locoID) << " direction is " << directionText;
		addUpdate(status.str());
	}

	void Console::locoFunction(const managerID_t managerID, const locoID_t locoID, const function_t function, const bool state) {
		std::stringstream status;
		status << manager.getLocoName(locoID) << " f" << (unsigned int)function << " is " << (state ? "on" : "off");
		addUpdate(status.str());
	}

	void Console::accessory(const managerID_t managerID, const accessoryID_t accessoryID, const accessoryState_t state) {
		std::stringstream status;
		unsigned char color;
		unsigned char on;
		char* colorText;
		char* stateText;
		datamodel::Accessory::getAccessoryTexts(state, color, on, colorText, stateText);
		status << manager.getAccessoryName(accessoryID) << " " << colorText << " is " << stateText;
		addUpdate(status.str());
	}

	void Console::feedback(const managerID_t managerID, const feedbackPin_t pin, const feedbackState_t state) {
		std::stringstream status;
		status << "Feedback " << pin << " is " << (state ? "on" : "off");
		addUpdate(status.str());
	}

	void Console::block(const managerID_t managerID, const blockID_t blockID, const blockState_t state) {
		std::stringstream status;
		char* stateText;
		datamodel::Block::getTexts(state, stateText);
		status << manager.getBlockName(blockID) << " is " << stateText;
		addUpdate(status.str());
	}

	void Console::handleSwitch(const managerID_t managerID, const switchID_t switchID, const switchState_t state) {
		std::stringstream status;
		char* stateText;
		datamodel::Switch::getTexts(state, stateText);
		status << manager.getSwitchName(switchID) << " is " << stateText;
		addUpdate(status.str());
	}

	void Console::locoIntoBlock(const locoID_t locoID, const blockID_t blockID) {
		std::stringstream status;
		status << manager.getLocoName(locoID) << " is in block " << manager.getBlockName(blockID);
		addUpdate(status.str());
	}

	void Console::locoStreet(const locoID_t locoID, const streetID_t streetID, const blockID_t blockID) {
		std::stringstream status;
		status << manager.getLocoName(locoID) << " runs on street " << manager.getStreetName(streetID) << " with destination block " << manager.getBlockName(blockID);
		addUpdate(status.str());
	}

	void Console::locoDestinationReached(const locoID_t locoID, const streetID_t streetID, const blockID_t blockID) {
		std::stringstream status;
		status << manager.getLocoName(locoID) << " has reached the destination block " << manager.getBlockName(blockID) << " on street " << manager.getStreetName(streetID);
		addUpdate(status.str());
	}

	void Console::locoStart(const locoID_t locoID) {
		std::stringstream status;
		status << manager.getLocoName(locoID) << " is in auto mode";
		addUpdate(status.str());
	}

	void Console::locoStop(const locoID_t locoID) {
		std::stringstream status;
		status << manager.getLocoName(locoID) << " is in manual mode";
		addUpdate(status.str());
	}

}; // namespace console
