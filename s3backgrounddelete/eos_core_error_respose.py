import json

class EOSCoreErrorResponse:
    _http_status = -1
    _error_message = ""
    _error_reason = ""

    def __init__(self, error_status, error_msg, error_reason):
        self._http_status = error_status
        self._error_message = error_msg
        self._error_reason = error_reason

    def get_error_status(self):
        return self._http_status

    def get_error_message(self):
        return self._error_message

    def get_error_reason(self):
        return self._error_reason

