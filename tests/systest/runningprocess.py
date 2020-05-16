import logging
import subprocess


class RunningProcess:
    def __init__(self, cmd):
        self.cmd = cmd
        self.proc = None

    def __enter__(self):
        self.proc = subprocess.Popen(self.cmd)
        return self.proc

    def __exit__(self, type_, value, traceback):
        optional_exit_code = self.proc.poll()
        procname = self.cmd[0].split()[0]
        if optional_exit_code is not None:
            errtext = "%s exited prematurely with exit code: %d"
            raise Exception(errtext % (procname, optional_exit_code))

        logging.info("Terminating %s" % procname)
        self.proc.terminate()
        try:
            exit_code = self.proc.wait(3)
        except subprocess.TimeoutExpired:
            logging.error("%s did not die in 3 seconds, killing..." % procname)
            self.proc.kill()
            try:
                exit_code = self.proc.wait(5)
            except subprocess.TimeoutExpired:
                errtext = "WARNING: %s did not die in 5 seconds.\n" % procname
                errtext += "You should clean up manually."
                raise Exception(errtext)

        if exit_code != 0:
            raise Exception("%s returned exit code: %d" % (procname, exit_code))


def run_process(cmd):
    return RunningProcess(cmd)
