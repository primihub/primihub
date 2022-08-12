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

client = pa.flight.connect("grpc://0.0.0.0:8815")

# Upload a new dataset
data_table = pa.table(
    [["Mario", "Luigi", "Peach"]],
    names=["Character"]
)
upload_descriptor = pa.flight.FlightDescriptor.for_path("uploaded.parquet")
writer, _ = client.do_put(upload_descriptor, data_table.schema)
writer.write_table(data_table)
writer.close()

# Retrieve metadata of newly uploaded dataset
flight = client.get_flight_info(upload_descriptor)
descriptor = flight.descriptor
print("Path:", descriptor.path[0].decode('utf-8'), "Rows:", flight.total_records, "Size:", flight.total_bytes)
print("=== Schema ===")
print(flight.schema)
print("==============")
