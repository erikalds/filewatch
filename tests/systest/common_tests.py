import logging
import subprocess
from systest.runningprocess import run_process


def simple_start_stop_test(path_to_executable, args):
    logging.info("Running simple start/stop test...")
    try:
        with run_process([path_to_executable] + args) as p:
            try:
                p.wait(timeout=1) # should be able to run for 1 second
                logging.error("%s failed with return code %d within 1 second"
                              % (path_to_executable, p.poll()))
            except subprocess.TimeoutExpired:
                pass # expected to time out

        logging.info("Simple start/stop test PASSED")
        return 0

    except Exception as e:
        logging.error("Simple start/stop test FAILED")
        logging.error(str(e))
        return 1
