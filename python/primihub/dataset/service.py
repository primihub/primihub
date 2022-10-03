import sqlite3
# import centralized_service_pb2_grpc
# import centralized_service_pb2
import sys
import logging

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
    logger.info("Delete record with key {}".format(name))
    

if __name__ == '__main__':
    db_dir = sys.argv[1]
    init_db(db_dir)
    insert_dataset_meta(db_dir, "123", "124")
    lookup_dataset_meta(db_dir, "123")
    print(get_all_dataset_meta(db_dir))
    delete_dataset_meta(db_dir, "123")
    print(get_all_dataset_meta(db_dir))
