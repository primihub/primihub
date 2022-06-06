/* Copyright (C) 2012-2017 IBM Corp.
 * This program is Licensed under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. See accompanying LICENSE file.
 *
 * @file NumbTh.h
 * @brief Miscellaneous utility functions.
 *
 * Modified for CrypTFlow2. Stripped-down version of the original file.
 */

#ifndef ARGMAPPING_H
#define ARGMAPPING_H
#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

typedef std::unordered_map<std::string, const char *> argmap_t;

inline bool parseArgs(int argc, char *argv[], argmap_t &argmap) {
  for (long i = 1; i < argc; i++) {
    char *x = argv[i];
    long j = 0;
    while (x[j] != '=' && x[j] != '\0')
      j++;
    if (x[j] == '\0')
      return false;
    std::string arg(x, j);
    if (argmap[arg] == NULL)
      return false;
    argmap[arg] = x + j + 1;
  }
  return true;
}

inline bool doArgProcessing(std::string *value, const char *s) {
  *value = std::string(s);
  return true;
}

template <class T> inline bool doArgProcessing(T *value, const char *s) {
  std::string ss(s);
  std::stringstream sss(ss);
  return bool(sss >> *value);
}

class ArgProcessor {
public:
  virtual bool process(const char *s) = 0;
};

/* ArgProcessorDerived: templated subclasses */

template <class T> class ArgProcessorDerived : public ArgProcessor {
public:
  T *value;

  virtual bool process(const char *s) { return doArgProcessing(value, s); }

  ArgProcessorDerived(T *_value) : value(_value) {}
  virtual ~ArgProcessorDerived(){};
};

class ArgMapping {
public:
  std::unordered_map<std::string, std::shared_ptr<ArgProcessor>> map;
  std::stringstream doc;

  // no documentation
  template <class T> void arg(const char *name, T &value) {
    std::shared_ptr<ArgProcessor> ap =
        std::shared_ptr<ArgProcessor>(new ArgProcessorDerived<T>(&value));

    assert(!map[name]);
    map[name] = ap;
  }

  // documentation + standard default info
  template <class T> void arg(const char *name, T &value, const char *doc1) {
    arg(name, value);
    doc << "\t" << name << " \t" << doc1 << "  [ default=" << value << " ]"
        << "\n";
  }

  // documentation + standard non-standard default info:
  // NULL => no default info
  template <class T>
  void arg(const char *name, T &value, const char *doc1, const char *info) {
    arg(name, value);
    doc << "\t" << name << " \t" << doc1;
    if (info)
      doc << "  [ default=" << info << " ]"
          << "\n";
    else
      doc << "\n";
  }

  inline void note(const char *s);
  inline void usage(const char *prog);
  inline void parse(int argc, char **argv);
  inline std::string documentation();
};

void ArgMapping::note(const char *s) { doc << "\t\t   " << s << "\n"; }

void ArgMapping::usage(const char *prog) {
  std::cerr << "Usage: " << prog << " [ name=value ]...\n";
  std::cerr << documentation();
  exit(0);
}

void ArgMapping::parse(int argc, char **argv) {
  for (long i = 1; i < argc; i++) {
    const char *x = argv[i];
    long j = 0;
    while (x[j] != '=' && x[j] != '\0')
      j++;
    if (x[j] == '\0')
      usage(argv[0]);
    std::string name(x, j);
    const char *s = x + j + 1;

    std::shared_ptr<ArgProcessor> ap = map[name];
    if (!ap)
      return usage(argv[0]);
    if (!ap->process(s))
      usage(argv[0]);
  }
}

std::string ArgMapping::documentation() { return doc.str(); }

#endif // ARGMAPPING_H
