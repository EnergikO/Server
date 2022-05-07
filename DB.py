import sqlite3
import json


class Worker:
    def __init__(self, db_name):
        self.sqlite_connection = None
        self.cursor = None
        self.db_name = db_name

        self.start()

    def __del__(self):
        self.stop()

    def add(self, table_name: str, fields: str, values: str) -> json:
        sqlite_select_query = f"INSERT INTO {table_name} ({fields}) VALUES ({str(values)});"
        try:
            self.cursor.execute(sqlite_select_query)

        except Exception as err:
            return json.dumps({"status": "Error", "message": str(err)}, separators=(',', ':'))

        self.save()

        return json.dumps({})

    def execute(self, command: str) -> json:
        return self.execute_and_send_message(command)

    def get_fields_by(self, table_name: str, fields: str, filter_condition: str) -> json:
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

    def execute_and_send_message(self, command):
        try:
            self.cursor.execute(command)
            return json.dumps({"status": "Success", "data": self.cursor.fetchall()}, separators=(',', ':'))

        except Exception as err:
            return json.dumps({"status": "Error", "message": err}, separators=(',', ':'))
