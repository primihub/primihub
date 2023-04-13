class FL_helper:
    def __init__(self,task_parameter,party_access_info):
        self.task_parameter = task_parameter
        self.party_access_info = party_access_info
    
    def get_roles(self):
        return self.task_parameter['all_roles'].keys()
    
    def get_parties(self):
        res = []
        for k,v in self.task_parameter['all_roles'].items():
            res.extend(v)
        return res
    
    def party2role(self, party):
        for k,v in self.task_parameter['all_roles'].items():
            for p in v:
                if v == party:
                    return k
        
    def role2party(self,role):
        return self.task_parameter['all_roles'][role]