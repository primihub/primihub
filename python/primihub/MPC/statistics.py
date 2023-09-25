import ph_secure_lib as ph_slib
from primihub.context import Context

class MPCJointStatistics:
    def __init__(self, protocal="ABY3"):
        self.mpc_executor = ph_slib.MPCExecutor(Context.message, protocal)

    def max(self, input):
        """
        Input:
          input: local max data for each colums
        Output:
          max result
        """
        return self.mpc_executor.max(input)

    def min(self, input):
        """
        Input:
          input: local min data for each colums
          rows_of_columns: rows num of each colums
        Output:
          min result
        """
        return self.mpc_executor.min(input)

    def avg(self, input, rows_of_columns):
        """
        Input:
          input: local sum data for each colums
          rows_of_columns: rows num of each colums
        Output:
          avg result
        """
        return self.mpc_executor.avg(input, rows_of_columns)

    def sum(self, input):
        """
        Input:
          input: local sum data for each colums
        Output:
          sum result
        """
        return self.mpc_executor.sum(input)

