/*
Copyright (C) 2008-2014 Vana Development Team

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
#include "ValidCharDataProvider.hpp"
#include "Algorithm.hpp"
#include "Database.hpp"
#include "GameConstants.hpp"
#include "GameLogicUtilities.hpp"
#include "InitializeCommon.hpp"
#include "StringUtilities.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>

auto ValidCharDataProvider::loadData() -> void {
	std::cout << std::setw(Initializing::OutputWidth) << std::left << "Initializing Char Info... ";

	loadForbiddenNames();
	loadCreationItems();

	std::cout << "DONE" << std::endl;
}

auto ValidCharDataProvider::loadForbiddenNames() -> void {
	m_forbiddenNames.clear();

	soci::rowset<> rs = (Database::getDataDb().prepare << "SELECT * FROM " << Database::makeDataTable("character_forbidden_names"));

	for (const auto &row : rs) {
		m_forbiddenNames.push_back(row.get<string_t>("forbidden_name"));
	}
}

auto ValidCharDataProvider::loadCreationItems() -> void {
	m_adventurer.clear();
	m_cygnus.clear();

	soci::rowset<> rs = (Database::getDataDb().prepare << "SELECT * FROM " << Database::makeDataTable("character_creation_data"));

	for (const auto &row : rs) {
		int8_t genderId = GameLogicUtilities::getGenderId(row.get<string_t>("gender"));
		int32_t objectId = row.get<int32_t>("objectid");
		int8_t classId = -1;

		StringUtilities::runEnum(row.get<string_t>("character_type"), [&classId](const string_t &cmp) {
			if (cmp == "regular") classId = Adventurer;
			else if (cmp == "cygnus") classId = Cygnus;
		});

		auto &items = genderId == Gender::Male ?
			(classId == Adventurer ? m_adventurer.male : m_cygnus.male) :
			(classId == Adventurer ? m_adventurer.female : m_cygnus.female);

		StringUtilities::runEnum(row.get<string_t>("object_type"), [&items, &objectId](const string_t &cmp) {
			if (cmp == "face") items.faces.push_back(objectId);
			else if (cmp == "hair") items.hair.push_back(objectId);
			else if (cmp == "haircolor") items.haircolor.push_back(objectId);
			else if (cmp == "skin") items.skin.push_back(objectId);
			else if (cmp == "top") items.top.push_back(objectId);
			else if (cmp == "bottom") items.bottom.push_back(objectId);
			else if (cmp == "shoes") items.shoes.push_back(objectId);
			else if (cmp == "weapon") items.weapons.push_back(objectId);
		});
	}
}

auto ValidCharDataProvider::isForbiddenName(const string_t &cmp) const -> bool {
	string_t c = StringUtilities::removeSpaces(StringUtilities::toLower(cmp));
	return ext::any_of(m_forbiddenNames, [&c](const string_t &s) -> bool {
		return c.find(s, 0) != string_t::npos;
	});
}

auto ValidCharDataProvider::isValidCharacter(int8_t genderId, int32_t hair, int32_t haircolor, int32_t eyes, int32_t skin, int32_t top, int32_t bottom, int32_t shoes, int32_t weapon, int8_t classId) const -> bool {
	if (genderId != Gender::Male && genderId != Gender::Female) {
		return false;
	}

	auto &items = getItems(genderId, classId);
	bool valid = isValidItem(hair, items, ValidItemType::Hair);
	if (valid) valid = isValidItem(haircolor, items, ValidItemType::HairColor);
	if (valid) valid = isValidItem(eyes, items, ValidItemType::Face);
	if (valid) valid = isValidItem(skin, items, ValidItemType::Skin);
	if (valid) valid = isValidItem(top, items, ValidItemType::Top);
	if (valid) valid = isValidItem(bottom, items, ValidItemType::Bottom);
	if (valid) valid = isValidItem(shoes, items, ValidItemType::Shoes);
	if (valid) valid = isValidItem(weapon, items, ValidItemType::Weapon);
	return valid;
}

auto ValidCharDataProvider::isValidItem(int32_t id, const ValidItems &items, ValidItemType type) const -> bool {
	auto idTest = [id](int32_t test) -> bool { return id == test; };

	switch (type) {
		case ValidItemType::Face: return ext::any_of(items.faces, idTest);
		case ValidItemType::Hair: return ext::any_of(items.hair, idTest);
		case ValidItemType::HairColor: return ext::any_of(items.haircolor, idTest);
		case ValidItemType::Skin: return ext::any_of(items.skin, idTest);
		case ValidItemType::Top: return ext::any_of(items.top, idTest);
		case ValidItemType::Bottom: return ext::any_of(items.bottom, idTest);
		case ValidItemType::Shoes: return ext::any_of(items.shoes, idTest);
		case ValidItemType::Weapon: return ext::any_of(items.weapons, idTest);
	}
	throw std::domain_error("Missing ValidItemType");
}

auto ValidCharDataProvider::getItems(int8_t genderId, int8_t classId) const -> const ValidItems & {
	return genderId == Gender::Male ?
		(classId == Adventurer ? m_adventurer.male : m_cygnus.male) :
		(classId == Adventurer ? m_adventurer.female : m_cygnus.female);
}