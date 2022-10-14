-- MySQL dump 10.13  Distrib 5.7.36, for Linux (x86_64)

-- Server version	5.7.36

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;



CREATE DATABASE IF NOT EXISTS `resource_demo1` Character SET utf8 COLLATE utf8_bin;

USE `resource_demo1`;

--
-- Current Database: `resource_demo2`
--

CREATE DATABASE IF NOT EXISTS `resource_demo2` Character SET utf8 COLLATE utf8_bin;

USE `resource_demo2`;

--
-- Current Database: `resource_demo3`
--

CREATE DATABASE IF NOT EXISTS `resource_demo3` Character SET utf8 COLLATE utf8_bin;

USE `resource_demo3`;

--
-- Current Database: `nacos_config`
--

CREATE DATABASE IF NOT EXISTS `nacos_config` Character SET utf8 COLLATE utf8_bin;

USE `nacos_config`;

--
-- Table structure for table `config_info`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `config_info` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT 'id',
  `data_id` varchar(255) COLLATE utf8_bin NOT NULL COMMENT 'data_id',
  `group_id` varchar(255) COLLATE utf8_bin DEFAULT NULL,
  `content` longtext COLLATE utf8_bin NOT NULL COMMENT 'content',
  `md5` varchar(32) COLLATE utf8_bin DEFAULT NULL COMMENT 'md5',
  `gmt_create` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `gmt_modified` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `src_user` text COLLATE utf8_bin COMMENT 'source user',
  `src_ip` varchar(50) COLLATE utf8_bin DEFAULT NULL COMMENT 'source ip',
  `app_name` varchar(128) COLLATE utf8_bin DEFAULT NULL,
  `tenant_id` varchar(128) COLLATE utf8_bin DEFAULT '',
  `c_desc` varchar(256) COLLATE utf8_bin DEFAULT NULL,
  `c_use` varchar(64) COLLATE utf8_bin DEFAULT NULL,
  `effect` varchar(64) COLLATE utf8_bin DEFAULT NULL,
  `type` varchar(64) COLLATE utf8_bin DEFAULT NULL,
  `c_schema` text COLLATE utf8_bin,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_configinfo_datagrouptenant` (`data_id`,`group_id`,`tenant_id`)
) ENGINE=InnoDB AUTO_INCREMENT=135 DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='config_info';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `config_info`
--

LOCK TABLES `config_info` WRITE;
/*!40000 ALTER TABLE `config_info` DISABLE KEYS */;
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (101, 'database.yaml', 'DEFAULT_GROUP', 'spring:\n  datasource:\n    type: com.alibaba.druid.pool.DruidDataSource\n    sql-script-encoding: utf-8\n    druid:\n      primary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/privacy_demo1?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n      secondary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/privacy_demo1?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n      resourcePrimary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/resource_demo1?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n      resourceSecondary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/resource_demo1?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n', 'bc1fbb8795a977bcbe1f3ca854d8941d', '2022-06-16 09:01:01', '2022-06-17 09:05:06', 'nacos', '10.244.0.0', '', '46b6b568-e6ae-45ca-baa1-819932fc8947', '', '', '', 'yaml', '');
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (102, 'base.json', 'DEFAULT_GROUP', '{\n    \"tokenValidateUriBlackList\":[\n      \"/user/login\",\n      \"/user/register\",\n      \"/user/sendVerificationCode\",\n      \"/user/forgetPassword\",\n      \"/captcha/get\",\n      \"/captcha/check\",\n      \"/oauth/getAuthList\",\n      \"/oauth/authLogin\",\n      \"/oauth/authRegister\",\n      \"/oauth/github/render\",\n      \"/oauth/github/callback\",\n      \"/common/getValidatePublicKey\",\n      \"/shareData/syncProject\",\n      \"/shareData/syncModel\"\n    ],\n    \"needSignUriList\":[\n\n    ],\n    \"authConfigs\":{\n        \"github\": {\n            \"redirectUrl\":\"\",\n            \"authEnable\": 1,\n            \"clientId\": \"\",\n            \"clientSecret\": \"\",\n            \"redirectUri\": \"\"\n        }\n    },\n    \"adminUserIds\": [1],\n    \"defaultPassword\":\"123456\",\n    \"defaultPasswordVector\":\"excalibur\",\n    \"primihubOfficalService\": \"\",\n    \"grpcClientAddress\": \"primihub-node0\",\n    \"grpcClientPort\": 50050,\n    \"grpcDataSetClientAddress\": \"primihub-node0\",\n    \"grpcDataSetClientPort\": 50050,\n    \"grpcServerPort\": 9090,\n    \"uploadUrlDirPrefix\": \"/data/upload/\",\n    \"resultUrlDirPrefix\": \"/data/result/\",\n    \"runModelFileUrlDirPrefix\": \"/data/result/runModel/\",\n    \"usefulToken\": \"excalibur_forever_ABCDEFGHIJKLMN\",\n    \"model_components\": [\n        {\n            \"component_code\": \"start\",\n            \"component_name\": \"开始\",\n            \"is_show\": 0,\n            \"is_mandatory\": 0,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"taskName\",\n                    \"type_name\": \"任务名称\",\n                    \"input_type\": \"text\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"taskDesc\",\n                    \"type_name\": \"任务描述\",\n                    \"input_type\": \"textarea\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                }\n            ]\n        },\n        {\n            \"component_code\": \"dataSet\",\n            \"component_name\": \"选择数据集\",\n            \"is_show\": 0,\n            \"is_mandatory\": 0,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"selectData\",\n                    \"type_name\": \"选择数据\",\n                    \"input_type\": \"none\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                }\n            ]\n        },\n        {\n            \"component_code\": \"features\",\n            \"component_name\": \"特征筛选\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"features\",\n                    \"type_name\": \"特征筛选\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"唯一值筛选\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"缺失值比例筛选\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"IV值筛选\"\n                        },\n                        {\n                            \"key\": \"4\",\n                            \"val\": \"相关性筛选\"\n                        },\n                        {\n                            \"key\": \"5\",\n                            \"val\": \"用户自定义筛选\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"sample\",\n            \"component_name\": \"样本抽样设计\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"sample\",\n                    \"type_name\": \"样本抽样设计\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"最大/最小样本\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"提出灰样本\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"处理样本不均衡和分层\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"exception\",\n            \"component_name\": \"异常处理\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"exception\",\n                    \"type_name\": \"异常处理\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"删除\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"视为缺失值\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"平均值修正\"\n                        },\n                        {\n                            \"key\": \"4\",\n                            \"val\": \"盖帽法\"\n                        },\n                        {\n                            \"key\": \"5\",\n                            \"val\": \"分箱法\"\n                        },\n                        {\n                            \"key\": \"6\",\n                            \"val\": \"回归插补\"\n                        },\n                        {\n                            \"key\": \"7\",\n                            \"val\": \"多重插补\"\n                        },\n                        {\n                            \"key\": \"8\",\n                            \"val\": \"不处理\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"featureCoding\",\n            \"component_name\": \"特征编码\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"featureCoding\",\n                    \"type_name\": \"特征编码\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"标签编码\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"哈希编码\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"独热编码\"\n                        },\n                        {\n                            \"key\": \"4\",\n                            \"val\": \"计数编码\"\n                        },\n                        {\n                            \"key\": \"5\",\n                            \"val\": \"直方图编码\"\n                        },\n                        {\n                            \"key\": \"6\",\n                            \"val\": \"WOE编码\"\n                        },\n                        {\n                            \"key\": \"7\",\n                            \"val\": \"目标编码\"\n                        },\n                        {\n                            \"key\": \"8\",\n                            \"val\": \"平均编码\"\n                        },\n                        {\n                            \"key\": \"9\",\n                            \"val\": \"模型编码\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"model\",\n            \"component_name\": \"模型选择\",\n            \"is_show\": 0,\n            \"is_mandatory\": 0,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"modelType\",\n                    \"type_name\": \"模型选择\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"纵向-XGBoost\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"横向-LR\"\n                        }\n                    ]\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"modelName\",\n                    \"type_name\": \"模型名称\",\n                    \"input_type\": \"text\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"modelDesc\",\n                    \"type_name\": \"模型描述\",\n                    \"input_type\": \"textarea\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"arbiterOrgan\",\n                    \"type_name\": \"可信第三方选择\",\n                    \"input_type\": \"button\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                }\n            ]\n        },\n        {\n            \"component_code\": \"assessment\",\n            \"component_name\": \"评估模型\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": []\n        }\n    ]\n}', 'f8af9487b5f6d0c438ea71395d138378', '2022-06-16 09:26:31', '2022-10-14 10:38:49', 'nacos', '103.85.177.130', '', '46b6b568-e6ae-45ca-baa1-819932fc8947', '', '', '', 'json', '');
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (103, 'redis.yaml', 'DEFAULT_GROUP', 'spring:\n  datasource:\n    redis:\n      primary:\n        hostName: redis\n        port: 6379\n        password: primihub\n        database: 0\n        minIdle: 0\n        maxIdle: 10\n        maxTotal: 100\n        lifo: false\n        maxWaitMillis: 1000\n        minEvictableIdleTimeMillis: 1800000\n        softMinEvictableIdleTimeMillis: 1800000', 'c7410257a901774ad260e6fa8740f87a', '2022-06-16 09:28:22', '2022-06-17 09:18:27', 'nacos', '10.244.0.0', '', '46b6b568-e6ae-45ca-baa1-819932fc8947', '', '', '', 'yaml', '');
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (106, 'database.yaml', 'DEFAULT_GROUP', 'spring:\n  datasource:\n    type: com.alibaba.druid.pool.DruidDataSource\n    sql-script-encoding: utf-8\n    druid:\n      primary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/privacy_demo2?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n      secondary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/privacy_demo2?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n      resourcePrimary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/resource_demo2?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n      resourceSecondary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/resource_demo2?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n', 'de333f9c4e8fccdf735752d168bd416d', '2022-06-16 09:29:29', '2022-06-17 09:20:37', 'nacos', '10.244.0.0', '', '71843998-b60a-42ed-81d7-c3c9940d11c0', '', '', '', 'yaml', '');
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (107, 'base.json', 'DEFAULT_GROUP', '{\n    \"tokenValidateUriBlackList\":[\n      \"/user/login\",\n      \"/user/register\",\n      \"/user/sendVerificationCode\",\n      \"/user/forgetPassword\",\n      \"/captcha/get\",\n      \"/captcha/check\",\n      \"/oauth/getAuthList\",\n      \"/oauth/authLogin\",\n      \"/oauth/authRegister\",\n      \"/oauth/github/render\",\n      \"/oauth/github/callback\",\n      \"/common/getValidatePublicKey\",\n      \"/shareData/syncProject\",\n      \"/shareData/syncModel\"\n    ],\n    \"needSignUriList\":[\n\n    ],\n    \"authConfigs\":{\n        \"github\": {\n            \"redirectUrl\":\"\",\n            \"authEnable\": 1,\n            \"clientId\": \"\",\n            \"clientSecret\": \"\",\n            \"redirectUri\": \"\"\n        }\n    },\n    \"adminUserIds\": [1],\n    \"defaultPassword\":\"123456\",\n    \"defaultPasswordVector\":\"excalibur\",\n    \"primihubOfficalService\": \"\",\n    \"grpcClientAddress\": \"primihub-node1\",\n    \"grpcClientPort\": 50051,\n    \"grpcDataSetClientAddress\": \"primihub-node1\",\n    \"grpcDataSetClientPort\": 50051,\n    \"grpcServerPort\": 9090,\n    \"uploadUrlDirPrefix\": \"/data/upload/\",\n    \"resultUrlDirPrefix\": \"/data/result/\",\n    \"runModelFileUrlDirPrefix\": \"/data/result/runModel/\",\n    \"usefulToken\": \"excalibur_forever_ABCDEFGHIJKLMN\",\n    \"model_components\": [\n        {\n            \"component_code\": \"start\",\n            \"component_name\": \"开始\",\n            \"is_show\": 0,\n            \"is_mandatory\": 0,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"taskName\",\n                    \"type_name\": \"任务名称\",\n                    \"input_type\": \"text\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"taskDesc\",\n                    \"type_name\": \"任务描述\",\n                    \"input_type\": \"textarea\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                }\n            ]\n        },\n        {\n            \"component_code\": \"dataSet\",\n            \"component_name\": \"选择数据集\",\n            \"is_show\": 0,\n            \"is_mandatory\": 0,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"selectData\",\n                    \"type_name\": \"选择数据\",\n                    \"input_type\": \"none\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                }\n            ]\n        },\n        {\n            \"component_code\": \"features\",\n            \"component_name\": \"特征筛选\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"features\",\n                    \"type_name\": \"特征筛选\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"唯一值筛选\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"缺失值比例筛选\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"IV值筛选\"\n                        },\n                        {\n                            \"key\": \"4\",\n                            \"val\": \"相关性筛选\"\n                        },\n                        {\n                            \"key\": \"5\",\n                            \"val\": \"用户自定义筛选\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"sample\",\n            \"component_name\": \"样本抽样设计\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"sample\",\n                    \"type_name\": \"样本抽样设计\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"最大/最小样本\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"提出灰样本\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"处理样本不均衡和分层\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"exception\",\n            \"component_name\": \"异常处理\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"exception\",\n                    \"type_name\": \"异常处理\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"删除\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"视为缺失值\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"平均值修正\"\n                        },\n                        {\n                            \"key\": \"4\",\n                            \"val\": \"盖帽法\"\n                        },\n                        {\n                            \"key\": \"5\",\n                            \"val\": \"分箱法\"\n                        },\n                        {\n                            \"key\": \"6\",\n                            \"val\": \"回归插补\"\n                        },\n                        {\n                            \"key\": \"7\",\n                            \"val\": \"多重插补\"\n                        },\n                        {\n                            \"key\": \"8\",\n                            \"val\": \"不处理\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"featureCoding\",\n            \"component_name\": \"特征编码\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"featureCoding\",\n                    \"type_name\": \"特征编码\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"标签编码\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"哈希编码\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"独热编码\"\n                        },\n                        {\n                            \"key\": \"4\",\n                            \"val\": \"计数编码\"\n                        },\n                        {\n                            \"key\": \"5\",\n                            \"val\": \"直方图编码\"\n                        },\n                        {\n                            \"key\": \"6\",\n                            \"val\": \"WOE编码\"\n                        },\n                        {\n                            \"key\": \"7\",\n                            \"val\": \"目标编码\"\n                        },\n                        {\n                            \"key\": \"8\",\n                            \"val\": \"平均编码\"\n                        },\n                        {\n                            \"key\": \"9\",\n                            \"val\": \"模型编码\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"model\",\n            \"component_name\": \"模型选择\",\n            \"is_show\": 0,\n            \"is_mandatory\": 0,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"modelType\",\n                    \"type_name\": \"模型选择\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"纵向-XGBoost\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"横向-LR\"\n                        }\n                    ]\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"modelName\",\n                    \"type_name\": \"模型名称\",\n                    \"input_type\": \"text\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"modelDesc\",\n                    \"type_name\": \"模型描述\",\n                    \"input_type\": \"textarea\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"arbiterOrgan\",\n                    \"type_name\": \"可信第三方选择\",\n                    \"input_type\": \"button\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                }\n            ]\n        },\n        {\n            \"component_code\": \"assessment\",\n            \"component_name\": \"评估模型\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": []\n        }\n    ]\n}', '89625baece834a171b3a5a627f4f72d0', '2022-06-16 09:29:29', '2022-10-14 10:39:02', 'nacos', '103.85.177.130', '', '71843998-b60a-42ed-81d7-c3c9940d11c0', '', '', '', 'json', '');
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (108, 'redis.yaml', 'DEFAULT_GROUP', 'spring:\n  datasource:\n    redis:\n      primary:\n        hostName: redis\n        port: 6379\n        password: primihub\n        database: 1\n        minIdle: 0\n        maxIdle: 10\n        maxTotal: 100\n        lifo: false\n        maxWaitMillis: 1000\n        minEvictableIdleTimeMillis: 1800000\n        softMinEvictableIdleTimeMillis: 1800000', 'cac51f9a54a982b8210c36f1708fe486', '2022-06-16 09:29:29', '2022-06-17 09:21:22', 'nacos', '10.244.0.0', '', '71843998-b60a-42ed-81d7-c3c9940d11c0', '', '', '', 'yaml', '');
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (119, 'organ_info.json', 'DEFAULT_GROUP', '{\"fusionMap\":{\"http://fusion:8080/\":{\"registered\":true,\"serverAddress\":\"http://fusion:8080/\",\"show\":true}},\"gatewayAddress\":\"http://gateway1:8080\",\"organId\":\"7aeeb3aa-75cc-4e40-8692-ea5fd5f5f9f0\",\"organName\":\"Primihub01\",\"pinCode\":\"71HtcET1ZaUlLcLu\"}', '97d5103de211b612098a24082c74c1d7', '2022-06-29 06:57:06', '2022-08-04 08:17:12', NULL, '10.244.3.185', '', '46b6b568-e6ae-45ca-baa1-819932fc8947', NULL, NULL, NULL, 'json', NULL);
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (121, 'organ_info.json', 'DEFAULT_GROUP', '{\"fusionMap\":{\"http://fusion:8080/\":{\"registered\":true,\"serverAddress\":\"http://fusion:8080/\",\"show\":true}},\"gatewayAddress\":\"http://gateway2:8080\",\"organId\":\"3abfcb2a-8335-4bcc-b6f9-704a92e392fd\",\"organName\":\"Primihub02\",\"pinCode\":\"yGxfgPu0s2Bsh5ol\"}', '605ad2914c35c9347c9fc9980c1c1352', '2022-06-29 06:58:11', '2022-08-01 12:09:43', NULL, '10.244.3.186', '', '71843998-b60a-42ed-81d7-c3c9940d11c0', NULL, NULL, NULL, 'json', NULL);
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (127, 'database.yaml', 'DEFAULT_GROUP', 'spring:\n  datasource:\n    type: com.alibaba.druid.pool.DruidDataSource\n    sql-script-encoding: utf-8\n    druid:\n      primary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/privacy_demo3?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n      secondary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/privacy_demo3?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n      resourcePrimary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/resource_demo3?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n      resourceSecondary:\n        driver-class-name: com.mysql.cj.jdbc.Driver\n        username: primihub\n        url: jdbc:mysql://mysql:3306/resource_demo3?characterEncoding=UTF-8&zeroDateTimeBehavior=convertToNull&allowMultiQueries=true&serverTimezone=GMT&useSSL=false\n        password: primihub@123\n        connection-properties: config.decrypt=true;config.decrypt.key=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJI/xqbyvpVttxfAKulKeSTIb7tZAGaFcPyTnE2r7AHTQ8kOnqKXDda4u59umt9XBFxi7db28KxeVooB138zuRUCAwEAAQ==\n        filter:\n          config:\n            enabled: true\n        initial-size: 3\n        min-idle: 3\n        max-active: 20\n        max-wait: 60000\n        test-while-idle: true\n        time-between-connect-error-millis: 60000\n        min-evictable-idle-time-millis: 30000\n        validationQuery: select \'x\'\n        test-on-borrow: false\n        test-on-return: false\n        pool-prepared-statements: true\n        max-pool-prepared-statement-per-connection-size: 20\n        use-global-data-source-stat: false\n        filters: stat,wall,slf4j\n        connect-properties: druid.stat.mergeSql=true;druid.stat.slowSqlMillis=5000\n        time-between-log-stats-millis: 300000\n        web-stat-filter:\n          enabled: true\n          url-pattern: \'/*\'\n          exclusions: \'*.js,*.gif,*.jpg,*.bmp,*.png,*.css,*.ico,/druid/*\'\n        stat-view-servlet:\n          enabled: true\n          url-pattern: \'/druid/*\'\n          reset-enable: false\n          login-username: admin\n          login-password: admin\n', '51397bfdb70c60a18443eda93e915a4e', '2022-06-29 09:36:09', '2022-06-29 10:49:44', 'nacos', '118.26.65.66', '', '35674f9d-3369-42f6-9f30-c8de2535adc6', '', '', '', 'yaml', '');
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (128, 'base.json', 'DEFAULT_GROUP', '{\n    \"tokenValidateUriBlackList\":[\n      \"/user/login\",\n      \"/user/register\",\n      \"/user/sendVerificationCode\",\n      \"/user/forgetPassword\",\n      \"/captcha/get\",\n      \"/captcha/check\",\n      \"/oauth/getAuthList\",\n      \"/oauth/authLogin\",\n      \"/oauth/authRegister\",\n      \"/oauth/github/render\",\n      \"/oauth/github/callback\",\n      \"/common/getValidatePublicKey\",\n      \"/shareData/syncProject\",\n      \"/shareData/syncModel\"\n    ],\n    \"needSignUriList\":[\n\n    ],\n    \"authConfigs\":{\n        \"github\": {\n            \"redirectUrl\":\"\",\n            \"authEnable\": 1,\n            \"clientId\": \"\",\n            \"clientSecret\": \"\",\n            \"redirectUri\": \"\"\n        }\n    },\n    \"adminUserIds\": [1],\n    \"defaultPassword\":\"123456\",\n    \"defaultPasswordVector\":\"excalibur\",\n    \"primihubOfficalService\": \"\",\n    \"grpcClientAddress\": \"primihub-node2\",\n    \"grpcClientPort\": 50052,\n    \"grpcDataSetClientAddress\": \"primihub-node2\",\n    \"grpcDataSetClientPort\": 50052,\n    \"grpcServerPort\": 9090,\n    \"uploadUrlDirPrefix\": \"/data/upload/\",\n    \"resultUrlDirPrefix\": \"/data/result/\",\n    \"runModelFileUrlDirPrefix\": \"/data/result/runModel/\",\n    \"usefulToken\": \"excalibur_forever_ABCDEFGHIJKLMN\",\n    \"model_components\": [\n        {\n            \"component_code\": \"start\",\n            \"component_name\": \"开始\",\n            \"is_show\": 0,\n            \"is_mandatory\": 0,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"taskName\",\n                    \"type_name\": \"任务名称\",\n                    \"input_type\": \"text\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"taskDesc\",\n                    \"type_name\": \"任务描述\",\n                    \"input_type\": \"textarea\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                }\n            ]\n        },\n        {\n            \"component_code\": \"dataSet\",\n            \"component_name\": \"选择数据集\",\n            \"is_show\": 0,\n            \"is_mandatory\": 0,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"selectData\",\n                    \"type_name\": \"选择数据\",\n                    \"input_type\": \"none\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                }\n            ]\n        },\n        {\n            \"component_code\": \"features\",\n            \"component_name\": \"特征筛选\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"features\",\n                    \"type_name\": \"特征筛选\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"唯一值筛选\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"缺失值比例筛选\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"IV值筛选\"\n                        },\n                        {\n                            \"key\": \"4\",\n                            \"val\": \"相关性筛选\"\n                        },\n                        {\n                            \"key\": \"5\",\n                            \"val\": \"用户自定义筛选\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"sample\",\n            \"component_name\": \"样本抽样设计\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"sample\",\n                    \"type_name\": \"样本抽样设计\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"最大/最小样本\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"提出灰样本\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"处理样本不均衡和分层\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"exception\",\n            \"component_name\": \"异常处理\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"exception\",\n                    \"type_name\": \"异常处理\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"删除\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"视为缺失值\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"平均值修正\"\n                        },\n                        {\n                            \"key\": \"4\",\n                            \"val\": \"盖帽法\"\n                        },\n                        {\n                            \"key\": \"5\",\n                            \"val\": \"分箱法\"\n                        },\n                        {\n                            \"key\": \"6\",\n                            \"val\": \"回归插补\"\n                        },\n                        {\n                            \"key\": \"7\",\n                            \"val\": \"多重插补\"\n                        },\n                        {\n                            \"key\": \"8\",\n                            \"val\": \"不处理\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"featureCoding\",\n            \"component_name\": \"特征编码\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"featureCoding\",\n                    \"type_name\": \"特征编码\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"1\",\n                            \"val\": \"标签编码\"\n                        },\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"哈希编码\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"独热编码\"\n                        },\n                        {\n                            \"key\": \"4\",\n                            \"val\": \"计数编码\"\n                        },\n                        {\n                            \"key\": \"5\",\n                            \"val\": \"直方图编码\"\n                        },\n                        {\n                            \"key\": \"6\",\n                            \"val\": \"WOE编码\"\n                        },\n                        {\n                            \"key\": \"7\",\n                            \"val\": \"目标编码\"\n                        },\n                        {\n                            \"key\": \"8\",\n                            \"val\": \"平均编码\"\n                        },\n                        {\n                            \"key\": \"9\",\n                            \"val\": \"模型编码\"\n                        }\n                    ]\n                }\n            ]\n        },\n        {\n            \"component_code\": \"model\",\n            \"component_name\": \"模型选择\",\n            \"is_show\": 0,\n            \"is_mandatory\": 0,\n            \"component_types\": [\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"modelType\",\n                    \"type_name\": \"模型选择\",\n                    \"input_type\": \"select\",\n                    \"input_value\": \"\",\n                    \"input_values\": [\n                        {\n                            \"key\": \"2\",\n                            \"val\": \"纵向-XGBoost\"\n                        },\n                        {\n                            \"key\": \"3\",\n                            \"val\": \"横向-LR\"\n                        }\n                    ]\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 1,\n                    \"type_code\": \"modelName\",\n                    \"type_name\": \"模型名称\",\n                    \"input_type\": \"text\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"modelDesc\",\n                    \"type_name\": \"模型描述\",\n                    \"input_type\": \"textarea\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                },\n                {\n                    \"is_show\": 0,\n                    \"is_required\": 0,\n                    \"type_code\": \"arbiterOrgan\",\n                    \"type_name\": \"可信第三方选择\",\n                    \"input_type\": \"button\",\n                    \"input_value\": \"\",\n                    \"input_values\": []\n                }\n            ]\n        },\n        {\n            \"component_code\": \"assessment\",\n            \"component_name\": \"评估模型\",\n            \"is_show\": 0,\n            \"is_mandatory\": 1,\n            \"component_types\": []\n        }\n    ]\n}', '08f26894d7520aa0be50a64fd02e36b1', '2022-06-29 09:36:09', '2022-10-14 10:39:15', 'nacos', '103.85.177.130', '', '35674f9d-3369-42f6-9f30-c8de2535adc6', '', '', '', 'json', '');
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (129, 'redis.yaml', 'DEFAULT_GROUP', 'spring:\n  datasource:\n    redis:\n      primary:\n        hostName: redis\n        port: 6379\n        password: primihub\n        database: 2\n        minIdle: 0\n        maxIdle: 10\n        maxTotal: 100\n        lifo: false\n        maxWaitMillis: 1000\n        minEvictableIdleTimeMillis: 1800000\n        softMinEvictableIdleTimeMillis: 1800000', '5a01168cd8d0120ed837fac415a0cf17', '2022-06-29 09:36:09', '2022-09-15 05:26:07', 'nacos', '103.85.177.130', '', '35674f9d-3369-42f6-9f30-c8de2535adc6', '', '', '', 'yaml', '');
INSERT INTO `config_info` (`id`, `data_id`, `group_id`, `content`, `md5`, `gmt_create`, `gmt_modified`, `src_user`, `src_ip`, `app_name`, `tenant_id`, `c_desc`, `c_use`, `effect`, `type`, `c_schema`) VALUES (134, 'organ_info.json', 'DEFAULT_GROUP', '{\"fusionMap\":{\"http://fusion:8080/\":{\"registered\":true,\"serverAddress\":\"http://fusion:8080/\",\"show\":true}},\"gatewayAddress\":\"http://gateway3:8080\",\"organId\":\"eb734dd0-773e-411b-ba29-794e41ba0e63\",\"organName\":\"Primihub03\",\"pinCode\":\"82r0WRMBX2S8WodL\"}', '32c3cf3dadbcf89568ae2ab8b4b55dc9', '2022-06-29 11:28:23', '2022-08-19 08:19:16', NULL, '10.244.3.103', '', '35674f9d-3369-42f6-9f30-c8de2535adc6', NULL, NULL, NULL, 'json', NULL);
/*!40000 ALTER TABLE `config_info` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `config_info_aggr`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `config_info_aggr` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT 'id',
  `data_id` varchar(255) COLLATE utf8_bin NOT NULL COMMENT 'data_id',
  `group_id` varchar(255) COLLATE utf8_bin NOT NULL COMMENT 'group_id',
  `datum_id` varchar(255) COLLATE utf8_bin NOT NULL COMMENT 'datum_id',
  `content` longtext COLLATE utf8_bin NOT NULL,
  `gmt_modified` datetime NOT NULL,
  `app_name` varchar(128) COLLATE utf8_bin DEFAULT NULL,
  `tenant_id` varchar(128) COLLATE utf8_bin DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_configinfoaggr_datagrouptenantdatum` (`data_id`,`group_id`,`tenant_id`,`datum_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `config_info_aggr`
--

LOCK TABLES `config_info_aggr` WRITE;
/*!40000 ALTER TABLE `config_info_aggr` DISABLE KEYS */;
/*!40000 ALTER TABLE `config_info_aggr` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `config_info_beta`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `config_info_beta` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT 'id',
  `data_id` varchar(255) COLLATE utf8_bin NOT NULL COMMENT 'data_id',
  `group_id` varchar(128) COLLATE utf8_bin NOT NULL COMMENT 'group_id',
  `app_name` varchar(128) COLLATE utf8_bin DEFAULT NULL COMMENT 'app_name',
  `content` longtext COLLATE utf8_bin NOT NULL COMMENT 'content',
  `beta_ips` varchar(1024) COLLATE utf8_bin DEFAULT NULL COMMENT 'betaIps',
  `md5` varchar(32) COLLATE utf8_bin DEFAULT NULL COMMENT 'md5',
  `gmt_create` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `gmt_modified` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `src_user` text COLLATE utf8_bin COMMENT 'source user',
  `src_ip` varchar(50) COLLATE utf8_bin DEFAULT NULL COMMENT 'source ip',
  `tenant_id` varchar(128) COLLATE utf8_bin DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_configinfobeta_datagrouptenant` (`data_id`,`group_id`,`tenant_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='config_info_beta';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `config_info_beta`
--

LOCK TABLES `config_info_beta` WRITE;
/*!40000 ALTER TABLE `config_info_beta` DISABLE KEYS */;
/*!40000 ALTER TABLE `config_info_beta` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `config_info_tag`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `config_info_tag` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT 'id',
  `data_id` varchar(255) COLLATE utf8_bin NOT NULL COMMENT 'data_id',
  `group_id` varchar(128) COLLATE utf8_bin NOT NULL COMMENT 'group_id',
  `tenant_id` varchar(128) COLLATE utf8_bin DEFAULT '' COMMENT 'tenant_id',
  `tag_id` varchar(128) COLLATE utf8_bin NOT NULL COMMENT 'tag_id',
  `app_name` varchar(128) COLLATE utf8_bin DEFAULT NULL COMMENT 'app_name',
  `content` longtext COLLATE utf8_bin NOT NULL COMMENT 'content',
  `md5` varchar(32) COLLATE utf8_bin DEFAULT NULL COMMENT 'md5',
  `gmt_create` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `gmt_modified` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `src_user` text COLLATE utf8_bin COMMENT 'source user',
  `src_ip` varchar(50) COLLATE utf8_bin DEFAULT NULL COMMENT 'source ip',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_configinfotag_datagrouptenanttag` (`data_id`,`group_id`,`tenant_id`,`tag_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='config_info_tag';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `config_info_tag`
--

LOCK TABLES `config_info_tag` WRITE;
/*!40000 ALTER TABLE `config_info_tag` DISABLE KEYS */;
/*!40000 ALTER TABLE `config_info_tag` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `config_tags_relation`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `config_tags_relation` (
  `id` bigint(20) NOT NULL COMMENT 'id',
  `tag_name` varchar(128) COLLATE utf8_bin NOT NULL COMMENT 'tag_name',
  `tag_type` varchar(64) COLLATE utf8_bin DEFAULT NULL COMMENT 'tag_type',
  `data_id` varchar(255) COLLATE utf8_bin NOT NULL COMMENT 'data_id',
  `group_id` varchar(128) COLLATE utf8_bin NOT NULL COMMENT 'group_id',
  `tenant_id` varchar(128) COLLATE utf8_bin DEFAULT '' COMMENT 'tenant_id',
  `nid` bigint(20) NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`nid`),
  UNIQUE KEY `uk_configtagrelation_configidtag` (`id`,`tag_name`,`tag_type`),
  KEY `idx_tenant_id` (`tenant_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='config_tag_relation';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `config_tags_relation`
--

LOCK TABLES `config_tags_relation` WRITE;
/*!40000 ALTER TABLE `config_tags_relation` DISABLE KEYS */;
/*!40000 ALTER TABLE `config_tags_relation` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `group_capacity`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `group_capacity` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT 'ID',
  `group_id` varchar(128) COLLATE utf8_bin NOT NULL DEFAULT '' COMMENT 'Group ID',
  `quota` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '0',
  `usage` int(10) unsigned NOT NULL DEFAULT '0',
  `max_size` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '0',
  `max_aggr_count` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '0',
  `max_aggr_size` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '0',
  `max_history_count` int(10) unsigned NOT NULL DEFAULT '0',
  `gmt_create` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `gmt_modified` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_group_id` (`group_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='Group';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `group_capacity`
--

LOCK TABLES `group_capacity` WRITE;
/*!40000 ALTER TABLE `group_capacity` DISABLE KEYS */;
/*!40000 ALTER TABLE `group_capacity` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `his_config_info`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `his_config_info` (
  `id` bigint(64) unsigned NOT NULL,
  `nid` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `data_id` varchar(255) COLLATE utf8_bin NOT NULL,
  `group_id` varchar(128) COLLATE utf8_bin NOT NULL,
  `app_name` varchar(128) COLLATE utf8_bin DEFAULT NULL COMMENT 'app_name',
  `content` longtext COLLATE utf8_bin NOT NULL,
  `md5` varchar(32) COLLATE utf8_bin DEFAULT NULL,
  `gmt_create` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `gmt_modified` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `src_user` text COLLATE utf8_bin,
  `src_ip` varchar(50) COLLATE utf8_bin DEFAULT NULL,
  `op_type` char(10) COLLATE utf8_bin DEFAULT NULL,
  `tenant_id` varchar(128) COLLATE utf8_bin DEFAULT '',
  PRIMARY KEY (`nid`),
  KEY `idx_gmt_create` (`gmt_create`),
  KEY `idx_gmt_modified` (`gmt_modified`),
  KEY `idx_did` (`data_id`)
) ENGINE=InnoDB AUTO_INCREMENT=291 DEFAULT CHARSET=utf8 COLLATE=utf8_bin;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `permissions`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `permissions` (
  `role` varchar(50) NOT NULL,
  `resource` varchar(255) NOT NULL,
  `action` varchar(8) NOT NULL,
  UNIQUE KEY `uk_role_permission` (`role`,`resource`,`action`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `permissions`
--

LOCK TABLES `permissions` WRITE;
/*!40000 ALTER TABLE `permissions` DISABLE KEYS */;
/*!40000 ALTER TABLE `permissions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `roles`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `roles` (
  `username` varchar(50) NOT NULL,
  `role` varchar(50) NOT NULL,
  UNIQUE KEY `idx_user_role` (`username`,`role`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `roles`
--

LOCK TABLES `roles` WRITE;
/*!40000 ALTER TABLE `roles` DISABLE KEYS */;
INSERT INTO `roles` VALUES ('nacos','ROLE_ADMIN');
/*!40000 ALTER TABLE `roles` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tenant_capacity`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `tenant_capacity` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT 'ID',
  `tenant_id` varchar(128) COLLATE utf8_bin NOT NULL DEFAULT '' COMMENT 'Tenant ID',
  `quota` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '0',
  `usage` int(10) unsigned NOT NULL DEFAULT '0',
  `max_size` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '0',
  `max_aggr_count` int(10) unsigned NOT NULL DEFAULT '0',
  `max_aggr_size` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '0',
  `max_history_count` int(10) unsigned NOT NULL DEFAULT '0',
  `gmt_create` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `gmt_modified` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_tenant_id` (`tenant_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tenant_capacity`
--

LOCK TABLES `tenant_capacity` WRITE;
/*!40000 ALTER TABLE `tenant_capacity` DISABLE KEYS */;
/*!40000 ALTER TABLE `tenant_capacity` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tenant_info`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `tenant_info` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT 'id',
  `kp` varchar(128) COLLATE utf8_bin NOT NULL COMMENT 'kp',
  `tenant_id` varchar(128) COLLATE utf8_bin DEFAULT '' COMMENT 'tenant_id',
  `tenant_name` varchar(128) COLLATE utf8_bin DEFAULT '' COMMENT 'tenant_name',
  `tenant_desc` varchar(256) COLLATE utf8_bin DEFAULT NULL COMMENT 'tenant_desc',
  `create_source` varchar(32) COLLATE utf8_bin DEFAULT NULL COMMENT 'create_source',
  `gmt_create` bigint(20) NOT NULL,
  `gmt_modified` bigint(20) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_tenant_info_kptenantid` (`kp`,`tenant_id`),
  KEY `idx_tenant_id` (`tenant_id`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='tenant_info';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tenant_info`
--

LOCK TABLES `tenant_info` WRITE;
/*!40000 ALTER TABLE `tenant_info` DISABLE KEYS */;
INSERT INTO `tenant_info` VALUES (3,'1','46b6b568-e6ae-45ca-baa1-819932fc8947','demo1','demo1','nacos',1655369938742,1655369938742),(4,'1','71843998-b60a-42ed-81d7-c3c9940d11c0','demo2','demo2','nacos',1655369947525,1655369947525),(5,'1','35674f9d-3369-42f6-9f30-c8de2535adc6','demo3','demo3','nacos',1656495327434,1656495327434);
/*!40000 ALTER TABLE `tenant_info` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `users`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `users` (
  `username` varchar(50) NOT NULL,
  `password` varchar(500) NOT NULL,
  `enabled` tinyint(1) NOT NULL,
  PRIMARY KEY (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `users`
--

LOCK TABLES `users` WRITE;
/*!40000 ALTER TABLE `users` DISABLE KEYS */;
INSERT INTO `users` VALUES ('nacos','$2a$10$EuWPZHzz32dJN7jexM34MOeYirDdFAZm2kuWj7VEOJhhZkDrxfvUu',1);
/*!40000 ALTER TABLE `users` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Current Database: `fusion_dc`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `fusion_dc` /*!40100 DEFAULT CHARACTER SET utf8 */;

USE `fusion_dc`;

--
-- Table structure for table `data_fusion_copy_task`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `data_fusion_copy_task` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
  `task_type` tinyint(4) NOT NULL COMMENT '任务类型 1 批量 2 单条',
  `current_offset` bigint(20) NOT NULL COMMENT '当前偏移量',
  `target_offset` bigint(20) NOT NULL COMMENT '目标便宜量',
  `task_table` varchar(64) NOT NULL COMMENT '复制任务表名',
  `fusion_server_address` varchar(64) NOT NULL COMMENT '连接中心地址',
  `latest_error_msg` varchar(1024) NOT NULL COMMENT '最近一次复制失败原因',
  `is_del` tinyint(4) NOT NULL COMMENT '是否删除',
  `c_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
  `u_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
  PRIMARY KEY (`id`) USING BTREE,
  KEY `current_offset_ix` (`current_offset`) USING BTREE,
  KEY `target_offset_ix` (`target_offset`) USING BTREE,
  KEY `c_time_ix` (`c_time`) USING BTREE,
  KEY `u_time_ix` (`u_time`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `data_fusion_copy_task`
--

LOCK TABLES `data_fusion_copy_task` WRITE;
/*!40000 ALTER TABLE `data_fusion_copy_task` DISABLE KEYS */;
/*!40000 ALTER TABLE `data_fusion_copy_task` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `fusion_go`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `fusion_go` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
  `group_id` bigint(20) NOT NULL COMMENT '群组id',
  `organ_global_id` varchar(64) NOT NULL COMMENT '机构唯一id',
  `is_del` tinyint(4) NOT NULL COMMENT '是否删除',
  `c_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
  `u_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE KEY `group_id_and global_id_ix` (`group_id`,`organ_global_id`) USING BTREE,
  KEY `group_id_ix` (`group_id`) USING BTREE,
  KEY `global_id_ix` (`organ_global_id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=12 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `fusion_go`
--

LOCK TABLES `fusion_go` WRITE;
/*!40000 ALTER TABLE `fusion_go` DISABLE KEYS */;
INSERT INTO `fusion_go` VALUES (2,1,'7aeeb3aa-75cc-4e40-8692-ea5fd5f5f9f0',0,'2022-06-29 06:59:51.959','2022-06-29 06:59:51.959'),(5,2,'eb734dd0-773e-411b-ba29-794e41ba0e63',0,'2022-07-04 09:44:04.469','2022-07-04 09:44:04.469'),(7,1,'eb734dd0-773e-411b-ba29-794e41ba0e63',0,'2022-07-04 09:49:56.932','2022-07-04 09:49:56.932'),(9,1,'3abfcb2a-8335-4bcc-b6f9-704a92e392fd',0,'2022-07-04 11:04:26.621','2022-07-04 11:04:26.621'),(10,2,'3abfcb2a-8335-4bcc-b6f9-704a92e392fd',0,'2022-07-05 03:18:04.235','2022-07-05 03:18:04.235'),(11,3,'3abfcb2a-8335-4bcc-b6f9-704a92e392fd',0,'2022-07-06 02:42:39.478','2022-07-06 02:42:39.478');
/*!40000 ALTER TABLE `fusion_go` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `fusion_group`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `fusion_group` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
  `group_name` varchar(64) NOT NULL COMMENT '群组名称',
  `group_organ_id` varchar(64) NOT NULL COMMENT '群主id',
  `is_del` tinyint(4) NOT NULL COMMENT '是否删除',
  `c_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
  `u_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `fusion_group`
--

LOCK TABLES `fusion_group` WRITE;
/*!40000 ALTER TABLE `fusion_group` DISABLE KEYS */;
INSERT INTO `fusion_group` VALUES (1,'企业1','3abfcb2a-8335-4bcc-b6f9-704a92e392fd',0,'2022-06-29 06:59:04.428','2022-06-29 06:59:04.428'),(2,'群组二','eb734dd0-773e-411b-ba29-794e41ba0e63',0,'2022-07-04 09:44:04.467','2022-07-04 09:44:04.467'),(3,'zane test','3abfcb2a-8335-4bcc-b6f9-704a92e392fd',0,'2022-07-06 02:42:39.476','2022-07-06 02:42:39.476');
/*!40000 ALTER TABLE `fusion_group` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `fusion_organ`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `fusion_organ` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增id',
  `global_id` varchar(64) NOT NULL COMMENT '唯一id',
  `global_name` varchar(64) NOT NULL COMMENT '机构名称',
  `pin_code_md` varchar(64) NOT NULL COMMENT 'pin码md5',
  `gateway_address` varchar(255) NOT NULL COMMENT '网关地址',
  `register_time` datetime(3) NOT NULL COMMENT '注册时间',
  `is_del` tinyint(4) NOT NULL COMMENT '是否删除',
  `c_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
  `u_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
  PRIMARY KEY (`id`) USING BTREE,
  KEY `global_id_ix` (`global_id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `fusion_organ`
--

LOCK TABLES `fusion_organ` WRITE;
/*!40000 ALTER TABLE `fusion_organ` DISABLE KEYS */;
INSERT INTO `fusion_organ` VALUES (1,'7aeeb3aa-75cc-4e40-8692-ea5fd5f5f9f0','Primihub01','98E2104F2B75D3F1B03CABE884774A4B','http://gateway1:8080','2022-06-29 14:57:14.344',0,'2022-06-29 06:57:14.439','2022-07-04 03:45:07.449'),(2,'3abfcb2a-8335-4bcc-b6f9-704a92e392fd','Primihub02','7F852120E99F8E351186AE92EF68C650','http://gateway2:8080','2022-06-29 14:58:21.635',0,'2022-06-29 06:58:21.636','2022-07-04 05:30:16.289'),(3,'eb734dd0-773e-411b-ba29-794e41ba0e63','Primihub03','42CEC0EBB928AF1418FAD01D22FDDF42','http://gateway3:8080','2022-06-29 19:28:38.090',0,'2022-06-29 11:28:38.093','2022-07-04 07:28:34.797');
/*!40000 ALTER TABLE `fusion_organ` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `fusion_public_ro`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `fusion_public_ro` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID',
  `resource_id` bigint(20) DEFAULT NULL COMMENT '资源ID',
  `organ_id` varchar(255) DEFAULT NULL COMMENT '机构ID',
  `is_del` tinyint(4) NOT NULL COMMENT '是否删除',
  `c_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
  `u_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
  PRIMARY KEY (`id`) USING BTREE,
  KEY `resource_id_ix` (`resource_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `fusion_public_ro`
--

LOCK TABLES `fusion_public_ro` WRITE;
/*!40000 ALTER TABLE `fusion_public_ro` DISABLE KEYS */;
/*!40000 ALTER TABLE `fusion_public_ro` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `fusion_resource`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `fusion_resource` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID',
  `resource_id` varchar(64) DEFAULT NULL COMMENT '资源ID',
  `resource_name` varchar(255) DEFAULT NULL COMMENT '资源名称',
  `resource_desc` varchar(255) DEFAULT NULL COMMENT '资源描述',
  `resource_type` tinyint(4) DEFAULT NULL COMMENT '资源类型 上传...',
  `resource_auth_type` tinyint(4) DEFAULT NULL COMMENT '授权类型（公开，私有，可见性）',
  `resource_rows_count` int(11) DEFAULT NULL COMMENT '资源行数',
  `resource_column_count` int(11) DEFAULT NULL COMMENT '资源列数',
  `resource_column_name_list` blob COMMENT '字段列表',
  `resource_contains_y` tinyint(4) DEFAULT NULL COMMENT '资源字段中是否包含y字段 0否 1是',
  `resource_y_rows_count` int(11) DEFAULT NULL COMMENT '文件字段y值内容不为空和0的行数',
  `resource_y_ratio` decimal(10,2) DEFAULT NULL COMMENT '文件字段y值内容不为空的行数在总行的占比',
  `resource_tag` varchar(255) DEFAULT NULL COMMENT '资源标签 格式tag,tag',
  `resource_hash_code` varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '资源hash值',
  `resource_state` tinyint NOT NULL DEFAULT '0' COMMENT '资源状态 0上线 1下线',
  `organ_id` varchar(64) DEFAULT NULL COMMENT '机构ID',
  `is_del` tinyint(4) NOT NULL COMMENT '是否删除',
  `c_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
  `u_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE KEY `resource_id_ix` (`resource_id`) USING BTREE,
  KEY `organ_id_ix` (`organ_id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=69 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `fusion_resource_field`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `fusion_resource_field` (
  `field_id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '字段id',
  `resource_id` bigint(20) DEFAULT NULL COMMENT '资源id',
  `field_name` varchar(255) DEFAULT NULL COMMENT '字段名称',
  `field_as` varchar(255) DEFAULT NULL COMMENT '字段别名',
  `field_type` int(11) DEFAULT '0' COMMENT '字段类型 默认0 string',
  `field_desc` varchar(255) DEFAULT NULL COMMENT '字段描述',
  `relevance` int(11) DEFAULT '0' COMMENT '关键字 0否 1是',
  `grouping` int(11) DEFAULT '0' COMMENT '分组 0否 1是',
  `protection_status` int(11) DEFAULT '0' COMMENT '保护开关 0关闭 1开启',
  `is_del` tinyint(4) DEFAULT '0' COMMENT '是否删除',
  `c_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
  `u_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '修改时间',
  PRIMARY KEY (`field_id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;


-- ----------------------------
-- Table structure for fusion_organ_resource_auth
-- ----------------------------

DROP TABLE IF EXISTS `fusion_organ_resource_auth`;
CREATE TABLE `fusion_organ_resource_auth`  (
                                               `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
                                               `resource_id` bigint(20) NOT NULL COMMENT '资源id',
                                               `organ_id` bigint(20) NOT NULL COMMENT '机构id',
                                               `project_id` varchar(255) DEFAULT NULL COMMENT '项目ID',
                                               `audit_status` tinyint(4) NOT NULL DEFAULT '1' COMMENT '审核状态',
                                               `is_del` tinyint(4) NOT NULL COMMENT '是否删除',
                                               `c_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
                                               `u_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
                                               PRIMARY KEY (`id`) USING BTREE,
                                               INDEX `resource_id_ix`(`resource_id`) USING BTREE,
                                               INDEX `organ_id_ix`(`organ_id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = DYNAMIC;


--
-- Table structure for table `fusion_resource_tag`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `fusion_resource_tag` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID',
  `name` varchar(255) DEFAULT NULL COMMENT '标签名称',
  `is_del` tinyint(4) NOT NULL COMMENT '是否删除',
  `c_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
  `u_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=71 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `fusion_resource_visibility_auth`
--

/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `fusion_resource_visibility_auth` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '主键',
  `resource_id` varchar(64) NOT NULL COMMENT '资源id',
  `organ_global_id` varchar(64) NOT NULL COMMENT '机构id',
  `is_del` tinyint(4) NOT NULL COMMENT '是否删除',
  `c_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
  `u_time` datetime(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
  PRIMARY KEY (`id`) USING BTREE,
  KEY `resource_id_ix` (`resource_id`) USING BTREE,
  KEY `organ_global_id_ix` (`organ_global_id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=35 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `fusion_resource_visibility_auth`
--

LOCK TABLES `fusion_resource_visibility_auth` WRITE;
/*!40000 ALTER TABLE `fusion_resource_visibility_auth` DISABLE KEYS */;
INSERT INTO `fusion_resource_visibility_auth` VALUES (7,'ea5fd5f5f9f0-ff02f012-5d9a-4a6c-af3e-97e17e954a90','3abfcb2a-8335-4bcc-b6f9-704a92e392fd',0,'2022-07-09 08:00:00.030','2022-07-09 08:00:00.030'),(23,'ea5fd5f5f9f0-1183f807-ebd8-4e14-a904-a3855bcce5f7','3abfcb2a-8335-4bcc-b6f9-704a92e392fd',0,'2022-07-11 07:10:00.122','2022-07-11 07:10:00.122'),(32,'ea5fd5f5f9f0-8ba68b34-865c-4259-a0bb-1f43b2121ff2','3abfcb2a-8335-4bcc-b6f9-704a92e392fd',0,'2022-07-15 07:40:00.033','2022-07-15 07:40:00.033'),(33,'ea5fd5f5f9f0-bd49b3d8-2a49-4a2f-9ff0-31fb69d362ee','3abfcb2a-8335-4bcc-b6f9-704a92e392fd',0,'2022-07-15 07:40:00.048','2022-07-15 07:40:00.048'),(34,'ea5fd5f5f9f0-bd49b3d8-2a49-4a2f-9ff0-31fb69d362ee','eb734dd0-773e-411b-ba29-794e41ba0e63',0,'2022-07-15 07:40:00.048','2022-07-15 07:40:00.048');
/*!40000 ALTER TABLE `fusion_resource_visibility_auth` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2022-07-30 11:47:30

GRANT ALL ON *.* TO 'primihub'@'%';