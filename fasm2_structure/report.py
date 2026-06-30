from __future__ import annotations

import html
import json
from pathlib import Path
from typing import Any

from .analysis import StructureModel, graph_adjacency, tarjan_scc, condensation_layers

PRESSURE_COLORS = {
    "pure_leaf": "#d9fdd3",
    "tail_abi": "#d7ecff",
    "abi_boundary": "#fff3bf",
    "abi_state_pressure": "#ffd6d6",
}


def path_module(path: str) -> str:
    parts = tuple(Path(path).parts)
    if not parts:
        return path
    if parts[0] in {"source", "examples", "tests"} and len(parts) >= 2:
        return "/".join(parts[:2])
    return parts[0]


def build_report_data(model: StructureModel) -> dict[str, Any]:
    adj = graph_adjacency(model)
    sccs = tarjan_scc(adj)
    layers = condensation_layers(adj, sccs)
    comp_id = {node: i for i, comp in enumerate(sccs) for node in comp}
    layer_by_comp = {id(comp): layer_i for layer_i, layer in enumerate(layers) for comp in layer}
    # Map by frozenset because condensation_layers returns the original comp list objects.
    layer_by_set = {frozenset(comp): layer_i for layer_i, layer in enumerate(layers) for comp in layer}

    functions = {name: sym for name, sym in model.symbols.items() if sym.kind == "function"}
    metrics = model.metrics

    scc_nodes = []
    for i, comp in enumerate(sccs):
        comp_metrics = [metrics[n] for n in comp if n in metrics]
        max_pressure = max((m.abi_pressure for m in comp_metrics), default=0)
        pressure_classes = sorted({m.pressure_class for m in comp_metrics})
        files = sorted({functions[n].path for n in comp if n in functions})
        scc_nodes.append(
            {
                "id": i,
                "label": f"SCC {i}",
                "functions": comp,
                "size": len(comp),
                "layer": layer_by_set.get(frozenset(comp), 0),
                "recursive": len(comp) > 1 or any(n in adj.get(n, set()) for n in comp),
                "max_abi_pressure": max_pressure,
                "pressure_classes": pressure_classes,
                "files": files,
                "modules": sorted({path_module(p) for p in files}),
            }
        )

    scc_edges_seen: set[tuple[int, int]] = set()
    scc_edges = []
    for edge in model.edges:
        if edge.source in comp_id and edge.target in comp_id:
            a, b = comp_id[edge.source], comp_id[edge.target]
            if a != b and (a, b) not in scc_edges_seen:
                scc_edges_seen.add((a, b))
                scc_edges.append({"source": a, "target": b, "kind": edge.kind})

    module_nodes: dict[str, dict[str, Any]] = {}
    module_edges: dict[tuple[str, str], int] = {}
    for name, sym in functions.items():
        module = path_module(sym.path)
        m = metrics.get(name)
        node = module_nodes.setdefault(module, {"id": module, "functions": 0, "max_abi_pressure": 0, "classes": set()})
        node["functions"] += 1
        if m:
            node["max_abi_pressure"] = max(node["max_abi_pressure"], m.abi_pressure)
            node["classes"].add(m.pressure_class)
    for edge in model.edges:
        src = functions.get(edge.source)
        dst = functions.get(edge.target)
        if not src or not dst:
            continue
        a, b = path_module(src.path), path_module(dst.path)
        if a != b:
            module_edges[(a, b)] = module_edges.get((a, b), 0) + 1
    module_graph = {
        "nodes": [{**v, "classes": sorted(v["classes"])} for v in sorted(module_nodes.values(), key=lambda x: x["id"])],
        "edges": [{"source": a, "target": b, "weight": w} for (a, b), w in sorted(module_edges.items())],
    }

    function_rows = []
    for name, sym in sorted(functions.items()):
        m = metrics.get(name)
        if not m:
            continue
        function_rows.append(
            {
                "name": name,
                "path": sym.path,
                "line": sym.line_no,
                "module": path_module(sym.path),
                "scc": comp_id.get(name),
                "layer": next((n["layer"] for n in scc_nodes if n["id"] == comp_id.get(name)), 0),
                "total_calls": m.total_calls,
                "internal_calls": m.internal_calls,
                "abi_calls": m.abi_calls,
                "tail_abi_calls": m.tail_abi_calls,
                "parameter_uses_after_abi_call": m.parameter_uses_after_abi_call,
                "abi_pressure": m.abi_pressure,
                "pressure_class": m.pressure_class,
                "notes": m.notes,
            }
        )

    smells = []
    for row in function_rows:
        if row["abi_pressure"] >= 20:
            smells.append({"severity": "high", "kind": "high_abi_pressure", "function": row["name"], "detail": f"pressure={row['abi_pressure']}"})
        if row["layer"] == 0 and row["abi_pressure"] >= 10:
            smells.append({"severity": "medium", "kind": "leaf_layer_high_pressure", "function": row["name"], "detail": f"layer=0 pressure={row['abi_pressure']}"})
    for node in scc_nodes:
        if node["recursive"]:
            smells.append({"severity": "high" if node["size"] > 1 else "medium", "kind": "recursive_scc", "scc": node["id"], "detail": f"size={node['size']}"})
        if len(node["files"]) > 1:
            smells.append({"severity": "medium", "kind": "cross_file_scc", "scc": node["id"], "detail": f"files={len(node['files'])}"})

    return {
        "summary": {
            "functions": len(functions),
            "edges": len(model.edges),
            "sccs": len(sccs),
            "layers": len(layers),
            "pure_leaves": sum(1 for r in function_rows if r["abi_pressure"] == 0),
            "max_abi_pressure": max((r["abi_pressure"] for r in function_rows), default=0),
        },
        "functions": function_rows,
        "edges": [edge.__dict__ for edge in model.edges],
        "scc": {"nodes": scc_nodes, "edges": scc_edges, "layers_leaf_first": layers},
        "module_graph": module_graph,
        "smells": smells,
    }


