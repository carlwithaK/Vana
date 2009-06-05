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
#ifndef PLAYERLOGIN_H
#define PLAYERLOGIN_H

#include "AbstractPlayer.h"
#include "PlayerStatus.h"
#include "Types.h"

class PacketReader;

class PlayerLogin : public AbstractPlayer {
public:
	PlayerLogin() : status(PlayerStatus::NotLoggedIn), invalid_logins(0) { }

	~PlayerLogin();

	void realHandleRequest(PacketReader &packet);

	void setGender(int8_t gender) { this->gender=gender; }
	void setWorld(int8_t world) { this->world = world; }
	void setChannel(uint16_t channel) {	this->channel = channel; }
	void setUserId(int32_t id) { this->userid = id; }
	void setStatus(PlayerStatus::PlayerStatus status) { this->status = status; }
	void setPin(int32_t pin) { this->pin = pin; }
	void setCharDeletePassword(int32_t char_delete_password) { this->char_delete_password = char_delete_password; }

	int8_t getGender() const { return gender; }
	int8_t getWorld() const { return world; }
	uint16_t getChannel() const { return channel; }
	int32_t getUserId() const { return userid; }
	PlayerStatus::PlayerStatus getStatus() const { return status; }
	int32_t getPin() const { return pin; }
	int32_t getCharDeletePassword() const { return char_delete_password; }

	int32_t addInvalidLogin() {	return ++invalid_logins; }
	void setOnline(bool online);
private:
	int8_t gender;
	int8_t world;
	uint16_t channel;
	PlayerStatus::PlayerStatus status;
	int32_t userid;
	int32_t pin;
	int32_t invalid_logins;
	int32_t char_delete_password;
	bool checked_pin;
};

class PlayerLoginFactory : public AbstractPlayerFactory {
public:
	AbstractPlayer * createPlayer() {
		return new PlayerLogin();
	}
};

#endif
