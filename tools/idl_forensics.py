#!/usr/bin/env python3

# COM IDL Forensic Analyzer v3.1 - Robust Edition
# Handles Microsoft SDK IDLs (MsHTML, Shell, etc.) with complex preprocessing
#
# Generate forensic XML + XSLT viewer
#     python idl_forensics.py shell32.idl -o report.xml --xslt
#
# C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\umMsHTML.Idl
#
# View in browser
#     firefox report.xml  # Renders via XSLT

import sys
import re
import uuid
import struct
import hashlib
import argparse
import xml.etree.ElementTree as ET
from xml.dom import minidom
from datetime import datetime, timezone
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple, Set, Any
from enum import IntEnum


class GUIDVersion(IntEnum):
    TIME_BASED = 1
    DCE_SECURITY = 2
    NAME_BASED_MD5 = 3
    RANDOM = 4
    NAME_BASED_SHA1 = 5


@dataclass
class Parameter:
    name: str
    type: str
    direction: str
    attributes: List[str] = field(default_factory=list)
    is_string: bool = False
    is_interface: bool = False
    is_variant: bool = False
    size_is: Optional[str] = None
    
    def to_xml(self, parent: ET.Element):
        elem = ET.SubElement(parent, "parameter", {
            "name": self.name,
            "type": self.type,
            "direction": self.direction,
            "is_string": str(self.is_string).lower(),
            "is_interface": str(self.is_interface).lower()
        })
        if self.size_is:
            elem.set("size_is", self.size_is)
        for attr in self.attributes:
            ET.SubElement(elem, "attribute").text = attr
        return elem


@dataclass 
class Method:
    name: str
    ret_type: str
    dispid: Optional[int] = None
    params: List[Parameter] = field(default_factory=list)
    attributes: List[str] = field(default_factory=list)
    vtable_slot: int = -1
    is_dangerous: bool = False
    threat_category: str = ""
    is_propget: bool = False
    is_propput: bool = False
    doc_string: str = ""
    
    def to_xml(self, parent: ET.Element, idx: int):
        elem = ET.SubElement(parent, "method", {
            "name": self.name,
            "index": str(idx),
            "vtable_slot": str(self.vtable_slot),
            "is_dangerous": str(self.is_dangerous).lower(),
            "threat_category": self.threat_category if self.is_dangerous else "none"
        })
        if self.dispid is not None:
            elem.set("dispid", str(self.dispid))
        
        sig = ET.SubElement(elem, "signature")
        sig.set("return_type", self.ret_type)
        params_elem = ET.SubElement(sig, "parameters")
        for param in self.params:
            param.to_xml(params_elem)
            
        if self.attributes:
            attrs = ET.SubElement(elem, "attributes")
            for attr in self.attributes:
                ET.SubElement(attrs, "attr").text = attr
                
        return elem


@dataclass
class Interface:
    name: str
    iid: str
    base: str
    methods: List[Method] = field(default_factory=list)
    attributes: List[str] = field(default_factory=list)
    is_dispatch: bool = False
    is_dual: bool = False
    is_hidden: bool = False
    is_restricted: bool = False
    is_ole_automation: bool = False
    is_dispinterface: bool = False  # New: dispinterface flag
    doc_string: str = ""
    definition_start: int = 0
    definition_end: int = 0
    
    def to_xml(self, parent: ET.Element):
        elem = ET.SubElement(parent, "interface", {
            "name": self.name,
            "iid": self.iid,
            "base_interface": self.base,
            "line_start": str(self.definition_start),
            "line_end": str(self.definition_end),
            "is_dispatch": str(self.is_dispatch).lower(),
            "is_dispinterface": str(self.is_dispinterface).lower(),
            "is_dual": str(self.is_dual).lower(),
            "is_hidden": str(self.is_hidden).lower(),
            "is_restricted": str(self.is_restricted).lower(),
            "is_ole_automation": str(self.is_ole_automation).lower()
        })
        
        guid_elem = ET.SubElement(elem, "guid_analysis")
        self._analyze_guid(guid_elem)
        
        vtable = ET.SubElement(elem, "vtable")
        vtable.set("entry_count", str(len(self.methods) + 3))
        vtable.set("size_bytes", str((len(self.methods) + 3) * 8))
        
        for i, name in enumerate(["QueryInterface", "AddRef", "Release"]):
            slot = ET.SubElement(vtable, "slot", {
                "index": str(i),
                "name": name,
                "offset": hex(i * 8),
                "inherited": "true"
            })
        
        for idx, method in enumerate(self.methods):
            method.vtable_slot = idx + 3
            method.to_xml(vtable, idx)
            
        return elem
    
    def _analyze_guid(self, parent: ET.Element):
        try:
            guid = uuid.UUID(self.iid)
            bytes_le = guid.bytes_le
            version = (bytes_le[6] & 0xF0) >> 4
            variant = (bytes_le[8] & 0xC0) >> 6
            
            ver_elem = ET.SubElement(parent, "version")
            ver_elem.set("type", str(version))
            ver_elem.text = self._get_guid_version_name(version)
            
            if version == GUIDVersion.TIME_BASED:
                low = struct.unpack_from("<I", bytes_le, 0)[0]
                mid = struct.unpack_from("<H", bytes_le, 4)[0]
                hi = struct.unpack_from("<H", bytes_le, 6)[0] & 0x0FFF
                
                timestamp = ((hi << 48) | (mid << 32) | low)
                unix_timestamp = (timestamp - 0x01b21dd213814000) / 10000000
                dt = datetime.utcfromtimestamp(unix_timestamp)
                
                time_elem = ET.SubElement(parent, "timestamp")
                time_elem.set("generated", dt.isoformat())
                time_elem.set("forensics_note", "Time-based GUID reveals component creation time")
                
        except Exception as e:
            ET.SubElement(parent, "parse_error").text = str(e)
    
    def _get_guid_version_name(self, v: int) -> str:
        names = {1: "time_based", 2: "dce_security", 3: "name_based_md5", 
                4: "random", 5: "name_based_sha1"}
        return names.get(v, "unknown")


