/*
Navicat MySQL Data Transfer

Source Server         : 192.168.117.129
Source Server Version : 50132
Source Host           : 192.168.117.129:3306
Source Database       : dcmd

Target Server Type    : MYSQL
Target Server Version : 50132
File Encoding         : 65001

Date: 2014-04-21 14:57:51
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for dcmd_app
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_app`;
CREATE TABLE `dcmd_app` (
  `app_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `app` varchar(128) NOT NULL,
  `gid` int(10) unsigned NOT NULL,
  `department` varchar(64) NOT NULL,
  `owner` varchar(256) NOT NULL,
  `owner_email` varchar(512) NOT NULL,
  `owner_tel` varchar(512) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`app_id`),
  UNIQUE KEY `app` (`app`)
) ENGINE=InnoDB AUTO_INCREMENT=56 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_center
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_center`;
CREATE TABLE `dcmd_center` (
  `host` varchar(32) NOT NULL,
  `master` int(11) NOT NULL,
  `update_time` datetime NOT NULL,
  PRIMARY KEY (`host`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_command
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_command`;
CREATE TABLE `dcmd_command` (
  `cmd_id` bigint(20) NOT NULL AUTO_INCREMENT,
  `task_id` int(10) unsigned NOT NULL,
  `subtask_id` bigint(20) NOT NULL,
  `svr_pool` varchar(128) DEFAULT NULL,
  `svr_pool_id` int(10) unsigned NOT NULL,
  `svr_name` varchar(128) DEFAULT NULL,
  `ip` varchar(16) NOT NULL,
  `cmd_type` int(11) NOT NULL,
  `state` int(11) NOT NULL,
  `err_msg` varchar(512) DEFAULT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`cmd_id`),
  KEY `idx_command_svr` (`svr_name`,`ip`)
) ENGINE=InnoDB AUTO_INCREMENT=340 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_command_history
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_command_history`;
CREATE TABLE `dcmd_command_history` (
  `cmd_id` bigint(20) NOT NULL AUTO_INCREMENT,
  `task_id` int(10) unsigned NOT NULL,
  `subtask_id` bigint(20) NOT NULL,
  `svr_pool` varchar(128) DEFAULT NULL,
  `svr_pool_id` int(10) unsigned NOT NULL,
  `svr_name` varchar(128) DEFAULT NULL,
  `ip` varchar(16) NOT NULL,
  `cmd_type` int(11) NOT NULL,
  `state` int(11) NOT NULL,
  `err_msg` varchar(512) DEFAULT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`cmd_id`),
  KEY `idx_command_svr` (`svr_name`,`ip`)
) ENGINE=InnoDB AUTO_INCREMENT=340 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_group
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_group`;
CREATE TABLE `dcmd_group` (
  `gid` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `group_name` varchar(64) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`gid`),
  UNIQUE KEY `group_name` (`group_name`)
) ENGINE=InnoDB AUTO_INCREMENT=112 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_group_opr_cmd
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_group_opr_cmd`;
CREATE TABLE `dcmd_group_opr_cmd` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `gid` int(10) unsigned NOT NULL,
  `opr_cmd_id` int(10) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `gid` (`gid`,`opr_cmd_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_group_service
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_group_service`;
CREATE TABLE `dcmd_group_service` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `gid` int(10) unsigned NOT NULL,
  `svr_id` int(10) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `gid` (`gid`,`svr_id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=36 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_node
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_node`;
CREATE TABLE `dcmd_node` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `ip` varchar(16) NOT NULL,
  `nid` int(10) unsigned NOT NULL,
  `nodename` varchar(128) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `ip` (`ip`),
  KEY `idx_node_host` (`nodename`)
) ENGINE=InnoDB AUTO_INCREMENT=60 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_node_group
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_node_group`;
CREATE TABLE `dcmd_node_group` (
  `nid` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `node_group` varchar(128) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`nid`),
  UNIQUE KEY `node_group` (`node_group`)
) ENGINE=InnoDB AUTO_INCREMENT=24 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_opr_cmd
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_opr_cmd`;
CREATE TABLE `dcmd_opr_cmd` (
  `opr_cmd_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `opr_cmd` varchar(64) NOT NULL,
  `ui_name` varchar(255) NOT NULL,
  `run_user` varchar(64) NOT NULL,
  `script_md5` varchar(32) NOT NULL DEFAULT '',
  `agent_mutable` int(11) NOT NULL,
  `timeout` int(11) NOT NULL,
  `comment` varchar(512) DEFAULT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`opr_cmd_id`),
  UNIQUE KEY `opr_cmd` (`opr_cmd`),
  UNIQUE KEY `ui_name` (`ui_name`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_opr_cmd_arg
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_opr_cmd_arg`;
CREATE TABLE `dcmd_opr_cmd_arg` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `opr_cmd_id` int(10) unsigned NOT NULL,
  `arg_name` varchar(32) NOT NULL,
  `optional` int(11) NOT NULL,
  `mutable` int(11) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `opr_cmd_id` (`opr_cmd_id`,`arg_name`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_opr_cmd_exec
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_opr_cmd_exec`;
CREATE TABLE `dcmd_opr_cmd_exec` (
  `exec_id` bigint(20) NOT NULL AUTO_INCREMENT,
  `exec_name` varchar(64) NOT NULL,
  `opr_cmd_id` int(10) unsigned NOT NULL,
  `opr_cmd` varchar(64) NOT NULL,
  `run_user` varchar(64) NOT NULL,
  `timeout` int(11) NOT NULL,
  `ip` text NOT NULL,
  `is_repeat` int(11) NOT NULL,
  `cache_time` int(11) NOT NULL,
  `arg_mutable` int(11) NOT NULL,
  `arg` text,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`exec_id`)
) ENGINE=InnoDB AUTO_INCREMENT=8 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_opr_cmd_exec_history
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_opr_cmd_exec_history`;
CREATE TABLE `dcmd_opr_cmd_exec_history` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `exec_id` bigint(20) NOT NULL,
  `exec_name` varchar(64) NOT NULL,
  `opr_cmd_id` int(10) unsigned NOT NULL,
  `opr_cmd` varchar(64) NOT NULL,
  `run_user` varchar(64) NOT NULL,
  `timeout` int(11) NOT NULL,
  `ip` text NOT NULL,
  `is_repeat` int(11) NOT NULL,
  `cache_time` int(11) NOT NULL,
  `arg_mutable` int(11) NOT NULL,
  `arg` text,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_exec_id` (`exec_id`)
) ENGINE=InnoDB AUTO_INCREMENT=458 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_opr_log
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_opr_log`;
CREATE TABLE `dcmd_opr_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `log_table` varchar(64) NOT NULL,
  `opr_type` int(11) NOT NULL,
  `sql_statement` text NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_opr_log_table` (`log_table`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_service
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_service`;
CREATE TABLE `dcmd_service` (
  `svr_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `svr_name` varchar(128) NOT NULL,
  `app_id` int(10) unsigned NOT NULL,
  `gid` int(10) unsigned NOT NULL,
  `owner` varchar(256) NOT NULL,
  `owner_email` varchar(512) NOT NULL,
  `owner_tel` varchar(512) NOT NULL,
  `svr_path` varchar(256) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`svr_id`),
  UNIQUE KEY `svr_name` (`svr_name`)
) ENGINE=InnoDB AUTO_INCREMENT=30 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_service_pool
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_service_pool`;
CREATE TABLE `dcmd_service_pool` (
  `svr_pool_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `svr_pool` varchar(128) NOT NULL,
  `svr_id` int(10) unsigned NOT NULL,
  `app_id` int(10) NOT NULL,
  `gid` int(10) unsigned NOT NULL,
  `repo` varchar(512) NOT NULL,
  `run_user` varchar(32) NOT NULL,
  `env_ver` varchar(64) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`svr_pool_id`),
  UNIQUE KEY `svr_pool` (`svr_pool`),
  KEY `idx_app_pool_service` (`svr_id`)
) ENGINE=InnoDB AUTO_INCREMENT=18 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_service_pool_node
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_service_pool_node`;
CREATE TABLE `dcmd_service_pool_node` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `svr_pool_id` int(10) unsigned NOT NULL,
  `svr_id` int(10) unsigned NOT NULL,
  `nid` int(11) NOT NULL,
  `app_id` int(11) NOT NULL,
  `ip` varchar(16) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `svr_pool_id` (`svr_pool_id`,`ip`)
) ENGINE=InnoDB AUTO_INCREMENT=20 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_service_task_cmd
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_service_task_cmd`;
CREATE TABLE `dcmd_service_task_cmd` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `svr_id` int(10) unsigned NOT NULL,
  `task_cmd_id` int(10) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `svr_id` (`svr_id`,`task_cmd_id`)
) ENGINE=InnoDB AUTO_INCREMENT=40 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_softpkg
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_softpkg`;
CREATE TABLE `dcmd_softpkg` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `app_id` int(11) unsigned NOT NULL,
  `svr_id` int(11) unsigned NOT NULL,
  `svr_version` varchar(64) NOT NULL,
  `repo_file` varchar(256) NOT NULL,
  `upload_file` varchar(256) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `repo_file` (`repo_file`)
) ENGINE=MyISAM AUTO_INCREMENT=26 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_task
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_task`;
CREATE TABLE `dcmd_task` (
  `task_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `task_name` varchar(255) DEFAULT NULL,
  `task_cmd` varchar(64) NOT NULL,
  `depend_task_id` int(10) unsigned NOT NULL,
  `depend_task_name` varchar(255) DEFAULT NULL,
  `svr_id` int(10) unsigned NOT NULL,
  `svr_name` varchar(128) NOT NULL,
  `gid` int(10) unsigned NOT NULL,
  `group_name` varchar(64) NOT NULL,
  `tag` varchar(128) NOT NULL,
  `update_env` int(10) NOT NULL,
  `update_tag` int(10) NOT NULL,
  `state` int(11) NOT NULL,
  `freeze` int(11) NOT NULL,
  `valid` int(11) NOT NULL,
  `pause` int(11) NOT NULL,
  `err_msg` varchar(512) NOT NULL,
  `concurrent_num` int(10) NOT NULL,
  `concurrent_rate` int(10) NOT NULL,
  `timeout` int(10) NOT NULL,
  `auto` int(11) NOT NULL,
  `process` int(10) NOT NULL,
  `task_arg` text,
  `svr_path` varchar(256) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`task_id`),
  UNIQUE KEY `idx_task_name` (`task_name`),
  KEY `idx_dcmd_svr_task_name` (`svr_name`,`task_name`)
) ENGINE=InnoDB AUTO_INCREMENT=76 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_task_cmd
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_task_cmd`;
CREATE TABLE `dcmd_task_cmd` (
  `task_cmd_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `task_cmd` varchar(64) NOT NULL,
  `script_md5` varchar(32) NOT NULL,
  `timeout` int(11) NOT NULL,
  `comment` varchar(512) DEFAULT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`task_cmd_id`),
  UNIQUE KEY `task_cmd` (`task_cmd`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_task_cmd_arg
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_task_cmd_arg`;
CREATE TABLE `dcmd_task_cmd_arg` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `task_cmd_id` int(10) unsigned NOT NULL,
  `arg_name` varchar(32) NOT NULL,
  `optional` int(10) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `task_cmd` (`task_cmd_id`,`arg_name`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_task_history
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_task_history`;
CREATE TABLE `dcmd_task_history` (
  `task_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `task_name` varchar(255) DEFAULT NULL,
  `task_cmd` varchar(64) NOT NULL,
  `depend_task_id` int(10) unsigned NOT NULL,
  `depend_task_name` varchar(255) DEFAULT NULL,
  `svr_id` int(10) unsigned NOT NULL,
  `svr_name` varchar(128) NOT NULL,
  `gid` int(10) unsigned NOT NULL,
  `group_name` varchar(64) NOT NULL,
  `tag` varchar(128) NOT NULL,
  `update_env` int(10) NOT NULL,
  `update_tag` int(10) NOT NULL,
  `state` int(11) NOT NULL,
  `freeze` int(11) NOT NULL,
  `valid` int(11) NOT NULL,
  `pause` int(11) NOT NULL,
  `err_msg` varchar(512) NOT NULL,
  `concurrent_num` int(10) NOT NULL,
  `concurrent_rate` int(10) NOT NULL,
  `timeout` int(10) NOT NULL,
  `auto` int(11) NOT NULL,
  `process` int(10) NOT NULL,
  `task_arg` text,
  `svr_path` varchar(256) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`task_id`),
  UNIQUE KEY `idx_task_finish_name` (`task_name`),
  KEY `idx_task_svr_task_finish_name` (`svr_name`,`task_name`)
) ENGINE=InnoDB AUTO_INCREMENT=76 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_task_node
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_task_node`;
CREATE TABLE `dcmd_task_node` (
  `subtask_id` bigint(20) NOT NULL AUTO_INCREMENT,
  `task_id` int(10) unsigned NOT NULL,
  `task_cmd` varchar(64) NOT NULL,
  `svr_pool` varchar(128) DEFAULT NULL,
  `svr_name` varchar(128) DEFAULT NULL,
  `ip` varchar(16) NOT NULL,
  `state` int(11) NOT NULL,
  `ignored` int(11) NOT NULL,
  `start_time` datetime NOT NULL,
  `finish_time` datetime NOT NULL,
  `process` varchar(32) DEFAULT NULL,
  `err_msg` varchar(512) DEFAULT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`subtask_id`),
  UNIQUE KEY `task_id` (`task_id`,`ip`,`svr_pool`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=70 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_task_node_history
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_task_node_history`;
CREATE TABLE `dcmd_task_node_history` (
  `subtask_id` bigint(20) NOT NULL AUTO_INCREMENT,
  `task_id` int(10) unsigned NOT NULL,
  `task_cmd` varchar(64) NOT NULL,
  `svr_pool` varchar(128) DEFAULT NULL,
  `svr_name` varchar(128) DEFAULT NULL,
  `ip` varchar(16) NOT NULL,
  `state` int(11) NOT NULL,
  `ignored` int(11) NOT NULL,
  `start_time` datetime NOT NULL,
  `finish_time` datetime NOT NULL,
  `process` varchar(32) DEFAULT NULL,
  `err_msg` varchar(512) DEFAULT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`subtask_id`),
  UNIQUE KEY `task_id` (`task_id`,`ip`,`svr_pool`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=70 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_task_service_pool
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_task_service_pool`;
CREATE TABLE `dcmd_task_service_pool` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `task_id` int(10) unsigned NOT NULL,
  `task_cmd` varchar(64) NOT NULL,
  `svr_pool` varchar(128) DEFAULT NULL,
  `svr_pool_id` int(10) unsigned NOT NULL,
  `env_ver` varchar(64) NOT NULL,
  `repo` varchar(512) NOT NULL,
  `run_user` varchar(32) NOT NULL,
  `undo_node` int(10) unsigned NOT NULL,
  `doing_node` int(10) unsigned NOT NULL,
  `finish_node` int(10) unsigned NOT NULL,
  `fail_node` int(10) unsigned NOT NULL,
  `ignored_fail_node` int(10) unsigned NOT NULL,
  `ignored_doing_node` int(10) unsigned NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique_task_id_svr_pool_id` (`task_id`,`svr_pool_id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=124 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_task_service_pool_history
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_task_service_pool_history`;
CREATE TABLE `dcmd_task_service_pool_history` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `task_id` int(10) unsigned NOT NULL,
  `task_cmd` varchar(64) NOT NULL,
  `svr_pool` varchar(128) DEFAULT NULL,
  `svr_pool_id` int(10) unsigned NOT NULL,
  `env_ver` varchar(64) NOT NULL,
  `repo` varchar(512) NOT NULL,
  `run_user` varchar(32) NOT NULL,
  `undo_node` int(10) unsigned NOT NULL,
  `doing_node` int(10) unsigned NOT NULL,
  `finish_node` int(10) unsigned NOT NULL,
  `fail_node` int(10) unsigned NOT NULL,
  `ignored_fail_node` int(10) unsigned NOT NULL,
  `ignored_doing_node` int(10) unsigned NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `task_id` (`task_id`,`svr_pool`)
) ENGINE=InnoDB AUTO_INCREMENT=124 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_task_template
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_task_template`;
CREATE TABLE `dcmd_task_template` (
  `task_tmpt_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `task_tmpt_name` varchar(255) DEFAULT NULL,
  `task_cmd_id` int(10) NOT NULL,
  `task_cmd` varchar(64) NOT NULL,
  `svr_id` int(10) unsigned NOT NULL,
  `svr_name` varchar(128) NOT NULL,
  `app_id` int(10) NOT NULL,
  `gid` int(10) unsigned NOT NULL,
  `group_name` varchar(64) NOT NULL,
  `update_env` int(10) NOT NULL,
  `concurrent_num` int(10) NOT NULL,
  `concurrent_rate` int(10) NOT NULL,
  `timeout` int(10) NOT NULL,
  `process` int(10) NOT NULL,
  `auto` int(11) NOT NULL,
  `task_arg` text,
  `comment` varchar(512) DEFAULT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`task_tmpt_id`),
  UNIQUE KEY `task_tmpt_name` (`task_tmpt_name`)
) ENGINE=InnoDB AUTO_INCREMENT=36 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_task_template_service_pool
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_task_template_service_pool`;
CREATE TABLE `dcmd_task_template_service_pool` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `task_tmpt_id` int(10) unsigned NOT NULL,
  `svr_pool_id` int(10) unsigned NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `task_tmpt_id` (`task_tmpt_id`,`svr_pool_id`)
) ENGINE=InnoDB AUTO_INCREMENT=26 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_user
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_user`;
CREATE TABLE `dcmd_user` (
  `uid` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `username` varchar(128) NOT NULL,
  `passwd` varchar(64) NOT NULL,
  `sa` int(10) NOT NULL,
  `department` varchar(64) NOT NULL,
  `tel` varchar(128) NOT NULL,
  `email` varchar(64) NOT NULL,
  `state` int(11) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`uid`),
  UNIQUE KEY `username` (`username`)
) ENGINE=InnoDB AUTO_INCREMENT=20 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for dcmd_user_group
-- ----------------------------
DROP TABLE IF EXISTS `dcmd_user_group`;
CREATE TABLE `dcmd_user_group` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `uid` int(10) unsigned NOT NULL,
  `gid` int(10) unsigned NOT NULL,
  `rolename` varchar(16) NOT NULL,
  `readonly` int(10) NOT NULL,
  `comment` varchar(512) NOT NULL,
  `utime` datetime NOT NULL,
  `ctime` datetime NOT NULL,
  `opr_uid` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uid` (`uid`,`gid`)
) ENGINE=InnoDB AUTO_INCREMENT=38 DEFAULT CHARSET=utf8;


-- ----------------------------
-- Records of dcmd_user
-- ----------------------------
INSERT INTO `dcmd_user` VALUES ('1', 'admin', '7f0e2bda548768347223d279fb940c48', '1', '', '', '', '0', '', '2014-02-20 15:55:05', '2014-02-20 15:55:02', '1');


