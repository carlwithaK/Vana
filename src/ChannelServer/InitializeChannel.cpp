/*
Copyright (C) 2008-2009 Vana Development Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "InitializeChannel.h"
#include "BeautyDataProvider.h"
#include "ChatHandler.h"
#include "Database.h"
#include "DropDataProvider.h"
#include "EventDataProvider.h"
#include "GameLogicUtilities.h"
#include "InitializeCommon.h"
#include "ItemDataProvider.h"
#include "MapleVersion.h"
#include "MiscUtilities.h"
#include "MobDataProvider.h"
#include "Pets.h"
#include "Quests.h"
#include "Reactors.h"
#include "ScriptDataProvider.h"
#include "ShopDataProvider.h"
#include "SkillDataProvider.h"
#include <cstdio>
#include <iostream>

using std::string;
using MiscUtilities::atob;

void Initializing::checkMcdbVersion() {
	mysqlpp::Query query = Database::getDataDB().query("SELECT * FROM mcdb_info LIMIT 1");
	mysqlpp::StoreQueryResult res = query.store();

	int32_t version = (int32_t) res[0]["version"];
	int32_t subversion = (int32_t) res[0]["subversion"];
	int32_t maple_version = (int32_t) res[0]["maple_version"];

	if (version != McdbVersion || subversion != McdbSubVersion) {
		// MCDB incompatible
		std::cout << "ERROR: MCDB version imcompatible. Expected: " << McdbVersion << "." << McdbSubVersion << " ";
		std::cout << "Have: " << version << "." << subversion << std::endl;
		std::cout << "Press enter to quit ...";
		getchar();
		exit(4);
	}

	if (maple_version != MAPLE_VERSION) {
		std::cout << "WARNING: Your copy of MCDB is based on an incongruent version of the WZ files. Vana: " << MAPLE_VERSION << " MCDB: " << maple_version << std::endl;
	}
}

void Initializing::loadData() {
	ItemDataProvider::Instance()->loadData();
	ShopDataProvider::Instance()->loadData();
	MobDataProvider::Instance()->loadData();
	DropDataProvider::Instance()->loadData();
	BeautyDataProvider::Instance()->loadData();
	ScriptDataProvider::Instance()->loadData();
	SkillDataProvider::Instance()->loadData();
	EventDataProvider::Instance()->loadEvents();
	initializeReactors();
	initializeQuests();
	initializePets();
};

void Initializing::initializeReactors() {
	std::cout << std::setw(outputWidth) << std::left << "Initializing Reactors... ";
	mysqlpp::Query query = Database::getDataDB().query("SELECT * FROM reactoreventdata ORDER BY reactorid, state ASC");
	mysqlpp::UseQueryResult res = query.use();

	MYSQL_ROW reactorRow;
	ReactorEventInfo revent;

	while (reactorRow = res.fetch_raw_row()) {
		// Col0 : Row ID
		//    1 : Reactor ID
		//    2 : State
		//    3 : Type
		//    4 : Item ID
		//    5 : LT X
		//    6 : LT Y
		//    7 : RB X
		//    8 : RB Y
		//    9 : Next State
		//   10 : Repeat
		//   11 : Timeout

		int32_t id = atoi(reactorRow[1]);
		uint8_t state = atoi(reactorRow[2]);

		revent.type = atoi(reactorRow[3]);
		revent.itemid = atoi(reactorRow[4]);
		revent.lt.x = atoi(reactorRow[5]);
		revent.lt.y = atoi(reactorRow[6]);
		revent.rb.x = atoi(reactorRow[7]);
		revent.rb.y = atoi(reactorRow[8]);
		revent.nextstate = atoi(reactorRow[9]);
		revent.repeat = atob(reactorRow[10]);
		revent.timeout = atoi(reactorRow[11]);

		Reactors::setMaxStates(id, revent.nextstate);
		Reactors::addEventInfo(id, state, revent);
	}

	std::cout << "DONE" << std::endl;
}

void Initializing::initializeQuests() {
	std::cout << std::setw(outputWidth) << std::left << "Initializing Quests... ";
	// Quests
	mysqlpp::Query query = Database::getDataDB().query("SELECT * FROM questdata");
	mysqlpp::UseQueryResult res = query.use();

	MYSQL_ROW Row;
	Quest curquest;

	enum QuestData {
		QuestIdData = 0,
		NextQuest
	};
	while (Row = res.fetch_raw_row()) {
		int16_t questid = atoi(Row[QuestIdData]);

		curquest.setNextQuest(atoi(Row[NextQuest]));
		curquest.setQuestId(questid);

		Quests::quests[questid] = curquest;
	}

	// Quest requests
	int16_t currentid = 0;
	int16_t previousid = -1;

	query << "SELECT * FROM questrequestdata ORDER BY questid, id ASC";
	res = query.use();

	enum QuestRequestData {
		TableId = 0,
		QuestId, IsMob, IsItem, IsQuest, ObjectIdRequest,
		CountRequest
	};
	while (Row = res.fetch_raw_row()) {
		currentid = atoi(Row[QuestId]);

		if (previousid == -1) {
			curquest = Quests::quests[currentid];
		}
		if (currentid != previousid && previousid != -1) {
			Quests::quests[previousid] = curquest;
			curquest = Quests::quests[currentid];
		}
		
		bool ismob = atob(Row[IsMob]);
		bool isitem = atob(Row[IsItem]);
		bool isquest = atob(Row[IsQuest]);
		int32_t id = atoi(Row[ObjectIdRequest]);
		int16_t count = atoi(Row[CountRequest]);

		if (ismob) {
			curquest.addMobRequest(id, count);
		}
		else if (isitem) {
			curquest.addItemRequest(id, count);
		}
		else if (isquest) {
			curquest.addQuestRequest(static_cast<int16_t>(id), static_cast<int8_t>(count));
		}

		previousid = currentid;
	}
	if (previousid != -1) {
		Quests::quests[previousid] = curquest;
	}

	// Quest Rewards
	query << "SELECT * FROM questrewarddata ORDER BY questid ASC";
	res = query.use();

	currentid = 0;
	previousid = -1;
	QuestRewardInfo rwa;

	enum QuestRewardData {
		RowId = 0,
		QuestIdReward, Start, Item, Exp, Mesos,
		Fame, Skill, MasterLevelOnly, Buff, ObjectIdReward,
		CountReward, MasterLevel, Gender, Job, Prop
	};
	while (Row = res.fetch_raw_row()) {
		currentid = atoi(Row[QuestIdReward]);

		if (currentid != previousid && previousid != -1) {
			Quests::quests[previousid] = curquest;
			curquest = Quests::quests[currentid];
		}

		int16_t job = atoi(Row[Job]);
		bool start = !atob(Row[Start]);

		rwa.isitem = atob(Row[Item]);
		rwa.isexp = atob(Row[Exp]);
		rwa.ismesos = atob(Row[Mesos]);
		rwa.isfame = atob(Row[Fame]);
		rwa.isskill = atob(Row[Skill]);
		rwa.ismasterlevelonly = atob(Row[MasterLevelOnly]);
		rwa.isbuff = atob(Row[Buff]);
		rwa.id = atoi(Row[ObjectIdReward]);
		rwa.count = atoi(Row[CountReward]);
		rwa.masterlevel = atoi(Row[MasterLevel]); 
		rwa.gender = atoi(Row[Gender]);
		rwa.prop = atoi(Row[Prop]);

		if ((job == -1) || (GameLogicUtilities::isNonBitJob(job))) {
			curquest.addReward(start, rwa, job);
		}
		else { // Job tracks are indicated by series of bits between 1 and 32
			 // Beginners
			if ((job & 0x01) != 0) {
				curquest.addReward(start, rwa, Jobs::JobIds::Beginner);
			}
			// Warriors
			if ((job & 0x02) != 0) {
				curquest.addReward(start, rwa, Jobs::JobIds::Swordsman);
				curquest.addReward(start, rwa, Jobs::JobIds::Fighter);
				curquest.addReward(start, rwa, Jobs::JobIds::Crusader);
				curquest.addReward(start, rwa, Jobs::JobIds::Hero);
				curquest.addReward(start, rwa, Jobs::JobIds::Page);
				curquest.addReward(start, rwa, Jobs::JobIds::WhiteKnight);
				curquest.addReward(start, rwa, Jobs::JobIds::Paladin);
				curquest.addReward(start, rwa, Jobs::JobIds::Spearman);
				curquest.addReward(start, rwa, Jobs::JobIds::DragonKnight);
				curquest.addReward(start, rwa, Jobs::JobIds::DarkKnight);
			}
			// Magicians
			if ((job & 0x04) != 0) {
				curquest.addReward(start, rwa, Jobs::JobIds::Magician);
				curquest.addReward(start, rwa, Jobs::JobIds::FPWizard);
				curquest.addReward(start, rwa, Jobs::JobIds::FPMage);
				curquest.addReward(start, rwa, Jobs::JobIds::FPArchMage);
				curquest.addReward(start, rwa, Jobs::JobIds::ILWizard);
				curquest.addReward(start, rwa, Jobs::JobIds::ILMage);
				curquest.addReward(start, rwa, Jobs::JobIds::ILArchMage);
				curquest.addReward(start, rwa, Jobs::JobIds::Cleric);
				curquest.addReward(start, rwa, Jobs::JobIds::Priest);
				curquest.addReward(start, rwa, Jobs::JobIds::Bishop);
			}
			// Bowmen
			if ((job & 0x08) != 0) {
				curquest.addReward(start, rwa, Jobs::JobIds::Archer);
				curquest.addReward(start, rwa, Jobs::JobIds::Hunter);
				curquest.addReward(start, rwa, Jobs::JobIds::Ranger);
				curquest.addReward(start, rwa, Jobs::JobIds::Bowmaster);
				curquest.addReward(start, rwa, Jobs::JobIds::Crossbowman);
				curquest.addReward(start, rwa, Jobs::JobIds::Sniper);
				curquest.addReward(start, rwa, Jobs::JobIds::Marksman);
			}
			// Thieves
			if ((job & 0x10) != 0) {
				curquest.addReward(start, rwa, Jobs::JobIds::Rogue);
				curquest.addReward(start, rwa, Jobs::JobIds::Assassin);
				curquest.addReward(start, rwa, Jobs::JobIds::Hermit);
				curquest.addReward(start, rwa, Jobs::JobIds::NightLord);
				curquest.addReward(start, rwa, Jobs::JobIds::Bandit);
				curquest.addReward(start, rwa, Jobs::JobIds::ChiefBandit);
				curquest.addReward(start, rwa, Jobs::JobIds::Shadower);
			}
			// Pirates
			if ((job & 0x20) != 0) {
				curquest.addReward(start, rwa, Jobs::JobIds::Pirate);
				curquest.addReward(start, rwa, Jobs::JobIds::Brawler);
				curquest.addReward(start, rwa, Jobs::JobIds::Marauder);
				curquest.addReward(start, rwa, Jobs::JobIds::Buccaneer);
				curquest.addReward(start, rwa, Jobs::JobIds::Gunslinger);
				curquest.addReward(start, rwa, Jobs::JobIds::Outlaw);
				curquest.addReward(start, rwa, Jobs::JobIds::Corsair);
			}
		}

		previousid = currentid;
	}
	if (previousid != -1) {
		Quests::quests[previousid] = curquest;
	}
	std::cout << "DONE" << std::endl;
}

void Initializing::initializePets() {
	std::cout << std::setw(outputWidth) << std::left << "Initializing Pets... ";

	mysqlpp::Query query = Database::getDataDB().query("SELECT id, name, hunger FROM petdata");
	mysqlpp::UseQueryResult res = query.use();

	MYSQL_ROW Row;
	PetInfo pet;

	while (Row = res.fetch_raw_row()) {
		// 0 : Pet id
		// 1 : Pet breed name
		// 2 : Pet hunger level

		pet.name = Row[1];
		pet.hunger = atoi(Row[2]);
		Pets::petsInfo[atoi(Row[0])] = pet;
	}
	
	// Pet command info
	query << "SELECT * FROM petinteractdata";
	res = query.use();
	PetInteractInfo petinteract;

	while (Row = res.fetch_raw_row()) {
		// 0 : Id
		// 1 : Command
		// 2 : Increase
		// 3 : Prob

		petinteract.increase = atoi(Row[2]);
		petinteract.prob = atoi(Row[3]);
		Pets::petsInteractInfo[atoi(Row[0])][atoi(Row[1])] = petinteract;
	}
	std::cout << "DONE" << std::endl;
}

void Initializing::initializeChat() {
	std::cout << std::setw(outputWidth) << std::left << "Initializing Commands... ";
	ChatHandler::initializeCommands();
	std::cout << "DONE" << std::endl;
}
