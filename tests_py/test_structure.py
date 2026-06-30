import tempfile
import unittest
from pathlib import Path

from fasm2_structure.analysis import build_structure, condensation_layers, graph_adjacency, tarjan_scc
from fasm2_structure.asm_parser import parse_tree
from fasm2_structure.report import build_report_data, write_report


class StructureAnalysisTests(unittest.TestCase):
    def test_parse_and_analyze_fixture(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "sample.asm").write_text(
                """
extrn 'MessageBoxA' as MessageBoxA
public start

proc helper, value
        mov eax,ecx
        ret
endp

proc wrapper, arg
        invoke MessageBoxA,0,arg,0,0
        mov eax,ecx
        ret
endp

start:
        call helper
        jmp wrapper

payload db 'x',0
""".strip()
                + "\n"
            )
            parsed = parse_tree(root)
            model = build_structure(parsed)
            self.assertIn("helper", model.metrics)
            self.assertEqual(model.metrics["helper"].abi_pressure, 0)
            self.assertEqual(model.metrics["helper"].pressure_class, "pure_leaf")
            self.assertIn("pure leaf", " ".join(model.metrics["helper"].notes))
            self.assertGreaterEqual(model.metrics["wrapper"].parameter_uses_after_abi_call, 1)
            self.assertEqual(
                model.metrics["wrapper"].abi_pressure,
                model.metrics["wrapper"].abi_calls + model.metrics["wrapper"].parameter_uses_after_abi_call,
            )
            self.assertEqual(model.metrics["wrapper"].pressure_class, "abi_state_pressure")
            adj = graph_adjacency(model)
            layers = condensation_layers(adj, tarjan_scc(adj))
            self.assertTrue(layers)
            self.assertTrue(any(edge.target == "MessageBoxA" and edge.kind == "abi" for edge in model.edges))
            report_data = build_report_data(model)
            self.assertEqual(report_data["summary"]["functions"], 3)
            self.assertIn("module_graph", report_data)
            paths = write_report(root / "analysis", model)
            for path in paths.values():
                self.assertTrue(path.exists(), path)
                self.assertGreater(path.stat().st_size, 0, path)


if __name__ == "__main__":
    unittest.main()
