#!/usr/bin/env python
# -*- coding: utf-8 -*-
from os import path
import primihub as ph
from primihub import dataset as ds
import pandas as pd
import primihub.dataset.dataset_warpper as dataset_warpper

# define dataset
ph.dataset.define("guest_dataset")
ph.dataset.define("label_dataset")

here = path.abspath(path.dirname(__file__))
print("here: ", here)

@ph.function(role='host', protocol='xgboost', datasets=["label_dataset"], next_peer="*:5555")
def xgb_host_logic():
    print("start xgb host logic...")
    dataset_service_ptr = ph.context.Context.get_dataset_service("host")
    if dataset_service_ptr is None:
        print("ERROR: No dataset service")
        
    dr = ds.driver("csv")
    cursor = ds.driver("csv")().read(path=here+'/data/Pokemon.csv')
    d = cursor.read()
    print(type(d.as_arrow()))
    
    dataset_warpper.reg_arrow_table_as_ph_dataset(dataset_service_ptr,
                                                  "test_dataset_host_from_py",
                                                  d.as_arrow())

@ph.function(role='guest', protocol='xgboost', datasets=["guest_dataset"], next_peer="localhost:5555")
def xgb_guest_logic():
    print("start xgb guest logic...")
   
