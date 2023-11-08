class _PreprocessBase:
    def __init__(self, FL_type: str, role: str, channel=None):
        # check FL type
        FL_type = FL_type.upper()
        valid_FL_type = {"V", "H"}

        if FL_type in valid_FL_type:
            self.FL_type = FL_type
        else:
            raise ValueError(
                f"Unsupported FL type: {FL_type}, use {valid_FL_type} instead."
            )

        # check role
        role = role.lower()
        valid_VFL_roles = {"host", "guest"}
        valid_HFL_roles = {"client", "server"}

        if (FL_type == "V" and role in valid_VFL_roles) or (
            FL_type == "H" and role in valid_HFL_roles
        ):
            self.role = role
        else:
            error_msg = f"Unsupported role: {role} for {FL_type}FL,"
            if FL_type == "V":
                error_msg += f" use {valid_VFL_roles} instead."
            else:
                error_msg += f" use {valid_HFL_roles} instead."
            raise ValueError(error_msg)

        self.channel = channel

    def check_channel(self):
        if self.channel is None:
            raise ValueError(
                f"For {self.__class__.__name__},"
                f" channel cannot be None in {self.FL_type}FL"
            )

    def Vfit(self, X):
        return self.module.fit(X)

    def Hfit(self, X):
        return self.module.fit(X)

    def fit(self, X=None):
        if self.FL_type == "V":
            return self.Vfit(X)
        else:
            return self.Hfit(X)

    def transform(self, X):
        return self.module.transform(X)

    def fit_transform(self, X):
        return self.fit(X).transform(X)
