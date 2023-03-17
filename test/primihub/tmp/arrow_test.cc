
#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/io/api.h>
#include <arrow/io/file.h>
#include <arrow/result.h>
#include <arrow/type.h>
#include <glog/logging.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <parquet/exception.h>
#include <parquet/stream_reader.h>

#include <iostream>

#include "gtest/gtest.h"
#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/data_store/factory.h"
using arrow::Array;
using arrow::DoubleArray;
using arrow::Int64Array;
using arrow::Table;
using namespace primihub;

void spiltStr(std::string str, const std::string& split,
              std::vector<std::string>& strlist) {
  strlist.clear();
  if (str == "") return;
  std::string strs = str + split;
  size_t pos = strs.find(split);
  int steps = split.size();

  while (pos != strs.npos) {
    std::string temp = strs.substr(0, pos);
    strlist.push_back(temp);
    strs = strs.substr(pos + steps, strs.size());
    pos = strs.find(split);
  }
}

int LoadDatasetFromCSV(std::string& filename) {
  std::string nodeaddr("test address");  // TODO
  std::shared_ptr<DataDriver> driver =
      DataDirverFactory::getDriver("CSV", nodeaddr);
  auto cursor = driver->read(filename);
  auto ds = cursor->read();
  std::shared_ptr<Table> table = std::get<std::shared_ptr<Table>>(ds->data);

  // Label column.
  std::vector<std::string> col_names = table->ColumnNames();
  // for (auto itr = col_names.begin(); itr != col_names.end(); itr++) {
  //   LOG(INFO) << *itr;
  // }
  bool errors = false;
  int num_col = table->num_columns();
  auto csv_array = table->column(0)->chunk(0);
  LOG(INFO) << csv_array->length();
  LOG(INFO) << csv_array->ToString();
  //   for (int64_t j = 0; j < csv_array->length(); j++) {
  //     LOG(INFO) << j << ":" << csv_array->Value(j);
  //   }
  // 'array' include values in a column of csv file.
  auto array = std::static_pointer_cast<DoubleArray>(
      table->column(num_col - 1)->chunk(0));
  int64_t array_len = array->length();
  LOG(INFO) << "Label column '" << col_names[num_col - 1] << "' has "
            << array_len << " values.";

  // Force the same value count in every column.
  for (int i = 0; i < num_col; i++) {
    auto array =
        std::static_pointer_cast<DoubleArray>(table->column(i)->chunk(0));
    std::vector<double> tmp_data;
    for (int64_t j = 0; j < array->length(); j++) {
      tmp_data.push_back(array->Value(j));
      LOG(INFO) << j << ":" << array->Value(j);
    }
    if (array->length() != array_len) {
      LOG(ERROR) << "Column "
                 << " has " << array->length()
                 << " value, but other column has " << array_len << " value.";
      errors = true;
      break;
    }
  }
  if (errors) return -1;

  return array->length();
}

// #0 Build dummy data to pass around
// To have some input data, we first create an Arrow Table that holds
// some data.
std::shared_ptr<arrow::Table> generate_table() {
  arrow::Int64Builder i64builder;

  PARQUET_THROW_NOT_OK(i64builder.AppendValues({1, 2, 3, 4, 5}));
  std::shared_ptr<arrow::Array> i64array;
  PARQUET_THROW_NOT_OK(i64builder.Finish(&i64array));

  arrow::StringBuilder strbuilder;
  PARQUET_THROW_NOT_OK(strbuilder.Append("some"));
  PARQUET_THROW_NOT_OK(strbuilder.Append("string"));
  PARQUET_THROW_NOT_OK(strbuilder.Append("content"));
  PARQUET_THROW_NOT_OK(strbuilder.Append("in"));
  PARQUET_THROW_NOT_OK(strbuilder.Append("rows"));
  std::shared_ptr<arrow::Array> strarray;
  PARQUET_THROW_NOT_OK(strbuilder.Finish(&strarray));

  std::shared_ptr<arrow::Schema> schema =
      arrow::schema({arrow::field("int", arrow::int64()),
                     arrow::field("str", arrow::utf8())});

  return arrow::Table::Make(schema, {i64array, strarray});
}

