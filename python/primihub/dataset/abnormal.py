import primihub as ph
import pyarrow
import json
import pandas as pd
from loguru import logger


def handle_mixed_column(col):
    col_sum = 0
    col_count = 0
    index_list = []
    for index, val in col.items():
        if val == "NA":
            logger.info("Column {} index {} has empty value.".format(
                col.name, index))
            continue

        try:
            tmp = float(val)
            col_sum = col_sum + tmp
            col_count = col_count + 1
        except:
            index_list.append(index)

    if len(index_list) != 0 and col_sum != 0:
        col_avg = col_sum / col_count
        for index in index_list:
            logger.info("Replace {} with {}, column {}, index {}.".format(
                col[index], col_avg, col.name, index))
            col[index] = str(col_avg)
    else:
        logger.info(
            "All content of column {} is string, do nothing.".format(col.name))


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
                col = df[col_name]
                if col.dtype == object:
                    handle_mixed_column(col)

        df.to_csv("/tmp/output.csv")
