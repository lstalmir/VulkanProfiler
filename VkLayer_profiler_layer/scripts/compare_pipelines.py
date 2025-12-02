# Copyright (c) 2025 Lukasz Stalmirski
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

import pathlib
import json

def extract_pipelines(file_name):
    with open(file_name) as f:
        data = json.load(f)
    pipelines = {}
    current_pipeline = None
    current_pipeline_name = ''
    for event in data['traceEvents']:
        if event['cat'] == 'Pipelines':
            if event['ph'] == 'B':
                if current_pipeline:
                    print(f'warn: skipping {event['name']} nested pipeline')
                    continue
                current_pipeline = event
                current_pipeline_name = event['name']
                continue
            if event['ph'] == 'E':
                if event['name'] != current_pipeline_name:
                    continue
                pipeline_time = event['ts'] - current_pipeline['ts']
                pipelines[current_pipeline_name] = pipelines.get(current_pipeline_name, 0) + pipeline_time
                current_pipeline = None
                continue
    return pipelines

def compare_pipelines(trace_a, trace_b, name_a = None, name_b = None):
    pipelines_a = extract_pipelines(trace_a)
    pipelines_b = extract_pipelines(trace_b)
    if not name_a:
        name_a = pathlib.Path(trace_a).stem
    if not name_b:
        name_b = pathlib.Path(trace_b).stem
    print(f'Pipeline,{name_a},{name_b},Delta,Delta %')
    for pipeline_name in pipelines_a.keys():
        try:
            pipeline_time_a = pipelines_a[pipeline_name]
            if pipeline_time_a == 0:
                continue
            pipeline_time_b = pipelines_b[pipeline_name]
            delta_time = pipeline_time_b - pipeline_time_a
            delta_percent = delta_time / pipeline_time_a
            print(f'{pipeline_name.replace(',', ';')},{pipeline_time_a},{pipeline_time_b},{delta_time},{delta_percent}')
        except KeyError:
            pass

    for pipeline_name in (pipelines_a.keys() - pipelines_b.keys()):
        pipeline_time_a = pipelines_a[pipeline_name]
        if pipeline_time_a == 0:
            continue
        print(f'{pipeline_name.replace(',', ';')},{pipeline_time_a},0,-{pipeline_time_a},-1')

    for pipeline_name in (pipelines_b.keys() - pipelines_a.keys()):
        pipeline_time_b = pipelines_b[pipeline_name]
        if pipeline_time_b == 0:
            continue
        print(f'{pipeline_name.replace(',', ';')},0,{pipeline_time_b},{pipeline_time_b},1')

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('trace_a', help='Path to the first trace')
    parser.add_argument('trace_b', help='Path to the second trace')
    parser.add_argument('--name_a', help='Name of the first column in the output file', required=False, default=None, dest='name_a')
    parser.add_argument('--name_b', help='Name of the second column in the output file', required=False, default=None, dest='name_b')
    args = parser.parse_args()
    compare_pipelines(args.trace_a, args.trace_b, args.name_a, args.name_b)
