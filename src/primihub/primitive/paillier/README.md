# PAILLIER-CPP 库

Paillier半同态密码算法，适用于外包计算环境下，加密方用自己的公钥加密数据时使用。其中，加密方会在本地传入自己的公私钥对，完成外包前的信息保密任务。

## 编译
* Linux/OSX x86_64
  
  ```bash
  bazel build //primitive/paillier:lib_paillier
  ```

* OSX arm64
  
  ```bash
  bazel build --cpu=darwin_x86_64 //primitive/paillier:lib_paillier
  ```

## 测试


```bash
  bazel build --cpu=darwin_x86_64 //tests:paillier_test
  ./bazel-bin/tests/paillier_test
```

## 使用
0. 本项目基于C++17与MIRACL第三方库，MIRACL库的GitHub地址是https://github.com/miracl/MIRACL
1. 如果报错说fadd, fsub, fmul, fdiv 未定义，请到miracl/mr_include/miracl.h中把1348-1351行的被注释的代码取消注释
2. 密钥生成阶段（Paillier实例化阶段）时间较长，请等待
3. 本项目参考论文https://dl.acm.org/doi/10.1145/3485832.3485842
4. fixed-base优化，我们的窗口大小为16，因此table大小为16*2^17 B，略小于16MB=16*2^20。如果需要测试其他大小的table，可以去Paillier::encryptionCrtPrepare()函数下修改brick_init。

## 开发
目前，本项目加解密均使用CRT优化，这种优化是需要私钥的参与的。因此适应于外包计算的开发需求。

TODO:
后续继续开发明文打包、非私钥参与的密码协议。

### 代码结构

```
  paillier
    |-src
        |-numTools.h # 定义基本数论算法的工具类，基于MIRACl的基本语句封装
        |-paillier.h # 定义了在外包计算环境下适用的Paillier密码体制和公私钥信息
        |-numTools.cpp # 对numTools.h定义的类的实现
        |-paillier.cpp # 对paillier.cpp定义的类的实现
    |-test
        |-Main.cpp # 对Paillier密码系统的测试和性能分析，考虑到人工智能的应用场景，测试使用的数据均为无符号64位整型
    |-READEME.txt
  third_party/miracl
    |-miracl
        |-mr_include # 包含MIRACL第三方库需要的两个核心头文件：miracl.h和mirdef.h（miracl.h的fadd,fsub,fmul,fdiv已经被注释）
        |-mr_src  # 包含linux64需要的全部源代码
  ```

## 许可证
AGPL3




## 已知问题
- [ ] miracl 只能在X86_64 CPU 运行
- [ ] windows 还未支持
