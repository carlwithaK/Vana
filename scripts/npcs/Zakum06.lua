--[[
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
--]]
-- Amon; Zakum's Altar bouncer

if getMap() == 280030000 then -- Zakum's Altar
	addText("Are you sure you want to leave this place? You are entitled to enter the Zakum Altar up to twice a day, and by leaving right now, you may only re-enter this shrine once more for the rest of the day.");
	yes = askYesNo();
    
	if yes == 1 then
		if getNumPlayers(280030000) == 1 then
			setReactorState(280030000, 2111001, 0); -- Zakum's altar
			setReactorState(211042300, 2118002, 0); -- Zakum's door
			clearDrops();
			clearMobs();
		end
		setMap(211042300); -- Door to Zakum
	end
else -- Maps 280020000 and 280020001, Zakum's jump quest
	addText("Are you sure you want to quit and leave this place? Next time you come back in, you'll have to start all over again.");
	yes = askYesNo();
    
	if yes == 1 then
		setMap(211042300); -- Door to Zakum
	end
end