@dataclass
class CoClass:
    name: str
    clsid: str
    interfaces: List[Tuple[str, str]] = field(default_factory=list)
    threading_model: str = "apartment"
    is_control: bool = False
    is_appobject: bool = False
    is_can_create: bool = True
    
    def to_xml(self, parent: ET.Element):
        elem = ET.SubElement(parent, "coclass", {
            "name": self.name,
            "clsid": self.clsid,
            "threading_model": self.threading_model,
            "can_create": str(self.is_can_create).lower()
        })
        
        for iface_name, role in self.interfaces:
            ET.SubElement(elem, "interface_ref", {
                "name": iface_name,
                "role": role
            })
        return elem


@dataclass
class EnumDef:
    name: str
    values: Dict[str, int] = field(default_factory=dict)
    
    def to_xml(self, parent: ET.Element):
        elem = ET.SubElement(parent, "enum", {"name": self.name})
        for name, val in self.values.items():
            ET.SubElement(elem, "member", {"name": name, "value": str(val)})


class IDLForensicParser:
    DANGEROUS_PATTERNS = [
        (r'(?i)^(?:Exec|Execute|Shell|WinExec|CreateProcess|Spawn)', "CODE_EXECUTION"),
        (r'(?i)(?:Write|Delete|Remove|Clear|Erase|Unlink|RmDir)(?:File|Dir|Path)?$', "DATA_DESTRUCTION"),
        (r'(?i)(?:Save|Load|Import|Export|Read|Open)(?:File|Url|Path|Stream)?$', "FILE_OPERATION"),
        (r'(?i)(?:RegSet|RegDelete|RegCreate|CreateKey|DeleteKey)', "REGISTRY_MANIPULATION"),
        (r'(?i)(?:Navigate|GoHome|Navigate2|Refresh)$', "URL_NAVIGATION"),
        (r'(?i)(?:CreateObject|GetObject|CoCreateInstance|Activate)$', "OBJECT_INSTANTIATION"),
        (r'(?i)(?:Alloc|Free|Memcpy|Strcpy|Strcat|Realloc)$', "MEMORY_MANIPULATION"),
        (r'(?i)(?:Invoke|Call|Eval|ExecScript)$', "SCRIPT_EXECUTION")
    ]
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.interfaces: List[Interface] = []
        self.coclasses: List[CoClass] = []
        self.enums: List[EnumDef] = []
        self.typedefs: Dict[str, Any] = {}
        self.imports: List[str] = []
        self.pragmas: List[str] = []
        self.raw_discoveries: List[Dict] = []
        self.debug_stats = {
            "lines_processed": 0,
            "blocks_found": 0,
            "regex_attempts": 0
        }
        
    def log(self, msg: str):
        if self.verbose:
            print(f"[DEBUG] {msg}")
        
    def parse_file(self, filepath: str) -> 'IDLForensicParser':
        path = Path(filepath)
        if not path.exists():
            raise FileNotFoundError(f"IDL file not found: {filepath}")
            
        self.source_path = str(path.resolve())
        self.source_hash = hashlib.sha256(path.read_bytes()).hexdigest()
        self.source_size = path.stat().st_size
        
        content = path.read_text(encoding='utf-8', errors='ignore')
        return self.parse_content(content)
    
    def parse_content(self, content: str) -> 'IDLForensicParser':
        self.lines = content.split('\n')
        self.content = content
        self.debug_stats["lines_processed"] = len(self.lines)
        
        # Preprocess: handle continuation lines and remove comments
        clean_content = self._preprocess(content)
        
        # Parse various IDL elements
        self._parse_imports(clean_content)
        self._parse_forward_decls(clean_content)  # New
        self._parse_interfaces_robust(clean_content)  # Improved
        self._parse_dispinterfaces(clean_content)  # New
        self._parse_coclasses_robust(clean_content)  # Improved
        self._parse_enums(clean_content)
        self._parse_typedefs(clean_content)
        
        self._analyze_inheritance()
        self._calculate_risk()
        
        return self
    
    def _preprocess(self, content: str) -> str:
        """Advanced preprocessing for Microsoft IDL format"""
        # Remove C++ style comments but track line numbers via placeholder
        lines = content.split('\n')
        result = []
        
        for i, line in enumerate(lines):
            # Handle midl_pragma
            if line.strip().startswith('midl_pragma'):
                result.append(f'// {line}')  # Keep as comment
                continue
                
            # Handle cpp_quote - extract the content
            cpp_match = re.match(r'cpp_quote\("(.+)"\)', line.strip())
            if cpp_match:
                # Keep cpp_quote content as it might contain forward declarations
                result.append(f'/* cpp_quote: {cpp_match.group(1)} */')
                continue
                
            # Remove line comments
            if '//' in line:
                line = line[:line.index('//')]
            result.append(line)
        
        content = '\n'.join(result)
        
        # Remove block comments
        content = re.sub(r'/\*.*?\*/', ' ', content, flags=re.DOTALL)
        
        # Handle line continuation with backslash
        content = re.sub(r'\\\n', ' ', content)
        
        return content
    
    def _parse_imports(self, content: str):
        pattern = r'import\s+"([^"]+)"\s*;'
        self.imports = re.findall(pattern, content)
        self.log(f"Found {len(self.imports)} imports: {self.imports[:5]}...")
    
    def _parse_forward_decls(self, content: str):
        """Parse forward interface declarations to help resolve inheritance"""
        pattern = r'interface\s+(\w+)\s*;'
        self.forward_decls = set(re.findall(pattern, content))
        self.log(f"Forward declarations: {len(self.forward_decls)}")
    
    def _parse_interfaces_robust(self, content: str):
        """Multi-line aware interface parsing for complex SDK IDLs"""
        # Pattern 1: Standard interface with possible multi-line attributes
        # Handles: [uuid(...), object, ...] \n interface Name : Base {
        pattern = r'''
            \[\s*                                      # Opening bracket
            (?:[^\]]*?uuid\s*\(\s*([0-9a-fA-F\-]{36})\s*\)[^\]]*?)  # UUID capture
            (?:[^\]]*object[^\]]*)?                    # Optional object attribute
            (?:[^\]]*oleautomation[^\]]*)?             # Optional oleautomation
            (?:[^\]]*hidden[^\]]*)?                    # Optional hidden
            (?:[^\]]*dual[^\]]*)?                      # Optional dual
            (?:[^\]]*restricted[^\]]*)?                # Optional restricted
            [^\]]*                                     # Any other attributes
            \]\s*                                      # Closing bracket
            (?:\s*|\s*//[^\n]*\s*)                     # Optional comments
            interface\s+                               # Keyword
            (\w+)                                      # Interface name
            \s*:\s*                                    # Inheritance colon
            (\w+)                                      # Base interface
            \s*\{                                      # Opening brace
            ([^{}]*(?:\{[^{}]*\}[^{}]*)*)              # Body (handles nested braces)
            \}                                         # Closing brace
            (?:\s*;)?                                  # Optional semicolon
        '''
        
        matches = list(re.finditer(pattern, content, re.VERBOSE | re.DOTALL | re.IGNORECASE))
        self.log(f"Interface regex matches: {len(matches)}")
        
        for match in matches:
            self.debug_stats["blocks_found"] += 1
            attrs_block = match.group(0).split(']')[0] + ']'  # Extract attributes
            iid = match.group(1) or "00000000-0000-0000-0000-000000000000"
            name = match.group(2)
            base = match.group(3)
            body = match.group(4)
            
            self.log(f"Parsing interface: {name} (IID: {iid}) extends {base}")
            
            # Parse attributes
            is_dispatch = 'dual' in attrs_block.lower() or base == 'IDispatch'
            is_hidden = 'hidden' in attrs_block.lower()
            is_restricted = 'restricted' in attrs_block.lower()
            is_ole = 'oleautomation' in attrs_block.lower()
            
            # Parse methods
            methods = self._parse_methods_robust(body)
            self.log(f"  Found {len(methods)} methods")
            
            iface = Interface(
                name=name,
                iid=iid.upper(),
                base=base,
                methods=methods,
                attributes=[],  # Simplified for now
                is_dispatch=is_dispatch,
                is_dual='dual' in attrs_block.lower(),
                is_hidden=is_hidden,
                is_restricted=is_restricted,
                is_ole_automation=is_ole,
                is_dispinterface=False,
                definition_start=self._get_line_number(match.start()),
                definition_end=self._get_line_number(match.end())
            )
            
            self.interfaces.append(iface)
    
    def _parse_dispinterfaces(self, content: str):
        """Parse dispinterface definitions (dispatch-only interfaces)"""
        pattern = r'''
            \[\s*
            (?:[^\]]*?uuid\s*\(\s*([0-9a-fA-F\-]{36})\s*\)[^\]]*?)
            [^\]]*
            \]\s*
            dispinterface\s+
            (\w+)
            \s*\{
            ([^{}]*(?:\{[^{}]*\}[^{}]*)*)
            \}
        '''
        
        matches = re.finditer(pattern, content, re.VERBOSE | re.DOTALL | re.IGNORECASE)
        count = 0
        
        for match in matches:
            count += 1
            iid = match.group(1) or "00000000-0000-0000-0000-000000000000"
            name = match.group(2)
            body = match.group(3)
            
            self.log(f"Parsing dispinterface: {name}")
            
            # Dispinterfaces have properties and methods listed differently
            methods = self._parse_dispinterface_members(body)
            
            iface = Interface(
                name=name,
                iid=iid.upper(),
                base="IDispatch",
                methods=methods,
                attributes=["dispinterface"],
                is_dispatch=True,
                is_dispinterface=True,
                is_dual=False,
                is_hidden='hidden' in match.group(0).lower(),
                definition_start=self._get_line_number(match.start()),
                definition_end=self._get_line_number(match.end())
            )
            self.interfaces.append(iface)
        
        if count > 0:
            self.log(f"Found {count} dispinterfaces")
    
    def _parse_dispinterface_members(self, body: str) -> List[Method]:
        """Parse dispinterface properties and methods"""
        methods = []
        
        # properties:
        prop_pattern = r'(\w+)\s+(\w+)\s*\(\s*([^\)]*)\s*\)'
        for match in re.finditer(prop_pattern, body):
            ret_type = match.group(1)
            name = match.group(2)
            
            is_dangerous, threat = self._check_dangerous(name)
            methods.append(Method(
                name=name, ret_type=ret_type, is_dangerous=is_dangerous,
                threat_category=threat, is_propget=ret_type != "void"
            ))
        
        # methods:
        method_pattern = r'id\((\w+)\)\s*,\s*\w+\s+(\w+)'
        for match in re.finditer(method_pattern, body):
            name = match.group(2)
            is_dangerous, threat = self._check_dangerous(name)
            methods.append(Method(
                name=name, ret_type="HRESULT", is_dangerous=is_dangerous,
                threat_category=threat
            ))
            
        return methods
    
    def _parse_methods_robust(self, body: str) -> List[Method]:
        """Robust method parsing handling IDL attributes"""
        methods = []
        
        # Split by semicolons but respect braces
        statements = self._split_statements(body)
        
        for stmt in statements:
            stmt = stmt.strip()
            if not stmt:
                continue
                
            # Match method with optional attributes
            # Handles: [id(1)] HRESULT Method([in] Type name);
            method_pattern = r'''
                (?:\[(.*?)\]\s*)?           # Optional attributes
                (HRESULT|void|SCODE|DWORD)\s+  # Return type
                (\w+)                       # Method name
                \s*\(\s*                    # Opening paren
                ([^)]*)                     # Parameters
                \)\s*;?                     # Closing paren and semicolon
            '''
            
            match = re.match(method_pattern, stmt, re.VERBOSE)
            if match:
                attrs = match.group(1) or ""
                ret_type = match.group(2)
                name = match.group(3)
                params_str = match.group(4)
                
                is_dangerous, threat = self._check_dangerous(name)
                
                # Parse parameters
                params = self._parse_parameters(params_str) if params_str.strip() else []
                
                method = Method(
                    name=name,
                    ret_type=ret_type,
                    params=params,
                    is_dangerous=is_dangerous,
                    threat_category=threat,
                    is_propget='propget' in attrs.lower(),
                    is_propput='propput' in attrs.lower(),
                    attributes=[a.strip() for a in attrs.split(',') if a.strip()]
                )
                methods.append(method)
        
        return methods
    
    def _split_statements(self, text: str) -> List[str]:
        """Split IDL statements respecting braces and semicolons"""
        statements = []
        current = []
        depth = 0
        
        for char in text:
            if char == '{':
                depth += 1
            elif char == '}':
                depth -= 1
            elif char == ';' and depth == 0:
                statements.append(''.join(current))
                current = []
                continue
            current.append(char)
        
        if current:
            statements.append(''.join(current))
        return statements
    
    def _parse_parameters(self, params_str: str) -> List[Parameter]:
        params = []
        if not params_str.strip():
            return params
            
        param_parts = self._split_params(params_str)
        
        for part in param_parts:
            part = part.strip()
            if not part:
                continue
                
            directions = []
            if '[in]' in part:
                directions.append('in')
            if '[out]' in part:
                directions.append('out')
            if '[retval]' in part:
                directions.append('retval')
                
            direction = 'in' if 'in' in directions else 'out' if 'out' in directions else 'inout'
            
            clean = re.sub(r'\[.*?\]', '', part).strip()
            
            is_ptr = '*' in clean
            is_string = any(s in clean for s in ['BSTR', 'LPSTR', 'LPWSTR', 'LPCSTR'])
            is_interface = re.search(r'\bI\w+\b', clean) is not None
            is_variant = 'VARIANT' in clean
            
            size_is = None
            match = re.search(r'\[size_is\(([^)]+)\)\]', part)
            if match:
                size_is = match.group(1)
            
            tokens = clean.replace('*', ' ').replace('[', ' ').replace(']', ' ').split()
            if len(tokens) >= 2:
                param_name = tokens[-1]
                param_type = ' '.join(tokens[:-1])
            else:
                param_name = "unnamed"
                param_type = clean
                
            param = Parameter(
                name=param_name,
                type=param_type,
                direction=direction,
                is_string=is_string,
                is_interface=is_interface,
                is_variant=is_variant,
                size_is=size_is
            )
            params.append(param)
            
        return params
    
    def _split_params(self, s: str) -> List[str]:
        result = []
        depth = 0
        current = []
        
        for char in s:
            if char == '(' or char == '[' or char == '<':
                depth += 1
            elif char == ')' or char == ']' or char == '>':
                depth -= 1
            elif char == ',' and depth == 0:
                result.append(''.join(current))
                current = []
                continue
            current.append(char)
            
        if current:
            result.append(''.join(current))
        return result
    
    def _parse_coclasses_robust(self, content: str):
        """Robust coclass parsing with multi-line support"""
        pattern = r'''
            \[\s*
            (?:[^\]]*?uuid\s*\(\s*([0-9a-fA-F\-]{36})\s*\)[^\]]*?)
            [^\]]*
            \]\s*
            coclass\s+
            (\w+)
            \s*\{
            ([^{}]*(?:\{[^{}]*\}[^{}]*)*)
            \}
            (?:\s*;)?
        '''
        
        matches = re.finditer(pattern, content, re.VERBOSE | re.DOTALL | re.IGNORECASE)
        count = 0
        
        for match in matches:
            count += 1
            attrs = match.group(0).split(']')[0] + ']'
            clsid = match.group(1) or "00000000-0000-0000-0000-000000000000"
            name = match.group(2)
            body = match.group(3)
            
            self.log(f"Parsing coclass: {name}")
            
            # Parse implemented interfaces
            interfaces = []
            
            # Pattern for interface references: [attributes] interface Name;
            iface_ref_pattern = r'''
                (?:\[(.*?)\])?
                \s*interface\s+
                (\w+)
                \s*;
            '''
            
            for ref_match in re.finditer(iface_ref_pattern, body, re.VERBOSE):
                attr_str = ref_match.group(1) or ""
                iface_name = ref_match.group(2)
                
                role = "default"
                if 'source' in attr_str:
                    role = "source"
                elif 'default' in attr_str:
                    role = "default"
                if 'restricted' in attr_str:
                    role += "_restricted"
                    
                interfaces.append((iface_name, role))
            
            # Threading model
            threading = "apartment"
            if 'threading("both")' in attrs:
                threading = "both"
            elif 'threading("free")' in attrs:
                threading = "free"
            elif 'threading("neutral")' in attrs:
                threading = "neutral"
                
            coclass = CoClass(
                name=name,
                clsid=clsid.upper(),
                interfaces=interfaces,
                threading_model=threading,
                is_control='control' in attrs.lower(),
                is_appobject='appobject' in attrs.lower(),
                is_can_create='noncreatable' not in attrs.lower()
            )
            self.coclasses.append(coclass)
        
        if count > 0:
            self.log(f"Found {count} coclasses")
    
    def _parse_enums(self, content: str):
        # ... (same as before)
        pattern = r'typedef\s+enum\s+(?:\w+\s*)?\{([^}]+)\}\s*(\w+)\s*;'
        for match in re.finditer(pattern, content):
            body = match.group(1)
            name = match.group(2)
            
            values = {}
            current_val = 0
            
            for line in body.split(','):
                line = line.strip()
                if '=' in line:
                    name_part, val_part = line.split('=', 1)
                    try:
                        current_val = int(val_part.strip())
                    except:
                        pass
                    values[name_part.strip()] = current_val
                elif line:
                    values[line] = current_val
                current_val += 1
                
            self.enums.append(EnumDef(name=name, values=values))
    
    def _parse_typedefs(self, content: str):
        pattern = r'typedef\s+(\w+(?:\s*\*)?)\s+(\w+)\s*;'
        for match in re.finditer(pattern, content):
            base = match.group(1)
            name = match.group(2)
            self.typedefs[name] = {"base": base, "is_ptr": '*' in base}
    
    def _get_line_number(self, pos: int) -> int:
        return self.content[:pos].count('\n') + 1
    
    def _check_dangerous(self, name: str) -> tuple:
        name_lower = name.lower()
        for pattern, threat in self.DANGEROUS_PATTERNS:
            if re.search(pattern, name_lower):
                return True, threat
        return False, None
    
    def _analyze_inheritance(self):
        iface_map = {i.name: i for i in self.interfaces}
        
        for iface in self.interfaces:
            if iface.base in iface_map:
                base_iface = iface_map[iface.base]
                for method in iface.methods:
                    if any(bm.name == method.name and bm.is_dangerous 
                          for bm in base_iface.methods):
                        method.is_dangerous = True
                        method.threat_category = f"INHERITED_{base_iface.name}"
    
    def _calculate_risk(self):
        for iface in self.interfaces:
            if iface.is_hidden and (iface.is_dispatch or iface.is_dispinterface):
                self.raw_discoveries.append({
                    "type": "SUSPICIOUS_HIDDEN_DISPATCH",
                    "entity": iface.name,
                    "iid": iface.iid,
                    "description": "Hidden interface exposed to script engines",
                    "severity": "HIGH"
                })
            
            dangerous_count = sum(1 for m in iface.methods if m.is_dangerous)
            if dangerous_count > 0:
                self.raw_discoveries.append({
                    "type": "DANGEROUS_METHODS",
                    "entity": iface.name,
                    "count": dangerous_count,
                    "methods": [m.name for m in iface.methods if m.is_dangerous],
                    "severity": "CRITICAL" if dangerous_count > 2 else "HIGH"
                })
    
    def to_xml(self) -> ET.Element:
        root = ET.Element("com-forensics-report")
        root.set("generated", datetime.now(timezone.utc).isoformat())
        root.set("version", "3.1")
        root.set("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance")
        
        # Metadata
        meta = ET.SubElement(root, "metadata")
        ET.SubElement(meta, "source").text = getattr(self, 'source_path', 'memory')
        ET.SubElement(meta, "file_hash", {"algorithm": "SHA-256"}).text = getattr(self, 'source_hash', 'unknown')
        ET.SubElement(meta, "file_size").text = str(getattr(self, 'source_size', 0))
        ET.SubElement(meta, "parser_version").text = "3.1.0-forensic"
        
        # Debug info
        debug = ET.SubElement(meta, "parsing_debug")
        for key, val in self.debug_stats.items():
            ET.SubElement(debug, key.replace('_', '-')).text = str(val)
        
        # Statistics
        stats = ET.SubElement(root, "statistics")
        ET.SubElement(stats, "interfaces", {"count": str(len(self.interfaces))})
        ET.SubElement(stats, "coclasses", {"count": str(len(self.coclasses))})
        ET.SubElement(stats, "enums", {"count": str(len(self.enums))})
        ET.SubElement(stats, "imports", {"count": str(len(self.imports))})
        
        total_methods = sum(len(i.methods) for i in self.interfaces)
        dangerous_methods = sum(1 for i in self.interfaces for m in i.methods if m.is_dangerous)
        dispinterfaces = sum(1 for i in self.interfaces if i.is_dispinterface)
        
        ET.SubElement(stats, "methods", {
            "total": str(total_methods),
            "dangerous": str(dangerous_methods),
            "dispinterfaces": str(dispinterfaces)
        })
        
        # Risk Assessment
        risk = ET.SubElement(root, "risk_assessment")
        risk.set("score", self._calculate_total_risk())
        risk.set("level", self._get_risk_level())
        
        if self.raw_discoveries:
            discoveries = ET.SubElement(risk, "discoveries")
            for disc in self.raw_discoveries:
                elem = ET.SubElement(discoveries, "finding", {
                    "type": disc["type"],
                    "severity": disc["severity"],
                    "entity": disc.get("entity", "unknown")
                })
                if "iid" in disc:
                    elem.set("iid", disc["iid"])
                if "description" in disc:
                    desc = ET.SubElement(elem, "description")
                    desc.text = disc["description"]
                if "methods" in disc:
                    methods = ET.SubElement(elem, "methods")
                    for m in disc["methods"]:
                        ET.SubElement(methods, "method").text = m
        
        # Type Library Structure
        types = ET.SubElement(root, "type_library")
        
        if self.imports:
            imports_elem = ET.SubElement(types, "dependencies")
            for imp in self.imports:
                ET.SubElement(imports_elem, "import").text = imp
        
        ifaces = ET.SubElement(types, "interfaces")
        for iface in self.interfaces:
            iface.to_xml(ifaces)
            
        coclasses = ET.SubElement(types, "coclasses")
        for coclass in self.coclasses:
            coclass.to_xml(coclasses)
            
        if self.enums:
            enums = ET.SubElement(types, "enumerations")
            for enum in self.enums:
                enum.to_xml(enums)
        
        # Raw Analysis
        raw = ET.SubElement(root, "raw_analysis")
        preview = self.content[:2000] if hasattr(self, 'content') else ""
        ET.SubElement(raw, "content_preview").text = preview
        
        return root
    
    def _calculate_total_risk(self) -> str:
        score = 0
        for iface in self.interfaces:
            if iface.is_dispatch or iface.is_dispinterface: 
                score += 10
            if iface.is_hidden: 
                score += 20
            for m in iface.methods:
                if m.is_dangerous: 
                    score += 25
        return str(min(score, 500))
    
    def _get_risk_level(self) -> str:
        score = int(self._calculate_total_risk())
        if score > 200: 
            return "CRITICAL"
        if score > 100: 
            return "HIGH"
        if score > 50: 
            return "MEDIUM"
        return "LOW"
    
    def write_xml(self, output_path: str, pretty: bool = True):
        root = self.to_xml()
        
        if pretty:
            rough_string = ET.tostring(root, encoding='unicode')
            reparsed = minidom.parseString(rough_string)
            pretty_xml = reparsed.toprettyxml(indent="  ")
            lines = [line for line in pretty_xml.split('\n') if line.strip()]
            output = '\n'.join(lines)
        else:
            output = ET.tostring(root, encoding='unicode')
            
        Path(output_path).write_text(output, encoding='utf-8')
        
        self._generate_xslt(Path(output_path).with_suffix('.xsl'))
        return output_path
    
    def _generate_xslt(self, path: Path):
        xsl = '''<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:template match="/">
<html>
<head>
<title>COM Forensic Report</title>
<style>
body { font-family: 'Segoe UI', sans-serif; background: #0a0a0f; color: #e0e0ff; margin: 2rem; }
.header { border-bottom: 2px solid #00f0ff; padding-bottom: 1rem; margin-bottom: 2rem; }
.risk-critical { color: #ff004c; font-weight: bold; font-size: 1.2rem; }
.risk-high { color: #ff004c; }
.risk-medium { color: #ffcc00; }
.risk-low { color: #00ff9d; }
.interface { background: #13131f; border: 1px solid #2a2a3f; margin: 1rem 0; padding: 1rem; border-radius: 8px; }
.method { margin: 0.5rem 0; padding: 0.5rem; background: rgba(0,0,0,0.3); border-left: 3px solid #00f0ff; }
.dangerous { border-left-color: #ff004c; background: rgba(255,0,76,0.1); }
.vtable { font-family: monospace; font-size: 0.9rem; background: #0f0f1a; padding: 1rem; overflow-x: auto; }
table { border-collapse: collapse; width: 100%; margin: 1rem 0; }
th, td { text-align: left; padding: 0.5rem; border-bottom: 1px solid #2a2a3f; }
th { color: #00f0ff; background: rgba(0,240,255,0.1); }
.badge { display: inline-block; padding: 2px 8px; border-radius: 4px; font-size: 0.8rem; margin-left: 0.5rem; }
.badge-hidden { background: #ff004c; color: white; }
.badge-dispatch { background: #ffcc00; color: black; }
.badge-dispinterface { background: #00ff9d; color: black; }
td { vertical-align: top; }
</style>
</head>
<body>
<div class="header">
<h1>🔍 COM Interface Forensic Analysis</h1>
<p>Generated: <xsl:value-of select="com-forensics-report/@generated"/></p>
<p>Risk Level: <span class="risk-{translate(risk_assessment/@level, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')}">
<xsl:value-of select="risk_assessment/@level"/>
</span> (Score: <xsl:value-of select="risk_assessment/@score"/>)
</p>
<p>Interfaces: <xsl:value-of select="statistics/interfaces/@count"/> | 
   CoClasses: <xsl:value-of select="statistics/coclasses/@count"/> |
   Methods: <xsl:value-of select="statistics/methods/@total"/>
</p>
</div>

<xsl:if test="risk_assessment/discoveries/finding">
<div class="section">
<h2>Critical Findings</h2>
<xsl:for-each select="risk_assessment/discoveries/finding[@severity='CRITICAL' or @severity='HIGH']">
<div class="interface" style="border-color: #ff004c; margin-bottom: 1rem;">
<strong class="risk-high">[<xsl:value-of select="@severity"/>] <xsl:value-of select="@type"/></strong><br/>
Entity: <xsl:value-of select="@entity"/>
<xsl:if test="@iid"> (<code><xsl:value-of select="@iid"/></code>)</xsl:if><br/>
<xsl:if test="description"><xsl:value-of select="description"/><br/></xsl:if>
<xsl:if test="methods">
Methods: <code style="background: rgba(255,0,76,0.2); padding: 2px 4px; border-radius: 4px;">
<xsl:for-each select="methods/method"><xsl:value-of select="."/><xsl:if test="not(position() = last())">, </xsl:if></xsl:for-each>
</code>
</xsl:if>
</div>
</xsl:for-each>
</div>
</xsl:if>

<div class="section">
<h2>Interface Analysis</h2>
<xsl:for-each select="type_library/interfaces/interface">
<div class="interface">
<h3>
<xsl:value-of select="@name"/>
<xsl:if test="@is_hidden='true'"><span class="badge badge-hidden">HIDDEN</span></xsl:if>
<xsl:if test="@is_dispatch='true'"><span class="badge badge-dispatch">DISPATCH</span></xsl:if>
<xsl:if test="@is_dispinterface='true'"><span class="badge badge-dispinterface">DISPIFACE</span></xsl:if>
</h3>
<p>IID: <code style="font-size: 1.1rem; color: #00f0ff;"><xsl:value-of select="@iid"/></code> | 
   Base: <xsl:value-of select="@base_interface"/> |
   Lines: <xsl:value-of select="@line_start"/>-<xsl:value-of select="@line_end"/></p>

<div class="vtable">
<table>
<tr><th>Slot</th><th>Method</th><th>Parameters</th><th>Risk</th></tr>
<xsl:for-each select="vtable/slot[@inherited='true']">
<tr style="opacity: 0.7;">
<td><xsl:value-of select="@index"/></td>
<td><xsl:value-of select="@name"/></td>
<td colspan="2">Inherited from IUnknown</td>
</tr>
</xsl:for-each>
<xsl:for-each select="vtable/method">
<tr>
<td><xsl:value-of select="@vtable_slot"/></td>
<td><code><xsl:value-of select="@name"/></code></td>
<td>
<xsl:if test="signature/parameters/parameter">
<xsl:for-each select="signature/parameters/parameter">
<div style="font-size: 0.85rem; opacity: 0.9;">
<span style="color: #ffcc00;">[<xsl:value-of select="@direction"/>]</span>
<xsl:value-of select="@type"/> 
<strong><xsl:value-of select="@name"/></strong>
<xsl:if test="@is_string='true'"> <span style="color: #00ff9d;">[string]</span></xsl:if>
</div>
</xsl:for-each>
</xsl:if>
</td>
<td>
<xsl:if test="@is_dangerous='true'"><span class="risk-high">⚠️ <xsl:value-of select="@threat_category"/></span></xsl:if>
</td>
</tr>
</xsl:for-each>
</table>
</div>
</div>
</xsl:for-each>
</div>

<xsl:if test="type_library/coclasses/coclass">
<div class="section">
<h2>CoClass Implementations</h2>
<table>
<tr><th>Name</th><th>CLSID</th><th>Threading</th><th>Interfaces</th></tr>
<xsl:for-each select="type_library/coclasses/coclass">
<tr>
<td><strong><xsl:value-of select="@name"/></strong></td>
<td><code><xsl:value-of select="@clsid"/></code></td>
<td><xsl:value-of select="@threading_model"/></td>
<td>
<xsl:for-each select="interface_ref">
<div style="font-size: 0.85rem;">
<xsl:value-of select="@name"/>
<xsl:if test="@role != 'default'"> (<xsl:value-of select="@role"/>)</xsl:if>
</div>
</xsl:for-each>
</td>
</tr>
</xsl:for-each>
</table>
</div>
</xsl:if>

<div style="margin-top: 3rem; padding-top: 1rem; border-top: 1px solid #2a2a3f; font-size: 0.85rem; opacity: 0.7; text-align: center;">
Report generated by COM IDL Forensic Analyzer v3.1<br/>
<xsl:value-of select="metadata/file_hash/@algorithm"/>: <xsl:value-of select="metadata/file_hash"/>
</div>
</body>
</html>
</xsl:template>
</xsl:stylesheet>'''
        path.write_text(xsl, encoding='utf-8')
        print(f"[*] Generated XSLT: {path}")


