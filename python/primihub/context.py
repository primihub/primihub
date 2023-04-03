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
    def __init__(self) -> None:
        '''
        Set the message from protobuf
        '''
        self.message = None

Context = ContextAll()

def set_message(message):
    Context.message = message








