// Copyright [2023] <Primihub>
#include "src/primihub/task/semantic/pir_util.h"

#include <glog/logging.h>

#include <filesystem>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

using namespace apsi;

namespace primihub::pir::util {
namespace fs = std::filesystem;

retcode throw_if_file_invalid(const std::string& file_name) {
  return retcode::SUCCESS;
  fs::path file(file_name);

  if (!fs::exists(file)) {
    LOG(ERROR) << "File `" << file.string() << "` does not exist";
    return retcode::FAIL;
  }
  if (!fs::is_regular_file(file)) {
    LOG(ERROR) << "File `" << file.string() << "` is not a regular file";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

CSVReader::CSVReader(const std::string &file_name) : file_name_(file_name) {
  throw_if_file_invalid(file_name_);
}

auto CSVReader::read(std::istream& stream) -> std::pair<DBData, std::vector<std::string>> {
  std::string line;
  DBData result;
  std::vector<std::string> orig_items;

  if (!getline(stream, line)) {
    LOG(WARNING) << "Nothing to read in `" << file_name_ << "`";
    return {UnlabeledData{}, {}};
  }
  std::string orig_item;
  Item item;
  Label label;
  auto [has_item, has_label] = process_line(line, orig_item, item, label);
  if (!has_item) {
    LOG(WARNING) << "Failed to read item from `" << file_name_ << "`";
    return {UnlabeledData{}, {}};
  }
  orig_items.push_back(std::move(orig_item));
  if (has_label) {
    result = LabeledData{std::make_pair(std::move(item), std::move(label)) };
  } else {
    result = UnlabeledData{std::move(item) };
  }

  while (getline(stream, line)) {
    std::string orig_item;
    Item item;
    Label label;
    auto [has_item, _] = process_line(line, orig_item, item, label);

    if (!has_item) {
      // Something went wrong; skip this item and move on to the next
      LOG(WARNING) << "Failed to read item from `" << file_name_ << "`";
      continue;
    }

    orig_items.push_back(std::move(orig_item));
    if (std::holds_alternative<UnlabeledData>(result)) {
      std::get<UnlabeledData>(result).push_back(std::move(item));
    } else if (std::holds_alternative<LabeledData>(result)) {
      std::get<LabeledData>(result).push_back(std::make_pair(std::move(item), std::move(label)));
    } else {
      // Something is terribly wrong
      LOG(ERROR) << "Critical error reading data";
      return {UnlabeledData{}, {}};
    }
  }

  return {std::move(result), std::move(orig_items)};
}


auto CSVReader::read(std::atomic<bool>* stop_flag) -> std::pair<DBData, std::vector<std::string>> {
  throw_if_file_invalid(file_name_);
  std::ifstream file(file_name_);
  if (!file.is_open()) {
    LOG(ERROR) << "File `" << file_name_ << "` could not be opened for reading";
    throw std::runtime_error("could not open file");
  }

  // return read(file);
  VLOG(5) << "begin to pre_process";
  auto processed_data_info = pre_process(file);
  VLOG(5) << "end of pre_process";
  auto& processed_data = std::get<0>(processed_data_info);
  auto is_labeled_data = std::get<1>(processed_data_info);
  DBData result;
  std::vector<std::string> origin_item;
  VLOG(5) << "begin to build DBData";
  if (is_labeled_data) {
    LabeledData labeled_data;
    labeled_data.reserve(processed_data.size());
    origin_item.reserve(processed_data.size());
    for (auto& item_info : processed_data) {
      auto& item_str = item_info.first;
      auto& label_str = item_info.second;
      origin_item.push_back(item_str);
      Item item = item_str;
      Label label;
      label.reserve(label_str.size());
      copy(label_str.begin(), label_str.end(), back_inserter(label));
      labeled_data.push_back(std::make_pair(std::move(item), std::move(label)));
    }
    result = std::move(labeled_data);
  } else {
    UnlabeledData unlabled_data;
    unlabled_data.reserve(processed_data.size());
    origin_item.reserve(processed_data.size());
    for (auto& item_info : processed_data) {
      auto& item_str = item_info.first;
      origin_item.push_back(item_str);
      Item item = item_str;
      unlabled_data.push_back(std::move(item));
    }
    result = std::move(unlabled_data);
  }
  VLOG(5) << "end of build DBData";
  return {std::move(result), std::move(origin_item) };
}

auto CSVReader::read() -> std::pair<DBData, std::vector<std::string>> {
  throw_if_file_invalid(file_name_);

  std::ifstream file(file_name_);
  if (!file.is_open()) {
    LOG(ERROR) << "File `" << file_name_ << "` could not be opened for reading";
    throw std::runtime_error("could not open file");
  }

  // return read(file);
  auto processed_data_info = pre_process(file);
  auto& processed_data = std::get<0>(processed_data_info);
  auto is_labeled_data = std::get<1>(processed_data_info);
  DBData result;
  std::vector<std::string> origin_item;
  if (is_labeled_data) {
    LabeledData labeled_data;
    labeled_data.reserve(processed_data.size());
    origin_item.reserve(processed_data.size());
    for (auto& item_info : processed_data) {
      auto& item_str = item_info.first;
      auto& label_str = item_info.second;
      origin_item.push_back(item_str);
      Item item = item_str;
      Label label;
      label.reserve(label_str.size());
      copy(label_str.begin(), label_str.end(), back_inserter(label));
      labeled_data.push_back(std::make_pair(std::move(item), std::move(label)));
    }
    result = std::move(labeled_data);
  } else {
    UnlabeledData unlabled_data;
    unlabled_data.reserve(processed_data.size());
    origin_item.reserve(processed_data.size());
    for (auto& item_info : processed_data) {
        auto& item_str = item_info.first;
        origin_item.push_back(item_str);
        Item item = item_str;
        unlabled_data.push_back(std::move(item));
    }
    result = std::move(unlabled_data);
  }
  return {std::move(result), std::move(origin_item) };
}

std::pair<std::map<std::string, std::string>, bool>
CSVReader::pre_process(std::istream& stream) {
  std::string line;
  bool is_labeled_data{false};
  size_t line_no{0};
  std::map<std::string, std::string> processed_data;
  if (!getline(stream, line)) {
    LOG(WARNING) << "Nothing to read in `" << file_name_ << "`";
    return {processed_data, false};
  } else {
    std::string item;
    std::string label;
    auto [has_item, has_label] = process_line(line, item, label);
    if (has_item) {
      std::string _label;
      if (has_label) {
        _label = std::move(label);
        is_labeled_data = true;
      }
      processed_data.insert({std::move(item), std::move(_label)});
    }
  }

  if (is_labeled_data) {
    std::string item;
    std::string label;
    while (getline(stream, line)) {
      line_no++;

      auto [has_item, _] = process_line(line, item, label);
      if (!has_item) {
        // Something went wrong; skip this item and move on to the next
        LOG(WARNING) << "Failed to read item from `" << file_name_ << "`";
        continue;
      }
      auto it = processed_data.find(item);
      if (it != processed_data.end()) {
        // duplicate data
        auto& label_ = it->second;
        label_.append("#####").append(std::move(label));
      } else {
        processed_data.insert({std::move(item), std::move(label)});
      }
      // if (line_no % 10000 == 0) {
      //   LOG(INFO) << "line number: " << line_no;
      // }
    }
  } else {
    std::string item;
    std::string label;
    while (getline(stream, line)) {
      auto [has_item, _] = process_line(line, item, label);
      if (!has_item) {
        // Something went wrong; skip this item and move on to the next
        LOG(WARNING) << "Failed to read item from `" << file_name_ << "`";
        continue;
      }
      auto it = processed_data.find(item);
      if (it != processed_data.end()) {
        continue;
      } else {
        processed_data.insert({std::move(item), std::string("")});
      }
    }
  }
  return {std::move(processed_data), is_labeled_data};
}

std::pair<bool, bool>
CSVReader::process_line(const std::string& line, std::string& item, std::string& label) {
  std::stringstream ss(line);
  std::string token;

  // First is the item
  getline(ss, token, ',');
  // Trim leading whitespace
  token.erase(
      token.begin(), find_if(token.begin(), token.end(), [](int ch) { return !isspace(ch); }));

  // Trim trailing whitespace
  token.erase(
      find_if(token.rbegin(), token.rend(), [](int ch) { return !isspace(ch); }).base(),
      token.end());

  if (token.empty()) {
    // Nothing found
    return {false, false};
  }
  // Item can be of arbitrary length; the constructor of Item will automatically hash it
  item = std::move(token);

  // Second is the label
  token.clear();
  getline(ss, token);

  // Trim leading whitespace
  token.erase(
      token.begin(), find_if(token.begin(), token.end(), [](int ch) { return !isspace(ch); }));

  // Trim trailing whitespace
  token.erase(
      find_if(token.rbegin(), token.rend(), [](int ch) { return !isspace(ch); }).base(),
      token.end());

  label.clear();
  label = std::move(token);

  return {true, !label.empty() };
}


std::pair<bool, bool> CSVReader::process_line(const std::string &line,
                                        std::string &orig_item, Item &item, Label &label) const {
  std::stringstream ss(line);
  std::string token;

  // First is the item
  getline(ss, token, ',');

  // Trim leading whitespace
  token.erase(
      token.begin(), find_if(token.begin(), token.end(), [](int ch) { return !isspace(ch); }));

  // Trim trailing whitespace
  token.erase(
      find_if(token.rbegin(), token.rend(), [](int ch) { return !isspace(ch); }).base(),
      token.end());

  if (token.empty()) {
    // Nothing found
    return { false, false };
  }

  // Item can be of arbitrary length; the constructor of Item will automatically hash it
  orig_item = token;
  item = token;

  // Second is the label
  token.clear();
  getline(ss, token);

  // Trim leading whitespace
  token.erase(
      token.begin(), find_if(token.begin(), token.end(), [](int ch) { return !isspace(ch); }));

  // Trim trailing whitespace
  token.erase(
      find_if(token.rbegin(), token.rend(), [](int ch) { return !isspace(ch); }).base(),
      token.end());

  label.clear();
  label.reserve(token.size());
  copy(token.begin(), token.end(), back_inserter(label));

  return {true, !token.empty()};
}

}  // namespace primihub::pir::util
