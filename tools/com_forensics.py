#!/usr/bin/env python3
"""
COM Interface Forensic Analyzer v2.0
Supports: comtypes (rich analysis), PE parsing (offline forensics), and registry extraction

pip install comtypes pefile

C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64\MsXml.Tlb

"""

import sys
import struct
import json
import re
from pathlib import Path
from enum import IntEnum
from dataclasses import dataclass, asdict
from typing import List, Dict, Optional
import argparse

try:
    import comtypes
    from comtypes.typeinfo import LoadTypeLibEx, ITypeLib, ITypeInfo, TKIND_INTERFACE, TKIND_DISPATCH, TKIND_COCLASS
    from comtypes.tools.tlbparser import TlbParser
    HAS_COMTYPES = True
except ImportError:
    HAS_COMTYPES = False

try:
    import pefile
    HAS_PEFILE = True
except ImportError:
    HAS_PEFILE = False

try:
    import winreg
    HAS_WINREG = True
except ImportError:
    HAS_WINREG = False


class ForensicRisk:
    def __init__(self):
        self.high_risk = []
        self.medium_risk = []
        self.scriptable = []
        self.hidden = []
        self.attack_surface = 0
    
    def to_dict(self):
        return {
            "high_risk_findings": self.high_risk,
            "medium_risk_findings": self.medium_risk,
            "scriptable_interfaces": self.scriptable,
            "hidden_objects": self.hidden,
            "attack_surface_score": self.attack_surface
        }


@dataclass
class MethodInfo:
    name: str
    dispid: int
    params: List[Dict]
    is_dangerous: bool
    is_property: bool
    inv_kind: str
    
@dataclass 
class InterfaceInfo:
    name: str
    iid: str
    base: str
    is_dispatch: bool
    is_hidden: bool
    is_restricted: bool
    methods: List[MethodInfo]
    coclasses: List[str]


