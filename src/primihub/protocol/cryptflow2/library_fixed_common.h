/*
Authors: Nishant Kumar
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

/************************ Standard Conv **************************/

static void Conv2DReshapeFilter(int32_t FH, int32_t FW, int32_t CI, int32_t CO,
                                uint64_t *inputArr, uint64_t *outputArr) {
  for (uint32_t co = (int32_t)0; co < (uint32_t)CO; co++) {
    for (uint32_t fh = (int32_t)0; fh < (uint32_t)FH; fh++) {
      for (uint32_t fw = (int32_t)0; fw < (uint32_t)FW; fw++) {
        for (uint32_t ci = (int32_t)0; ci < (uint32_t)CI; ci++) {

          int32_t linIdx = ((((fh * FW) * CI) + (fw * CI)) + ci);
          Arr2DIdxRowM(outputArr, CO, ((FH * FW) * CI), co, linIdx) =
              Arr4DIdxRowM(inputArr, FH, FW, CI, CO, fh, fw, ci, co);
        }
      }
    }
  }
}

static void Conv2DReshapeMatMulOP(int32_t N, int32_t finalH, int32_t finalW,
                                  int32_t CO, uint64_t *inputArr,
                                  uint64_t *outputArr) {
  for (uint32_t co = (int32_t)0; co < (uint32_t)CO; co++) {
    for (uint32_t n = (int32_t)0; n < (uint32_t)N; n++) {
      for (uint32_t h = (int32_t)0; h < (uint32_t)finalH; h++) {
        for (uint32_t w = (int32_t)0; w < (uint32_t)finalW; w++) {
          Arr4DIdxRowM(outputArr, N, finalH, finalW, CO, n, h, w, co) =
              Arr2DIdxRowM(inputArr, CO, ((N * finalH) * finalW), co,
                           ((((n * finalH) * finalW) + (h * finalW)) + w));
        }
      }
    }
  }
}

static void Conv2DReshapeInput(int32_t N, int32_t H, int32_t W, int32_t CI,
                               int32_t FH, int32_t FW, int32_t zPadHLeft,
                               int32_t zPadHRight, int32_t zPadWLeft,
                               int32_t zPadWRight, int32_t strideH,
                               int32_t strideW, int32_t RRows, int32_t RCols,
                               uint64_t *inputArr, uint64_t *outputArr) {

  int32_t linIdxFilterMult = (int32_t)0;
  for (uint32_t n = (int32_t)0; n < (uint32_t)N; n++) {

    int32_t leftTopCornerH = ((int32_t)0 - zPadHLeft);

    int32_t extremeRightBottomCornerH = ((H - (int32_t)1) + zPadHRight);
    while (
        (((leftTopCornerH + FH) - (int32_t)1) <= extremeRightBottomCornerH)) {

      int32_t leftTopCornerW = ((int32_t)0 - zPadWLeft);

      int32_t extremeRightBottomCornerW = ((W - (int32_t)1) + zPadWRight);
      while (
          (((leftTopCornerW + FW) - (int32_t)1) <= extremeRightBottomCornerW)) {
        for (uint32_t fh = (int32_t)0; fh < (uint32_t)FH; fh++) {
          for (uint32_t fw = (int32_t)0; fw < (uint32_t)FW; fw++) {

            int32_t curPosH = (leftTopCornerH + fh);

            int32_t curPosW = (leftTopCornerW + fw);

            uint64_t val = funcSSCons((int64_t)0);
            for (uint32_t ci = (int32_t)0; ci < (uint32_t)CI; ci++) {
              if ((((curPosH < (int32_t)0) || (curPosH >= H)) ||
                   ((curPosW < (int32_t)0) || (curPosW >= W)))) {
                val = funcSSCons((int64_t)0);
              } else {
                val = Arr4DIdxRowM(inputArr, N, H, W, CI, n, curPosH, curPosW,
                                   ci);
              }
              Arr2DIdxRowM(outputArr, RRows, RCols,
                           ((((fh * FW) * CI) + (fw * CI)) + ci),
                           linIdxFilterMult) = val;
            }
          }
        }
        linIdxFilterMult = (linIdxFilterMult + (int32_t)1);
        leftTopCornerW = (leftTopCornerW + strideW);
      }

      leftTopCornerH = (leftTopCornerH + strideH);
    }
  }
}


