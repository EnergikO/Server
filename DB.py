import sqlite3
from Json import Json


class Worker:
    def __init__(self, db_name):
        self.sqlite_connection = None
        self.cursor = None
        self.db_name = db_name

        self.start()

    def __del__(self):
        self.stop()

    def add(self, table_name: str, fields: str, values: str) -> Json:
        sqlite_select_query = f"INSERT INTO {table_name} ({fields}) VALUES ({str(values)});"
        try:
            self.cursor.execute(sqlite_select_query)

        except Exception as err:
            return Json.json_from_dict({"status": "Error", "message": str(err)})

        self.save()

        return Json.json_from_dict(dict())

    def execute(self, command: str) -> Json:
        return self.execute_and_send_message(command)

    def get_fields_by(self, table_name: str, fields: str, filter_condition: str) -> Json:
        sqlite_select_query = f'SELECT {fields} FROM {table_name} WHERE {filter_condition};'
        return self.execute_and_send_message(sqlite_select_query)

    def start(self):
        self.sqlite_connection = sqlite3.connect(self.db_name)
        self.cursor = self.sqlite_connection.cursor()

    def stop(self):
        self.cursor.close()
        self.sqlite_connection.close()

    def save(self):
        self.sqlite_connection.commit()

    def execute_and_send_message(self, command) -> Json:
        try:
            self.cursor.execute(command)
            return Json.json_from_dict({"status": "Success", "data": self.cursor.fetchall()})

        except Exception as err:
            return Json.json_from_dict({"status": "Error", "message": err})