class COMForensicAnalyzer:
    DANGEROUS_PATTERNS = [
        (r'(exec|execute|shell|createprocess|winexec)\w*$', 'Code Execution'),
        (r'(write|delete|remove|clear|erase)\w*$', 'Data Destruction'),
        (r'(save|load|import|export)(file|url|path)', 'File Operations'),
        (r'(reg|registry|createkey|deletekey)', 'Registry Manipulation'),
        (r'(navigate|gohome|refresh)', 'URL Navigation'),
        (r'(spawn|createobject|activate)\w*$', 'Object Instantiation'),
        (r'(alloc|free|memcpy|strcpy)', 'Memory Operations')
    ]
    
    def __init__(self):
        self.risk = ForensicRisk()
        self.interfaces = []
        self.coclasses = []
        
    def analyze_file(self, filepath: str, mode: str = 'auto') -> Dict:
        """Primary analysis dispatcher"""
        path = Path(filepath)
        if not path.exists():
            raise FileNotFoundError(f"File not found: {filepath}")
        
        print(f"[*] Analyzing: {filepath}")
        print(f"[*] Mode: {mode}")
        
        if mode == 'auto':
            # Prefer COM types for rich analysis, fallback to PE parsing
            if HAS_COMTYPES:
                try:
                    return self._analyze_comtypes(filepath)
                except Exception as e:
                    print(f"[!] COM analysis failed ({e}), switching to PE mode...")
                    if HAS_PEFILE:
                        return self._analyze_pe(filepath)
                    else:
                        raise ImportError("Install 'comtypes' or 'pefile' for analysis")
            elif HAS_PEFILE:
                return self._analyze_pe(filepath)
            else:
                raise ImportError("Install 'comtypes' or 'pefile' to analyze files")
        
        elif mode == 'com':
            if not HAS_COMTYPES:
                raise ImportError("comtypes library required")
            return self._analyze_comtypes(filepath)
            
        elif mode == 'pe':
            if not HAS_PEFILE:
                raise ImportError("pefile library required")
            return self._analyze_pe(filepath)
            
        else:
            raise ValueError(f"Unknown mode: {mode}")

    def _analyze_comtypes(self, filepath: str) -> Dict:
        """Rich analysis using COM APIs"""
        try:
            tlib = LoadTypeLibEx(filepath, 0)  # 0 = REGKIND_NONE (don't register)
            
            # Iterate type infos
            count = tlib.GetTypeInfoCount()
            print(f"[*] Found {count} type definitions")
            
            for i in range(count):
                try:
                    info = tlib.GetTypeInfo(i)
                    attr = info.GetTypeAttr()
                    
                    if attr.typekind == TKIND_INTERFACE:
                        self._parse_interface(info, attr)
                    elif attr.typekind == TKIND_DISPATCH:
                        self._parse_dispatch(info, attr)
                    elif attr.typekind == TKIND_COCLASS:
                        self._parse_coclass(info, attr)
                        
                except Exception as e:
                    print(f"[!] Error parsing type {i}: {e}")
                    continue
            
            return self._generate_report()
            
        except Exception as e:
            print(f"[!] COM analysis error: {e}")
            raise

    def _parse_interface(self, info, attr):
        """Parse ITypeInfo for interface"""
        name = info.GetDocumentation(-1)[0]
        iid = str(attr.guid)
        
        # Get base interface
        base = "IUnknown"
        if attr.cImplTypes > 0:
            try:
                href = info.GetRefTypeOfImplType(0)
                base_info = info.GetRefTypeInfo(href)
                base = base_info.GetDocumentation(-1)[0]
            except:
                pass
        
        # Check flags
        is_hidden = bool(attr.wTypeFlags & 0x10)  # TYPEFLAG_FHIDDEN
        is_restricted = bool(attr.wTypeFlags & 0x100)  # TYPEFLAG_FRESTRICTED
        is_dispatch = (base == "IDispatch" or 'dispatch' in name.lower())
        
        if is_hidden:
            self.risk.hidden.append({"name": name, "iid": iid})
        
        if is_dispatch and not is_restricted:
            self.risk.scriptable.append({"name": name, "iid": iid})
        
        # Parse methods
        methods = []
        for j in range(attr.cFuncs):
            try:
                func_desc = info.GetFuncDesc(j)
                func_name = info.GetDocumentation(func_desc.memid)[0]
                
                # Check danger
                is_dangerous, threat_type = self._check_dangerous(func_name)
                
                if is_dangerous:
                    self.risk.high_risk.append({
                        "interface": name,
                        "method": func_name,
                        "threat_type": threat_type,
                        "dispid": func_desc.memid
                    })
                
                # Parse params
                params = []
                for k in range(func_desc.cParams):
                    param_name = f"param{k}"  # Simplified
                    params.append({
                        "name": param_name,
                        "type": str(func_desc.elemdescParam[k].tdesc.vt)
                    })
                
                methods.append(MethodInfo(
                    name=func_name,
                    dispid=func_desc.memid,
                    params=params,
                    is_dangerous=is_dangerous,
                    is_property=func_desc.invkind in [2, 4],  # INVOKE_PROPERTYGET/PUT
                    inv_kind=str(func_desc.invkind)
                ))
                
            except Exception as e:
                continue
        
        iface = InterfaceInfo(
            name=name,
            iid=iid,
            base=base,
            is_dispatch=is_dispatch,
            is_hidden=is_hidden,
            is_restricted=is_restricted,
            methods=methods,
            coclasses=[]
        )
        
        self.interfaces.append(iface)
        self._assess_risk(iface)

    def _parse_dispatch(self, info, attr):
        """Parse IDispatch interface (dual or disp-only)"""
        self._parse_interface(info, attr)  # Treat as interface for our purposes

    def _parse_coclass(self, info, attr):
        """Parse CoClass (implementation)"""
        name = info.GetDocumentation(-1)[0]
        clsid = str(attr.guid)
        
        self.coclasses.append({
            "name": name,
            "clsid": clsid,
            "threading_model": self._get_threading_model(info, attr)
        })

    def _get_threading_model(self, info, attr):
        """Extract threading model from registry if possible"""
        # Simplified - would check WinReg for ThreadingModel value
        return "Unknown"

    def _check_dangerous(self, name: str) -> tuple:
        """Check method name against forensic patterns"""
        name_lower = name.lower()
        for pattern, threat in self.DANGEROUS_PATTERNS:
            if re.search(pattern, name_lower):
                return True, threat
        return False, None

    def _assess_risk(self, iface: InterfaceInfo):
        """Calculate risk scores"""
        if iface.is_dispatch and not iface.is_restricted:
            self.risk.attack_surface += 10
            
        for m in iface.methods:
            if m.is_dangerous:
                self.risk.attack_surface += 25
            if m.is_property and iface.is_dispatch:
                self.risk.attack_surface += 5

    def _analyze_pe(self, filepath: str) -> Dict:
        """Binary analysis using PE parsing (no COM registration needed)"""
        print(f"[*] Performing binary analysis (PE mode)...")
        pe = pefile.PE(filepath)
        
        # Extract TYPELIB resource if present
        type_libs = []
        
        if hasattr(pe, 'DIRECTORY_ENTRY_RESOURCE'):
            for resource_type in pe.DIRECTORY_ENTRY_RESOURCE.entries:
                if resource_type.id == 2:  # RT_RCDATA or check name
                    # Look for TYPELIB (usually type ID 2)
                    for entry in resource_type.directory.entries:
                        try:
                            data_rva = entry.directory.entries[0].data.struct.OffsetToData
                            size = entry.directory.entries[0].data.struct.Size
                            data = pe.get_memory_mapped_image()[data_rva:data_rva+size]
                            
                            if data[:4] == b'MSFT':  # TypeLib magic
                                type_libs.append(self._parse_typelib_bytes(data))
                        except:
                            continue
        
        pe.close()
        
        if not type_libs:
            return {"error": "No TypeLib resources found in PE", "interfaces": [], "coclasses": []}
        
        # Merge results from all TypeLibs found
        for tl in type_libs:
            self.interfaces.extend(tl.get('interfaces', []))
            self.coclasses.extend(tl.get('coclasses', []))
        
        return self._generate_report()

    def _parse_typelib_bytes(self, data: bytes) -> Dict:
        """Parse raw TypeLib bytes (simplified MSFT format parser)"""
        # This is a basic parser for forensic extraction
        # Full MSFT format is complex, this extracts GUIDs and names
        
        results = {"interfaces": [], "coclasses": []}
        
        # Extract GUIDs (16 bytes each, look for valid patterns)
        guid_pattern = rb'([\x00-\xFF]{16})'
        potential_guids = re.findall(guid_pattern, data)
        
        # Look for strings (method names)
        strings = re.findall(rb'([A-Za-z_][A-Za-z0-9_]{2,32})\x00', data)
        
        # Heuristic: If we found strings that look like COM methods, report them
        com_indicators = [b'QueryInterface', b'AddRef', b'Release', b'IDispatch', b'IUnknown']
        if any(ind in data for ind in com_indicators):
            results["interfaces"].append({
                "name": "ExtractedInterface",
                "iid": "Unknown (Heuristic analysis)",
                "methods": [{"name": s.decode('ascii', errors='ignore')} for s in strings[:20]],
                "note": "Binary extraction - analysis limited"
            })
        
        return results

    def _generate_report(self) -> Dict:
        """Compile final forensic report"""
        return {
            "metadata": {
                "analyzer_version": "2.0",
                "total_interfaces": len(self.interfaces),
                "total_coclasses": len(self.coclasses)
            },
            "risk_assessment": self.risk.to_dict(),
            "interfaces": [asdict(i) if isinstance(i, InterfaceInfo) else i for i in self.interfaces],
            "coclasses": self.coclasses,
            "forensic_indicators": self._extract_indicators()
        }

    def _extract_indicators(self) -> List[Dict]:
        """Extract IoCs (Indicators of Compromise)"""
        indicators = []
        
        # Scriptable interfaces that are hidden (classic COM hijacking pattern)
        for iface in self.interfaces:
            if isinstance(iface, InterfaceInfo):
                if iface.is_hidden and iface.is_dispatch:
                    indicators.append({
                        "type": "suspicious_hidden_dispatch",
                        "confidence": "high",
                        "description": f"Hidden scriptable interface {iface.name} - potential persistence vector"
                    })
        
        return indicators


