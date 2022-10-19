import primihub as ph
import pyarrow
import json
import pandas as pd
from loguru import logger


def handle_mixed_row(row):
    row_sum = 0
    row_count = 0
    index_list = []
    for index, val in row.items():
        if val == "NA":
            continue

        try:
            tmp = float(val)
            row_sum = row_sum + tmp
            row_count = row_count + 1
        except:
            index_list.append(index)

    if len(index_list) != 0 and row_sum != 0:
        row_avg = row_sum / row_count
        for index in index_list:
            logger.info("Replace {} with {}, column {}, index {}.".format(
                row[index], row_avg, row.name, index))
            row[index] = str(row_avg)
    else:
        logger.info(
            "All content of column {} is string, do nothing.".format(row.name))


@ph.context.function(role="host", protocol="None", datasets=["train_party_0"], port="-1")
def run_abnormal_process():
    path_or_info = ph.context.Context.dataset_map["train_party_0"]

    use_db = True
    try:
        db_conn_info = json.loads(path_or_info)
        use_db = True
    except Exception as e:
        use_db = False

    col_info_str = ph.context.Context.params_map["ColumnDtype"]
    col_info = json.loads(col_info_str)

    if use_db is True:
        pass
    else:
        df = pd.read_csv(path_or_info)
        df.info(verbose=True)
        df = df.fillna("NA")

        for col_name, type in col_info.items():
            if type == 1 or type == 2 or type == 3:
                row = df[col_name]
                if row.dtype == object:
                    handle_mixed_row(row)

        df.to_csv("/tmp/output.csv")
