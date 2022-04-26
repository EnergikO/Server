import socket
import json


class Connection:
    def __init__(self, port):
        self.listener = socket.socket()
        self.listener.bind(('', port))

        self.listener.listen(5)

    def __del__(self):
        self.listener.close()

        del self.listener
        del self

    def connect(self):
        self.connection, self.address = self.listener.accept()
        print('Connect:', self.address)

    def disconnect(self):
        self.connection.close()

        print('Disconnect:', self.address)

        del self.connection
        del self.address

    def reconnect(self):
        self.disconnect()
        self.connect()

    def send_message(self, message: json = "{}"):
        self.connection.send(str(len(message.encode('utf8'))).encode('utf8'))
        self.take_message(2)
        self.connection.send(message.encode('utf8'))

    def send_error(self, error: str):
        self.connection.send(str(len(error.encode('utf8'))).encode('utf8'))
        self.take_message(2)
        self.connection.send(json.dumps({"status": "Error", "error": error}, separators=(',', ':')).encode('utf8'))

    def send_ok(self):
        self.connection.send('ok'.encode('utf8'))

    def take_message(self, volume=50):
        return self.connection.recv(int(volume)).decode("utf8")
