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
import random

from primihub.client.ph_grpc.event import listener
from primihub.client.ph_grpc.worker import WorkerClient


class GrpcClient(object):
    """primihub grpc client"""

    def __init__(self, node, cert):
        self.node = node
        self.cert = cert
        self.listener = listener

    def submit_task(self, code: str, job_id: str, task_id: str, submit_client_id: str):
        """submit task

        :param code: code str
        :type code: str
        :param job_id: job id
        :type job_id: str
        :param task_id: task id
        :type task_id: str
        :return: _description_
        :rtype: _type_
        """
        client = WorkerClient(self.node, self.cert)
        task_map = client.set_task_map(code=code.encode('utf-8'),
                                       job_id=bytes(str(job_id), "utf-8"),
                                       task_id=bytes(str(task_id), "utf-8"))

        request = client.push_task_request(intended_worker_id=b'1',
                                           task=task_map,
                                           sequence_number=random.randint(0, 9999),
                                           client_processed_up_to=random.randint(0, 9999),
                                           submit_client_id=bytes(str(submit_client_id), "utf-8")
                                           )
        res = client.submit_task(request)
        return res
