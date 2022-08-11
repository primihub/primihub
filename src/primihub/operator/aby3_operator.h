
#ifndef SRC_primihub_operator_ABY3_operator_H
#define SRC_primihub_operator_ABY3_operator_H

#include <algorithm>
#include <random>
#include <vector>

#include "src/primihub/common/defines.h"
#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/evaluator/piecewise.h"
#include "src/primihub/protocol/aby3/sh3_gen.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/session.h"

#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/common/type/type.h"
namespace primihub
{
  const Decimal D = D16;

  class MPCOperator
  {
  public:
    // ios：帮助执行网络通信
    IOService ios;
    // enc:用于执行分享揭露等算子
    Sh3Encryptor enc;
    // eval:用于执行交互性操作，如乘法
    Sh3Evaluator eval;
    // runtime：用于网络通信和帮助计算并行操作
    Sh3Runtime runtime;
    //本参与方的party id
    u64 partyIdx;
    //构建session时需提供的next_name和prev_name
    string next_name;
    string prev_name;

    //构造函数
    //功能：初始化partyIdx，next_name，prev_name
    MPCOperator(u64 partyIdx_, string NextName, string PrevName) : partyIdx(partyIdx_), next_name(NextName), prev_name(PrevName)
    {
    }

    //功能：初始化构建channel需要的IP、next_port、prev_port
    int setup(std::string ip, u32 next_port, u32 prev_port);

    template <Decimal D>
    sf64Matrix<D> createShares(const eMatrix<double> &vals, sf64Matrix<D> &sharedMatrix)
    {
      /*
      功能：进行小数矩阵的本地分享
      参数1：vals为输入矩阵
      参数2：sharedMatrix为分享值矩阵
      返回值：为分享值矩阵
      */
      f64Matrix<D> fixedMatrix(vals.rows(), vals.cols());
      for (u64 i = 0; i < vals.size(); ++i)
        fixedMatrix(i) = vals(i);
      enc.localFixedMatrix(runtime, fixedMatrix, sharedMatrix).get();
      return sharedMatrix;
    }

    template <Decimal D>
    sf64Matrix<D> createShares(sf64Matrix<D> &sharedMatrix)
    {
      /*
      功能：进行小数矩阵的远程分享
      参数1：sharedMatrix为分享值矩阵
      返回值：为分享值矩阵
      */
      enc.remoteFixedMatrix(runtime, sharedMatrix).get();
      return sharedMatrix;
    }

    si64Matrix createShares(const i64Matrix &vals, si64Matrix &sharedMatrix)
    {
      /*
      功能：进行整数矩阵的本地分享
      参数1：vals为输入矩阵
      参数2：sharedMatrix为分享值矩阵
      返回值：为分享值矩阵
      */
      enc.localIntMatrix(runtime, vals, sharedMatrix).get();
      return sharedMatrix;
    }

    si64Matrix createShares(si64Matrix &sharedMatrix)
    {
      /*
      功能：进行整数矩阵的远程分享
      参数1：sharedMatrix为分享值矩阵
      返回值：为分享值矩阵
      */
      enc.remoteIntMatrix(runtime, sharedMatrix).get();
      return sharedMatrix;
    }

    template <Decimal D>
    sf64<D> createShares(double vals, sf64<D> &sharedFixedInt)
    {
      /*
        功能：进行小数的本地分享
        参数1：vals为输入小数
        参数2：sharedFixedInt为分享值
        返回值：为分享值
      */
      enc.localFixed<D>(runtime, vals, sharedFixedInt).get();
      return sharedFixedInt;
    }

    template <Decimal D>
    sf64<D> createShares(sf64<D> &sharedFixedInt)
    {
      /*
        功能：进行小数的远程分享
        参数1：sharedFixedInt为分享值
        返回值：为分享值
      */
      enc.remoteFixed<D>(runtime, sharedFixedInt).get();
      return sharedFixedInt;
    }

