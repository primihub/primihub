import ph_secure_lib as ph_slib
from primihub.context import Context

def stop_auxiliary_party():
  cert_config = Context.cert_config
  root_ca_path = cert_config.get("root_ca_path", "")
  key_path = cert_config.get("key_path", "")
  cert_path = cert_config.get("cert_path", "")
  mpc_executor = ph_slib.MPCExecutor(Context.message, "ABY3",
                                      root_ca_path, key_path, cert_path)
  mpc_executor.stop_task()