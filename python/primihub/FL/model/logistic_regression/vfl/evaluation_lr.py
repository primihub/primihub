from sklearn import metrics

class evaluator:
    regression_metrics= ["MEAN SQUARED ERROR","EXPLAINED VARIANCE", "MEAN ABSOLUTE ERROR", "MEAN SQUARED LOG ERROR", "MEDIAN ABSOLUTE ERROR","R2 SCORE","ROOT MEAN SQUARED ERROR"]
    regression_method =   ["getMSE","getEV","getMAE","getMSLE","getMEDIAN_ABSOLUTE_ERROR","getR2_SCORE","getRMSE"]

    classification_metrics = ["ConfusionMatrix","AUC","Precision","Recall","Accuracy","F1_score","KS"]
    classification_method = ["getConfusionMatrix","getAUC","getPrecision","getRecall","getAccuracy","getF1_score","getKS"]
    need_prob = ["AUC","KS"]


    """
    evaluation methods for classification:
        AUC
        Precision
        Recall
        Accuracy
        MSE
    """
    @staticmethod
    def getConfusionMatrix(y,y_hat):
        """
        :param y: real label
        :param y_hat: predicted label
        :return: ConfusionMatrix
                row:real class
                column:predicted class
        """
        return metrics.confusion_matrix(y,y_hat)

    @staticmethod
    def getAUC(y,y_pred_prob):
        """
        :param y: real label
        :param y_pred_prob:  probability of true
        :return: auc
        """
        return metrics.roc_auc_score(y,y_pred_prob)

    @staticmethod
    def getPrecision(y,y_hat):
        """
        :param y: real label
        :param y_hat: predicted label
        :return: Precision
        """
        return metrics.precision_score(y,y_hat)

    @staticmethod
    def getRecall(y,y_hat):
        """
        :param y: real label
        :param y_hat: predicted label
        :return: Recall
        """
        return metrics.recall_score(y,y_hat)

    @staticmethod
    def getAccuracy(y,y_hat):
        """
        :param y: real label
        :param y_hat: predicted label
        :return: Accuracy
        """
        return metrics.accuracy_score(y, y_hat)

    @staticmethod
    def getF1_score(y,y_hat):
        """
        :param y: real label
        :param y_hat: predicted label
        :return: F1-score
        """
        return metrics.f1_score(y, y_hat)

    @staticmethod
    def getKS(y_test, y_pred_prob):
        '''
        calculate value ks, list fpr , list tpr , list thresholds

        y_pred_prob: probability of true
        y_test: real y
        return :fpr, tpr, thresholds, ks.
        ROC curve：abscissa：fpr  ordinate：tpr
        ks curve：abscissa：thresholds ordinate：fpr and tpr two curves
        '''
        fpr, tpr, thresholds = metrics.roc_curve(y_test, y_pred_prob)
        ks = max(tpr - fpr)
        return fpr, tpr, thresholds, ks


    """
    evaluation methods for regression :
        EXPLAINED_VARIANCE
        MEAN_ABSOLUTE_ERROR
        MEAN_SQUARED_LOG_ERROR
        MEDIAN_ABSOLUTE_ERROR
        R2_SCORE
        ROOT_MEAN_SQUARED_ERROR
    """
    @staticmethod
    def getMSE(y, y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm MEAN_SQUARED_ERROR The mean square error is used to
                evaluate the degree of closeness between the predicted
                results and the real data set. The smaller the value,
                the better the fitting effect.
        """
        return metrics.mean_squared_error(y, y_hat)
    @staticmethod
    def getEV(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm EXPLAINED_VARIANCE To explain the variance score of regression model,
                its value range is [0,1]. The closer it is to 1, the more independent
                variable can explain the dependent variable
        """
        return metrics.explained_variance_score(y,y_hat)

    @staticmethod
    def getMAE(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm MEAN_ABSOLUTE_ERROR The mean absolute error is used to evaluate the degree
                of closeness between the predicted results and the real data set. The smaller
                the value, the better the fitting effect.
        """
        return metrics.mean_absolute_error(y,y_hat)

    @staticmethod
    def getMSLE(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm MEAN_SQUARED_LOG_ERROR The mean square logarithmic error is used to evaluate
                the degree of closeness between the predicted results and the real data set.
                The smaller the value, the better the fitting effect.
        """
        return metrics.mean_squared_log_error(y,y_hat)

    @staticmethod
    def getMEDIAN_ABSOLUTE_ERROR(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm MEDIAN_ABSOLUTE_ERROR The median absolute error is used to evaluate the degree
                of approximation between the predicted results and the real data set. The smaller
                the value, the better the fitting effect.
        """
        return metrics.median_absolute_error(y,y_hat)

    @staticmethod
    def getR2_SCORE(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm r2 score The meaning of the determination coefficient is also the variance score to
                explain the regression model, and its value range is [0,1]. The closer it is to 1,
                the more independent variable can explain the variance change of the dependent variable;
                the smaller the value is, the worse the effect is.
        """
        return metrics.r2_score(y,y_hat)

    @staticmethod
    def getRMSE(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm ROOT_MEAN_SQUARED_ERROR The square of MSE is used to evaluate the degree of closeness between
                the predicted results and the real data set. The smaller the value is, the better the fitting
                effect is.
        """
        return pow(evaluator.getMSE(y,y_hat),2)


class regression_eva:
    @staticmethod
    def getResult(y,y_hat):
        """
        :param y: real y
        :param y_hat:predicted y
        :return:regression metricss all supported.
        """
        res = []
        for i in range(len(evaluator.regression_metrics)):
            metrics = evaluator.regression_metrics[i]
            method = evaluator.regression_method[i]
            if hasattr(evaluator, method):
                f = getattr(evaluator, method)
                re = metrics + ":" + str(f(y, y_hat))
            else:
                re = f"{method} is not support。"
            res.append(re)
        return res

class classification_eva:
    @staticmethod
    def getResult(y,y_hat,y_prob):
        """
                :param y: real y
                :param y_hat:predicted y
                :return:classification metricss all supported.
                """
        res = []
        for i in range(len(evaluator.classification_metrics)):
            metrics = evaluator.classification_metrics[i]
            method = evaluator.classification_method[i]
            if hasattr(evaluator, method):
                f = getattr(evaluator, method)
                if (metrics in evaluator.need_prob):
                    mere = str(f(y, y_prob))
                else:
                    mere = str(f(y, y_hat))
                re = metrics + ":" + mere
            else:
                re = f"{method} is not support。"
            res.append(re)
        return(res)
