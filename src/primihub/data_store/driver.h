/*
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */


#ifndef SRC_PRIMIHUB_DATASTORE_DRIVER_H_
#define SRC_PRIMIHUB_DATASTORE_DRIVER_H_

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <exception>
#include <memory>



// #include "src/primihub/common/clp.h"
// #include "src/primihub/common/type/type.h"
#include "src/primihub/data_store/dataset.h"


namespace primihub
{



  // ====== Data Store Driver ======

  class Dataset;
  class Cursor {
    public:
      virtual std::shared_ptr<primihub::Dataset> read() = 0;
      virtual std::shared_ptr<primihub::Dataset> read(int64_t offset, int64_t limit) = 0;
      virtual int write(std::shared_ptr<primihub::Dataset> dataset) = 0;
      virtual void close() = 0;
  };


  class DataDriver
  {
    public:
      explicit DataDriver(const std::string& nodelet_addr) {
        nodelet_address = nodelet_addr;
      }
      virtual ~DataDriver() { };
      virtual std::string getDataURL() const = 0;
      virtual std::shared_ptr<Cursor>& read(const std::string &dataURL) = 0;
      virtual std::shared_ptr<Cursor>& initCursor(const std::string &dataURL) = 0;
  
      std::shared_ptr<Cursor>& getCursor();
      std::string getDriverType() const;
      std::string getNodeletAddress() const;

    protected:  
      void setCursor(std::shared_ptr<Cursor> &cursor) { this->cursor = cursor; }
      std::shared_ptr<Cursor> cursor;
      std::string driver_type;
      std::string nodelet_address;
  };


} // namespace primihub

#endif  // SRC_PRIMIHUB_DATASTORE_DRIVER_H_
