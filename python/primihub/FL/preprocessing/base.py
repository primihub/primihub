from primihub.utils.logger_util import logger


class PreprocessBase:

    def __init__(self, FL_type=None, role=None, channel=None):
        if isinstance(FL_type, str):
            FL_type = FL_type.upper()

        if FL_type in ['V', 'H']:
            self.FL_type = FL_type
        else:
            error_msg = f"Unsupported FL type: {FL_type}"
            logger.error(error_msg)
            raise ValueError(error_msg)
        
        if isinstance(role, str):
            role = role.lower()

        VFL_roles = ['host', 'guest']
        HFL_roles = ['client', 'server']
        
        if (FL_type == 'V' and role in VFL_roles) or \
            (FL_type == 'H' and role in HFL_roles):
            self.role = role
        else:
            error_msg = f"Unsupported role: {role}. For {FL_type}FL"
            if FL_type == 'V':
                error_msg += f', use {VFL_roles} instead.'
            else:
                error_msg += f', use {HFL_roles} instead.'
            logger.error(error_msg)
            raise ValueError(error_msg)
        
        self.channel = channel
    
    def check_channel(self):
        if self.channel is None:
            error_msg = (f"For {self.__class__.__name__},"
                         f"channel cannot be None in {self.FL_type}FL")
            logger.error(error_msg)
            raise ValueError(error_msg)

    def Vfit(self, X):
        return self.module.fit(X)

    def Hfit(self, X):
        return self.module.fit(X)

    def fit(self, X=None):
        if self.FL_type == 'V':
            return self.Vfit(X)
        else:
            return self.Hfit(X)

    def transform(self, X):
        return self.module.transform(X)
    
    def fit_transform(self, X):
        return self.fit(X).transform(X)
        