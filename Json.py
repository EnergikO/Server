import json


class Json(str):
    @staticmethod
    def json_from_dict(array: dict):
        return json.dumps(array)

    @staticmethod
    def json_from_str(json_string: str):
        return json.loads(json_string)

    @staticmethod
    def json_to_dict(json_string: str):
        return json.loads(json_string)
