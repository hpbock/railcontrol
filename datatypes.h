#pragma once

#include <string>

// common
typedef unsigned char controlType_t;
typedef unsigned char controlID_t;
typedef unsigned char hardwareType_t;
// booster
typedef unsigned char boosterStatus_t;

// objects in db
typedef unsigned short objectID_t;
// loco
typedef objectID_t locoID_t;
typedef unsigned char protocol_t;
typedef unsigned short address_t;
typedef unsigned char addressType_t;
typedef unsigned short speed_t;
typedef bool direction_t;
typedef unsigned char function_t;
// layoutItem
typedef unsigned char layoutRotation_t;
typedef unsigned char layoutItemSize_t;
typedef unsigned char layoutPosition_t;
// accessory
typedef objectID_t accessoryID_t;
typedef unsigned char accessoryType_t;
typedef unsigned char accessoryState_t;
typedef unsigned char accessoryColor_t;
typedef unsigned short accessoryTimeout_t;
// feedback
typedef objectID_t feedbackID_t;
typedef unsigned int feedbackPin_t;
typedef unsigned char feedbackState_t;
// block
typedef objectID_t blockID_t;
typedef unsigned char lockState_t;
// switch
typedef accessoryID_t switchID_t;
typedef accessoryType_t switchType_t;
typedef accessoryState_t switchState_t;
typedef accessoryTimeout_t switchTimeout_t;
// street
typedef objectID_t streetID_t;
typedef unsigned char lockState_t;

// relations in db
typedef unsigned short relationID_t;

// automode
typedef unsigned char locoState_t;

static const controlID_t ControlNone = 0;
static const address_t AddressNone = 0;
static const locoID_t LocoNone = 0;
static const accessoryID_t AccessoryNone = 0;
static const feedbackID_t FeedbackNone = 0;
static const blockID_t BlockNone = 0;
static const switchID_t SwitchNone = 0;
static const streetID_t StreetNone = 0;

static const speed_t MaxSpeed = 1023;
static const speed_t MinSpeed = 0;

static const layoutItemSize_t Width1 = 1;
static const layoutItemSize_t Height1 = 1;

enum controlTypes : controlType_t {
	ControlTypeHardware = 0,
	ControlTypeAutomode,
	ControlTypeConsole,
	ControlTypeWebserver
};

enum controlIDs : controlID_t {
	ControlIdNone = 0,
	ControlIdConsole,
	ControlIdWebserver,
	ControlIdFirstHardware = 10
};

enum boosterStatus : boosterStatus_t {
	BoosterStop = 0,
	BoosterGo
};

enum protocols : protocol_t {
	ProtocolNone = 0,
	ProtocolServer,
	ProtocolMM1,
	ProtocolMM2,
	ProtocolMFX,
	ProtocolDCC,
	ProtocolDCCShort,
	ProtocolDCCLong,
	ProtocolSX1,
	ProtocolSX2,
	ProtocolEnd = ProtocolSX2
};

static std::string protocolSymbols[] = {
	"none",
	"all",
	"MM1",
	"MM2",
	"mfx",
	"DCC",
	"SX1",
	"SX2"
};

enum addressTypes : addressType_t {
	AddressTypeLoco = 0,
	AddressTypeAccessory
};

enum hardwareTypes : hardwareType_t {
	HardwareTypeNone = 0,
	HardwareTypeVirt,
	HardwareTypeCS2,
	HardwareTypeNumbers
};

static std::string hardwareSymbols[] = {
	"none",
	"virtual",
	"cs2"
};

enum rotations : layoutRotation_t {
	Rotation0 = 0,
	Rotation90,
	Rotation180,
	Rotation270
};

typedef unsigned char objectType_t;

enum objectType : objectType_t {
	ObjectTypeLoco = 1,
	ObjectTypeBlock,
	ObjectTypeFeedback,
	ObjectTypeAccessory,
	ObjectTypeSwitch,
	ObjectTypeStreet
};

typedef unsigned char relationType_t;

enum relationType : relationType_t {
	RelationTypeBlockStreet = 0,
	RelationTypeStreetFeedback
};

enum accessoryColor : accessoryColor_t {
	AccessoryColorRed = 0,
	AccessoryColorGreen,
	AccessoryColorYellow,
	AccessoryColorWhite
};

enum accessoryType : accessoryType_t {
	AccessoryTypeDefault = 0
};

enum accessoryState : accessoryState_t {
	AccessoryStateOff = 0,
	AccessoryStateOn
};

enum feedbackState : feedbackState_t {
	FeedbackStateFree = 0,
	FeedbackStateOccupied
};

enum lockState : lockState_t {
	LockStateFree = 0,
	LockStateReserved,
	LockStateSoftLocked,
	LockStateHardLocked
};

enum directionState : direction_t {
	DirectionLeft = false,
	DirectionRight = true
};

enum switchType : switchType_t {
	SwitchStateLeft = 0,
	SwitchTypeRight
};

enum switchState : switchState_t {
	SwitchStateStraight = 0,
	SwitchStateTurnout
};

enum locoState : locoState_t {
	LocoStateManual = 0,
	LocoStateOff,
	LocoStateSearching,
	LocoStateRunning,
	LocoStateStopping,
	LocoStateError
};

