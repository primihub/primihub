import pyarrow as pa
import pyarrow.flight

client = pa.flight.connect("grpc://59.110.170.174:8815")
print(client.list_actions())

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

# do action
client.do_action(pa.flight.Action("do_something", "uploaded.parquet".encode('utf-8')))