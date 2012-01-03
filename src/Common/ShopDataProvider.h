/*
Copyright (C) 2008-2012 Vana Development Team

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
#pragma once

#include "Types.h"
#include <boost/tr1/unordered_map.hpp>
#include <boost/utility.hpp>
#include <map>
#include <vector>

using std::map;
using std::tr1::unordered_map;
using std::vector;

class PacketCreator;

struct ShopItemInfo {
	int32_t itemId;
	int32_t price;
	int16_t quantity;
};

struct ShopInfo {
	int32_t npc;
	vector<ShopItemInfo> items;
	int8_t rechargeTier;
};

class ShopDataProvider : boost::noncopyable {
public:
	static ShopDataProvider * Instance() {
		if (singleton == nullptr)
			singleton = new ShopDataProvider();
		return singleton;
	}
	void loadData();

	bool isShop(int32_t id) { return (m_shops.find(id) != m_shops.end()); }
	void showShop(int32_t id, int16_t rechargeableBonus, PacketCreator &packet);
	int32_t getPrice(int32_t shopId, uint16_t shopIndex);
	int32_t getItemId(int32_t shopId, uint16_t shopIndex);
	int16_t getAmount(int32_t shopId, uint16_t shopIndex);
	int32_t getRechargeCost(int32_t shopId, int32_t itemId, int16_t amount);
private:
	ShopDataProvider() {}
	static ShopDataProvider *singleton;

	void loadShops();
	void loadUserShops();
	void loadRechargeTiers();

	unordered_map<int32_t, ShopInfo> m_shops;
	unordered_map<int8_t, map<int32_t, double>> m_rechargeCosts;
};