def mermaid_id(prefix: str, value: Any) -> str:
    safe = "".join(ch if ch.isalnum() else "_" for ch in str(value))
    return f"{prefix}{safe}"


def scc_mermaid(data: dict[str, Any], max_nodes: int = 80) -> str:
    nodes = sorted(data["scc"]["nodes"], key=lambda n: (-n["max_abi_pressure"], n["layer"], n["id"]))[:max_nodes]
    keep = {n["id"] for n in nodes}
    lines = ["flowchart TD"]
    for n in nodes:
        node_id = mermaid_id("S", n["id"])
        label = f"SCC {n['id']}<br/>L{n['layer']} funcs={n['size']}<br/>pressure={n['max_abi_pressure']}"
        lines.append(f'  {node_id}["{label}"]')
    for e in data["scc"]["edges"]:
        if e["source"] in keep and e["target"] in keep:
            lines.append(f"  {mermaid_id('S', e['source'])} --> {mermaid_id('S', e['target'])}")
    lines += [
        "  classDef pure fill:#d9fdd3,stroke:#278a27",
        "  classDef pressure fill:#ffd6d6,stroke:#c22",
    ]
    for n in nodes:
        cls = "pure" if n["max_abi_pressure"] == 0 else "pressure" if n["max_abi_pressure"] >= 20 else ""
        if cls:
            lines.append(f"  class {mermaid_id('S', n['id'])} {cls}")
    return "\n".join(lines) + "\n"


def module_mermaid(data: dict[str, Any], max_edges: int = 120) -> str:
    graph = data["module_graph"]
    lines = ["flowchart LR"]
    for n in graph["nodes"]:
        lines.append(f'  {mermaid_id("M", n["id"])}["{html.escape(n["id"])}<br/>funcs={n["functions"]}<br/>maxP={n["max_abi_pressure"]}"]')
    for e in sorted(graph["edges"], key=lambda x: -x["weight"])[:max_edges]:
        lines.append(f'  {mermaid_id("M", e["source"])} -->|{e["weight"]}| {mermaid_id("M", e["target"])}')
    return "\n".join(lines) + "\n"


def top_pressure_mermaid(data: dict[str, Any], limit: int = 25) -> str:
    funcs = sorted(data["functions"], key=lambda r: (-r["abi_pressure"], r["name"]))[:limit]
    names = {f["name"] for f in funcs}
    lines = ["flowchart TD"]
    for f in funcs:
        lines.append(f'  {mermaid_id("F", f["name"])}["{html.escape(f["name"])}<br/>P={f["abi_pressure"]}<br/>{f["pressure_class"]}"]')
    for e in data["edges"]:
        if e["source"] in names and e["target"] in names:
            lines.append(f'  {mermaid_id("F", e["source"])} -->|{e["kind"]}| {mermaid_id("F", e["target"])}')
    return "\n".join(lines) + "\n"


