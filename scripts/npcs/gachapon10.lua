--[[
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
--]]
-- Gachapon - New Leaf City Town Street : New Leaf City - Town Center

dofile("scripts/utils/gachaponHelper.lua");

-- General rules:
-- Global item chance is not reduced
-- Gear for all classes from levels above 70 (incl 72 etc., but not 70 itself) and below or equal to 100
-- The formula used to derive weights was 90 / itemLevel

commonEquips = {};

warriorEquips = {
	{1002338, ["weight"] = 1.13}, {1002339, ["weight"] = 1.13},
	{1002340, ["weight"] = 1.13}, {1002377, ["weight"] = 0.90},
	{1002378, ["weight"] = 0.90}, {1002379, ["weight"] = 0.90},
	{1002528, ["weight"] = 1.00}, {1002529, ["weight"] = 1.00},
	{1002530, ["weight"] = 1.00}, {1002531, ["weight"] = 1.00},
	{1002532, ["weight"] = 1.00}, {1040111, ["weight"] = 1.00},
	{1040112, ["weight"] = 1.00}, {1040113, ["weight"] = 1.00},
	{1040120, ["weight"] = 0.90}, {1040121, ["weight"] = 0.90},
	{1040122, ["weight"] = 0.90}, {1041119, ["weight"] = 1.00},
	{1041120, ["weight"] = 1.00}, {1041121, ["weight"] = 1.00},
	{1041122, ["weight"] = 0.90}, {1041123, ["weight"] = 0.90},
	{1041124, ["weight"] = 0.90}, {1050080, ["weight"] = 1.13},
	{1050081, ["weight"] = 1.13}, {1050082, ["weight"] = 1.13},
	{1050083, ["weight"] = 1.13}, {1051077, ["weight"] = 1.13},
	{1051078, ["weight"] = 1.13}, {1051079, ["weight"] = 1.13},
	{1051080, ["weight"] = 1.13}, {1060100, ["weight"] = 1.00},
	{1060101, ["weight"] = 1.00}, {1060102, ["weight"] = 1.00},
	{1060109, ["weight"] = 0.90}, {1060110, ["weight"] = 0.90},
	{1060111, ["weight"] = 0.90}, {1061118, ["weight"] = 1.00},
	{1061119, ["weight"] = 1.00}, {1061120, ["weight"] = 1.00},
	{1061121, ["weight"] = 0.90}, {1061122, ["weight"] = 0.90},
	{1061123, ["weight"] = 0.90}, {1072196, ["weight"] = 1.00},
	{1072197, ["weight"] = 1.00}, {1072198, ["weight"] = 1.00},
	{1072210, ["weight"] = 1.13}, {1072211, ["weight"] = 1.13},
	{1072212, ["weight"] = 1.13}, {1072220, ["weight"] = 0.90},
	{1072221, ["weight"] = 0.90}, {1072222, ["weight"] = 0.90},
	{1082114, ["weight"] = 1.13}, {1082115, ["weight"] = 1.13},
	{1082116, ["weight"] = 1.13}, {1082117, ["weight"] = 1.13},
	{1082128, ["weight"] = 1.00}, {1082129, ["weight"] = 1.00},
	{1082130, ["weight"] = 1.00}, {1082139, ["weight"] = 0.90},
	{1082140, ["weight"] = 0.90}, {1082141, ["weight"] = 0.90},
	{1092023, ["weight"] = 1.13}, {1092024, ["weight"] = 1.13},
	{1092025, ["weight"] = 1.13}, {1092026, ["weight"] = 1.00},
	{1092027, ["weight"] = 1.00}, {1092028, ["weight"] = 1.00},
	{1092036, ["weight"] = 0.90}, {1092037, ["weight"] = 0.90},
	{1092038, ["weight"] = 0.90}, {1092061, ["weight"] = 0.90},
	{1302018, ["weight"] = 1.13}, {1302023, ["weight"] = 1.00},
	{1302056, ["weight"] = 0.90}, {1312011, ["weight"] = 1.13},
	{1312015, ["weight"] = 1.00}, {1312030, ["weight"] = 0.90},
	{1322020, ["weight"] = 1.20}, {1322028, ["weight"] = 1.13},
	{1322029, ["weight"] = 1.00}, {1322045, ["weight"] = 0.90},
	{1402004, ["weight"] = 1.13}, {1402005, ["weight"] = 1.00},
	{1402015, ["weight"] = 1.13}, {1402016, ["weight"] = 1.00},
	{1402035, ["weight"] = 0.90}, {1402037, ["weight"] = 0.90},
	{1402048, ["weight"] = 1.13}, {1402049, ["weight"] = 1.00},
	{1402050, ["weight"] = 1.00}, {1402051, ["weight"] = 1.13},
	{1412009, ["weight"] = 1.13}, {1412010, ["weight"] = 1.00},
	{1412021, ["weight"] = 0.90}, {1412040, ["weight"] = 1.00},
	{1422012, ["weight"] = 1.13}, {1422013, ["weight"] = 1.00},
	{1422027, ["weight"] = 0.90}, {1432010, ["weight"] = 1.13},
	{1432011, ["weight"] = 1.00}, {1432030, ["weight"] = 0.90},
	{1432056, ["weight"] = 0.90}, {1442019, ["weight"] = 1.13},
	{1442020, ["weight"] = 1.00}, {1442044, ["weight"] = 0.90},
};

