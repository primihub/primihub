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
import pathlib

import pyarrow as pa
import pyarrow.flight
import pyarrow.parquet


class FlightServer(pa.flight.FlightServerBase):

    def __init__(self, location="grpc://0.0.0.0:8815",
                 repo=pathlib.Path("./datasets"), **kwargs):
        super(FlightServer, self).__init__(location, **kwargs)
        self._location = location
        self._repo = repo

    def _make_flight_info(self, dataset):
        dataset_path = self._repo / dataset
        schema = pa.parquet.read_schema(dataset_path)
        metadata = pa.parquet.read_metadata(dataset_path)
        descriptor = pa.flight.FlightDescriptor.for_path(
            dataset.encode('utf-8')
        )
        endpoints = [pa.flight.FlightEndpoint(dataset, [self._location])]
        return pyarrow.flight.FlightInfo(schema,
                                         descriptor,
                                         endpoints,
                                         metadata.num_rows,
                                         metadata.serialized_size)

    def list_flights(self, context, criteria):
        for dataset in self._repo.iterdir():
            yield self._make_flight_info(dataset.name)

    def get_flight_info(self, context, descriptor):
        return self._make_flight_info(descriptor.path[0].decode('utf-8'))

    def do_put(self, context, descriptor, reader, writer):
        dataset = descriptor.path[0].decode('utf-8')
        dataset_path = self._repo / dataset
        # Read the uploaded data and write to Parquet incrementally
        with dataset_path.open("wb") as sink:
            with pa.parquet.ParquetWriter(sink, reader.schema) as writer:
                for chunk in reader:
                    writer.write_table(pa.Table.from_batches([chunk.data]))

    def do_get(self, context, ticket):
        dataset = ticket.ticket.decode('utf-8')
        # Stream data from a file
        dataset_path = self._repo / dataset
        reader = pa.parquet.ParquetFile(dataset_path)
        return pa.flight.GeneratorStream(
            reader.schema_arrow, reader.iter_batches())

    def list_actions(self, context):
        return [
            ("drop_dataset", "Delete a dataset."),
        ]

    def do_action(self, context, action):
        if action.type == "drop_dataset":
            self.do_drop_dataset(action.body.to_pybytes().decode('utf-8'))
        else:
            raise NotImplementedError

    def do_drop_dataset(self, dataset):
        dataset_path = self._repo / dataset
        dataset_path.unlink()


if __name__ == '__main__':
    server = FlightServer()
    server._repo.mkdir(exist_ok=True)
    server.serve()
