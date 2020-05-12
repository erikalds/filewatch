import grpc
import logging


class Connectivity:
    """Monitor connectivity of a gRPC channel.
    """
    def __init__(self, channel, try_to_connect=False):
        self._channel = channel
        self._connected = False
        self._try_to_connect = try_to_connect

    def __call__(self, connectivity):
        logging.debug("gRPC channel -> {}".format(connectivity))
        if connectivity == grpc.ChannelConnectivity.READY:
            self._connected = True

    def __enter__(self):
        self._channel.subscribe(self, try_to_connect=self._try_to_connect)
        return self

    def __exit__(self, type, value, traceback):
        self._channel.unsubscribe(self)

    def wait_connected(self):
        logging.info("wait for connection...")
        while not self._connected:
            import time
            time.sleep(0)
        logging.info("connected")
