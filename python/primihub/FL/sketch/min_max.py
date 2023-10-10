import numpy as np


def min_client(X, channel, ignore_nan=False):
    if ignore_nan:
        client_min = np.nanmin(X, axis=0)
    else:
        client_min = np.min(X, axis=0)
    channel.send("client_min", client_min)
    server_min = channel.recv("server_min")
    return server_min


def min_server(channel):
    client_min = channel.recv_all("client_min")
    server_min = np.max(client_min, axis=0)
    channel.send_all("server_min", server_min)
    return server_min


def max_client(X, channel, ignore_nan=False):
    if ignore_nan:
        client_max = np.nanmax(X, axis=0)
    else:
        client_max = np.max(X, axis=0)
    channel.send("client_max", client_max)
    server_max = channel.recv("server_max")
    return server_max


def max_server(channel):
    client_max = channel.recv_all("client_max")
    server_max = np.max(client_max, axis=0)
    server_max.send_all("server_max", server_max)
    return server_max


def min_max_client(X, channel, ignore_nan=False):
    if ignore_nan:
        client_min = np.nanmin(X, axis=0)
        client_max = np.nanmax(X, axis=0)
    else:
        client_min = np.min(X, axis=0)
        client_max = np.max(X, axis=0)
    channel.send("client_min_max", [client_min, client_max])
    server_min, server_max = channel.recv("server_min_max")
    return server_min, server_max


def min_max_server(channel):
    client_min_max = channel.recv_all("client_min_max")
    client_min_max = np.array(client_min_max)
    # 0: client_min, 1: client_max
    server_min = np.min(client_min_max[:,0,:], axis=0)
    server_max = np.max(client_min_max[:,1,:], axis=0)
    channel.send_all("server_min_max", (server_min, server_max))
    return server_min, server_max


def min_guest(X, channel):
    guest_min = np.min(X, axis=1)
    channel.send("guest_min", guest_min)
    global_min = channel.recv("global_min")
    return global_min


def min_host(X, channel):
    host_min = np.min(X, axis=1)
    guest_min = channel.recv_all("guest_min")
    guest_host_min = guest_min.append(host_min)
    global_min = np.min(guest_host_min, axis=0)
    channel.send_all("global_min")
    return global_min


def max_guest(X, channel):
    guest_max = np.max(X, axis=1)
    channel.send("guest_max", guest_max)
    global_max = channel.recv("global_max")
    return global_max


def max_host(X, channel):
    host_max = np.max(X, axis=1)
    guest_max = channel.recv_all("guest_max")
    guest_host_max = guest_max.append(host_max)
    global_max = np.max(guest_host_max, axis=0)
    channel.send_all("global_max")
    return global_max


def min_max_guest(X, channel):
    guest_min = np.min(X, axis=1)
    guest_max = np.max(X, axis=1)
    channel.send("guest_min_max", [guest_min, guest_max])
    global_min, global_max = channel.recv("global_min_max")
    return global_min, global_max


def min_max_host(X, channel):
    host_min = np.min(X, axis=1)
    host_max = np.max(X, axis=1)
    guest_min_max = channel.recv("guest_min_max")
    guest_host_min_max = guest_min_max.append([host_min, host_max])
    # 0: guest_host_min, 1: guest_host_max
    global_min = np.min(guest_host_min_max[:,0,:], axis=0)
    global_max = np.max(guest_host_min_max[:,1,:], axis=0)
    channel.send_all("global_min_max", (global_min, global_max))
    return global_min, global_max