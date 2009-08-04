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
#include "QuestDataProvider.h"
#include "Database.h"
#include "GameLogicUtilities.h"
#include "InitializeCommon.h"
#include "MiscUtilities.h"

using Initializing::outputWidth;
using MiscUtilities::atob;

QuestDataProvider * QuestDataProvider::singleton = 0;

void QuestDataProvider::loadData() {
	std::cout << std::setw(outputWidth) << std::left << "Initializing Quests... ";
	// Quests
	mysqlpp::Query query = Database::getDataDB().query("SELECT * FROM questdata");
	mysqlpp::UseQueryResult res = query.use();
	quests.clear();

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

		quests[questid] = curquest;
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
			curquest = quests[currentid];
		}
		if (currentid != previousid && previousid != -1) {
			quests[previousid] = curquest;
			curquest = quests[currentid];
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
		quests[previousid] = curquest;
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
			quests[previousid] = curquest;
			curquest = quests[currentid];
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
		quests[previousid] = curquest;
	}
	std::cout << "DONE" << std::endl;
}

int16_t QuestDataProvider::getItemRequest(int16_t questid, int32_t itemid) {
	if (quests.find(questid) != quests.end()) {
		return quests[questid].getItemRequestQuantity(itemid);
	}
	return 0;
}

void Quest::addItemRequest(int32_t itemid, int16_t quantity) {
	itemrequests[itemid] = quantity;
}

void Quest::addMobRequest(int32_t mobid, int16_t quantity) {
	mobrequests[mobid] = quantity;
}

void Quest::addQuestRequest(int16_t questid, int8_t state) {
	questrequests[questid] = state;
}

void Quest::addReward(bool start, const QuestRewardInfo &info, int16_t job) {
	if (start) {
		if (job == -1) {
			startrewards.rewards.push_back(info);
		}
		else {
			startrewards.jobrewards[job].push_back(info);
		}
	}
	else {
		if (job == -1) {
			endrewards.rewards.push_back(info);
		}
		else {
			endrewards.jobrewards[job].push_back(info);
		}
	}
}