/************************ Transpose Conv **************************/


static void ConvTranspose2DReshapeFilter(int64_t FH, int64_t FW, int64_t CO,
                                     int64_t CI, uint64_t *inputArr,
                                     uint64_t *outputArr){
  for (uint64_t co = (int32_t)0; co < CO; co++) {
    for (uint64_t fh = (int32_t)0; fh < FH; fh++) {
      for (uint64_t fw = (int32_t)0; fw < FW; fw++) {
        for (uint64_t ci = (int32_t)0; ci < CI; ci++) {

          uint64_t linIdx =((((fh * FW) * CI) + (fw * CI)) + ci);
           Arr2DIdxRowM(outputArr, CO, ((FH * FW) * CI), co, linIdx) =
              Arr4DIdxRowM(inputArr, FH, FW, CO, CI, ((FH-(int32_t)1)-fh), ((FW-(int32_t)1)-fw), co, ci);
        }
      }
    }
  }

 }

static void ConvTranspose2DReshapeMatMulOP(int32_t N, int32_t finalH, int32_t finalW,
                                  int32_t CO, uint64_t *inputArr,
                                  uint64_t *outputArr) {
  for (uint32_t co = (int32_t)0; co < (uint32_t)CO; co++) {
    for (uint32_t n = (int32_t)0; n < (uint32_t)N; n++) {
      for (uint32_t h = (int32_t)0; h < (uint32_t)finalH; h++) {
        for (uint32_t w = (int32_t)0; w < (uint32_t)finalW; w++) {
          Arr4DIdxRowM(outputArr, N, finalH, finalW, CO, n, h, w, co) =
              Arr2DIdxRowM(inputArr, CO, ((N * finalH) * finalW), co,
                           ((((n * finalH) * finalW) + (h * finalW)) + w));
        }
      }
    }
  }
}

static void ConvTranspose2DReshapeInput(int64_t N, int64_t HPrime,
                                    int64_t WPrime, int64_t CI, int64_t FH,
                                    int64_t FW, int64_t zPadTrHLeft,
                                    int64_t zPadTrHRight, int64_t zPadTrWLeft,
                                    int64_t zPadTrWRight, int64_t strideH,
                                    int64_t strideW, int64_t RRows,
                                    int64_t RCols, uint64_t *inputArr,
                                    uint64_t *outputArr) {

  int32_t linIdxFilterMult = (int32_t)0;
  for (uint32_t n = (int32_t)0; n < (uint32_t)N; n++) {

    int32_t leftTopCornerH = ((int32_t)0 - zPadTrHLeft);

    int32_t HPrimeTilde = (HPrime + ((HPrime-(int32_t)1) * (strideH - (int32_t)1)));

    int32_t extremeRightBottomCornerH = ((HPrimeTilde - (int32_t)1) + zPadTrHRight);
    while (
        (((leftTopCornerH + FH) - (int32_t)1) <= extremeRightBottomCornerH)) {

      int32_t leftTopCornerW = ((int32_t)0 - zPadTrWLeft);

      int32_t WPrimeTilde = (WPrime + ((WPrime-(int32_t)1) * (strideW - (int32_t)1)));

      int32_t extremeRightBottomCornerW = ((WPrimeTilde - (int32_t)1) + zPadTrWRight);
      while (
          (((leftTopCornerW + FW) - (int32_t)1) <= extremeRightBottomCornerW)) {
        for (uint32_t fh = (int32_t)0; fh < (uint32_t)FH; fh++) {
          for (uint32_t fw = (int32_t)0; fw < (uint32_t)FW; fw++) {

            int32_t curPosH = (leftTopCornerH + fh);

            int32_t curPosW = (leftTopCornerW + fw);

            uint64_t val = funcSSCons((int64_t)0);
            for (uint32_t ci = (int32_t)0; ci < (uint32_t)CI; ci++) {
              if ((((curPosH < (int32_t)0) || (curPosH >= HPrimeTilde)) ||
                   ((curPosW < (int32_t)0) || (curPosW >= WPrimeTilde)))) {
                val = funcSSCons((int64_t)0);
              } else {
                if(((curPosH % strideH) == (int32_t)0) && ((curPosW % strideW) == (int32_t)0)){
                  uint64_t idxInputH = (curPosH / strideH);

                  uint64_t idxInputW = (curPosW / strideW);
                  val = Arr4DIdxRowM(inputArr, N, HPrime, WPrime, CI, n, idxInputH, idxInputW,ci);
                }
                else{
                  val = (int64_t)0;
                }
              
              }
              Arr2DIdxRowM(outputArr, RRows, RCols,
                           ((((fh * FW) * CI) + (fw * CI)) + ci),
                           linIdxFilterMult) = val;
            }
          }
        }
        linIdxFilterMult = (linIdxFilterMult + (int32_t)1);
        leftTopCornerW = (leftTopCornerW + (int32_t)1);
      }

      leftTopCornerH = (leftTopCornerH + (int32_t)1);
    }
  }
}

