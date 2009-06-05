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
-- Athena Pierce - Bowman Instructor

if state == 0 then
	if getJob() == 0 then
		what = 1;
		addText("So you want to become the Bowman??? Well...you need to meet some requirements to do so...at ");
		addText("least #bLevel10, and 25 of DEX#k. Let's see...hmm...");
		sendNext();
	elseif ((getJob() == 300 and getLevel() < 30) or getJob() == 310 or getJob() == 320 or getJob() == 311 or getJob() == 321 or getJob() == 312 or getJob() == 322) then
		what = 2;
		addText("Do you have any questions regarding the life of the Bowman?\r\n");
		addText("#L0##bWhat are the basic characters of a Bowman?#k#l\r\n");
		addText("#L1##bWhat are the weapons that the Bowman use?#k#l\r\n");
		addText("#L2##bWhat are the armors that the bowman can wear?#k#l\r\n");
		addText("#L3##bWhat are the skills of the Bowman?#k#l");
		sendSimple();
	else
		what = 3;
		addText("Don't you want to feel the excitement of hunting down the monsters from out of nowhere? Only ");
		addText("the Bowman can do that...");
		sendNext();
	end
elseif state == 1 then
	if what == 1 then
		if (getLevel() >= 10 and getDEX() >= 25) then
			passed = true;
			addText("You look qualified for this. With a great pair of eyes being able to spot the real monsters ");
			addText("and have the coldhearted skills to shoot the arrow through them...we needed someone like that. ");
			addText("Do you want to become a Bowman?");
			sendYesNo();
		else
			addText("You need to train more. Don't think being the Bowman is a walk in the park...");
			sendBackNext();
		end 
	elseif what == 2 then
		explain = getSelected();
		if explain == 0 then
			addText("This is what being a bowman is all about. The bowman possesses just enough stamina and strength. ");
			addText("Their most important ability to use is DEX. They don't have much of a stamina, so please avoid ");
			addText("close combat if possible.");
			sendNext();
		elseif explain == 1 then
			addText("I'll explain the weapons that bowman use. Instead of using weapons to strike or slash the opponents, ");
			addText("they use long-distance weapons such as bows and rockbows to kill the monsters. They both have their ");
			addText("share of advantages and disadvantages.");
			sendNext();
		elseif explain == 2 then
			addText("I'll explain the armors the bowman use. They need to move around quickly so it won't be any good to ");
			addText("put on huge, elaborate armor. Clothes with long cumbersome laces are definitely off limits.");
			sendNext();
		elseif explain == 3 then
			addText("For bowman, skills that are available are the ones that puts their high accuracy and dexterity to good ");
			addText("use. It's a must for the Bowman to acquire skills that allows them to attack the enemies accurately.");
			sendNext();
		end
	else
		endNPC();
	end
elseif state == 2 then
	if what == 1 then
		if passed then
			if getSelected() == 1 then
				advancement = true;
				addText("Alright! You are the Bowman from here on out, because I said so...haha here's a little bit ");
				addText("of my power to you...Haahhhh!");
				sendNext();
			else
				addText("Really? Have to give more though to it, huh? Take your time, take your time. This is not ");
				addText("something you should take lightly...come talk to me once you have made your decision.");
				sendNext();
			end
		else
			endNPC();
		end
	elseif what == 2 then
		if explain == 0 then
			addText("The main advantage is that you can attack from afar, enabling you to avoid many close attacks by the ");
			addText("monsters. Not only that, but with high dexterity, you can avoid quite attacks up close. The higher the ");
			addText("DEX, the more damage you can dish out.");
			sendBackNext();
			endNPC();
		elseif explain == 1 then
			addText("Bows aren't as powerful as the rockbows, but they are much quicker to attack with. Rockbows, on the other ");
			addText("hand, are more powerful with less quickness. It'll be hard for you to make a decision on this...");
			sendBackNext();
		elseif explain == 2 then
			addText("But if you wear huge stiff armor that the warriors don, you'll be surrounded by the enemies in no time. ");
			addText("Equip yourself with simple, comfortable armor that fits you just fine and still does the job. It'll help ");
			addText("you a great deal when hunting down the monsters.");
			sendBackNext();
			endNPC();
		elseif explain == 3 then
			addText("There are two kinds of offensive skills for the bowman: #bArrow Blow#k and #bDouble Shot#k. Arrow Blow is ");
			addText("a nice, basic skills that allows you to highly damage the enemy with minimal use of MP.");
			sendBackNext();
		end
	end
elseif state == 3 then
	if what == 1 then
		if advancement then
			addText("I have added slots for your equipment and etc. inventory. You have also gotten much stronger. ");
			addText("Train harder, and you may one day reach the very top of the bowman. I'll be watching you from ");
			addText("afar. Please work hard.");
			setJob(300);
			giveSP((getLevel() - 10) * 3 + 1); -- Make up any SP for over-leveling like in GMS
			giveItem(1452051, 1); -- Beginner Bow
			giveItem(2060000, 2000); -- Arrow for Bow
			giveItem(2060000, 2000); -- Arrow for Bow
			giveItem(2060000, 2000); -- Arrow for Bow
			hpinc = 100 + getRandomNumber(50); -- Extra HP Given
			mpinc = 25 + getRandomNumber(25); -- Extra MP Given
			setMHP(getMHP() + hpinc); -- Set HP to current HP plus full amount of hp increase
			setRMHP(getRMHP() + hpinc);
			setMMP(getMMP() + mpinc); -- Set MP to current MP plus full amount of hp increase
			setRMMP(getRMMP() + mpinc);
			addSlots(1, 1);
			addSlots(2, 1);
			sendNext();
		else
			endNPC();
		end
	elseif what == 2 then
		if explain == 1 then
			addText("Good arrows and rockbows are available through monsters, so it's a must that you hunt often. It won't ");
			addText("be easy to obtain, however. Train yourself harder each and everyday, and you'll see some success coming your way ...");
			sendBackNext();
		elseif explain == 3 then
			addText("On the other hand, Double Shot allows you to attack the enemy twice using some MP. You'll only be able to ");
			addText("get it after boosting Arrow Blow to at least higher than 1, so remember that. Whatever the choice, make it your own.");
			sendBackNext();
		else
			endNPC();
		end
	end
elseif state == 4 then
	if what == 1 then
		addText("I just gave you a little bit of #bSP#k. When you open up the #bSkill menu#k on the lower left ");
		addText("corner of the screen, there are skills you can learn by using SP's. One warning, though: You ");
		addText("can't raise it all together all at once. There are also skills you can acquire only after having ");
		addText("learned a couple of skills first.");
		sendNext();
	elseif what == 2 then
		endNPC();
	end
elseif state == 5 then
	if what == 1 then
		addText("One more warning. Once you have chosen your job, try to stay alive as much as you can. Once you ");
		addText("reach that level, when you die, you will lose your experience level. You wouldn't want to lose ");
		addText("your hard-earned experience points, do you?");
		sendBackNext();
	end
elseif state == 6 then
	if what == 1 then
		addText("OK! This is all I can teach you. Go to places, train and better yourself. Find me when you feel ");
		addText("like you've done all you can, and need something interesting. I'll be waiting for you.");
		sendBackNext();
	end
elseif state == 7 then
	if what == 1 then
		addText("Oh, and... if you have any other questions about being the Bowman, feel free to ask. I don't ");
		addText("every single thing about  being the bowman, but I'll answer as many questions as I can. Til then...");
		sendBackNext();
	end
else
	endNPC();
end
