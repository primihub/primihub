from primihub.utils.logger_util import logger
from primihub.MPC.psi import TwoPartyPsi, PsiType, DataType
import random
import time

class Example:
  def run(self):
    psi_executor = TwoPartyPsi()
    data_len = 10
    #input_data = [str(i) for i in range(data_len)]
    input_data = [i for i in range(data_len)]
    # input_data = [f"中国{i}" for i in range(data_len)]
    input_data_str = [str(i) for i in range(data_len)]
    for i in range(1):
      result = psi_executor.run(input_data,
          ["Alice", "Bob"], "Alice", True,
          PsiType.ECDH, data_type = DataType.Interger)
      logger.info(f"xxxxxxxxxxx loop: {i} psi int result: {result}")
      result = psi_executor.run(input_data,
          ["Alice", "Bob"], "Alice", True,
          PsiType.KKRT, data_type = DataType.Interger)
      logger.info(f"xxxxxxxxxxx loop: {i} psi int result: {result}")
      result = psi_executor.run(input_data_str,
          ["Alice", "Bob"], "Alice", True, PsiType.ECDH)
      logger.info(f"xxxxxxxxxxx loop: {i} psi string result: {result}")
      result = psi_executor.run(input_data_str,
          ["Alice", "Bob"], "Alice", True, PsiType.KKRT)
      logger.info(f"xxxxxxxxxxx loop: {i} psi string result: {result}")


example = Example()
example.run()