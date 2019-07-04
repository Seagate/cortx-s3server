
class EOSCoreSuccessResponse:
    _body = ""

    def __init__(self, body):
        self._body = body.decode("utf-8")

    def get_response(self):
        return self._body