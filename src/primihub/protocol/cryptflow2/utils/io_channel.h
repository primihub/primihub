/*
Original Work Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
Modified Work Copyright (c) 2020 Microsoft Research

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

Enquiries about further applications and development opportunities are welcome.

Modified by Deevashwer Rathee
*/

#ifndef IO_CHANNEL_H__
#define IO_CHANNEL_H__
#include "src/primihub/protocol/cryptflow2/utils/block.h"
#include "src/primihub/protocol/cryptflow2/utils/group.h"

/** @addtogroup IO
  @{
 */

namespace primihub::sci {
template <typename T> class IOChannel {
public:
  void send_data(const void *data, int nbyte) {
    derived().send_data(data, nbyte);
  }
  void recv_data(void *data, int nbyte) { derived().recv_data(data, nbyte); }

  void send_block(const block128 *data, int nblock) {
    send_data(data, nblock * sizeof(block128));
  }

  void send_block(const block256 *data, int nblock) {
    send_data(data, nblock * sizeof(block256));
  }

  void recv_block(block128 *data, int nblock) {
    recv_data(data, nblock * sizeof(block128));
  }

  void send_pt(primihub::emp::Point *A, int num_pts = 1) {
    for (int i = 0; i < num_pts; ++i) {
      size_t len = A[i].size();
      A[i].group->resize_scratch(len);
      unsigned char *tmp = A[i].group->scratch;
      send_data(&len, 4);
      A[i].to_bin(tmp, len);
      send_data(tmp, len);
    }
  }

  void recv_pt(primihub::emp::Group *g, primihub::emp::Point *A, int num_pts = 1) {
    size_t len = 0;
    for (int i = 0; i < num_pts; ++i) {
      recv_data(&len, 4);
      g->resize_scratch(len);
      unsigned char *tmp = g->scratch;
      recv_data(tmp, len);
      A[i].from_bin(g, tmp, len);
    }
  }

private:
  T &derived() { return *static_cast<T *>(this); }
};
/**@}*/
} // namespace primihub::sci
#endif // IO_CHANNEL_H__
