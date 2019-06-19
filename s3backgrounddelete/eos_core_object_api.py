import http.client, urllib.parse
import sys
import datetime
import json
from eos_core_client import EOSCoreClient
from config import Config

# EOSCoreObjectApi supports object REST-API's Put, Get & Delete

class EOSCoreObjectApi(EOSCoreClient):

    def __init__(self):
        super(EOSCoreObjectApi, self).__init__()

    def put(self, oid, value):
        if oid is None:
            print("Object Id is required.")
            return

        request_body = value
        request_uri = '/objects/' + oid
        response = super().put(request_uri, request_body)
        if response['status'] == 201:
            print("Object added successfully.")
            return(response)
        else:
            print('Failed to add Object.')
            return(response)

    def get(self, oid):
        if oid is None:
            print("Object Id is required.")
            return
        request_uri = '/objects/' + oid
        response = super().get(request_uri)
        if response['status'] == 200:
            print('Successfully fetched object details.')
            return(json.loads(response['body'].decode("utf-8")))
        else:
            print('Failed to fetch object details.')
            return(response['body'].decode("utf-8"))

    def delete(self, oid):
        if oid is None:
            print("Object Id is required.")
            return
        request_uri = '/objects/' + oid
        response = super().delete(request_uri)
        if response['status'] == 204:
            print('Object deleted successfully.')
        else:
            print('Failed to delete Object.')
            return(response)
