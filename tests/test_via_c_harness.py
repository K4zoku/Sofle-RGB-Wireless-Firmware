import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).parents[1]


class ViaTranslationNativeTests(unittest.TestCase):
    def test_production_translation_c_vectors(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / "via_translation_harness"
            subprocess.run(
                [
                    "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                    "-I", str(ROOT / "config/include"),
                    str(ROOT / "config/src/via_translation.c"),
                    str(ROOT / "tests/via_translation_harness.c"),
                    "-o", str(binary),
                ],
                check=True,
            )
            result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
            self.assertIn("via translation vectors: ok", result.stdout)


if __name__ == "__main__":
    unittest.main()