magicianEquips = {
	{1002271, ["weight"] = 1.13}, {1002272, ["weight"] = 1.13},
	{1002273, ["weight"] = 1.13}, {1002274, ["weight"] = 1.13},
	{1002363, ["weight"] = 1.00}, {1002364, ["weight"] = 1.00},
	{1002365, ["weight"] = 1.00}, {1002366, ["weight"] = 1.00},
	{1002398, ["weight"] = 0.90}, {1002399, ["weight"] = 0.90},
	{1002400, ["weight"] = 0.90}, {1002401, ["weight"] = 0.90},
	{1050072, ["weight"] = 1.15}, {1050073, ["weight"] = 1.15},
	{1050074, ["weight"] = 1.15}, {1050092, ["weight"] = 1.02},
	{1050093, ["weight"] = 1.02}, {1050094, ["weight"] = 1.02},
	{1050095, ["weight"] = 1.02}, {1050102, ["weight"] = 0.92},
	{1050103, ["weight"] = 0.92}, {1050104, ["weight"] = 0.92},
	{1050105, ["weight"] = 0.92}, {1051056, ["weight"] = 1.15},
	{1051057, ["weight"] = 1.15}, {1051058, ["weight"] = 1.15},
	{1051094, ["weight"] = 1.02}, {1051095, ["weight"] = 1.02},
	{1051096, ["weight"] = 1.02}, {1051097, ["weight"] = 1.02},
	{1051101, ["weight"] = 0.92}, {1051102, ["weight"] = 0.92},
	{1051103, ["weight"] = 0.92}, {1051104, ["weight"] = 0.92},
	{1072177, ["weight"] = 1.13}, {1072178, ["weight"] = 1.13},
	{1072179, ["weight"] = 1.13}, {1072206, ["weight"] = 1.00},
	{1072207, ["weight"] = 1.00}, {1072208, ["weight"] = 1.00},
	{1072209, ["weight"] = 1.00}, {1072223, ["weight"] = 0.90},
	{1072224, ["weight"] = 0.90}, {1072225, ["weight"] = 0.90},
	{1072226, ["weight"] = 0.90}, {1082121, ["weight"] = 1.13},
	{1082122, ["weight"] = 1.13}, {1082123, ["weight"] = 1.13},
	{1082131, ["weight"] = 1.00}, {1082132, ["weight"] = 1.00},
	{1082133, ["weight"] = 1.00}, {1082134, ["weight"] = 1.00},
	{1082151, ["weight"] = 0.90}, {1082152, ["weight"] = 0.90},
	{1082153, ["weight"] = 0.90}, {1082154, ["weight"] = 0.90},
	{1372009, ["weight"] = 1.02}, {1372010, ["weight"] = 0.92},
	{1372016, ["weight"] = 1.15}, {1382008, ["weight"] = 1.06},
	{1382010, ["weight"] = 1.20}, {1382016, ["weight"] = 0.98},
	{1382035, ["weight"] = 0.95}, {1382056, ["weight"] = 1.13},
	{1382060, ["weight"] = 0.90},
};