/************************ Grouped Conv **************************/

static void Conv2DReshapeFilterGroup(int32_t FH, int32_t FW, int32_t CI,
                                     int32_t CO, int32_t g, int32_t G,
                                     uint64_t *inputArr, uint64_t *outputArr) {

  int32_t CIG = (CI / G);

  int32_t COG = (CO / G);

  int32_t startCO = (g * COG);
  for (uint32_t co = (int32_t)0; co < (uint32_t)COG; co++) {
    for (uint32_t fh = (int32_t)0; fh < (uint32_t)FH; fh++) {
      for (uint32_t fw = (int32_t)0; fw < (uint32_t)FW; fw++) {
        for (uint32_t ci = (int32_t)0; ci < (uint32_t)CIG; ci++) {

          int32_t linIdx = ((((fh * FW) * CIG) + (fw * CIG)) + ci);
          Arr2DIdxRowM(outputArr, (CO / G), ((FH * FW) * (CI / G)), co,
                       linIdx) = Arr4DIdxRowM(inputArr, FH, FW, (CI / G), CO,
                                              fh, fw, ci, (co + startCO));
        }
      }
    }
  }
}

static void Conv2DReshapeMatMulOPGroup(int32_t N, int32_t finalH,
                                       int32_t finalW, int32_t CO, int32_t g,
                                       int32_t G, uint64_t *inputArr,
                                       uint64_t *outputArr) {

  int32_t COG = (CO / G);

  int32_t startCO = (g * COG);
  for (uint32_t co = (int32_t)0; co < (uint32_t)COG; co++) {
    for (uint32_t n = (int32_t)0; n < (uint32_t)N; n++) {
      for (uint32_t h = (int32_t)0; h < (uint32_t)finalH; h++) {
        for (uint32_t w = (int32_t)0; w < (uint32_t)finalW; w++) {
          Arr4DIdxRowM(outputArr, N, finalH, finalW, CO, n, h, w,
                       (co + startCO)) =
              Arr2DIdxRowM(inputArr, (CO / G), ((N * finalH) * finalW), co,
                           ((((n * finalH) * finalW) + (h * finalW)) + w));
        }
      }
    }
  }
}

