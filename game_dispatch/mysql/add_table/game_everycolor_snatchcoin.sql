
use chess;
SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for game_everycolor_snatchcoin
-- ----------------------------
DROP TABLE IF EXISTS `game_everycolor_snatchcoin`;
CREATE TABLE `game_everycolor_snatchcoin` (
  `uid` int(11) DEFAULT '0' NOT NULL COMMENT '用户ID',
  `type` int(11) DEFAULT '3' NOT NULL COMMENT '夺宝类型',
  `card` varchar(512) DEFAULT '' NOT NULL COMMENT '扑克数据'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=COMPACT;
