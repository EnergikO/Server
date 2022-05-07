from Server import *
from DB import *
from config import *
from art import tprint
from time import time as now


def main():
    tprint(f"DataBase - {DB_VERSION}", font="big")
    remaining_lifetime = -1
    timer_start = 0

    workers = dict()

    server = Server(DB_ADDRESS, DB_PORT)
    server.connect()

    while True:
        try:
            if -1 < remaining_lifetime <= now() - timer_start:
                break

            data = json.loads(server.take_message())

        except Exception as err:

            try:
                server.send_error(str(err))
                continue

            except ConnectionAbortedError:
                server.reconnect()
                continue

            except ConnectionResetError:
                server.reconnect()
                continue

        try:
            print(data["request_name"])

            match data["request_name"]:

                # Working requests

                case "db_connect":
                    workers[data["request_args"]["db_name"]] = Worker(data["request_args"]["db_name"])
                    server.send_message()

                case "db_disconnect":
                    del workers[data["request_args"]["db_name"]]
                    server.send_message()

                case "terminate":
                    remaining_lifetime = int(data["request_args"]["remaining_lifetime"])
                    timer_start = now()

                # DB requests

                case "db_add":
                    response = workers[data["request_args"]["db_name"]].add(
                        table_name=data["request_args"]["table"],
                        fields=data["request_args"]["fields"],
                        values=data["request_args"]["values"]
                    )
                    server.send_message(response)

                case "db_execute":
                    response = workers[data["request_args"]["db_name"]].execute(data["request_args"]["command"])
                    server.send_message(response)

                case "db_get_fields_by":
                    response = workers[data["request_args"]["db_name"]].get_fields_by(
                        table_name=data["request_args"]["table"],
                        fields=data["request_args"]["fields"],
                        filter_condition=data["request_args"]["condition"]
                    )
                    server.send_message(response)

                case _:
                    server.send_error('Wrong command')

        except ConnectionAbortedError:
            server.reconnect()

        except ConnectionResetError:
            server.reconnect()

        except KeyError:
            server.send_message(Json.json_from_str(f'There is no active connection to {data["request_args"]["db_name"]}, '
                                f'please create one firstly.'))

        finally:
            print("Done.\n------")

    del server


if __name__ == "__main__":
    main()