static void Conv2DReshapeInputGroup(int32_t N, int32_t H, int32_t W, int32_t CI,
                                    int32_t FH, int32_t FW, int32_t zPadHLeft,
                                    int32_t zPadHRight, int32_t zPadWLeft,
                                    int32_t zPadWRight, int32_t strideH,
                                    int32_t strideW, int32_t g, int32_t G,
                                    int32_t RRows, int32_t RCols,
                                    uint64_t *inputArr, uint64_t *outputArr) {

  int32_t linIdxFilterMult = (int32_t)0;

  int32_t CIG = (CI / G);
  for (uint32_t n = (int32_t)0; n < (uint32_t)N; n++) {

    int32_t leftTopCornerH = ((int32_t)0 - zPadHLeft);

    int32_t extremeRightBottomCornerH = ((H - (int32_t)1) + zPadHRight);
    while (
        (((leftTopCornerH + FH) - (int32_t)1) <= extremeRightBottomCornerH)) {

      int32_t leftTopCornerW = ((int32_t)0 - zPadWLeft);

      int32_t extremeRightBottomCornerW = ((W - (int32_t)1) + zPadWRight);
      while (
          (((leftTopCornerW + FW) - (int32_t)1) <= extremeRightBottomCornerW)) {
        for (uint32_t fh = (int32_t)0; fh < (uint32_t)FH; fh++) {
          for (uint32_t fw = (int32_t)0; fw < (uint32_t)FW; fw++) {

            int32_t curPosH = (leftTopCornerH + fh);

            int32_t curPosW = (leftTopCornerW + fw);

            uint64_t val = funcSSCons((int64_t)0);

            int32_t startCI = (g * CIG);
            for (uint32_t ci = (int32_t)0; ci < (uint32_t)CIG; ci++) {
              if ((((curPosH < (int32_t)0) || (curPosH >= H)) ||
                   ((curPosW < (int32_t)0) || (curPosW >= W)))) {
                val = funcSSCons((int64_t)0);
              } else {
                val = Arr4DIdxRowM(inputArr, N, H, W, CI, n, curPosH, curPosW,
                                   (ci + startCI));
              }
              Arr2DIdxRowM(outputArr, RRows, RCols,
                           ((((fh * FW) * CIG) + (fw * CIG)) + ci),
                           linIdxFilterMult) = val;
            }
          }
        }
        linIdxFilterMult = (linIdxFilterMult + (int32_t)1);
        leftTopCornerW = (leftTopCornerW + strideW);
      }

      leftTopCornerH = (leftTopCornerH + strideH);
    }
  }
}

static void Conv2DGroup(int32_t N, int32_t H, int32_t W, int32_t CI, int32_t FH,
                        int32_t FW, int32_t CO, int32_t zPadHLeft,
                        int32_t zPadHRight, int32_t zPadWLeft,
                        int32_t zPadWRight, int32_t strideH, int32_t strideW,
                        int32_t G, uint64_t *inputArr, uint64_t *filterArr,
                        uint64_t *outArr) {

  int32_t CIG = (CI / G);

  int32_t reshapedFilterRows = (CO / G);

  int32_t reshapedFilterCols = ((FH * FW) * CIG);

  int32_t reshapedIPRows = ((FH * FW) * CIG);

  int32_t outH =
      ((((H + (zPadHLeft + zPadHRight)) - FH) / strideH) + (int32_t)1);

  int32_t outW =
      ((((W + (zPadWLeft + zPadWRight)) - FW) / strideW) + (int32_t)1);

  int32_t reshapedIPCols = ((N * outH) * outW);
  for (uint32_t g = (int32_t)0; g < (uint32_t)G; g++) {

    uint64_t *inputReshaped =
        make_array<uint64_t>(reshapedIPRows, reshapedIPCols);

    uint64_t *matmulOP =
        make_array<uint64_t>(reshapedFilterRows, reshapedIPCols);

    uint64_t *filterReshaped =
        make_array<uint64_t>(reshapedFilterRows, reshapedFilterCols);
    Conv2DReshapeFilterGroup(FH, FW, CI, CO, g, G, filterArr, filterReshaped);
    Conv2DReshapeInputGroup(N, H, W, CI, FH, FW, zPadHLeft, zPadHRight,
                            zPadWLeft, zPadWRight, strideH, strideW, g, G,
                            reshapedIPRows, reshapedIPCols, inputArr,
                            inputReshaped);
    MatMul2D(reshapedFilterRows, reshapedFilterCols, reshapedIPCols,
             filterReshaped, inputReshaped, matmulOP, 1);
    Conv2DReshapeMatMulOPGroup(N, outH, outW, CO, g, G, matmulOP, outArr);
    ClearMemSecret2(reshapedFilterRows, reshapedFilterCols, filterReshaped);
    ClearMemSecret2(reshapedIPRows, reshapedIPCols, inputReshaped);
    ClearMemSecret2(reshapedFilterRows, reshapedIPCols, matmulOP);
  }
}