def main():
    parser = argparse.ArgumentParser(description='COM IDL Forensic Analyzer - XML Report Generator')
    parser.add_argument('input', help='Input IDL file')
    parser.add_argument('-o', '--output', help='Output XML file (default: input.xml)', default=None)
    parser.add_argument('--xslt', action='store_true', help='Generate XSLT stylesheet', default=True)
    parser.add_argument('--compact', action='store_true', help='Compact XML output')
    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose parsing output')
    
    args = parser.parse_args()
    
    try:
        analyzer = IDLForensicParser(verbose=args.verbose)
        analyzer.parse_file(args.input)
        
        output = args.output or Path(args.input).with_suffix('.xml')
        
        print(f"[*] Parsing IDL: {args.input}")
        print(f"[*] File size: {analyzer.source_size:,} bytes")
        print(f"[*] Found {len(analyzer.interfaces)} interfaces, {len(analyzer.coclasses)} coclasses")
        
        if args.verbose:
            print(f"[*] Regex attempts: {analyzer.debug_stats['regex_attempts']}")
            print(f"[*] Blocks detected: {analyzer.debug_stats['blocks_found']}")
        
        if len(analyzer.interfaces) == 0 and len(analyzer.coclasses) == 0:
            print("[!] WARNING: No interfaces or coclasses found!")
            print("[!] This IDL may use constructs not yet supported by the parser")
            print("[!] Try --verbose to see debug information")
        
        analyzer.write_xml(output, pretty=not args.compact)
        
        print(f"[+] XML Report: {output}")
        print(f"[+] Risk: {analyzer._get_risk_level()} (Score: {analyzer._calculate_total_risk()})")
        
        if analyzer.raw_discoveries:
            print(f"[!] {len(analyzer.raw_discoveries)} security findings")
            
    except Exception as e:
        print(f"[!] Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
