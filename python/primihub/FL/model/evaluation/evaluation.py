from sklearn import metrics
import json
class Evaluator:
    regression_metrics= ["MEAN_SQUARED_ERROR","EXPLAINED_VARIANCE", "MEAN_ABSOLUTE_ERROR", "MEAN_SQUARED_LOG_ERROR", "MEDIAN_ABSOLUTE_ERROR","R2_SCORE","ROOT_MEAN_SQUARED_ERROR"]
    regression_method =   ["get_mse","get_ev","get_mae","get_msle","get_median_absolute_error","get_r2_score","get_rmse"]

    classification_metrics = ["AUC","Precision","Recall","Accuracy","F1_score","KS"]
    classification_method = ["get_auc","get_precision","get_recall","get_accuracy","get_f1_score","get_ks"]
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
    def get_confusionMatrix(y,y_hat):
        """
        :param y: real label
        :param y_hat: predicted label
        :return: ConfusionMatrix
                row:real class
                column:predicted class
        """
        return metrics.confusion_matrix(y,y_hat)

    @staticmethod
    def get_auc(y,y_pred_prob):
        """
        :param y: real label
        :param y_pred_prob:  probability of true
        :return: auc
        """
        return metrics.roc_auc_score(y,y_pred_prob)

    @staticmethod
    def get_precision(y,y_hat):
        """
        :param y: real label
        :param y_hat: predicted label
        :return: Precision
        """
        return metrics.precision_score(y,y_hat)

    @staticmethod
    def get_recall(y,y_hat):
        """
        :param y: real label
        :param y_hat: predicted label
        :return: Recall
        """
        return metrics.recall_score(y,y_hat)

    @staticmethod
    def get_accuracy(y,y_hat):
        """
        :param y: real label
        :param y_hat: predicted label
        :return: Accuracy
        """
        return metrics.accuracy_score(y, y_hat)

    @staticmethod
    def get_f1_score(y,y_hat):
        """
        :param y: real label
        :param y_hat: predicted label
        :return: F1-score
        """
        return metrics.f1_score(y, y_hat)

    @staticmethod
    def get_roc(y_test,y_pred_prob):
        roc = {}
        fpr, tpr, thresholds = metrics.roc_curve(y_test, y_pred_prob)
        roc["fpr"] = fpr
        roc["tpr"] = tpr
        roc["thresholds"] = thresholds
        return roc


    @staticmethod
    def get_ks(y_test, y_pred_prob):
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
        return ks


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
    def get_mse(y, y_hat):
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
    def get_ev(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm EXPLAINED_VARIANCE To explain the variance score of regression model,
                its value range is [0,1]. The closer it is to 1, the more independent
                variable can explain the dependent variable
        """
        return metrics.explained_variance_score(y,y_hat)

    @staticmethod
    def get_mae(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm MEAN_ABSOLUTE_ERROR The mean absolute error is used to evaluate the degree
                of closeness between the predicted results and the real data set. The smaller
                the value, the better the fitting effect.
        """
        return metrics.mean_absolute_error(y,y_hat)

    @staticmethod
    def get_msle(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm MEAN_SQUARED_LOG_ERROR The mean square logarithmic error is used to evaluate
                the degree of closeness between the predicted results and the real data set.
                The smaller the value, the better the fitting effect.
        """
        return metrics.mean_squared_log_error(y,y_hat)

    @staticmethod
    def get_median_absolute_error(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm MEDIAN_ABSOLUTE_ERROR The median absolute error is used to evaluate the degree
                of approximation between the predicted results and the real data set. The smaller
                the value, the better the fitting effect.
        """
        return metrics.median_absolute_error(y,y_hat)

    @staticmethod
    def get_r2_score(y,y_hat):
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
    def get_rmse(y,y_hat):
        """
        :param y: real y
        :param y_hat: predicted t
        :returm ROOT_MEAN_SQUARED_ERROR The square of MSE is used to evaluate the degree of closeness between
                the predicted results and the real data set. The smaller the value is, the better the fitting
                effect is.
        """
        return pow(Evaluator.get_mse(y,y_hat),2)

    @staticmethod
    def write_json(path,eval_result):
        with open(path,'w') as f:
            json.dump(eval_result,f)



class Regression_eva:
    @staticmethod
    def get_result(y,y_hat,path = "evaluation.json"):
        """
        :param y: real y
        :param y_hat:predicted y
        :return:regression metricss all supported.
        """
        me = ["train","test"]
        res = {"train":{},"test":{}}
        for k in me:
            y_true = y[k]
            y_pre = y_hat[k]
            for i in range(len(Evaluator.regression_metrics)):
                metrics = Evaluator.regression_metrics[i]
                method = Evaluator.regression_method[i]
                if hasattr(Evaluator, method):
                    f = getattr(Evaluator, method)
                    res[k][metrics] = f(y_true, y_pre)
                else:
                    res[k][metrics] = f"{method} is not support。"

        Evaluator.write_json(path,res)
        return res

class Classification_eva:
    @staticmethod
    def get_result(y_true,y_prob,path = "evaluation.json",eval_test = None):
        """
                :param y: real y
                :param y_hat:predicted y
                :return:classification metricss all supported.
                """
        lable = [0,1]
        lable_true = list(set(y_true))
        lable_true.sort()
        y = []
        if lable_true != lable:
            for i in y_true:
                if i == lable_true[0]:
                    y.append(lable[0])
                else:
                    y.append(lable[1])
        else:
            y = y_true

        roc = Evaluator.get_roc(y,y_prob)
        thresholds = roc["thresholds"]
        fpr = roc["fpr"]
        tpr = roc["tpr"]
        max_tf = 0
        threshold = 0
        for i in range(len(tpr)):
            this_tf = tpr[i]-fpr[i]
            if this_tf>max_tf:
                max_tf = this_tf
                threshold = thresholds[i]
        lable = list(set(y))
        y_hat = []
        for i in y_prob:
            if i<=threshold:
                y_hat.append(lable[0])
            else:
                y_hat.append(lable[1])
        res = {}
        print(y)
        print(y_hat)
        suffix = Evaluator.get_confusionMatrix(y,y_hat)
        print(suffix)
        for i in range(len(Evaluator.classification_metrics)):
            metrics = Evaluator.classification_metrics[i]
            method = Evaluator.classification_method[i]
            if hasattr(Evaluator, method):
                f = getattr(Evaluator, method)
                if (metrics in Evaluator.need_prob):
                    mere = f(y, y_prob)
                else:
                    mere = f(y, y_hat)
                res[metrics] = mere
            else:
                res[metrics] = f"{method} is not support。"
        Evaluator.write_json(path,res)
        return res


# if __name__ == "__main__":
#     a = {"train":[1, 0, 1, 1, 1],"test":[1,1,2]}
#     c = {"train":[0.8, 0.3, 0.2, 0.9, 0.7],"test":[0.9,0.7,0.8]}
#     print(Regression_eva.get_result(a,c))