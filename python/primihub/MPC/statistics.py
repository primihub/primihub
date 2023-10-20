import ph_secure_lib as ph_slib
from primihub.context import Context

class MPCJointStatistics:
    def __init__(self, protocol="ABY3"):
        self.mpc_executor = ph_slib.MPCExecutor(Context.message, protocol)

    def max(self, input):
        """
        Input:
          input: local max data for each columns
        Output:
          max result
        """
        return self.mpc_executor.max(input)

    def min(self, input):
        """
        Input:
          input: local min data for each columns
          rows_of_columns: rows num of each columns
        Output:
          min result
        """
        return self.mpc_executor.min(input)

    def avg(self, input, rows_of_columns):
        """
        Input:
          input: local sum data for each columns
          rows_of_columns: rows num of each columns
        Output:
          avg result
        """
        return self.mpc_executor.avg(input, rows_of_columns)

    def sum(self, input):
        """
        Input:
          input: local sum data for each columns
        Output:
          sum result
        """
        return self.mpc_executor.sum(input)

