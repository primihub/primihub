// Copyright [2021] <primihub.com>
#include "src/primihub/common/clp.h"

namespace primihub {

void CLP::parse(int argc, char const*const* argv) {
    if (argc > 0) {
        std::stringstream ss;
        auto ptr = argv[0];
        while (*ptr != 0)
            ss << *ptr++;
        mProgramName = ss.str();
    }

    for (int i = 1; i < argc;) {
        mFullStr += std::string(argv[i]) + " ";

        auto ptr = argv[i];
        if (*ptr++ != '-') {
            throw CommandLineParserError(
                "While parsing the argv string, "
                "one of the leading terms did not start with a - indicator.");
        }

        std::stringstream ss;

        while (*ptr != 0)
            ss << *ptr++;

        ++i;
        ptr = argv[i];

        std::pair<std::string, std::list<std::string>> keyValues;
        keyValues.first = ss.str();;

        while (i < argc && (ptr[0] != '-' || (ptr[0] == '-' && ptr[1] >= '0' && ptr[1] <= '9'))) {
            ss.str("");

            while (*ptr != 0)
                ss << *ptr++;

            keyValues.second.push_back(ss.str());

            ++i;
            ptr = argv[i];
        }

        mKeyValues.emplace(keyValues);
    }
}

std::vector<std::string> split(const std::string& s, char delim);

void CLP::setDefault(std::string key, std::string value) {
    if (!hasValue(key)) {
        if (isSet(key)) {
            mKeyValues[key].emplace_back(value);
        } else {
            auto parts = split(value, ' ');
            mKeyValues.emplace(std::make_pair(key, std::list<std::string>{ parts.begin(), parts.end()}));
        }
    }

}
void CLP::setDefault(std::vector<std::string> keys, std::string value) {
    if (hasValue(keys) == false) {
        setDefault(keys[0], value);
    }
}

void CLP::set(std::string name) {
    mKeyValues[name];
}

bool CLP::isSet(std::string name)const {
    return mKeyValues.find(name) != mKeyValues.end();
}

bool CLP::isSet(std::vector<std::string> names) const {
    for (auto& name : names) {
        if (isSet(name)) {
            return true;
        }
    }
    return false;
}

bool CLP::hasValue(std::string name) const {
    return mKeyValues.find(name) != mKeyValues.end() && mKeyValues.at(name).size();
}

bool CLP::hasValue(std::vector<std::string> names) const {
    for (auto name : names) {
        if (hasValue(name)) {
            return true;
        }
    }
    return false;
}

const std::list<std::string>& CLP::getList(const std::vector<std::string>& names) const {
    for (auto& name : names) {
        if (isSet(name)) {
            return mKeyValues.find(name)->second;
        }
    }
    throw CommandLineParserError("key not set");
}
}  // namespace primihub
