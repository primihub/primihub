import sqlite3
import centralized_service_pb2_grpc
import centralized_service_pb2
import sys
import logging
import grpc
from concurrent import futures
from concurrent.futures import ThreadPoolExecutor

LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
logging.basicConfig(level=logging.DEBUG,
                    format=LOG_FORMAT, datefmt=DATE_FORMAT)
logger = logging.getLogger("CentralizedDataservice")

def init_db(db_dir):
    db_path = db_dir + "dataservice.db"
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    sql_str = """
    create table if not exists dataset
    (name varchar(100) primary key,
    meta varchar(500));
    """
    cursor.execute(sql_str)
    logger.info("Init database finish.")



def insert_dataset_meta(db_dir, name, meta):
    db_path = db_dir + "dataservice.db"
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    sql_str = """
    select * from dataset where name=? 
    """
    cursor.execute(sql_str, (name,))
    rows = cursor.fetchall()
    if len(rows) != 0:
        raise Exception("Duplicate dataset name {} in database.".format(name))
    else:
        sql_str = """
        insert into dataset(name, meta) values(?,?)
        """
        cursor.execute(sql_str, (name, meta,))
        conn.commit()
        logger.info("Insert dataset name '{}' and it's meta '{}' finish.".format(name, meta))


def lookup_dataset_meta(db_dir, name):
    db_path = db_dir + "dataservice.db"
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    sql_str = """
    select * from dataset where name=? 
    """
    cursor.execute(sql_str, (name,))
    rows = cursor.fetchall()
    
    if len(rows) == 0:
        logger.warning("Can't find dataset meta with name {}.".format(name))
        return None
    elif len(rows) != 1:
        logger.warning("More than one record has key {}, this should be a bug.".format(name))
    else:
        logger.info("Lookup dataset meta finish, name: '{}', meta:'{}'.".format(rows[0][0], rows[0][1]))
    return rows


def get_all_dataset_meta(db_dir):
    db_path = db_dir + "dataservice.db"
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    sql_str = """
    select * from dataset
    """
    cursor.execute(sql_str)
    rows = cursor.fetchall()
    return rows


def delete_dataset_meta(db_dir, name):
    db_path = db_dir + "dataservice.db"
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    sql_str = """
    delete from dataset where name=?
    """
    cursor.execute(sql_str, (name,))
    conn.commit()
    logger.info("Delete record with key {}.".format(name))


class DatasetService(centralized_service_pb2_grpc.CentralizedDatasetService):
    def __init__(self, db_path):
        self.db_path = db_path
        init_db(db_path)

    def RegDataset(self, request, context):
        logger.info("Receive RegDataset request, name {}.".format(request.name))

        try:
            insert_dataset_meta(self.db_path, request.name, request.dataset_url) 
        except Exception as e:
            logger.error("Handle RegDataset request failed, {}.".format(repr(e)))
            errmsg = repr(e) 
            response = centralized_service_pb2.RegDatasetResponse()
            response.name = request.name
            response.status = centralized_service_pb2.RequestStatus.FAILED
            response.msg = errmsg
            return response

        response = centralized_service_pb2.RegDatasetResponse()
        response.name = request.name
        response.status = centralized_service_pb2.RequestStatus.FINISH
        logger.info("Handle RegDataset request finish.")
        return response


    def FindDataset(self, request, context):
        logger.info("Receive FindDataset request, name {}.".format(request.name))

        try:
            row = lookup_dataset_meta(self.db_path, request.name)
        except Exception as e:
            errmsg = repr(e)
            logger.error("Handle FindDataset request failed, {}.".format(errmsg))
            response = centralized_service_pb2.FindDatasetResponse()
            response.name = request.name
            response.status = centralized_service_pb2.RequestStatus.FAILED
            response.msg = errmsg
            return response

        response = centralized_service_pb2.RegDatasetResponse()
        response.name = row[0][0]
        response.msg = row[0][1]
        response.status = centralized_service_pb2.RequestStatus.FINISH
        logger.info("Handle FindDataset request finish.")
        return response
    
    def DelDataset(self, request, context):
        logger.info("Receive DelDataset request, name {}.".format(request.name))

        try:
            delete_dataset_meta(self.db_path, request.name)
        except Exception as e:
            errmsg = repr(e)
            logger.error("Handle DelDataset request failed, {}.".format(errmsg))
            response = centralized_service_pb2.DelDatasetResponse()
            response.name = request.name
            response.status = centralized_service_pb2.RequestStatus.FAILED
            response.msg = errmsg
            return response

        response = centralized_service_pb2.RegDatasetResponse()
        respnse.name = request.name
        response.status = centralized_service_pb2.RequestStatus.FINISH
        logger.info("Handle DelDataset request finish.")
        return response

    def ListDataset(self, request, context):
        logger.info("Receive ListDataset request.")

        try:
            rows = get_all_dataset_meta(self.db_path)
        except Exception as e:
            errmsg = repr(e)
            logger.info("Handle ListDataset request failed, {}.".format(errmsg))
            response = centralized_service_pb2.ListDatasetResponse()
            response.all_meta.append(errmsg)
            return response

        response = centralized_service_pb2.ListDatasetResponse()
        for row in rows:
            response.all_meta.append(row[1])
        logger.info("Handle ListDataset reques finish.")
        return response


if __name__ == '__main__':
    db_dir = sys.argv[1]
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    centralized_service_pb2_grpc.add_CentralizedDatasetServiceServicer_to_server(DatasetService(db_dir), server)
    server.add_insecure_port('[::]:10060')
    server.start()
    logger.info("Start grpc server at 10060.")
    server.wait_for_termination()

    # init_db(db_dir)
    # insert_dataset_meta(db_dir, "123", "124")
    # lookup_dataset_meta(db_dir, "123")
    # print(get_all_dataset_meta(db_dir))
    # delete_dataset_meta(db_dir, "123")
    # print(get_all_dataset_meta(db_dir))
