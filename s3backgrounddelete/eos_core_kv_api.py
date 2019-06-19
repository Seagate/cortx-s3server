import http.client, urllib.parse
import sys
import datetime

from eos_core_client import EOSCoreClient
from eos_kv_response import EOSCoreKVResponse
from config import Config

# EOSCoreKVApi supports key-value REST-API's Put, Get & Delete

class EOSCoreKVApi(EOSCoreClient):

    def __init__(self):
        super(EOSCoreKVApi, self).__init__()

    def put(self, index = None, key = None, value= None):
        if index is None:
            print("Index Id is required.")
            return
        if key is None:
            print("Key is required")
            return

        request_body = value
        request_uri = '/indexes/' + index + '/' + key
        response = super().put(request_uri, request_body)
        if response['status'] == 201:
            print("Key value details added successfully.")
            return(response)
        else:
            print('Failed to add key value details.')
            return(response)

    def get(self, index = None, key = None):
        if index is None:
            print("Index Id is required.")
            return
        if key is None:
            print("Key is required")
            return

        request_uri = '/indexes/' + index + '/' + key
        response = super().get(request_uri)
        return EOSCoreKVResponse(response['status'], response['reason'], key, response['body'].decode("utf-8"))

    def delete(self, index = None, key = None):
        if index is None:
            print("Index Id is required.")
            return
        if key is None:
            print("Key is required")
            return

        request_uri = '/indexes/' + index + '/' + key
        response = super().delete(request_uri)
        if response['status'] == 204:
            print('Key value deleted.')
            return(response)
        else:
            print('Failed to delete key value.')
            return(response)
