ALTER TABLE `characters`   
  CHANGE `chp` `chp` INT(11) DEFAULT 50  NOT NULL,
  CHANGE `mhp` `mhp` INT(11) DEFAULT 50  NOT NULL,
  CHANGE `cmp` `cmp` INT(11) DEFAULT 5  NOT NULL,
  CHANGE `mmp` `mmp` INT(11) DEFAULT 5  NOT NULL,
  CHANGE `fame` `fame` INT(11) DEFAULT 0  NOT NULL,
  CHANGE `pos` `pos` TINYINT(3) DEFAULT 0  NOT NULL;