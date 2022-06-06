/*
Authors: Deevashwer Rathee
Copyright:
Copyright (c) 2021 Microsoft Research
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef BOOL_DATA_H__
#define BOOL_DATA_H__

#include "Math/math-functions.h"

#define I 0
#define J 1
#define print_bool(vec)                                                        \
  {                                                                            \
    auto tmp_pub = bool_op->output(PUBLIC, vec).subset(I, I + J);              \
    cout << #vec << "_pub: " << tmp_pub << endl;                               \
  }

// A container to hold an array of boolean values
// If party is set as PUBLIC for a BoolArray instance, then the underlying array is known publicly and we maintain the invariant that both parties will hold identical data in that instance.
// Else, the underlying array is secret-shared and the class instance will hold the party's share of the secret array. In this case, the party data member denotes which party this share belongs to.
class BoolArray {
public:
  int party = sci::PUBLIC;
  int size = 0;            // size of array
  uint8_t *data = nullptr; // data

  BoolArray(){};

  BoolArray(int party_, int sz) {
    assert(party_ == sci::PUBLIC || party_ == sci::ALICE || party_ == sci::BOB);
    assert(sz > 0);
    this->party = party_;
    this->size = sz;
    data = new uint8_t[sz];
  }

  // copy constructor
  BoolArray(const BoolArray &other) {
    this->size = other.size;
    this->party = other.party;
    this->data = new uint8_t[size];
    memcpy(this->data, other.data, size * sizeof(uint8_t));
  }

  // move constructor
  BoolArray(BoolArray &&other) noexcept {
    this->size = other.size;
    this->party = other.party;
    this->data = other.data;
    other.data = nullptr;
  }

  ~BoolArray() { delete[] data; }

  // copy assignment
  BoolArray &operator=(const BoolArray &other) {
    if (this == &other) return *this;

    delete[] this->data;
    this->size = other.size;
    this->party = other.party;
    this->data = new uint8_t[size];
    memcpy(this->data, other.data, size * sizeof(uint8_t));
    return *this;
  }

  // move assignment
  BoolArray &operator=(BoolArray &&other) noexcept {
    if (this == &other) return *this;

    delete[] this->data;
    this->size = other.size;
    this->party = other.party;
    this->data = other.data;
    other.data = nullptr;
    return *this;
  }

  // BoolArray[i, j)
  BoolArray subset(int i, int j);
};

// prints the contents of other, which must be a PUBLIC BoolArray
std::ostream &operator<<(std::ostream &os, BoolArray &other);

class BoolOp {
  friend class FixOp;
  friend class FPOp;
  friend class FPMath;
public:
  int party;

  BoolOp(int party, sci::IOPack *iopack, sci::OTPack *otpack) {
    this->party = party;
    this->iopack = iopack;
    this->otpack = otpack;
    this->aux = new AuxProtocols(party, iopack, otpack);
  }

  ~BoolOp() { delete aux; }

  // input functions: return a BoolArray that stores data_
  // party_ denotes which party provides the input data_ and the data_ provided by the other party is ignored. If party_ is PUBLIC, then the data_ provided by both parties must be identical.
  // sz is the size of the returned BoolArray and the uint8_t array pointed by data_
  BoolArray input(int party_, int sz, uint8_t* data_);
  // same as the above function, except that it replicates data_ in all sz positions of the returned BoolArray
  BoolArray input(int party_, int sz, uint8_t data_);

  // output function: returns the secret array underlying x in the form of a PUBLIC BoolArray
  // party_ denotes which party will receive the output. If party_ is PUBLIC, both parties receive the output.
  BoolArray output(int party_, const BoolArray& x);

  // Multiplexers: return x[i] if cond[i] = 1; else return y[i]
  // cond must be a secret-shared BoolArray
  //// Both x and y can be PUBLIC or secret-shared
  //// cond, x, y must have equal size
  BoolArray if_else(const BoolArray &cond, const BoolArray &x, const BoolArray &y);
  //// x can be PUBLIC or secret-shared
  //// cond, x must have equal size
  //// y[i] = y
  BoolArray if_else(const BoolArray &cond, const BoolArray &x, uint8_t y);
  //// y can be PUBLIC or secret-shared
  //// cond, y must have equal size
  //// x[i] = x
  BoolArray if_else(const BoolArray &cond, uint8_t x, const BoolArray &y);

  // NOT Gate: return ! x[i]
  BoolArray NOT(const BoolArray &x);

  // Boolean Arithmetic Gates: return x[i] OP y[i], OP = {^, &, |}
  // x and y must have equal size
  //// Both x and y can be PUBLIC or secret-shared
  BoolArray XOR(const BoolArray &x, const BoolArray &y);
  //// At least one of x and y must be a secret-shared BoolArray
  BoolArray AND(const BoolArray &x, const BoolArray &y);
  //// At least one of x and y must be a secret-shared BoolArray
  BoolArray OR(const BoolArray &x, const BoolArray &y);

private:
  sci::IOPack *iopack;
  sci::OTPack *otpack;
  AuxProtocols *aux;
};

#endif // BOOL_DATA_H__