    template <Decimal D>
    eMatrix<double> revealAll(const sf64Matrix<D> &vals)
    {
      /*
        功能：揭露小数型计算值矩阵至各方
        参数1：vals为分享值矩阵
        返回值：揭露后矩阵
      */
      f64Matrix<D> temp(vals.rows(), vals.cols());
      enc.revealAll(runtime, vals, temp).get();

      eMatrix<double> ret(vals.rows(), vals.cols());
      for (u64 i = 0; i < ret.size(); ++i)
        ret(i) = static_cast<double>(temp(i));
      return ret;
    }

    i64Matrix revealAll(const si64Matrix &vals)
    {
      /*
        功能：揭露整型计算值矩阵至各方
        参数1：vals为整型分享值矩阵
        返回值：揭露后矩阵
      */
      i64Matrix ret(vals.rows(), vals.cols());
      enc.revealAll(runtime, vals, ret).get();
      return ret;
    }

    template <Decimal D>
    double revealAll(const sf64<D> &vals)
    {
      /*
        功能：揭露小数型计算值至各方
        参数1：vals为小数分享值
        返回值：揭露后小数型计算值
      */
      f64<D> ret;
      enc.revealAll(runtime, vals, ret).get();
      return static_cast<double>(ret);
    }

    template <Decimal D>
    eMatrix<double> reveal(const sf64Matrix<D> &vals)
    {
      /*
        功能：揭露小数型计算值矩阵至己方
        参数1：vals为分享值矩阵
        返回值：揭露后矩阵
      */
      f64Matrix<D> temp(vals.rows(), vals.cols());
      enc.reveal(runtime, vals, temp).get();
      eMatrix<double> ret(vals.rows(), vals.cols());
      for (u64 i = 0; i < ret.size(); ++i)
        ret(i) = static_cast<double>(temp(i));
      return ret;
    }

    template <Decimal D>
    void reveal(const sf64Matrix<D> &vals, u64 Idx)
    {
      /*
        功能：揭露小数型计算值矩阵至目标方
        参数1：vals为分享值矩阵
        参数2：分享到ID所指的目标方
      */
      enc.reveal(runtime, Idx, vals).get();
    }

    i64Matrix reveal(const si64Matrix &vals)
    {
      /*
        功能：揭露整数型计算值矩阵至己方
        参数1：vals为分享值矩阵
        返回值：揭露后矩阵
      */
      i64Matrix ret(vals.rows(), vals.cols());
      enc.reveal(runtime, vals, ret).get();
      return ret;
    }

    void reveal(const si64Matrix &vals, u64 Idx)
    {
      /*
        功能：揭露整数型计算值矩阵至目标方
        参数1：vals为分享值矩阵
        参数2：分享到Idx所指的目标方
      */
      enc.reveal(runtime, Idx, vals).get();
    }

    template <Decimal D>
    double reveal(const sf64<D> &vals, u64 Idx)
    {
      /*
        功能：揭露小数型计算值至目标方
        参数1：vals为计算值的分享
        参数2：分享到Idx所指的目标方
        返回值：揭露后计算值
      */
      if (Idx == partyIdx)
      {
        f64<D> ret;
        enc.reveal(runtime, vals, ret).get();
        return static_cast<double>(ret);
      }
      else
      {
        enc.reveal(runtime, Idx, vals).get();
      }
    }

    template <Decimal D>
    sf64<D> MPC_Add(std::vector<sf64<D>> sharedFixedInt, sf64<D> &sum)
    {
      /*
        功能：计算小数型数值三方和
        参数1：sharedFixedInt为分享值数组
        参数2：sum为加法计算后的分享值
        返回值：加法计算后的分享值
      */
      sum = sharedFixedInt[0];
      for (u64 i = 1; i < sharedFixedInt.size(); i++)
      {
        sum = sum + sharedFixedInt[i];
      }
      return sum;
    }

    template <Decimal D>
    sf64Matrix<D> MPC_Add(std::vector<sf64Matrix<D>> sharedFixedInt, sf64Matrix<D> &sum)
    {
      /*
        功能：计算小数型数值矩阵三方和
        参数1：sharedFixedInt为分享值数组
        参数2：sum为加法计算后的分享值
        返回值：加法计算后的分享值
      */
      sum = sharedFixedInt[0];
      for (u64 i = 1; i < sharedFixedInt.size(); i++)
      {
        sum = sum + sharedFixedInt[i];
      }
      return sum;
    }

