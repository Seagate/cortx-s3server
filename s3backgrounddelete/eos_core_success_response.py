"""This will generate success response"""


class EOSCoreSuccessResponse(object):
    """Generate success response."""
    _body = ""

    def __init__(self, body):
        """Initialise and parse success response."""
        self._body = body.decode("utf-8")

    def get_response(self):
        """Return generated response."""
        return self._body