bowmanEquips = {
	{1002275, ["weight"] = 1.13}, {1002276, ["weight"] = 1.13},
	{1002277, ["weight"] = 1.13}, {1002278, ["weight"] = 1.13},
	{1002402, ["weight"] = 1.00}, {1002403, ["weight"] = 1.00},
	{1002404, ["weight"] = 1.00}, {1002405, ["weight"] = 1.00},
	{1002406, ["weight"] = 0.90}, {1002407, ["weight"] = 0.90},
	{1002408, ["weight"] = 0.90}, {1050075, ["weight"] = 1.13},
	{1050076, ["weight"] = 1.13}, {1050077, ["weight"] = 1.13},
	{1050078, ["weight"] = 1.13}, {1050088, ["weight"] = 1.00},
	{1050089, ["weight"] = 1.00}, {1050090, ["weight"] = 1.00},
	{1050091, ["weight"] = 1.00}, {1050106, ["weight"] = 0.90},
	{1050107, ["weight"] = 0.90}, {1050108, ["weight"] = 0.90},
	{1051066, ["weight"] = 1.13}, {1051067, ["weight"] = 1.13},
	{1051068, ["weight"] = 1.13}, {1051069, ["weight"] = 1.13},
	{1051082, ["weight"] = 1.00}, {1051083, ["weight"] = 1.00},
	{1051084, ["weight"] = 1.00}, {1051085, ["weight"] = 1.00},
	{1051105, ["weight"] = 0.90}, {1051106, ["weight"] = 0.90},
	{1051107, ["weight"] = 0.90}, {1072182, ["weight"] = 1.13},
	{1072183, ["weight"] = 1.13}, {1072184, ["weight"] = 1.13},
	{1072185, ["weight"] = 1.13}, {1072203, ["weight"] = 1.00},
	{1072204, ["weight"] = 1.00}, {1072205, ["weight"] = 1.00},
	{1072227, ["weight"] = 0.90}, {1072228, ["weight"] = 0.90},
	{1072229, ["weight"] = 0.90}, {1082109, ["weight"] = 1.13},
	{1082110, ["weight"] = 1.13}, {1082111, ["weight"] = 1.13},
	{1082112, ["weight"] = 1.13}, {1082125, ["weight"] = 1.00},
	{1082126, ["weight"] = 1.00}, {1082127, ["weight"] = 1.00},
	{1082158, ["weight"] = 0.90}, {1082159, ["weight"] = 0.90},
	{1082160, ["weight"] = 0.90}, {1452012, ["weight"] = 1.13},
	{1452013, ["weight"] = 1.13}, {1452014, ["weight"] = 1.13},
	{1452015, ["weight"] = 1.13}, {1452017, ["weight"] = 1.00},
	{1452019, ["weight"] = 0.90}, {1452020, ["weight"] = 0.90},
	{1452021, ["weight"] = 0.90}, {1452025, ["weight"] = 1.00},
	{1452026, ["weight"] = 1.00}, {1452060, ["weight"] = 0.90},
	{1462010, ["weight"] = 1.13}, {1462011, ["weight"] = 1.13},
	{1462012, ["weight"] = 1.13}, {1462013, ["weight"] = 1.13},
	{1462015, ["weight"] = 0.90}, {1462016, ["weight"] = 0.90},
	{1462017, ["weight"] = 0.90}, {1462018, ["weight"] = 1.00},
	{1462021, ["weight"] = 1.00}, {1462022, ["weight"] = 1.00},
	{1462049, ["weight"] = 1.13}, {1462052, ["weight"] = 1.13},
	{1462053, ["weight"] = 1.00}, {1462054, ["weight"] = 1.00},
	{1462055, ["weight"] = 1.13},
};

