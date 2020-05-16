import logging
import os
import tempfile


class TemporaryDir:
    def __init__(self, containing_dir):
        self.containing_dir = containing_dir
        self.path = None

    def __enter__(self):
        self.path = tempfile.mkdtemp(dir=self.containing_dir)
        logging.info("Created tempdir: %s" % self.path)
        return self

    def __exit__(self, type_, value, traceback):
        os.rmdir(self.path)
        logging.info("Deleted tempdir: %s" % self.path)


def create_tempdir(containing_dir):
    return TemporaryDir(containing_dir)
