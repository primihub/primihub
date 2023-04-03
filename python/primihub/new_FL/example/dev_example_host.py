import data_helper as dh
class Dev_example_host:
    def run(self, context):
        process = context.task_parameter['process']
        X = context.task_parameter['data']['guest']['X']
        X = dh.read(X)
        if process == 'train':
            y = context.task_parameter['data']['guest']['y']
            y = dh.read(y)
            self.train(X,y)
        if process == 'predict':
            self.predict(X)
    

    def train(self, X, y):
        pass
    
    def predict(self, X):
        pass