'''
Generally, to run a task there are several important parts for a task.
1. The task configure (format fix): for the task, i.e. the task type, ID, also the function
2. The node configure (format fix): mainly for the communcation, the node configuraion is used
    communcation with other nodes and check the node info.
3. The task parametes (format is defined by user): The parameter fo the special algorithm.
Note, the dataset is also included in this parameter.
'''

class ContextAll:
    '''
    All the parameter is included in this context.
    '''
    def __init__(self, func, params, role) -> None:
        self.func = func
        self.params = params
        self.role = role

Context = ContextAll()

def set_task_param(params):
    Context.params = params

def set_task_code(func):
    Context.func = func

def set_role(role):
    Context.role = role







