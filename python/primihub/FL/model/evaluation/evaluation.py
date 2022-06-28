from sklearn import metrics

#混淆矩阵 metrics.confusion_matrix（实际y，预测y）
#输出：行：实际类别   列：预测类别

#AUC、Precision、Recall、Accuracy、MSE
class evaluator:
    regression_indicator= ["MEAN SQUARED ERROR","EXPLAINED VARIANCE", "MEAN ABSOLUTE ERROR", "MEAN SQUARED LOG ERROR", "MEDIAN ABSOLUTE ERROR","R2 SCORE","ROOT MEAN SQUARED ERROR"]
    regression_method =   ["getMSE","getEV","getMAE","getMSLE","getMEDIAN_ABSOLUTE_ERROR","getR2_SCORE","getRMSE"]

    classification_indicator = ["ConfusionMatrix","AUC","Precision","Recall","Accuracy","F1_score","KS"]
    classification_method = ["getConfusionMatrix","getAUC","getPrecision","getRecall","getAccuracy","getF1_score","getKS"]
    need_prob = ["AUC","KS"]
    #分类模型指标
    @staticmethod
    def getConfusionMatrix(y,y_hat):
        """
        :param y: 真实标签
        :param y_hat: 预测标签
        :return: 混淆矩阵
        """
        return metrics.confusion_matrix(y,y_hat)

    @staticmethod
    def getAUC(y,y_pred_prob):
        """
        :param y: 真实标签
        :param y_pred_prob: 预测为真的概率
        :return: 模型的auc指标
        """
        return metrics.roc_auc_score(y,y_pred_prob)

    @staticmethod
    def getPrecision(y,y_hat):
        """
        :param y: 真实标签
        :param y_hat: 预测标签
        :return: Precision精确率
        """
        return metrics.precision_score(y,y_hat)

    @staticmethod
    def getRecall(y,y_hat):
        """
        :param y: 真实标签
        :param y_hat: 预测标签
        :return: Recall召回率
        """
        return metrics.recall_score(y,y_hat)

    @staticmethod
    def getAccuracy(y,y_hat):
        """
        :param y: 真实标签
        :param y_hat: 预测标签
        :return: Accuracy准确率
        """
        return metrics.accuracy_score(y, y_hat)

    @staticmethod
    def getF1_score(y,y_hat):
        """
        :param y: 真实标签
        :param y_hat: 预测标签
        :return: F1-score
        """
        return metrics.f1_score(y, y_hat)

    @staticmethod
    def getKS(y_test, y_pred_prob):
        '''
        功能: 计算KS值，输出对应分割点和累计分布函数曲线图
        输入值:
        y_pred_prob: 一维数组或series，代表模型得分（一般为预测正类的概率）
        y_test: 真实值，一维数组或series，代表真实的标签（{0,1}或{-1,1}）
        返回fpr, tpr, thresholds, 和ks值，根据fpr,tpr和thresholds这三个list可以绘制出roc曲线和ks曲线。
        ROC曲线：横坐标：fpr  纵坐标：tpr
        ks曲线：横坐标：thresholds 纵坐标：fpr和tpr 两条曲线
        '''
        fpr, tpr, thresholds = metrics.roc_curve(y_test, y_pred_prob)
        ks = max(tpr - fpr)
        return fpr, tpr, thresholds, ks

    """
    EXPLAINED_VARIANCE
    MEAN_ABSOLUTE_ERROR
    MEAN_SQUARED_LOG_ERROR
    MEDIAN_ABSOLUTE_ERROR
    R2_SCORE
    ROOT_MEAN_SQUARED_ERROR
    """

    @staticmethod
    #回归模型指标
    def getMSE(y, y_hat):
        """
        :param y: 真实值
        :param y_hat: 预测值
        :returm MEAN_SQUARED_ERROR均方误差,用于评估预测结果和真实数据集的接近程度的程度，其其值越小说明拟合效果越好。
        """
        return metrics.mean_squared_error(y, y_hat)
    @staticmethod
    def getEV(y,y_hat):
        """
        :param y: 真实值
        :param y_hat: 预测值
        :returm EXPLAINED_VARIANCE 解释回归模型的方差得分，其值取值范围是[0,1]，越接近于1说明自变量越能解释因变量
          的方差变化，值越小则说明效果越差。
        """
        return metrics.explained_variance_score(y,y_hat)

    @staticmethod
    def getMAE(y,y_hat):
        """
        :param y: 真实值
        :param y_hat: 预测值
        :returm MEAN_ABSOLUTE_ERROR 平均绝对误差，用于评估预测结果和真实数据集的接近程度的程度，其其值越小说明拟合效果越好。
        """
        return metrics.mean_absolute_error(y,y_hat)

    @staticmethod
    def getMSLE(y,y_hat):
        """
        :param y: 真实值
        :param y_hat: 预测值
        :returm MEAN_SQUARED_LOG_ERROR 均方对数误差，用于评估预测结果和真实数据集的接近程度的程度，其其值越小说明拟合效果越好。
        """
        return metrics.mean_squared_log_error(y,y_hat)

    @staticmethod
    def getMEDIAN_ABSOLUTE_ERROR(y,y_hat):
        """
        :param y: 真实值
        :param y_hat: 预测值
        :returm MEDIAN_ABSOLUTE_ERROR 绝对误差的中位数，用于评估预测结果和真实数据集的接近程度的程度，其其值越小说明拟合效果越好。
        """
        return metrics.median_absolute_error(y,y_hat)

    @staticmethod
    def getR2_SCORE(y,y_hat):
        """
        :param y: 真实值
        :param y_hat: 预测值
        :returm r2 score 判定系数，其含义是也是解释回归模型的方差得分，其值取值范围是[0,1]，越接近于1说明自变量越能解释因
         变量的方差变化，值越小则说明效果越差。
        """
        return metrics.r2_score(y,y_hat)

    @staticmethod
    def getRMSE(y,y_hat):
        """
        :param y: 真实值
        :param y_hat: 预测值
        :returm ROOT_MEAN_SQUARED_ERROR MSE的平方，用于评估预测结果和真实数据集的接近程度的程度，其其值越小说明拟合效果越好。
        """
        return pow(evaluator.getMSE(y,y_hat),2)


class regression_eva:
    @staticmethod
    def getResult(y,y_hat):
        res = []
        for i in range(len(evaluator.regression_indicator)):
            indicator = evaluator.regression_indicator[i]
            method = evaluator.regression_method[i]
            if hasattr(evaluator, method):
                f = getattr(evaluator, method)
                re = indicator + ":" + str(f(y, y_hat))
            else:
                re = f"目前没有{method}指标。"
            res.append(re)
        return res

class classification_eva:
    @staticmethod
    def getResult(y,y_hat,y_prob):
        res = []
        for i in range(len(evaluator.classification_indicator)):
            indicator = evaluator.classification_indicator[i]
            method = evaluator.classification_method[i]
            if hasattr(evaluator, method):
                f = getattr(evaluator, method)
                if (indicator in evaluator.need_prob):
                    mere = str(f(y, y_prob))
                else:
                    mere = str(f(y, y_hat))
                re = indicator + ":" + mere
            else:
                re = f"目前没有{method}指标。"
            res.append(re)
        return(res)