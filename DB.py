import sqlite3
import json


class Worker:
    def __init__(self, db_name):
        self.db_name = db_name

        self.start()

    def __del__(self):
        self.stop()

        del self.db_name
        del self

    def add(self, table_name: str, fields: str, values: str) -> json:
        sqlite_select_query = f"INSERT INTO {table_name} ({fields}) VALUES ({str(values)});"
        try:
            self.cursor.execute(sqlite_select_query)

        except sqlite3.IntegrityError as err:
            return json.dumps({"status": "Error", "message": str(err)}, separators=(',', ':'))

        except Exception as err:
            return json.dumps({"status": "Error", "message": str(err)}, separators=(',', ':'))

        self.save()

        return json.dumps({})

    def execute(self, command: str) -> json:
        self.cursor.execute(command)
        return json.dumps({"data": self.cursor.fetchall()}, separators=(',', ':'))

    def get_fields_by(self, table_name: str, fields: str, filter_condition: str) -> json:
        sqlite_select_query = f'SELECT {fields} FROM {table_name} WHERE {filter_condition};'
        self.cursor.execute(sqlite_select_query)

        return json.dumps({"data": self.cursor.fetchall()}, separators=(',', ':'))

    def start(self):
        self.sqlite_connection = sqlite3.connect(self.db_name)
        self.cursor = self.sqlite_connection.cursor()

    def stop(self):
        self.cursor.close()
        self.sqlite_connection.close()

    def save(self):
        self.sqlite_connection.commit()
