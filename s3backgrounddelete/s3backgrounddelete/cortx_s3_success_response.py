"""This will generate success response"""


class CORTXS3SuccessResponse(object):
    """Generate success response."""
    _body = ""

    def __init__(self, body):
        """Initialise and parse success response."""
        self._body = body.decode("utf-8")

    def get_response(self):
        """Return generated response."""
        return self._body

    def get_success_status(self):
        """Return success status."""
        return self._body['status']
