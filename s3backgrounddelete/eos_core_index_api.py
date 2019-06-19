import http.client, urllib.parse
import sys
import datetime
import json

from eos_index_response import EOSCoreIndexResponse
from eos_core_client import EOSCoreClient
from config import Config

# EOSCoreIndexApi supports index REST-API's List & Put

class EOSCoreIndexApi(EOSCoreClient):

    def __init__(self):
        super(EOSCoreIndexApi, self).__init__()

    def list(self, index):

        if index is None:
            print("Index Id is required.")
            return

        request_uri = '/indexes/' + index
        response = super().get(request_uri)
        return EOSCoreIndexResponse(response['status'], response['reason'], response['body'])

    def put(self, index):

        if index is None:
            print("Index Id is required.")
            return

        request_uri = '/indexes/' + index
        response = super().put(request_uri)
        if response['status'] == 201:
            print('Successfully added Index.')
            return(response)
        else:
            print('Failed to add Index.')
            return(response)