def print_report(report: Dict):
    """Formatted console output"""
    risk = report.get('risk_assessment', {})
    
    print("\n" + "="*60)
    print("COM INTERFACE FORENSIC REPORT")
    print("="*60)
    
    print(f"\n[ATTACK SURFACE ANALYSIS]")
    print(f"  Risk Level: {get_risk_level(risk.get('attack_surface_score', 0))}")
    print(f"  Attack Surface Score: {risk.get('attack_surface_score', 0)}/500")
    print(f"  High Risk Methods: {len(risk.get('high_risk_findings', []))}")
    print(f"  Scriptable Interfaces: {len(risk.get('scriptable_interfaces', []))}")
    print(f"  Hidden Objects: {len(risk.get('hidden_objects', []))}")
    
    if risk.get('high_risk_findings'):
        print(f"\n[!] HIGH RISK METHODS DETECTED:")
        for finding in risk['high_risk_findings']:
            print(f"    - {finding['interface']}::{finding['method']} [{finding['threat_type']}]")
    
    if risk.get('hidden_objects'):
        print(f"\n[?] HIDDEN INTERFACES (Persistence/Stealth):")
        for obj in risk['hidden_objects']:
            print(f"    - {obj['name']} ({obj['iid']})")
    
    interfaces = report.get('interfaces', [])
    print(f"\n[STRUCTURE]")
    print(f"  Interfaces analyzed: {len(interfaces)}")
    print(f"  CoClasses found: {len(report.get('coclasses', []))}")
    
    for iface in interfaces[:5]:  # Show first 5
        name = iface.get('name', 'Unknown')
        iid = iface.get('iid', 'Unknown')
        methods = len(iface.get('methods', []))
        print(f"    + {name} ({methods} methods)")
    
    if len(interfaces) > 5:
        print(f"    ... and {len(interfaces)-5} more")
    
    print("\n" + "="*60)

def get_risk_level(score: int) -> str:
    if score > 200: return "CRITICAL"
    if score > 100: return "HIGH" 
    if score > 50: return "MEDIUM"
    return "LOW"

def main():
    parser = argparse.ArgumentParser(description='COM Interface Forensic Analyzer')
    parser.add_argument('file', help='Path to DLL/TLB/EXE')
    parser.add_argument('-o', '--output', help='Output JSON file')
    parser.add_argument('-m', '--mode', choices=['auto', 'com', 'pe'], default='auto',
                       help='Analysis mode: com=comtypes API, pe=binary parsing, auto=best available')
    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    
    args = parser.parse_args()
    
    try:
        analyzer = COMForensicAnalyzer()
        report = analyzer.analyze_file(args.file, mode=args.mode)
        
        print_report(report)
        
        if args.output:
            with open(args.output, 'w') as f:
                json.dump(report, f, indent=2, default=str)
            print(f"\n[+] Report saved to: {args.output}")
            
    except ImportError as e:
        print(f"[!] Missing dependency: {e}")
        print("[*] Install with: pip install comtypes pefile")
        sys.exit(1)
    except Exception as e:
        print(f"[!] Fatal error: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
