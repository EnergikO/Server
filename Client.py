import socket
from datetime import datetime
from Json import Json
from pathlib import Path


class Client:
    def __init__(self):
        self.address = None
        self.port = None
        self.socket = socket.socket()

    def __del__(self):
        self.disconnect()

    def connect(self, address: str, port: int):
        self.address = address
        self.port = port
        self.socket.connect((address, port))
        print(f"Connect: {address}: {port}\ntime: {datetime.now()}")

    def disconnect(self):
        if self.address:
            self.socket.close()
            print(f"Disconnect: {self.address}: {self.port}\ntime: {datetime.now()}\n--==--")

            self.address = None
            self.port = None

    def send_request(self, message: Json) -> None:
        self.socket.send(str("%08X" % len(message.encode('utf8'))).encode('utf8'))
        self.socket.send(message.encode('utf8'))

    def upload_picture(self, picture_filepath: str, tags: str = "") -> Json:
        if Path(picture_filepath).is_file():
            picture = open(picture_filepath, 'rb').read()

            self.send_request(Json.json_from_dict({
                "request_name": "upload_picture",
                "request_args": {
                    "filename": Path(picture_filepath).name,
                    "size": len(picture),
                    "tags": tags,
                }
            }))

            print("[.] Sending Picture ...")
            print("[+] Success!")

            self.socket.send(picture)

            print("[.] Waiting for server ...")

            return Json.json_from_str(self.socket.recv(int(self.socket.recv(8).decode('utf8'), base=16)).decode("utf8"))

        else:
            return Json.json_from_dict({"status": "Success", "message": "File not found!"})

    def take_picture(self, picture_id: str) -> Json:
        request = Json.json_from_dict({
            "request_name": "get_picture",
            "request_args": {
                "id": picture_id,
            }
        })

        self.send_request(request)
        response = Json.json_from_str(self.socket.recv(int(self.socket.recv(8).decode('utf8'), base=16)).decode("utf8"))

        if response["status"] == "Success":
            return Json.json_from_dict({
                "status": "Success",
                "picture": {
                    "size": response["size"],
                    "data": self.socket.recv(int(response["size"], base=16)),
                    "filename": response["filename"],
                    "tags": response["tags"],
                }
            })

        else:
            return response