// #1 Write out the data as a Parquet file
void write_parquet_file(const arrow::Table& table) {
  std::shared_ptr<arrow::io::FileOutputStream> outfile;
  PARQUET_ASSIGN_OR_THROW(outfile, arrow::io::FileOutputStream::Open(
                                       "parquet-arrow-example.parquet"));
  // The last argument to the function call is the size of the RowGroup in
  // the parquet file. Normally you would choose this to be rather large but
  // for the example, we use a small value to have multiple RowGroups.
  PARQUET_THROW_NOT_OK(parquet::arrow::WriteTable(
      table, arrow::default_memory_pool(), outfile, 3));
}

// #2: Fully read in the file
void read_whole_file() {
  std::cout << "Reading data.parquet at once" << std::endl;
  std::shared_ptr<arrow::io::ReadableFile> infile;
  PARQUET_ASSIGN_OR_THROW(
      infile, arrow::io::ReadableFile::Open("data_0.parquet",
                                            arrow::default_memory_pool()));

  std::unique_ptr<parquet::arrow::FileReader> reader;
  PARQUET_THROW_NOT_OK(
      parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));
  std::shared_ptr<arrow::Table> table;
  PARQUET_THROW_NOT_OK(reader->ReadTable(&table));
  std::cout << "Loaded " << table->num_rows() << " rows in "
            << table->num_columns() << " columns." << std::endl;

  // Label column.
  std::vector<std::string> col_names = table->ColumnNames();


  bool errors = false;
  int num_col = table->num_columns();
  int num_row = table->num_rows();

  std::vector<std::string> count_col_names = {"C", "D"};
  arrow::Result<std::shared_ptr<Table>> res_table;
  std::shared_ptr<arrow::Table> new_table;
  std::shared_ptr<arrow::Array> doublearray;
  for (auto itr = count_col_names.begin(); itr != count_col_names.end();
       itr++) {
    std::vector<std::string>::iterator t =
        std::find(col_names.begin(), col_names.end(), *itr);
    double col_sum = 0;
    if (t != col_names.end()) {
      int tmp_index = std::distance(col_names.begin(), t);
      auto array = std::static_pointer_cast<DoubleArray>(
          table->column(tmp_index)->chunk(0));
      for (int64_t j = 0; j < array->length(); j++) {
        col_sum += array->Value(j);
      }
      col_sum = col_sum / num_row;
    }
    //进行MPC计算，之后得到一个三方和，将和除以3，
    ///把计算结果写入至本列中的null_index中，
    ///
    //找到原数据列，并构造新数据列csv_array
    int tmp_index = std::distance(col_names.begin(), t);
    auto csv_array = std::static_pointer_cast<DoubleArray>(
        table->column(tmp_index)->chunk(0));
    LOG(INFO) << csv_array->length();
    std::string data_str;
    std::vector<std::string> tmp1;
    std::vector<int> null_index;

    data_str = csv_array->ToString();
    LOG(INFO) << data_str;
    spiltStr(data_str, ",", tmp1);
    for (auto itr = tmp1.begin(); itr != tmp1.end(); itr++) {
      LOG(INFO) << *itr;
    }
    for (int i = 0; i < tmp1.size(); i++) {
      if (tmp1[i].find("null") != tmp1[i].npos) {
        null_index.push_back(i);
      }
    }
    //
    std::vector<double> new_col;
    for (int64_t i = 0; i < csv_array->length(); i++) {
      new_col.push_back(csv_array->Value(i));
    }

    for (auto itr = null_index.begin(); itr != null_index.end(); itr++) {
      new_col[*itr] = 1;
      LOG(INFO) << *itr;
    }
    arrow::DoubleBuilder double_builder;
    double_builder.AppendValues(new_col);
    double_builder.Finish(&doublearray);

    arrow::ChunkedArray *new_array=new arrow::ChunkedArray(doublearray);

    auto ptr_Array=std::shared_ptr<arrow::ChunkedArray>(new_array);
    res_table = table->SetColumn(
        tmp_index, arrow::field(*itr, arrow::float64()), ptr_Array);
    table = res_table.ValueUnsafe();
    std::vector<std::string> col_names1 = table->ColumnNames();
    for (auto itr = col_names1.begin(); itr != col_names1.end(); itr++) {
      LOG(INFO) << *itr;
    }
    auto array = std::static_pointer_cast<DoubleArray>(
        table->column(tmp_index)->chunk(0));
    for (int64_t j = 0; j < array->length(); j++) {
      LOG(INFO) << j << ":" << array->Value(j);
    }
  }
}

