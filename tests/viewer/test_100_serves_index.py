import myhttp
import unittest


class TestCase(unittest.TestCase):
    def test_serves_html_at_index_page(self):
        response = myhttp.GET('http://127.0.0.1:8086/')
        self.assertEqual(200, response.code, msg=response.body)
        expected = "<!doctype html>"
        self.assertEqual(response.body[:len(expected)].decode(), expected)


def run_test(tempdir):
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(unittest.makeSuite(TestCase))
    return result.wasSuccessful()