HTML_TEMPLATE = """<!doctype html>
<meta charset="utf-8">
<title>fasm2 structure report</title>
<style>
body { font: 14px system-ui, sans-serif; margin: 0; color: #1f2937; }
header { padding: 1rem; background: #111827; color: white; }
main { display: grid; grid-template-columns: 23rem 1fr; gap: 1rem; padding: 1rem; }
.panel { border: 1px solid #d1d5db; border-radius: 8px; padding: .75rem; margin-bottom: 1rem; background: #fff; }
#graph { height: 68vh; border: 1px solid #d1d5db; border-radius: 8px; overflow: auto; background: #f9fafb; position: relative; }
.node { position: absolute; border: 1px solid #6b7280; border-radius: 8px; padding: .35rem .5rem; max-width: 16rem; box-shadow: 0 1px 3px #0002; cursor: pointer; }
.edge { color: #6b7280; font-size: 12px; }
input, select { width: 100%; box-sizing: border-box; margin: .2rem 0 .6rem; }
table { border-collapse: collapse; width: 100%; font-size: 12px; }
th, td { border-bottom: 1px solid #e5e7eb; padding: .25rem; text-align: left; }
tr:hover { background: #f3f4f6; }
.badge { display: inline-block; padding: .1rem .35rem; border-radius: .5rem; background: #e5e7eb; }
</style>
<header><h1>fasm2 structure report</h1><div id="summary"></div></header>
<main>
  <aside>
    <div class="panel">
      <label>View</label><select id="view"><option value="scc">SCC/layer condensation</option><option value="module">Module graph</option><option value="function">Function neighborhood</option></select>
      <label>Search/function focus</label><input id="search" placeholder="function/module substring">
      <label>Neighborhood depth</label><input id="depth" type="number" min="0" max="5" value="1">
      <label>Minimum ABI pressure</label><input id="minPressure" type="number" min="0" value="0">
      <label><input id="hidePure" type="checkbox" style="width:auto"> hide pure leaves</label>
      <button id="render">Render</button>
    </div>
    <div class="panel"><h3>Structure smells</h3><div id="smells"></div></div>
  </aside>
  <section>
    <div id="graph"></div>
    <div class="panel"><h3>Functions</h3><div id="table"></div></div>
  </section>
</main>
<script id="report-data" type="application/json">__DATA__</script>
<script>
const data = JSON.parse(document.getElementById('report-data').textContent);
const colors = {pure_leaf:'#d9fdd3', tail_abi:'#d7ecff', abi_boundary:'#fff3bf', abi_state_pressure:'#ffd6d6'};
const byName = Object.fromEntries(data.functions.map(f => [f.name, f]));
document.getElementById('summary').textContent = JSON.stringify(data.summary);
document.getElementById('render').onclick = render;
for (const id of ['view','search','depth','minPressure','hidePure']) document.getElementById(id).onchange = render;
function esc(s){return String(s).replace(/[&<>]/g, c=>({'&':'&amp;','<':'&lt;','>':'&gt;'}[c]));}
function smells(){document.getElementById('smells').innerHTML = data.smells.slice(0,60).map(s=>`<div><span class=badge>${s.severity}</span> ${esc(s.kind)} ${esc(s.function||('SCC '+s.scc))}: ${esc(s.detail)}</div>`).join('') || 'No smells at current thresholds.';}
function table(rows){document.getElementById('table').innerHTML = '<table><tr><th>P</th><th>class</th><th>function</th><th>layer</th><th>file</th></tr>'+rows.slice(0,200).map(f=>`<tr onclick="focusFn('${esc(f.name)}')"><td>${f.abi_pressure}</td><td>${f.pressure_class}</td><td>${esc(f.name)}</td><td>${f.layer}</td><td>${esc(f.path)}:${f.line}</td></tr>`).join('')+'</table>';}
function focusFn(name){document.getElementById('view').value='function'; document.getElementById('search').value=name; render();}
function place(nodes, edges){
 const g=document.getElementById('graph'); g.innerHTML='';
 const w=Math.max(g.clientWidth,900), xgap=220, ygap=76;
 const layers={}; nodes.forEach(n=>{const l=n.layer||0; (layers[l]??=[]).push(n);});
 Object.keys(layers).sort((a,b)=>a-b).forEach(l=>layers[l].forEach((n,i)=>{n.x=20+Number(l)*xgap; n.y=20+i*ygap;}));
 const svg=document.createElementNS('http://www.w3.org/2000/svg','svg'); svg.style.position='absolute'; svg.style.left=0; svg.style.top=0; svg.setAttribute('width',w); svg.setAttribute('height',Math.max(700,...nodes.map(n=>n.y+80))); g.appendChild(svg);
 const pos=Object.fromEntries(nodes.map(n=>[n.id,n]));
 for(const e of edges){ if(!pos[e.source]||!pos[e.target]) continue; const line=document.createElementNS('http://www.w3.org/2000/svg','line'); line.setAttribute('x1',pos[e.source].x+120); line.setAttribute('y1',pos[e.source].y+20); line.setAttribute('x2',pos[e.target].x); line.setAttribute('y2',pos[e.target].y+20); line.setAttribute('stroke','#9ca3af'); svg.appendChild(line); }
 for(const n of nodes){ const d=document.createElement('div'); d.className='node'; d.style.left=n.x+'px'; d.style.top=n.y+'px'; d.style.background=n.color||'#fff'; d.innerHTML=n.html; g.appendChild(d); }
}
function render(){
 smells(); const view=document.getElementById('view').value, q=document.getElementById('search').value.toLowerCase(), minP=Number(document.getElementById('minPressure').value||0), hidePure=document.getElementById('hidePure').checked;
 let rows=data.functions.filter(f=>f.abi_pressure>=minP && (!hidePure||f.abi_pressure>0) && (!q||f.name.toLowerCase().includes(q)||f.path.toLowerCase().includes(q)||f.module.toLowerCase().includes(q))).sort((a,b)=>b.abi_pressure-a.abi_pressure||a.name.localeCompare(b.name)); table(rows);
 if(view==='module'){
   const nodes=data.module_graph.nodes.filter(n=>(!q||n.id.toLowerCase().includes(q)) && n.max_abi_pressure>=minP).map((n,i)=>({id:n.id, layer:i%4, html:`<b>${esc(n.id)}</b><br>funcs=${n.functions}<br>maxP=${n.max_abi_pressure}`, color:n.max_abi_pressure? '#fff3bf':'#d9fdd3'}));
   place(nodes, data.module_graph.edges.map(e=>({source:e.source,target:e.target}))); return;
 }
 if(view==='function'){
   const focus=rows[0]; if(!focus){place([],[]); return;} const depth=Number(document.getElementById('depth').value||1); let keep=new Set([focus.name]);
   for(let i=0;i<depth;i++) for(const e of data.edges) if(keep.has(e.source)||keep.has(e.target)){keep.add(e.source); keep.add(e.target)}
   const fns=[...keep].map(n=>byName[n]).filter(Boolean).filter(f=>f.abi_pressure>=minP && (!hidePure||f.abi_pressure>0));
   place(fns.map(f=>({id:f.name, layer:f.layer, html:`<b>${esc(f.name)}</b><br>P=${f.abi_pressure} ${f.pressure_class}<br>${esc(f.path)}:${f.line}`, color:colors[f.pressure_class]})), data.edges.filter(e=>keep.has(e.source)&&keep.has(e.target))); return;
 }
 const nodes=data.scc.nodes.filter(n=>n.max_abi_pressure>=minP && (!hidePure||n.max_abi_pressure>0) && (!q||n.functions.join(' ').toLowerCase().includes(q)||n.files.join(' ').toLowerCase().includes(q))).map(n=>({id:n.id, layer:n.layer, html:`<b>SCC ${n.id}</b><br>L${n.layer} funcs=${n.size}<br>maxP=${n.max_abi_pressure}<br>${esc(n.modules.join(','))}`, color:n.max_abi_pressure>=20?'#ffd6d6':n.max_abi_pressure?'#fff3bf':'#d9fdd3'}));
 place(nodes, data.scc.edges);
}
render();
</script>
"""


def write_report(out_dir: str | Path, model: StructureModel) -> dict[str, Path]:
    out = Path(out_dir)
    mermaid_dir = out / "mermaid"
    out.mkdir(parents=True, exist_ok=True)
    mermaid_dir.mkdir(parents=True, exist_ok=True)
    data = build_report_data(model)
    data_path = out / "report-data.json"
    data_path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    html_payload = json.dumps(data, separators=(",", ":")).replace("</", "<\\/")
    html_path = out / "report.html"
    html_path.write_text(HTML_TEMPLATE.replace("__DATA__", html_payload), encoding="utf-8")
    scc_path = mermaid_dir / "scc-condensation.mmd"
    module_path = mermaid_dir / "module-graph.mmd"
    pressure_path = mermaid_dir / "top-pressure.mmd"
    scc_path.write_text(scc_mermaid(data), encoding="utf-8")
    module_path.write_text(module_mermaid(data), encoding="utf-8")
    pressure_path.write_text(top_pressure_mermaid(data), encoding="utf-8")
    return {"html": html_path, "data": data_path, "scc_mermaid": scc_path, "module_mermaid": module_path, "top_pressure_mermaid": pressure_path}