thiefEquips = {
	{1002323, ["weight"] = 1.00}, {1002324, ["weight"] = 1.00},
	{1002325, ["weight"] = 1.00}, {1002326, ["weight"] = 1.00},
	{1002327, ["weight"] = 1.13}, {1002328, ["weight"] = 1.13},
	{1002329, ["weight"] = 1.13}, {1002330, ["weight"] = 1.13},
	{1002380, ["weight"] = 0.90}, {1002381, ["weight"] = 0.90},
	{1002382, ["weight"] = 0.90}, {1002383, ["weight"] = 0.90},
	{1040108, ["weight"] = 1.13}, {1040109, ["weight"] = 1.13},
	{1040110, ["weight"] = 1.13}, {1040115, ["weight"] = 1.00},
	{1040116, ["weight"] = 1.00}, {1040117, ["weight"] = 1.00},
	{1040118, ["weight"] = 1.00}, {1041105, ["weight"] = 1.13},
	{1041106, ["weight"] = 1.13}, {1041107, ["weight"] = 1.13},
	{1041115, ["weight"] = 1.00}, {1041116, ["weight"] = 1.00},
	{1041117, ["weight"] = 1.00}, {1041118, ["weight"] = 1.00},
	{1050096, ["weight"] = 0.90}, {1050097, ["weight"] = 0.90},
	{1050098, ["weight"] = 0.90}, {1050099, ["weight"] = 0.90},
	{1051090, ["weight"] = 0.90}, {1051091, ["weight"] = 0.90},
	{1051092, ["weight"] = 0.90}, {1051093, ["weight"] = 0.90},
	{1060097, ["weight"] = 1.13}, {1060098, ["weight"] = 1.13},
	{1060099, ["weight"] = 1.13}, {1060104, ["weight"] = 1.00},
	{1060105, ["weight"] = 1.00}, {1060106, ["weight"] = 1.00},
	{1060107, ["weight"] = 1.00}, {1061104, ["weight"] = 1.13},
	{1061105, ["weight"] = 1.13}, {1061106, ["weight"] = 1.13},
	{1061114, ["weight"] = 1.00}, {1061115, ["weight"] = 1.00},
	{1061116, ["weight"] = 1.00}, {1061117, ["weight"] = 1.00},
	{1072172, ["weight"] = 1.13}, {1072173, ["weight"] = 1.13},
	{1072174, ["weight"] = 1.13}, {1072192, ["weight"] = 1.00},
	{1072193, ["weight"] = 1.00}, {1072194, ["weight"] = 1.00},
	{1072195, ["weight"] = 1.00}, {1072213, ["weight"] = 0.90},
	{1072214, ["weight"] = 0.90}, {1072215, ["weight"] = 0.90},
	{1072216, ["weight"] = 0.90}, {1082118, ["weight"] = 1.13},
	{1082119, ["weight"] = 1.13}, {1082120, ["weight"] = 1.13},
	{1082135, ["weight"] = 0.90}, {1082136, ["weight"] = 0.90},
	{1082137, ["weight"] = 0.90}, {1082138, ["weight"] = 0.90},
	{1082142, ["weight"] = 1.00}, {1082143, ["weight"] = 1.00},
	{1082144, ["weight"] = 1.00}, {1092050, ["weight"] = 1.13},
	{1332023, ["weight"] = 1.13}, {1332027, ["weight"] = 1.00},
	{1332052, ["weight"] = 0.90}, {1332069, ["weight"] = 1.13},
	{1332072, ["weight"] = 1.13}, {1332077, ["weight"] = 1.13},
	{1332078, ["weight"] = 1.00}, {1332079, ["weight"] = 1.00},
	{1332080, ["weight"] = 1.13}, {1472031, ["weight"] = 1.13},
	{1472033, ["weight"] = 1.00}, {1472053, ["weight"] = 0.90},
	{1472072, ["weight"] = 1.13}, {1472073, ["weight"] = 1.00},
	{1472074, ["weight"] = 1.00}, {1472075, ["weight"] = 1.13},
};

pirateEquips = {
	{1002640, ["weight"] = 1.13}, {1002643, ["weight"] = 1.00},
	{1002646, ["weight"] = 0.90}, {1052125, ["weight"] = 1.13},
	{1052128, ["weight"] = 1.00}, {1052131, ["weight"] = 0.90},
	{1072312, ["weight"] = 1.13}, {1072315, ["weight"] = 1.00},
	{1072318, ["weight"] = 0.90}, {1082207, ["weight"] = 1.13},
	{1082210, ["weight"] = 1.00}, {1082213, ["weight"] = 0.90},
	{1482010, ["weight"] = 1.13}, {1482011, ["weight"] = 1.00},
	{1482012, ["weight"] = 0.90}, {1492010, ["weight"] = 1.13},
	{1492011, ["weight"] = 1.00}, {1492012, ["weight"] = 0.90},
};

items = merge(commonEquips, warriorEquips, magicianEquips, bowmanEquips, thiefEquips, pirateEquips);

gachapon({
	["items"] = items,
});