import unittest
import sys
import os

# Add parent directory to sys.path so we can import server
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from server import app

class TestServerSyntax(unittest.TestCase):
    def test_routes_exist(self):
        self.assertIsNotNone(app)
        # Verify app has the required endpoints registered
        routes = [r.rule for r in app.url_map.iter_rules()]
        self.assertIn('/', routes)
        self.assertIn('/api/status', routes)
        self.assertIn('/api/speed', routes)
        self.assertIn('/api/stop', routes)
        self.assertIn('/api/presets', routes)
        self.assertIn('/api/presets/<name>', routes)
        self.assertIn('/api/status/stream', routes)

if __name__ == '__main__':
    unittest.main()
