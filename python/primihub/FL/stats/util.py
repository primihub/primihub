def check_channel(channel, send: bool, recv: bool):
    if (send or recv) and channel is None:
        raise ValueError("channel can't be None if send or recv is True")


def check_role(role: str):
    role = role.lower()
    valid_role = {"client", "server", "guest", "host"}
    if role not in valid_role:
        raise ValueError(f"Unsupported role: {role}, use {valid_role} instead")
