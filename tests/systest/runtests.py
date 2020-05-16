import logging
import os
import re


def is_sorted(list_):
    if not list_ or len(list_) == 1:
        return True

    for i in range(1, len(list_)):
        if list_[i - 1] > list_[i]:
            return False

    return True


def group_tests(testlist):
    groups = dict()
    for test in testlist:
        mo = re.match(".*?test_([0-9]).*", test)
        if not mo:
            logging.warning("Ignoring test %s." % test)
            continue

        group = int(mo.group(1))
        if group not in groups:
            groups[group] = list()

        groups[group].append(os.path.split(test)[-1])

    return groups


def run_tests_grouped(tests, tempdir, failures):
    all_tests = group_tests(tests)
    keys = list(all_tests.keys())
    keys.sort()
    assert(is_sorted(keys))
    for group in keys:
        for test in all_tests[group]:
            logging.info("Running test %s..." % test)
            testmod = os.path.splitext(test)[0]
            try:
                exec("import %s" % testmod)
                retval = eval("%s.run_test(tempdir.path)" % testmod)
                if retval is not None and retval is False:
                    logging.error("test %s failed" % test)
                    failures += 1
                else:
                    logging.info("test %s succeeded" % test)
            except:
                import traceback
                traceback.print_exc()
                failures += 1
                logging.error("test %s failed" % test)

        if failures > 0:
            logging.error("%d tests failed in group %d. Bailing out."
                          % (failures, group))
            break

    return failures