// #3: Read only a single RowGroup of the parquet file
void read_single_rowgroup() {
  std::cout << "Reading first RowGroup of parquet-arrow-example.parquet"
            << std::endl;
  std::shared_ptr<arrow::io::ReadableFile> infile;
  PARQUET_ASSIGN_OR_THROW(
      infile, arrow::io::ReadableFile::Open("parquet-arrow-example.parquet",
                                            arrow::default_memory_pool()));

  std::unique_ptr<parquet::arrow::FileReader> reader;
  PARQUET_THROW_NOT_OK(
      parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));
  std::shared_ptr<arrow::Table> table;
  PARQUET_THROW_NOT_OK(reader->RowGroup(0)->ReadTable(&table));
  std::cout << "Loaded " << table->num_rows() << " rows in "
            << table->num_columns() << " columns." << std::endl;
}

// #4: Read only a single column of the whole parquet file
void read_single_column() {
  std::cout << "Reading first column of parquet-arrow-example.parquet"
            << std::endl;
  std::shared_ptr<arrow::io::ReadableFile> infile;
  // parquet-arrow-example
  PARQUET_ASSIGN_OR_THROW(
      infile, arrow::io::ReadableFile::Open("data.parquet",
                                            arrow::default_memory_pool()));

  std::unique_ptr<parquet::arrow::FileReader> reader;
  PARQUET_THROW_NOT_OK(
      parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));
  std::shared_ptr<arrow::ChunkedArray> array;
  PARQUET_THROW_NOT_OK(reader->ReadColumn(1, &array));
  PARQUET_THROW_NOT_OK(arrow::PrettyPrint(*array, 4, &std::cout));
  // for (int64_t j = 0; j < array->length(); j++) {
  //   LOG(INFO) << j << ":" << array->Value(j);
  // }
  std::cout << std::endl;
}

// #5: Read only a single column of a RowGroup (this is known as ColumnChunk)
//     from the Parquet file.
void read_single_column_chunk() {
  std::cout << "Reading first ColumnChunk of the first RowGroup of "
               "parquet-arrow-example.parquet"
            << std::endl;
  std::shared_ptr<arrow::io::ReadableFile> infile;
  PARQUET_ASSIGN_OR_THROW(
      infile, arrow::io::ReadableFile::Open("parquet-arrow-example.parquet",
                                            arrow::default_memory_pool()));

  std::unique_ptr<parquet::arrow::FileReader> reader;
  PARQUET_THROW_NOT_OK(
      parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));
  std::shared_ptr<arrow::ChunkedArray> array;
  PARQUET_THROW_NOT_OK(reader->RowGroup(0)->Column(0)->Read(&array));
  PARQUET_THROW_NOT_OK(arrow::PrettyPrint(*array, 4, &std::cout));
  std::cout << std::endl;
}

TEST(null_test, arrow_test) {
  // std::string filename = "./data/arrow_0.csv";
  // int ret = LoadDatasetFromCSV(filename);
  // if (ret <= 0) {
  //   LOG(ERROR) << "Load dataset for train failed.";
  // }

  // std::shared_ptr<arrow::io::ReadableFile> infile;
  // PARQUET_ASSIGN_OR_THROW(infile,
  //                         arrow::io::ReadableFile::Open("data.parquet"));

  // parquet::StreamReader is{parquet::ParquetFileReader::Open(infile)};

  // Article arti;

  // while (!is.eof()) {
  //   is >> arti.name >> arti.price >> arti.quantity >> parquet::EndRow;
  //   std::cout << arti.name << " " << arti.price << " " << arti.quantity
  //             << std::endl;
  // }
  std::shared_ptr<arrow::Table> table = generate_table();
  // write_parquet_file(*table);
  read_whole_file();
  // read_single_rowgroup();
  // read_single_column();
  // read_single_column_chunk();
}
