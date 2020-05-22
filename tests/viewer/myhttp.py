import json
import logging
import unittest
import urllib.request

class Response:
    def __init__(self, resp=None, code=None, headers=None, body=None):
        if resp is None:
            assert(code is not None
                   and headers is not None
                   and body is not None)
            self.code = code
            self.headers = headers
            self.body = body
        else:
            self.code = resp.code
            self.headers = { k: v for k, v in resp.getheaders() }
            self.body = resp.read()

    def json(self):
        assert 'Content-Type' in self.headers
        msg = 'Content-Type was: {}'.format(self.headers['Content-Type'])
        assert 'application/json' == self.headers['Content-Type'], msg
        try:
            return json.loads(self.body)
        except:
            logging.warn("HTTP Response body was: {}".format(self.body))
            raise


def GET(resource):
    try:
        return Response(resp=urllib.request.urlopen(resource))
    except urllib.error.HTTPError as e:
        return Response(code=e.code, headers=e.hdrs, body=e.msg)
