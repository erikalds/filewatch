import myhttp
import unittest

from systest.filesys import FilesystemCleanup


temporarydir = None

class TestCase(unittest.TestCase):
    def test_serves_all_files_in_root_dir(self):
        with FilesystemCleanup(temporarydir) as fs:
            fs.create_file("file1.txt", "contents")
            fs.create_file("file2.txt", "contents")
            response = myhttp.GET("http://127.0.0.1:8086/v1.0/files")
            self.assertEqual(200, response.code)
            data = response.json()
            self.assertIn('/', data)
            self.assertEqual('/', data['/']['label'])
            self.assertIsInstance(data['/']['index'], int)
            self.assertIn('nodes', data['/'])
            self.assertIn('file1.txt', data['/']['nodes'])
            self.assertIn('file2.txt', data['/']['nodes'])
            self.assertEqual('file1.txt', data['/']['nodes']['file1.txt']['label'])
            self.assertEqual('file2.txt', data['/']['nodes']['file2.txt']['label'])
            self.assertIsInstance(data['/']['nodes']['file1.txt']['index'], int)
            self.assertIsInstance(data['/']['nodes']['file2.txt']['index'], int)
            self.assertNotEqual(data['/']['nodes']['file1.txt']['index'],
                                data['/']['nodes']['file2.txt']['index'])

            fs.create_file("file3.txt", 'contents')
            response = myhttp.GET("http://127.0.0.1:8086/v1.0/files")
            self.assertEqual(200, response.code)
            data = response.json()
            self.assertIn('/', data)
            self.assertIn('nodes', data['/'])
            self.assertIn('file1.txt', data['/']['nodes'])
            self.assertIn('file2.txt', data['/']['nodes'])
            self.assertIn('file3.txt', data['/']['nodes'])
            self.assertEqual('file3.txt', data['/']['nodes']['file3.txt']['label'])
            self.assertIsInstance(data['/']['nodes']['file3.txt']['index'], int)
            self.assertNotEqual(data['/']['nodes']['file1.txt']['index'],
                                data['/']['nodes']['file3.txt']['index'])
            self.assertNotEqual(data['/']['nodes']['file2.txt']['index'],
                                data['/']['nodes']['file3.txt']['index'])

    def test_serves_mtime_of_files_and_dirs(self):
        with FilesystemCleanup(temporarydir) as fs:
            fs.create_file("file1.txt", "contents")
            response = myhttp.GET("http://127.0.0.1:8086/v1.0/files")
            self.assertEqual(200, response.code)
            data = response.json()
            self.assertIn('/', data)
            self.assertIn('mtime', data['/'])
            self.assertEqual(fs.mtime('.'), data['/']['mtime'])
            self.assertIn('file1.txt', data['/']['nodes'])
            self.assertIn('mtime', data['/']['nodes']['file1.txt'])
            self.assertEqual(fs.mtime('file1.txt'), data['/']['nodes']['file1.txt']['mtime'])


def run_test(tempdir):
    global temporarydir
    temporarydir = tempdir
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(unittest.makeSuite(TestCase))
    return result.wasSuccessful()
