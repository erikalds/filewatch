import urllib.request

class Response:
    def __init__(self, r=None, code=None, headers=None, body=None):
        if r is None:
            assert(code is not None
                   and headers is not None
                   and body is not None)
            self.code = code
            self.headers = headers
            self.body = body
        else:
            self.code = r.code
            self.headers = { k: v for k, v in r.getheaders() }
            self.body = r.read()


def GET(resource):
    try:
        return Response(urllib.request.urlopen(resource))
    except urllib.error.HTTPError as e:
        return Response(code=e.code, headers=e.hdrs, body=e.msg)
