#!/usr/bin/env python
import socket
from mprpc import RPCClient
import usrp_mpm as mpm
import argparse


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--address", default="0.0.0.0", type=str, help="Destination address")
    parser.add_argument("-p", "--port", default=0, type=int, help="Destination port")
    sub_parsers = parser.add_subparsers(dest="command")

    rpc_parser = sub_parsers.add_parser("rpc", help="Issue RPC")
    rpc_parser.add_argument("-c", "--call", required=True, help="command to issue")
    rpc_parser.add_argument("arguments", nargs="*")

    disc_parser = sub_parsers.add_parser("disc", help="Issue discovery")
    return parser.parse_args()


def rpc(address, port, command, *args):
    if not port:
        port = mpm.types.MPM_RPC_PORT
    client = RPCClient(address, port)
    if args:
        result = client.call(command, *args)
    else:
        result = client.call(command)
    return result


def discovery(address, port):
    if not port:
        port = mpm.types.MPM_DISCOVERY_PORT
    sock = socket.socket(
        socket.AF_INET,
        socket.SOCK_DGRAM)
    sock.sendto(mpm.types.MPM_DISCOVERY_MESSAGE, (address, port))
    sock.settimeout(1.0) # wait max 1 second
    while True:
        try:
            data, sender = sock.recvfrom(4096)
            print("Received respons from: {}".format(sender[0]))
            print("Dicovery data: {}".format(data))
        except:
            break


def main():
    args = parse_args()
    if args.command == "rpc":
        if args.arguments:
            result = rpc(args.address, args.port, args.call, *args.arguments)
        else:
            result = rpc(args.address, args.port, args.call)
        print(result)
    elif args.command == "disc":
        discovery(args.address, args.port)


if __name__ == "__main__":
    exit(not(main()))
