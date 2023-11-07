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

   def SubmitTask(self, task_content):
      task_info = jpype.JString(task_content)
      engine_id = random.randint(1, 1000000)
      jobInstanceId = jpype.JLong(random.randint(1, 1000000))
      jobEngineId = jpype.JLong(engine_id)
      self.instance.restoreJobContent(task_info, jobInstanceId, jobEngineId)
      logger.info("submit task success")
      count = 0
      while True:
        result_info = self.instance.getJobPipelineStatusStr(jpype.JString(str(engine_id)))
        dict_info = json.loads(str(result_info))
        time.sleep(1)
        job_status = dict_info["jobStatus"]
        if job_status in ["FINISHED"]:
            logger.info("task is finished, status {}".format(job_status))
            break
        elif job_status in ["FAILED", "CANCELED", "UNKNOWABLE"]:
            logger.info("task is failed, status {}".format(job_status))
            raise Exception(json.dumps(dict_info))
        else:
            if count % 10 == 0:
                logger.info(dict_info)
        count = count + 1

if __name__ == '__main__':
    # caution test example,
    # it must modify the following config before run it successfully
    # global_config = {
    #   "env": {
    #     "hazelcast.client.config": "./hazelcast-client.yaml",
    #     "SEATUNNEL_HOME": "path for seatunnel",
    #   },

    #   "JAR_PATH": [
    #     "./plugins/seatunnel-client-1.0-SNAPSHOT-jar-with-dependencies.jar",
    #   ],
    # }
    # task_content = {
    #   "env" : {
    #     "job.mode" : "STREAMING",
    #     "job.name" : "SeaTunnel_Job"
    #   },
    #   "source" : [
    #     {
    #       "password" : "123456",
    #       "driver" : "com.mysql.cj.jdbc.Driver",
    #       "parallelism" : 1,
    #       "query" : "SELECT * FROM `jeecg-boot`.`sys_log` LIMIT 0,50",
    #       "connection_check_timeout_sec" : 30,
    #       "batch_size":1,
    #       "fetch_size" : "1",
    #       "plugin_name" : "Jdbc",
    #       "user" : "root",
    #       "url" : "url for mysql"
    #     }
    #   ],
    #   "sink" : [
    #     {
    #       "plugin_name" : "Grpc",
    #       "host" : "172.21.1.63",
    #       "port" : 50051,
    #       "dataSetId":"dataSetId",
    #       "traceId":"traceId"
    #     }
    #   ]
    # }
    # client = SeatunnelClient(global_config)
    # task_config = json.dumps(task_content)
    # client.SubmitTask(task_config)
    pass
