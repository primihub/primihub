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
import pyarrow as pa
import pyarrow.flight

client = pa.flight.connect("grpc+tcp://localhost:8815")

# Upload a new dataset
NUM_BATCHES = 1024
ROWS_PER_BATCH = 4096
upload_descriptor = pa.flight.FlightDescriptor.for_path("streamed.parquet")
batch = pa.record_batch([
    pa.array(range(ROWS_PER_BATCH)),
], names=["ints"])
writer, _ = client.do_put(upload_descriptor, batch.schema)
with writer:
    for _ in range(NUM_BATCHES):
        writer.write_batch(batch)

# Read content of the dataset
flight = client.get_flight_info(upload_descriptor)
reader = client.do_get(flight.endpoints[0].ticket)
total_rows = 0
# read_table = reader.read_all()
# print(read_table.to_pandas().head())
for chunk in reader:
    print(chunk.data)
    total_rows += chunk.data.num_rows
print("Got", total_rows, "rows total, expected", NUM_BATCHES * ROWS_PER_BATCH)
