#!/usr/bin/env python3
"""Run a Python file inside the Unreal editor/game MCP session (execute_python) and print the result.
usage: python run_editor_py.py <port> <file.py> [--save-images DIR]"""
import json, os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'Tools'))
import td_mcp

def main():
    port = int(sys.argv[1]); path = sys.argv[2]
    src = open(path, encoding='utf-8').read()
    c = td_mcp.McpClient('http://127.0.0.1:%d/mcp' % port, 900)
    c.connect()
    r = c.tool('call_tool', {'toolset_name': 'editor_toolset.toolsets.python_scripting.PythonScriptingTools',
                             'tool_name': 'execute_python',
                             'arguments': {'script': {'content': src, 'language': 'Python'}}})
    for b in r.get('content', []):
        if b.get('type') == 'text':
            t = b['text']
            try:
                j = json.loads(t)
                if isinstance(j, dict) and 'returnValue' in j and isinstance(j['returnValue'], str):
                    print(j['returnValue'])
                else:
                    print(json.dumps(j, indent=1))
            except ValueError:
                print(t)
    return 1 if r.get('isError') else 0

if __name__ == '__main__':
    sys.exit(main())
