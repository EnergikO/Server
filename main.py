from pprint import pprint
from Connection import *
from DB import *


workers = dict()
data = dict()
volume = 0


connection = Connection(9090)
connection.connect()


while True:
    try:
        volume = connection.take_message()
        connection.send_ok()

        if volume:
            data = json.loads(connection.take_message(int(volume)))

        else:
            connection.send_error("Volume is empty")
            continue

    except Exception as err:
        try:
            connection.send_message(str(err))

        except ConnectionAbortedError:
            connection.reconnect()
            continue

        except ConnectionResetError:
            connection.reconnect()
            continue

    try:
        print(data["request_name"])

        match data["request_name"]:

            # Working requets

            case "db_connect":
                workers[data["request_args"]["db_name"]] = Worker(data["request_args"]["db_name"])
                connection.send_message()

            case "db_disconnect":
                del workers[data["request_args"]["db_name"]]
                connection.send_message()

            case "terminate":
                break

            # DB requets

            case "db_add":
                response = workers[data["request_args"]["db_name"]].add(
                    table_name=data["request_args"]["table"],
                    fields=data["request_args"]["fields"],
                    values=data["request_args"]["values"]
                )
                connection.send_message(response)

            case "db_execute":
                response = workers[data["request_args"]["db_name"]].execute(data["request_args"]["command"])
                connection.send_message(response)

            case "db_get_fields_by":
                response = workers[data["request_args"]["db_name"]].get_fields_by(
                    table_name=data["request_args"]["table"],
                    fields=data["request_args"]["fields"],
                    filter_condition=data["request_args"]["condition"]
                )
                connection.send_message(response)

            case _:
                connection.send_error('Wrong command')

    except ConnectionAbortedError:
        connection.reconnect()

    except ConnectionResetError:
        connection.reconnect()

    except KeyError as err:
        connection.send_message(f'There is no active connection to {data["request_args"]["db_name"]}, please create one firstly')

    finally:
        print("Done.\n")

del connection
