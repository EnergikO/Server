from Client import *
from config import *
from art import tprint


def main():
    tprint(f"Client - {CLIENT_VERSION}", font="big")
    client = Client()

    while True:
        try:
            client.connect(SERVER_ADDRESS, SERVER_PORT)
            break

        except TimeoutError:
            pass

    filepath = r"V:\for Python\fun\PythonToday\txtIntoMp3\neko.mp3"
    print(client.upload_picture(picture_filepath=filepath, tags="anime_booba booba anime neko"))

    client.disconnect()


if __name__ == "__main__":
    main()
