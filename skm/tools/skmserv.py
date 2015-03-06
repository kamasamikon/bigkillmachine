#!/usr/bin/env python

import socket, select
import sys, time

EOL = b'\n'

try:
    port = int(sys.argv[1])
except:
    port = 54112


serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
serversocket.bind(('0.0.0.0', port))
serversocket.listen(50)
serversocket.setblocking(0)

epoll = select.epoll()
epoll.register(serversocket.fileno(), select.EPOLLIN)

class ClientInfo:
    def __init__(self, connection, addr):
        self.connection = connection
        self.addr = addr

        self.request = b""

        self.client = '?'

    def __del__(self):
        self.connection.close()

try:
    connections = {}

    while True:
        events = epoll.poll(1)
        for fileno, event in events:
            if fileno == serversocket.fileno():
                connection, address = serversocket.accept()
                connection.setblocking(0)
                epoll.register(connection.fileno(), select.EPOLLIN)

                ci = ClientInfo(connection, address)
                connections[connection.fileno()] = ci

                print "Welcome new connection from:", address, fileno
                print "Current time is:", time.asctime()
                continue

            if event & select.EPOLLIN:
                if not connections.has_key(fileno):
                    continue

                ci = connections[fileno]

                epoll.modify(fileno, select.EPOLLIN)

                try:
                    data = ci.connection.recv(1024)
                except:
                    data = None
                if not data:
                    del connections[fileno]
                    continue

                print data ,
                ci.request += data
                if EOL in ci.request:
                    # request = ci.request.replace("\0", "").strip()
                    # print ci.request
                    pass

                continue

            if event & select.EPOLLHUP:
                epoll.unregister(fileno)
                del connections[fileno]
                continue
finally:
    epoll.unregister(serversocket.fileno())
    epoll.close()
    serversocket.close()

# vim:set et ts=4 sw=4 sts=4 ff=unix:

