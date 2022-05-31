import csv
import sys


def Combine(file1, file2):
    x = []
    y = []
    
    with open(file1) as f:
        reader = csv.reader(f)
        for row in reader:
            x.append(row)
    
    with open(file2) as f:
        reader = csv.reader(f)
        for row in reader:
            y.append(row[0])
    
    ds = []
    for x_v, y_v in zip(x,y):
        x_v.append(y_v)
        ds.append(x_v)

    return ds 


def LR_Generate3PCDataset(data_file, label_file, new_name):
    dataset = Combine(data_file, label_file)
    ds_len = len(dataset) 
    
    dim = len(dataset[0])
    head = []
    for i in range(dim-1):
        head.append("x_{}".format(i))
    head.append("y")

    unit = ds_len // 3;

    party_0_file = new_name + "_party_0.csv"
    with open(party_0_file, "w") as f:
        writer = csv.writer(f)
        writer.writerow(head)
        for v in dataset[0:unit]:
            writer.writerow(v)

    party_1_file = new_name + "_party_1.csv"
    with open(party_1_file, "w") as f:
        writer = csv.writer(f)
        writer.writerow(head)
        for v in dataset[unit: unit*2]:
            writer.writerow(v)

    party_2_file = new_name + "_party_2.csv"
    with open(party_2_file, "w") as f:
        writer = csv.writer(f)
        writer.writerow(head)
        for v in dataset[unit*2: unit*3]:
            writer.writerow(v)


if __name__ == '__main__':
    LR_Generate3PCDataset(sys.argv[1], sys.argv[2], sys.argv[3])
