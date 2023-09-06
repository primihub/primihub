
from primihub.utils.logger_util import logger
from primihub.context import Context

class Example:
  def run(self):
    logger.info(f"request id: {Context.request_id()}")
    logger.info(f"party name: {Context.party_name()}")
    logger.info(f"code: {Context.task_code()}")

example = Example()
example.run()