import socket
from datetime import datetime
from Json import Json


class Server:
    def __init__(self, address: str, port: int):
        self.connection = None
        self.address = None
        self.socket = socket.socket()
        self.socket.bind((address, port))

        self.socket.listen(5)

    def __del__(self):
        self.socket.close()

    def connect(self):
        self.connection, self.address = self.socket.accept()
        print(f"Connect: {self.address}\ntime: {datetime.now()}\n------")

    def disconnect(self):
        self.connection.close()

        print(f"Disconnect: {self.address}\ntime: {datetime.now()}\n--==--")

        self.connection = None
        self.address = None

    def reconnect(self):
        self.disconnect()
        self.connect()

    def send_message(self, message: Json = "{}"):
        self.connection.send(str("%08X" % len(message.encode('utf8'))).encode('utf8'))
        self.connection.send(message.encode('utf8'))

    def send_error(self, error: str) -> None:
        data = Json.json_from_dict({"status": "Error", "error": error}).encode('utf8')
        self.connection.send(str("%08X" % len(data)).encode('utf8'))
        self.connection.send(data)

    def take_message(self) -> dict:
        length = int(self.connection.recv(8).decode('utf8'), base=16)
        return Json.json_to_dict(self.connection.recv(length).decode("utf8"))
