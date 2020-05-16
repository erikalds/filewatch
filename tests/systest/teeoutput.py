import sys


class CapturingStreamer:
    def __init__(self, file_, stream):
        self._file = file_
        self._stream = stream

    def flush(self):
        self._file.flush()
        self._stream.flush()

    def write(self, text):
        self._file.write(text)
        self._stream.write(text)


class TeeOutput:
    def __init__(self, filename):
        self._file = open(filename, 'w')
        self._ostreams = [CapturingStreamer(self._file, sys.stdout),
                          CapturingStreamer(self._file, sys.stderr)]
        sys.stdout = self._ostreams[0]
        sys.stderr = self._ostreams[1]
