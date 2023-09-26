import ph_secure_lib as ph_slib
from primihub.context import Context

def stop_auxiliary_party():
  mpc_executor = ph_slib.MPCExecutor(Context.message, "ABY3")
  mpc_executor.stop_task()