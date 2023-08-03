from primihub.utils.logger_util import logger


class PreprocessBase:

    def __init__(self, FL_type=None, role=None, channel=None):
        if FL_type in ['V', 'H']:
            self.FL_type = FL_type
        else:
            error_msg = f"Unsupported FL type: {FL_type}"
            logger.error(error_msg)
            raise ValueError(error_msg)
        
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
        
        if FL_type == 'H':
            if channel is None:
                error_msg = f"For HFL, channel cannot be none"
                logger.error(error_msg)
                raise ValueError(error_msg)
            else:
                self.channel = channel

    def Vfit(self, x):
        return self.module.fit(x)

    def Hfit(self, x):
        pass

    def fit(self, x=None):
        if self.FL_type == 'V':
            return self.Vfit(x)
        else:
            return self.Hfit(x)

    def transform(self, x):
        return self.module.transform(x)
    
    def fit_transform(self, x):
        self.fit(x)
        return self.transform(x)
        