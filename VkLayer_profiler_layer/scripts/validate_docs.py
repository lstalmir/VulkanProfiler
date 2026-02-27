# Copyright (c) 2026 Lukasz Stalmirski
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import sys
import os
import re
import json
import argparse

RE_CONFVAL_DEFINITION = re.compile(r'\.\.\s+confval::\s+(?P<name>\w+)$')
RE_CONFVAL_REFERENCE = re.compile(r':confval:`(?P<name>\w+)`( is set to (?:\*\*)?(?P<value>\w+)(?:\*\*)?)?')
RE_CONFVAL_GLOSSARY = re.compile(r'\.\.\s+glossary::$')

def validate_confvals(manifest, docs_root_dir):
    class cval_def_t:
        def __init__(self, name):
            self.name = name
            self.type = None
            self.default = None
            self.glossary = []

        def __repr__(self):
            return f'{{name={self.name}, type={self.type}, default={self.default}, glossary={self.glossary}}}'

    class cval_ref_t:
        def __init__(self, name, value = None):
            self.name = name
            self.value = value

        def __repr__(self):
            return f'{{name={self.name}, value={self.value}}}'

    class state_t:
        def __init__(self):
            self.confval_defs = {}
            self.confval_refs = []
            self.current_confval = None

    class setting_t:
        def __init__(self, entry):
            self.name = entry['key']
            self.env = entry['env']
            self.type = entry['type'].lower()
            self.default = entry['default']
            self.flags = []
            if 'flags' in entry.keys():
                for item in entry['flags']:
                    self.flags.append(item['key'])

        def __repr__(self):
            return f'{{name={self.name}, env={self.env}, type={self.type}, default={self.default}, flags={self.flags}}}'

    def process_line(line, state, f):
        match = RE_CONFVAL_DEFINITION.search(line)
        if match:
            if state.current_confval:
                state.confval_defs[state.current_confval.name] = state.current_confval
            state.current_confval = cval_def_t(match['name'])
            return

        match = RE_CONFVAL_GLOSSARY.search(line)
        if match and state.current_confval:
            glossary_indent = match.span()[0]
            for line in f:
                line_lstrip = line.lstrip()
                if not line_lstrip:
                    continue
                line_indent = len(line) - len(line_lstrip)
                if line_indent <= glossary_indent:
                    process_line(line, state, f)
                    return
                if line_indent == glossary_indent + 4:
                    state.current_confval.glossary.append(line.strip())
            return

        for match in RE_CONFVAL_REFERENCE.finditer(line):
            state.confval_refs.append(cval_ref_t(match['name'], match['value']))

    def read_confvals_from_file(state, file):
        state.current_confval = None
        with open(file) as f:
            for line in f:
                process_line(line, state, f)
        if state.current_confval:
            state.confval_defs[state.current_confval.name] = state.current_confval

    def read_confvals_from_dir(state, dir):
        for entry in os.listdir(dir):
            entry_path = os.path.join(dir, entry)
            if os.path.isdir(entry_path):
                read_confvals_from_dir(state, entry_path)
                continue
            if os.path.isfile(entry_path) and entry.endswith('.rst'):
                read_confvals_from_file(state, entry_path)
                continue

    def read_settings_from_manifest(manifest, settings_dict):
        for setting in manifest:
            s = setting_t(setting)
            settings_dict[s.name] = s
            if 'settings' in setting.keys():
                read_settings_from_manifest(setting['settings'], settings_dict)

    state = state_t()
    read_confvals_from_dir(state, docs_root_dir)

    settings = {}
    read_settings_from_manifest(manifest['layer']['features']['settings'], settings)

    # First, validate that all settings have corresponding env vars defined, and their names are matching.
    for setting in settings.values():
        if setting.env != 'VKPROF_' + setting.name:
            raise Exception(f'Incorrect env variable name found for setting \'{setting.name}\'')

    # Check if all settings are defined as confvals in the documentation.
    for setting in settings.values():
        if not setting.name in state.confval_defs.keys():
            raise Exception(f'Missing documentation for setting \'{setting.name}\'')
    for confval in state.confval_defs:
        if not confval in settings.keys():
            raise Exception(f'Unexpected documentation for setting \'{confval.name}\'')

    # Check if enum settings are correctly defined.
    for setting in settings.values():
        if setting.type == 'enum':
            confval = state.confval_defs[setting.name]
            for value in setting.flags:
                if value not in confval.glossary:
                    raise Exception(f'Missing enum value documentation for setting \'{setting.name}\':\'{value}\'')
            for value in confval.glossary:
                if value not in setting.flags:
                    raise Exception(f'Unexpected enum value documentation for setting \'{setting.name}\':\'{value}\'')

    # Check if all references are valid.
    for ref in state.confval_refs:
        if ref.name not in state.confval_defs.keys():
            raise Exception(f'Reference to an undefined setting \'{ref.name}\'')
        if ref.value:
            confval = state.confval_defs[ref.name]
            if ref.value not in confval.glossary:
                raise Exception(f'Reference to an undefined setting value \'{ref.name}\':\'{ref.value}\'')

if __name__ == "__main__":
    LAYER_JSON_PATH = sys.argv[1]
    LAYER_DOCS_PATH = sys.argv[2]

    # Read layer manifest that defines the configuration vars.
    with open(LAYER_JSON_PATH) as f:
        manifest = json.load(f)

    # Validate the documentation.
    validate_confvals(manifest, LAYER_DOCS_PATH)
