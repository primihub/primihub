"""
 Copyright 2022 Primihub

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

import pyarrow as pa
import pyarrow.flight

class DatasetClient:
    def do_put(self, df_data, data_url:str):
        raise NotImplementedError()
    def do_get(self, ticket):
        raise NotImplementedError()


class FlightClient(DatasetClient):
    def __init__(self, flight_server_addr:str) -> None:
        super().__init__()
        self.client = pa.flight.connect(flight_server_addr)
        
    def do_put(self, df_data, data_url:str):
        upload_descriptor = pa.flight.FlightDescriptor.for_path(data_url)
        # df_data to arrow table
        table = pyarrow.Table.from_pandas(df_data)
        writer, u = self.client.do_put(upload_descriptor, table.schema)
        # TODO test print
        print(u)
        writer.write_table(table)
        writer.close()

    def do_get(self, ticket):
        # TODO
        pass


class DatasetClientFactory:
    """Dataset client factory
    """
    @staticmethod
    def create(type:str, node: str, cert: str) -> DatasetClient:
        """Create a dataset client
        """
        if type == "flight":
            return FlightClient(node)
        else:
            raise NotImplementedError()