    si64Matrix MPC_Add(std::vector<si64Matrix> sharedInt, si64Matrix &sum)
    {
      /*
        功能：计算整数型数值矩阵三方和
        参数1：sharedInt为分享值数组
        参数2：sum为加法计算后的分享值
        返回值：加法计算后的分享值
      */
      sum = sharedInt[0];
      for (u64 i = 1; i < sharedInt.size(); i++)
      {
        sum = sum + sharedInt[i];
      }
      return sum;
    }

    template <Decimal D>
    sf64<D> MPC_Sub(sf64<D> minuend, std::vector<sf64<D>> subtrahends, sf64<D> &difference)
    {
      /*
        功能：计算小数型数值三方差
        参数1：minuend为被减数分享值
        参数2：subtrahends为减数分享值数组
        参数3：difference为差值的分享值
        返回值：差值的分享值
      */
      difference = minuend;
      for (u64 i = 0; i < subtrahends.size(); i++)
      {
        difference = difference - subtrahends[i];
      }
      return difference;
    }

    template <Decimal D>
    sf64Matrix<D> MPC_Sub(sf64Matrix<D> minuend, std::vector<sf64Matrix<D>> subtrahends, sf64Matrix<D> &difference)
    {
      /*
        功能：计算小数型数值矩阵三方差
        参数1：minuend为被减数分享值
        参数2：subtrahends为减数分享值数组
        参数3：difference为差值的分享值
        返回值：差值的分享值
      */
      difference = minuend;
      for (u64 i = 0; i < subtrahends.size(); i++)
      {
        difference = difference - subtrahends[i];
      }
      return difference;
    }

    si64Matrix MPC_Sub(si64Matrix minuend, std::vector<si64Matrix> subtrahends, si64Matrix &difference)
    {
      /*
        功能：计算整数型数值矩阵三方差
        参数1：minuend为被减数分享值
        参数2：subtrahends为减数分享值数组
        参数3：difference为差值的分享值
        返回值：差值的分享值
      */
      difference = minuend;
      for (u64 i = 0; i < subtrahends.size(); i++)
      {
        difference = difference - subtrahends[i];
      }
      return difference;
    }

    template <Decimal D>
    sf64<D> MPC_Mul(std::vector<sf64<D>> sharedFixedInt, sf64<D> &prod)
    {
      /*
        功能：计算小数型数值三方乘积
        参数1：sharedFixedInt为乘数分享值数组
        参数2：prod为乘积的分享值
        返回值：乘积的分享值
      */
      prod = sharedFixedInt[0];
      for (u64 i = 1; i < sharedFixedInt.size(); ++i)
        eval.asyncMul(runtime, prod, sharedFixedInt[i], prod).get();
      return prod;
    }

    template <Decimal D>
    sf64Matrix<D> MPC_Mul(std::vector<sf64Matrix<D>> sharedFixedInt, sf64Matrix<D> &prod)
    {
      /*
        功能：计算小数型数值矩阵三方乘积
        参数1：sharedFixedInt为乘数分享值数组
        参数2：prod为乘积的分享值
        返回值：乘积的分享值
      */
      prod = sharedFixedInt[0];
      for (u64 i = 1; i < sharedFixedInt.size(); ++i)
        eval.asyncMul(runtime, prod, sharedFixedInt[i], prod).get();
      return prod;
    }

    si64Matrix MPC_Mul(std::vector<si64Matrix> sharedInt, si64Matrix &prod)
    {
      /*
        功能：计算整数型数值矩阵三方乘积
        参数1：sharedInt为乘数分享值数组
        参数2：prod为乘积的分享值
        返回值：乘积的分享值
      */
      prod = sharedInt[0];
      for (u64 i = 1; i < sharedInt.size(); ++i)
        eval.asyncMul(runtime, prod, sharedInt[i], prod).get();
      return prod;
    }
  };

#endif // SRC_primihub_operator_ABY3_operator_H
} // namespace primihub

