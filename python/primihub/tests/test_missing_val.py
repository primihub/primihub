import json
import sys
sys.path.append("/usr/lib/python3.9/site-packages")

from primihub.client.ph_grpc.src.primihub.protos import common_pb2
from primihub.client.ph_grpc.event import listener
from primihub.client.ph_grpc.worker import WorkerClient



params = common_pb2.Params()
value = params.param_map["Col_And_Dtype"]
value.var_type = 2
value.is_array = False
value.value_string = "x3-2;x4-2;C-2;D-2"

value = params.param_map["ResFileName"]
value.var_type = 2
value.is_array = False
value.value_string = "res"

value=params.param_map["Data_File"]
value.var_type = 2
value.is_array = False
value.value_string = "arrow_0;arrow_1;arrow_2"


client = WorkerClient("192.168.99.21:50050", None)



task = client.set_task_map(common_pb2.TaskType.ACTOR_TASK, # Task type.
                           b"missing_val_processing",          # Name.
                           common_pb2.Language.PROTO,     # Language.
                           params,                         # Params.
                           b"missing_val_processing",       # Code.
                           None,                           # Node map.
                           ["Data_File"],              # Input dataset.  
                           "test_job".encode("utf-8"),     # Job id.
                           "test_task".encode("utf-8"),    # Task id.
                           None)                           # Channel map.


request = client.push_task_request(b'1',  # Intended_worker_id.
                                   task,  # Task.
                                   11,    # Sequence_number.
                                   22,    # Client_processed_up_to.
                                   b"")   # Submit_client_id.

reply = client.submit_task(request)
print(reply)
