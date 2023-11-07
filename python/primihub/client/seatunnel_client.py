# 引入jpype模块
import jpype
import json
import time
import random
from primihub.utils.logger_util import logger

class SeatunnelClient():
   def __init__(self, config_bytes) -> None:
      config_str = str(config_bytes, 'utf-8')
      config = json.loads(config_str)
      jar_path = config["JAR_PATH"]
      final_path = ".:"
      for item in jar_path:
        final_path = f"{final_path}:{item}"
      logger.debug(final_path)
      jvmPath = jpype.getDefaultJVMPath()
      jpype.startJVM(jvmPath, "-ea", "-Djava.class.path={}".format(final_path))
      system = jpype.JClass('java.lang.System')
      local_env = config["env"]
      for key, val in local_env.items():
        system.setProperty(key, val)

      j_pkg = jpype.JPackage("org.apache.seatunnel")
      self.instance = j_pkg.SeaTunnelEngineProxy.getInstance()

   def SubmitTask(self, json_file_path):
      task_info = jpype.JString(json_file_path)
      engine_id = random.randint(1, 1000000)
      jobInstanceId = jpype.JLong(random.randint(1, 1000000))
      jobEngineId = jpype.JLong(engine_id)
      try:
        self.instance.restoreJobContent(task_info, jobInstanceId, jobEngineId)
        logger.info("submit task success")
        count = 0
        while True:
          result_info = self.instance.getJobPipelineStatusStr(jpype.JString(str(engine_id)))
          dict_info = json.loads(str(result_info))
          logger.info(dict_info)
          time.sleep(1)
          if (dict_info["jobStatus"] in ["FINISHED"]):
             logger.info("task is finished, status {}".format(dict_info["jobStatus"]))
             break
          if (dict_info["jobStatus"] in ["FAILED"]):
             logger.info("task is failed, status {}".format(dict_info["jobStatus"]))
             return -1
      except Exception as e:
        logger.error(e)
        return -1
      return 0

if __name__ == '__main__':
    global_config = {
      "env": {
        "hazelcast.client.config": "/home/cuibo/workspace/myproject/seatunnel/client/singleton/config/hazelcast-client.yaml",
        "SEATUNNEL_HOME": "/home/cuibo/workspace/myproject/seatunnel/seatunnel",
      },

      "JAR_PATH": [
        "./plugins/seatunnel-client-1.0-SNAPSHOT-jar-with-dependencies.jar",
        "/home/cuibo/workspace/myproject/seatunnel/seatunnel/connectors/seatunnel/connector-jdbc-2.3.3-SNAPSHOT.jar",
        "/home/cuibo/workspace/myproject/seatunnel/seatunnel/connectors/seatunnel/connector-grpc-2.3.3-SNAPSHOT.jar",
      ],
    }
    task_content = {
      "env" : {
        "job.mode" : "STREAMING",
        "job.name" : "SeaTunnel_Job"
      },
      "source" : [
        {
          "password" : "123456",
          "driver" : "com.mysql.cj.jdbc.Driver",
          "parallelism" : 1,
          "query" : "SELECT * FROM `jeecg-boot`.`sys_log` LIMIT 0,50",
          "connection_check_timeout_sec" : 30,
          "batch_size":1,
          "fetch_size" : "1",
          "plugin_name" : "Jdbc",
          "user" : "root",
          "url" : "jdbc:mysql://172.21.1.78:3306/jeecg-boot?characterEncoding=UTF-8&useUnicode=true&useSSL=false&tinyInt1isBit=false&allowPublicKeyRetrieval=true&serverTimezone=Asia/Shanghai"
        }
      ],
      "sink" : [
        {
          "plugin_name" : "Grpc",
          "host" : "172.21.1.63",
          "port" : 50051,
          "dataSetId":"dataSetId",
          "traceId":"traceId"
        }
      ]
    }
    client = SeatunnelClient(global_config)
    #task_confg = "/home/cuibo/workspace/myproject/seatunnel/client/singleton/config/job_cfg.json"
    task_config = json.dumps(task_content)
    client.SubmitTask(task_config)
