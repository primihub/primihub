import csv
import sys
import random
import string

def gen_csv(row_num,col_num):
    party_0_file = "/tmp/train_party_0.csv"
    with open(party_0_file, "w") as f:
        writer = csv.writer(f)
        for i in range(row_num):
            row = []
            for j in range(col_num):
                row.append(random.randrange(0, 255))
            writer.writerow(row)
    
    party_1_file = "/tmp/train_party_1.csv"
    with open(party_1_file, "w") as f:
        writer = csv.writer(f)
        for i in range(row_num):
            row = []
            for j in range(col_num):
                row.append(random.randrange(0, 255))
            writer.writerow(row)


if __name__ == '__main__':
    # call this script with gen_maxpool_data x y,
    # it will generate data with x rows and y columns.
    row_num = int(sys.argv[1])
    col_num = int(sys.argv[2])
    gen_csv(row_num,col_num)
