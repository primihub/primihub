"""
* Copyright (c) 2023 by PrimiHub
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      https://www.apache.org/licenses/
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
"""
import grpc
from .src.primihub.protos import metadatastream_pb2
from .src.primihub.protos import metadatastream_pb2_grpc

class DataStreamGrpcClient:
    def __init__(self, node) -> None:
        self.channel = grpc.insecure_channel(node)
        self.stub = metadatastream_pb2_grpc.MetaStreamServiceStub(self.channel)

    def featch_data(self, dataset_id, trace_id):
        request = metadatastream_pb2.MetaDataSetDataStream()
        request.data_set_id = dataset_id
        request.trace_id = trace_id
        recv_data = {}
        with self.channel:
            response = self.stub.FetchData(request)
            for item in response:
                data = item.data
                for key, value_info in data.items():
                    recv_data.setdefault(key, [])
                    type = value_info.var_type
                    if type == metadatastream_pb2.INT32:
                        value = value_info.value_int32
                    elif type == metadatastream_pb2.INT64:
                        value = value_info.value_int64
                    elif type == metadatastream_pb2.STRING:
                        value = value_info.value_string.decode()
                    elif type == metadatastream_pb2.FLOAT:
                        value = value_info.value_float
                    elif type == metadatastream_pb2.DOUBLE:
                        value = value_info.value_double
                    elif type == metadatastream_pb2.BOOL:
                        value = value_info.value_bool
                    elif type == metadatastream_pb2.BYTE:
                        value = value_info.value_bytes
                    else:
                        raise Exception(f"unsupported value type {type}")
                    recv_data[key].append(value)
        return recv_data
