"""
Copyright 2022 PrimiHub

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
import time
import grpc

from .src.primihub.protos import common_pb2, worker_pb2, worker_pb2_grpc  # noqa


class WorkerClient:
    """primihub gRPC worker client

    :return: A primihub gRPC worker client.
    """

    def __init__(self,
                 node,
                 cert=None,
                 task_type: common_pb2.TaskType = 0,
                 task_name: str = "",
                 language: common_pb2.Language = 0,
                 params: common_pb2.Params = None,
                 code: dict = None,
                 node_map: dict = None,
                 input_datasets: str = None,
                 task_info: common_pb2.TaskContext = None,
                 party_name: str = None,
                 party_datasets: dict = None,
                 party_access_info: dict = None) -> None:

        self.node = node
        self.cert = cert
        self.channel = grpc.insecure_channel(node)
        self.task_info = task_info
        self.stub = worker_pb2_grpc.VMNodeStub(self.channel)
        self.task = {
            "type": task_type,
            "name": task_name,
            "language": language,
            "params": params,
            "code": code,
            "node_map": node_map,
            "input_datasets": input_datasets,
            "task_info": task_info,
            "party_name": party_name,
            "party_datasets": party_datasets,
            "party_access_info": party_access_info
        }

    def push_task_request(self,
                          intended_worker_id=None,
                          task=None,
                          sequence_number=None,
                          client_processed_up_to=None,
                          submit_client_id=None):
        request_data = {
            "intended_worker_id": intended_worker_id,
            "task": task,
            "sequence_number": sequence_number,
            "client_processed_up_to": client_processed_up_to,
            "submit_client_id": submit_client_id
        }
        request = worker_pb2.PushTaskRequest(**request_data)
        return request

    def submit_task(self):
        """gRPC submit task

        :returns: gRPC reply
        :rtype: :obj:`worker_pb2.PushTaskReply`
        """
        PushTaskRequest = self.push_task_request(task=self.task)
        print(PushTaskRequest)
        print(20*'-')

        with self.channel:
            PushTaskReply = self.stub.SubmitTask(PushTaskRequest)
            start_time = time.time()

            ret_code_map = {0: 'success', 1: 'doing', 2: 'error'}
            print('ret_code:', ret_code_map[PushTaskReply.ret_code])
            print('task_info:', PushTaskReply.task_info)
            print('party_count:', PushTaskReply.party_count)
            print('task_server:', PushTaskReply.task_server)
            print(20*'-')

            task_info = PushTaskReply.task_info
            status_map = {
                0: 'RUNNING',
                1: 'SUCCESS',
                2: 'FAIL',
                3: 'NONEXIST',
                4: "FINISHED"
            }
            party_status = {}
            is_fail = False

            while True:
                time.sleep(1)

                TaskStatusReply = self.stub.FetchTaskStatus(task_info)
                for task_status in TaskStatusReply.task_status:
                    party = task_status.party
                    status = task_status.status

                    if status == worker_pb2.TaskStatus.StatusCode.FAIL or \
                       status == worker_pb2.TaskStatus.StatusCode.NONEXIST:
                        is_fail = True

                    if is_fail:
                        break

                    if party:
                        print('party:', party)
                        print('status:', status_map[status])
                        print(20*'-')

                        if status != worker_pb2.TaskStatus.StatusCode.RUNNING:
                            party_status[party] = status_map[status]

                if is_fail or len(party_status) == PushTaskReply.party_count:
                    break

            end_time = time.time()
            print(f'time spend: {end_time - start_time:.3f} s')

            if is_fail:
                print(f"fail: {TaskStatusReply}")
            else:
                print(f'status: {party_status}')
