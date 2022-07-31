import pyarrow as pa
import pyarrow.flight



def upload_dataset(dataset, server_addr="grpc://59.110.170.174:8815"):
    client = pa.flight.connect(server_addr)
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
    print("uploaded done")