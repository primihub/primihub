"""
Copyright 2022 PrimiHub

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     https: //www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

import primihub as ph
# from primihub import context, dataset
from primihub.dataset.dataset import register_dataset
from concurrent.futures import ThreadPoolExecutor
import json
import csv
import copy
import codecs
import mysql.connector
from datetime import datetime
from primihub.utils.logger_util import logger


class DataAlign:
    def __init__(self, **kwargs):
        self.kwargs = kwargs
        self.common_params = self.kwargs['common_params']
        self.role_params = self.kwargs['role_params']
        self.node_info = self.kwargs['node_info']
        self.task_info = self.kwargs['task_info']
        self.party_name = self.role_params["self_name"]
        logger.info(self.common_params)
        logger.info(self.role_params)
        logger.info(self.node_info)


    def parse_dataset_param(self):
        self_host_info = self.node_info.get(self.party_name, None)
        if self_host_info is None:
          raise Exception("Party access info for self is not found {}".format(self.party_name))
        use_tls = 1 if self_host_info.use_tls else 0
        service_addr = "{}:{}:{}".format(self_host_info.ip.decode(),
                                        self_host_info.port,
                                        use_tls)
        logger.info(service_addr)
        if not self.role_params:
          raise Exception("no meta info found for {}".format(self.party_name))
        # find detail of dataset
        data_access_info = self.role_params["data"]
        logger.info("data_access_info: {}".format(data_access_info))

        local_meta_info = copy.deepcopy(self.role_params)
        local_meta_info["localdata_path"] = data_access_info
        local_meta_info["host_address"] = service_addr
        logger.info(local_meta_info)
        return local_meta_info

    def run(self):
      logger.info("DataAlign run")
      local_meta_info = self.parse_dataset_param()
      self.filter_rows_with_intersect_ids(local_meta_info)

    def register_new_dataset(self, sever_address, dataset_type, dataset_path, dataset_id):
        logger.info("register_new_dataset: {} {} {} {}".format(
            sever_address, dataset_type, dataset_path, dataset_id))
        register_dataset(sever_address, dataset_type, dataset_path, dataset_id)


    def mysql_thread_query(self, db_info, intersect_ids, start_index, end_index, column_str):
        start_tm = datetime.now()
        try:
            db = mysql.connector.connect(host=db_info["host"],
                                        port=db_info["port"],
                                        database=db_info["dbName"],
                                        username=db_info["username"],
                                        password=db_info["password"])
            cursor = db.cursor()
        except Exception as e:
            logger.error("Connect to mysql server failed or init cursor failed.")
            logger.error(e)
            raise e

        batch_size = 1000
        num_iter, reminder = divmod(end_index - start_index, batch_size)
        result_list = []
        for i in range(num_iter):
            sql_fmt = "select {} from {} where {} in {}"

            inner_start_index = start_index + i * batch_size
            inner_end_index = inner_start_index + batch_size
            query_ids = tuple(intersect_ids[inner_start_index:inner_end_index])
            sql = sql_fmt.format(
                column_str, db_info["tableName"], db_info["index_column"], query_ids)
            try:
                cursor.execute(sql)
            except Exception as e:
                logger.error("Run sql '{}...' failed, query range [{}, {}).".format(
                    sql[0:-1], inner_start_index, inner_end_index))
                logger.error(e)
                raise e
            else:
                result_list.extend(cursor.fetchall())

        if reminder != 0:
            sql_fmt = "select {} from {} where {} in {}"

            inner_start_index = start_index + num_iter * batch_size
            inner_end_index = inner_start_index + reminder
            sql = sql_fmt.format(column_str, db_info["tableName"], db_info["index_column"],
                                tuple(intersect_ids[inner_start_index:inner_end_index]))
            try:
                cursor.execute(sql)
            except Exception as e:
                logger.error("Run sql '{}...' failed, query range [{}, {}).".format(
                    sql[0:100], inner_start_pos, inner_end_pos))
                logger.error(e)
                raise e
            else:
                result_list.extend(cursor.fetchall())

        end_tm = datetime.now()
        diff_time = end_tm - start_tm

        logger.info("Query id in range ({}, {}) finish, cost {} seconds.".format(
            start_index, end_index,
            diff_time.total_seconds()))

        return result_list


    def mysql_run_query_multithread(self,
                                    db_info,     # Database connection,
                                    pool,        # Thread pool,
                                    curr_iter,   # Current iterate,
                                    batch_size,  # Count of id each thread handle,
                                    num_thread,  # Total thead in this pool,
                                    id_list,     # All intersect ids,
                                    columns):    # Column to query.
        index_base = curr_iter * batch_size * num_thread
        result_list = []
        query_futs = []

        # Run query in thread pool.
        for j in range(num_thread):
            start_index = index_base + batch_size * j
            end_index = start_index + batch_size
            fut = pool.submit(self.mysql_thread_query, db_info, id_list,
                              start_index, end_index, columns)
            query_futs.append(fut)

        # Check error during query.
        query_error = False
        for fut in query_futs:
            thread_error = fut.exception()
            if thread_error is not None:
                logger.error(thread_error)
                query_error = True

        # Drop the result or reserve them.
        if query_error:
            for fut in query_futs:
                try:
                    result = fut.result()
                except Exception as e:
                    continue
                else:
                    del result

            return None
        else:
            for fut in query_futs:
                result_list.extend(fut.result())

            result_list.sort(key=lambda v: v[-1])
            return result_list


    def generate_new_datast_from_mysql(self, meta_info, query_thread_num):
        db_info = meta_info["localdata_path"]
        # Connect to mysql server and create cursor.
        try:
            cxn = mysql.connector.connect(host=db_info["host"],
                                          port=db_info["port"],
                                          username=db_info["username"],
                                          password=db_info["password"])
            cursor = cxn.cursor()
        except Exception as e:
            logger.error("Connect to mysql server or create cursor failed.")
            logger.error(e)
            raise e

        # Get column name except for id column.
        # sql_template = ("SELECT column_name, data_type FROM information_schema.COLUMNS "
        #                 "WHERE TABLE_NAME='{}' and TABLE_SCHEMA='{}' ORDER BY column_name ASC;")
        # sql = sql_template.format(db_info["tableName"], db_info["dbName"])
        # selected_columns = []
        # try:
        #     cursor.execute(sql)
        # except Exception as e:
        #     logger.error("Run sql 'desc {}' failed.".format(db_info["tableName"]))
        #     logger.error(e)
        #     raise e
        # else:
        #     table_columns = []
        #     for col_info in cursor.fetchall():
        #         table_columns.append(col_info[0])

        #     index_column = table_columns[meta_info["index"][0]]
        #     db_info["index_column"] = index_column

        #     logger.info("The column corresponds to index {} is {}.".format(
        #         meta_info["index"], index_column))

        #     selected_columns = []
        #     for col_name in table_columns:
        #         selected_columns.append(col_name)
        # for mysql, just support for one selected index
        index = meta_info["index"][0]
        schema = json.loads(db_info["schema"])
        selected_columns = []
        for field in schema:
          for field_name in field:
            selected_columns.append(field_name)
        index_col_name = selected_columns[index]
        db_info["index_column"] = index_col_name
        logger.info("Column name of table {} is {}.".format(db_info["tableName"], selected_columns))

        selected_column_str = "`{}`".format(selected_columns[0])
        for i in range(len(selected_columns) - 1):
            new_str = "`{}`".format(selected_columns[i+1])
            selected_column_str = selected_column_str + "," + new_str
        # Collect all ids that PSI output.
        intersect_ids = []
        try:
            code_type = "utf-8"
            if self.hasBOM(meta_info["psiPath"]):
                code_type = "utf-8-sig"
            with open(meta_info["psiPath"], encoding=code_type) as in_f:
                reader = csv.reader(in_f)
                next(reader)
                for id in reader:
                    intersect_ids.append(id[0])
        except OSError as e:
            logger.error("Open file {} for read failed.".format(
                meta_info["psiPath"]))
            logger.error(e)
            raise e
        if (len(intersect_ids) == 0):
            raise Exception("PSI result is empty, no intersection is found")
        # Open new file to save all value of these ids.
        writer = None
        try:
            out_f = open(meta_info["outputPath"], "w")
        except OSError as e:
            logger.error("Open file {} for write failed.".format(
                meta_info["outputPath"]))
            logger.error(e)
            raise e
        else:
            writer = csv.writer(out_f)
            writer.writerow(selected_columns)

        # Run multithread query in batch mode, first query value of ids in current
        # batch then saves them to csv in every batch.
        num_intersect = len(intersect_ids)
        if num_intersect < 10000:
            batch_size = 1000
        elif num_intersect < 100000:
            batch_size = 10000
        else:
            batch_size = 100000

        num_thread = query_thread_num
        num_rows = 0

        thread_error = False
        num_batch, reminder = divmod(num_intersect, batch_size)
        with ThreadPoolExecutor(max_workers=num_thread) as pool:
            num_iter, remain_thread = divmod(num_batch, num_thread)
            for i in range(num_iter):
                # Run query in thread pool.
                result = self.mysql_run_query_multithread(
                    db_info, pool, i, batch_size,
                    num_thread, intersect_ids, selected_column_str)
                if result is None:
                    thread_error = True
                    break

                # Write value from database into out file.
                num_rows = num_rows + len(result)
                for thread_result in result:
                    writer.writerow(thread_result)

                del result

            if not thread_error and remain_thread != 0:
                # Run query in thread pool.
                result = self.mysql_run_query_multithread(
                    db_info, pool, num_iter, batch_size,
                    remain_thread, intersect_ids, selected_column_str)
                if result is None:
                    thread_errors = True

                # Write value from database into out file.
                num_rows = num_rows + len(result)
                for thread_result in result:
                    writer.writerow(thread_result)

                del result

        if reminder != 0 and thread_error is False:
            try:
                start_pos = num_batch * batch_size
                end_pos = start_pos + reminder
                thread_result = self.mysql_thread_query(
                    db_info, intersect_ids, start_pos, end_pos, selected_column_str)
            except Exception as e:
                thread_errors = True
            else:
                num_rows = num_rows + len(thread_result)
                thread_result.sort(key=lambda v: v[-1])
                for result in thread_result:
                    writer.writerow(result)

        if thread_error is True:
            logger.error("Error occurs, delete destination file.")
            out_f.close()
            os.remove(meta_info["outputPath"])
            return

        out_f.close()

        if num_rows != len(intersect_ids):
            raise RuntimeError("Expect query {} rows from mysql but mysql return {} rows, this should be a bug.".format(
                len(intersect_ids), num_rows))
        else:
            new_dataset_id = meta_info["newDataSetId"]
            host_address = meta_info["host_address"]
            new_dataset_output_path = meta_info["outputPath"]
            self.register_new_dataset(host_address, "CSV",
                                new_dataset_output_path, new_dataset_id)
            logger.info("Finish.")

        return

    def hasBOM(self, csv_file):
        with open(csv_file, 'rb') as f:
            bom = f.read(3)
            if bom.startswith(codecs.BOM_UTF8):
                return True
            else:
                return False

    def generate_new_dataset_from_csv(self, meta_info):
        psi_path = meta_info["psiPath"]
        new_dataset_id = meta_info["newDataSetId"]
        new_dataset_output_path = meta_info["outputPath"]
        psi_index = meta_info["index"]
        old_dataset_path = meta_info["localdata_path"]["data_path"]
        host_address = meta_info["host_address"]

        log_template = ("psi_path: {}, new_dataset_id {}, new_dataset_output_path {} "
                        "psi_index {} old_dataset_path {}")
        log_msg = log_template.format(psi_path, new_dataset_id,
                                      new_dataset_output_path,
                                      psi_index, old_dataset_path)
        logger.info(log_msg)

        intersection_map = {}
        intersection_set = set()
        intersection_list = list()

        code_type = "utf-8"
        if self.hasBOM(psi_path):
            code_type = "utf-8-sig"

        with open(psi_path, encoding=code_type) as f:
            f_csv = csv.reader(f)
            header = next(f_csv)
            for items in f_csv:
                item = items[0].strip()
                if not item:
                    continue
                intersection_set.add(item)
                intersection_list.append(item)
        if (len(intersection_list) == 0):
            raise Exception("PSI result is empty, no intersection is found")

        code_type = "utf-8"
        if self.hasBOM(old_dataset_path):
            code_type = "utf-8-sig"
        with open(old_dataset_path, encoding=code_type) as old_f, \
                open(new_dataset_output_path, 'w') as new_f:
            f_csv = csv.reader(old_f)
            header = next(f_csv)
            print(header)
            new_file_writer = csv.writer(new_f)
            new_file_writer.writerow(header)
            for row in f_csv:
                psi_key = ""
                if isinstance(psi_index, list):
                    for _index in psi_index:
                        psi_key += row[_index]
                else:
                    psi_key = row[psi_index]
                if psi_key not in intersection_set:
                    continue
                else:
                    if psi_key != intersection_list[0]:  # save to map
                        intersection_map[psi_key] = row
                    else:
                        new_file_writer.writerow(row)
                        del intersection_list[0]
                    while True:
                        if len(intersection_list) == 0:
                            break
                        psi_key = intersection_list[0]
                        if psi_key in intersection_map:
                            new_file_writer.writerow(intersection_map[psi_key])
                            del intersection_list[0]
                        else:
                            break
        self.register_new_dataset(host_address, "csv", new_dataset_output_path, new_dataset_id)


    def filter_rows_with_intersect_ids(self, meta_info, thread_num=5):
        dataset_model = meta_info["localdata_path"]["type"]
        if dataset_model.lower() == "csv":
            self.generate_new_dataset_from_csv(meta_info)
        elif dataset_model.lower() == "mysql":
            self.generate_new_datast_from_mysql(meta_info, thread_num)
        else:
            raise RuntimeError(
                "Don't support dataset model {} now.".format(dataset_model))

