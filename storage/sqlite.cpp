#include <map>
#include <sstream>

#include "sqlite.h"
#include "util.h"

using datamodel::Accessory;
using datamodel::Block;
using datamodel::Feedback;
using datamodel::Loco;
using datamodel::Switch;
using hardware::HardwareParams;
using std::map;
using std::string;
using std::stringstream;
using std::vector;

namespace storage {

	// create instance of sqlite
	extern "C" SQLite* create_sqlite(const StorageParams& params) {
		return new SQLite(params);
  }

	// delete instance of sqlite
  extern "C" void destroy_sqlite(SQLite* sqlite) {
    delete(sqlite);
  }

	SQLite::SQLite(const StorageParams& params) {
		int rc;
		char* dbError = NULL;

		xlog("Loading SQLite database with filename %s", params.filename.c_str());
		rc = sqlite3_open(params.filename.c_str(), &db);
		if (rc) {
			xlog("Unable to load SQLite database: %s", sqlite3_errmsg(db));
			sqlite3_close(db);
			db = NULL;
			return;
		}

		// check if needed tables exist
		map<string, bool> tablenames;
		rc = sqlite3_exec(db, "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;", callbackListTables, &tablenames, &dbError);
		if (rc != SQLITE_OK) {
			xlog("SQLite error: %s", dbError);
			sqlite3_free(dbError);
			sqlite3_close(db);
			db = NULL;
			return;
		}

		// create hardware table if needed
		if (tablenames["hardware"] != true) {
			xlog("Creating table hardware");
			rc = sqlite3_exec(db, "CREATE TABLE hardware (controlid UNSIGNED TINYINT PRIMARY KEY, hardwareid UNSIGNED TINYINT, name VARCHAR(50), ip VARCHAR(46));", NULL, NULL, &dbError);
			if (rc != SQLITE_OK) {
				xlog("SQLite error: %s", dbError);
				sqlite3_free(dbError);
				sqlite3_close(db);
				db = NULL;
				return;
			}
		}

		// create objects table if needed
		if (tablenames["objects"] != true) {
			xlog("Creating table objects");
			rc = sqlite3_exec(db, "CREATE TABLE objects ("
				"objecttype UNSIGNED TINYINT, "
				"objectid UNSIGNED SHORTINT, "
				"name VARCHAR(50), "
				"object SHORTTEXT,"
				"PRIMARY KEY (objecttype, objectid));",
				NULL, NULL, &dbError);
			if (rc != SQLITE_OK) {
				xlog("SQLite error: %s", dbError);
				sqlite3_free(dbError);
				sqlite3_close(db);
				db = NULL;
				return;
			}
		}
	}

	SQLite::~SQLite() {
		if (db) {
			xlog("Closing SQLite database");
			sqlite3_close(db);
			db = NULL;
		}
	}

	int SQLite::callbackListTables(void* v, int argc, char **argv, char **colName) {
		map<string, bool>* tablenames = static_cast<map<string, bool>*>(v);
		(*tablenames)[argv[0]] = true;
		return 0;
	}

	void SQLite::hardwareParams(const hardware::HardwareParams& hardwareParams) {
		if (db) {
			stringstream ss;
			char* dbError = NULL;
			ss << "INSERT OR REPLACE INTO hardware VALUES (" << (int)hardwareParams.controlID << ", " << (int)hardwareParams.hardwareID << ", '" << hardwareParams.name << "', '" << hardwareParams.ip << "');";
			int rc = sqlite3_exec(db, ss.str().c_str(), NULL, NULL, &dbError);
			if (rc != SQLITE_OK) {
				xlog("SQLite error: %s", dbError);
				sqlite3_free(dbError);
			}
		}
	}

	void SQLite::allHardwareParams(std::map<controlID_t,hardware::HardwareParams*>& hardwareParams) {
		if (db) {
			char* dbError = 0;
			int rc = sqlite3_exec(db, "SELECT controlid, hardwareid, name, ip FROM hardware ORDER BY controlid;", callbackAllHardwareParams, &hardwareParams, &dbError);
			if (rc != SQLITE_OK) {
				xlog("SQLite error: %s", dbError);
				sqlite3_free(dbError);
			}
		}
	}

	// callback read hardwareparams
	int SQLite::callbackAllHardwareParams(void* v, int argc, char **argv, char **colName) {
		map<controlID_t,HardwareParams*>* hardwareParams = static_cast<map<controlID_t,HardwareParams*>*>(v);
		if (argc != 4) {
			return 0;
		}
		controlID_t controlID = atoi(argv[0]);
		if (hardwareParams->count(controlID)) {
			xlog("Control with ID %i already exists", controlID);
		}
		HardwareParams* params = new HardwareParams(controlID, atoi(argv[1]), argv[2], argv[3]);
		(*hardwareParams)[controlID] = params;
		return 0;
	}

	// save datamodelobject
	void SQLite::saveObject(const objectType_t objectType, const objectID_t objectID, const std::string& name, const std::string& object) {
		if (db) {
			stringstream ss;
			char* dbError = NULL;
			// FIXME: escape "'" in object
			ss << "INSERT OR REPLACE INTO objects (objecttype, objectid, name, object) VALUES (" << (int)objectType << ", " << (int)objectID << ", '" << name << "', '" << object << "');";
			string s(ss.str());
			int rc = sqlite3_exec(db, s.c_str(), NULL, NULL, &dbError);
			if (rc != SQLITE_OK) {
				xlog("SQLite error: %s", dbError);
				sqlite3_free(dbError);
			}
		}
	}

	// read datamodelobject
	void SQLite::objectsOfType(const objectType_t objectType, vector<string>& objects) {
		if (db) {
			char* dbError = 0;
			stringstream ss;
			ss << "SELECT object FROM objects WHERE objecttype = " << (int)objectType << " ORDER BY objectid;";
			string s(ss.str());
			int rc = sqlite3_exec(db, s.c_str(), callbackObjectsOfType, &objects, &dbError);
			if (rc != SQLITE_OK) {
				xlog("SQLite error: %s", dbError);
				sqlite3_free(dbError);
			}
		}
	}

	// callback read all datamodelobject
	int SQLite::callbackObjectsOfType(void* v, int argc, char **argv, char **colName) {
		vector<string>* objects = static_cast<vector<string>*>(v);
		if (argc != 1) {
			return 0;
		}
		objects->push_back(argv[0]);
		return 0;
	}

} // namespace storage
