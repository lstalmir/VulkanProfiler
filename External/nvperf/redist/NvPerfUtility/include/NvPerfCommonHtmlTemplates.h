/*
* Copyright 2014-2025 NVIDIA Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#include <string>

namespace nv { namespace perf {

    inline std::string GetReadMeHtml()
    {
        return R"(
<html>

  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>

  <head>
    <title>NvPerfSDK</title>
    <style id="ReportStyle">
      .titlearea {
        display: flex;
        align-items: center;
        color: white;
        font-family: verdana;
      }

      .titlebar {
        margin-left: 0;
        margin-right: auto;
      }

      .title {
        font-size: 28px;
        margin-left: 10px;
      }

      .section {
        border-radius: 15px;
        padding: 10px;
        background: #FFFFFF;
        margin: 10px;
      }

      .section_title {
        font-family: verdana;
        font-weight: bold;
        color: black;
      }

      li {
        white-space: normal;;
      }
    </style>
  </head>

  <body style="background-color:#202020;">
    <div>
      <div class="titlearea">
        <div class="titlebar">
          <svg width="150" height="26" viewBox="0 0 150 26" fill="none" xmlns="http://www.w3.org/2000/svg" class="svelte-gccl40">
            <path d="M140.061 20.8363V20.4478H140.312C140.449 20.4478 140.637 20.4579 140.637 20.624C140.637 20.8045 140.541 20.8363 140.378 20.8363H140.061V20.8363ZM140.061 21.1093H140.229L140.619 21.7896H141.047L140.616 21.0819C140.839 21.066 141.022 20.9605 141.022 20.6615C141.022 20.2918 140.765 20.1719 140.33 20.1719H139.7V21.7882H140.062V21.1079L140.061 21.1093ZM141.894 20.9808C141.894 20.0318 141.15 19.48 140.322 19.48C139.495 19.48 138.747 20.0303 138.747 20.9808C138.747 21.9312 139.489 22.483 140.322 22.483C141.156 22.483 141.894 21.9298 141.894 20.9808ZM141.44 20.9808C141.44 21.6726 140.928 22.1363 140.322 22.1363V22.132C139.7 22.1363 139.198 21.6726 139.198 20.9808C139.198 20.2889 139.701 19.8266 140.322 19.8266C140.944 19.8266 141.44 20.2889 141.44 20.9808Z" fill="white"></path>
            <path d="M84.2106 4.92277V21.9614H89.0584V4.92277H84.2106V4.92277ZM46.0831 4.89966V21.9614H50.9731V8.71733L54.7878 8.73033C56.042 8.73033 56.9106 9.02933 57.5144 9.66921C58.2811 10.481 58.5939 11.7882 58.5939 14.1802V21.9614H63.3311V12.535C63.3311 5.80677 59.0115 4.89966 54.7864 4.89966H46.0831ZM92.0148 4.92277V21.96H99.8757C104.064 21.96 105.431 21.2681 106.91 19.7182C107.955 18.6291 108.63 16.24 108.63 13.6284C108.63 11.2335 108.058 9.09721 107.062 7.76688C105.266 5.38788 102.679 4.92277 98.8166 4.92277H92.0148V4.92277ZM96.8219 8.6321H98.9053C101.929 8.6321 103.884 9.97977 103.884 13.4768C103.884 16.9738 101.929 18.3229 98.9053 18.3229H96.8219V8.6321ZM77.2227 4.92277L73.178 18.4254L69.3021 4.92277H64.0702L69.6062 21.96H76.5912L82.1709 4.92277H77.2241H77.2227ZM110.885 21.96H115.733V4.92277H110.884V21.96H110.885ZM124.473 4.92855L117.704 21.9542H122.484L123.555 18.9454H131.564L132.578 21.9542H137.766L130.947 4.9271H124.473V4.92855ZM127.618 8.03555L130.554 16.0118H124.589L127.618 8.03555Z" fill="white"></path>
            <path d="M14.8242 7.76243V5.41809C15.054 5.4022 15.2854 5.3892 15.5211 5.38198C21.9809 5.17976 26.2191 10.8925 26.2191 10.8925C26.2191 10.8925 21.6419 17.2048 16.7345 17.2048C16.0274 17.2048 15.3945 17.0921 14.8242 16.9014V9.79042C17.3397 10.0923 17.8446 11.1944 19.3562 13.6962L22.7185 10.881C22.7185 10.881 20.2641 7.68442 16.1263 7.68442C15.6767 7.68442 15.2461 7.7162 14.8227 7.76098L14.8242 7.76243ZM14.8227 0.0158691V3.51865C15.054 3.49987 15.2868 3.48543 15.5196 3.47676C24.5023 3.17631 30.3554 10.7914 30.3554 10.7914C30.3554 10.7914 23.6337 18.9063 16.6297 18.9063C15.9881 18.9063 15.3872 18.8471 14.8227 18.7489V20.9141C15.3057 20.9748 15.8062 21.0109 16.3271 21.0109C22.8437 21.0109 27.5576 17.7074 32.1217 13.7959C32.8782 14.3968 35.9758 15.86 36.613 16.5013C32.273 20.1081 22.1613 23.0143 16.4275 23.0143C15.8746 23.0143 15.3436 22.9811 14.8227 22.932V25.974H39.5927V0.0173139H14.8242L14.8227 0.0158691ZM14.8227 16.9V18.7489C8.79499 17.6814 7.12183 11.4616 7.12183 11.4616C7.12183 11.4616 10.0157 8.27809 14.8227 7.76243V9.79042C14.8227 9.79042 14.8169 9.79042 14.814 9.79042C12.2912 9.48998 10.3212 11.83 10.3212 11.83C10.3212 11.83 11.4255 15.769 14.8242 16.9029L14.8227 16.9ZM4.11888 11.1944C4.11888 11.1944 7.6907 5.9612 14.8242 5.41954V3.52154C6.92396 4.15131 0.0814819 10.7943 0.0814819 10.7943C0.0814819 10.7943 3.95593 21.9165 14.8242 22.9349V20.917C6.84831 19.9203 4.11888 11.1944 4.11888 11.1944V11.1944Z" fill="#74B71B"></path>
          </svg>
          <span class="title" id="titlebar_text">Nsight Perf SDK</span>
        </div>
      </div>
    </div>

    <div class="section" id="intro">
      <h2>Nsight Perf SDK HTML Report</h2>
      <p>Navigate to the <a href="summary.html">summary.html</a> to begin.
    </div>

    <div class="section" id="unintended_use">
      <h2>Unintended Use of Product</h2>
      <p>The Nsight Perf SDK should not be used for benchmarking absolute performance, nor for comparing results between GPUs, due to the following factors:</p>
      <ul>
        <li>To ensure stable measurements, the Perf SDK encourages users to lock the GPU to its <a href=https://en.wikipedia.org/wiki/Thermal_design_power target="_blank">rated TDP (thermal design power)</a>. This forces thermally stable clock rates and disables boost clocks, ensuring consistent performance, but preventing the GPU from reaching its absolute peak performance.</li>
        <li>The Perf SDK disables certain GPU power management settings during profiling, to meet hardware requirements.</li>
        <li>Not all metrics are comparable between GPUs or architectures. For example, a more powerful GPU may complete a workload in less time while showing lower %%-of-peak throughput values, when compared against a less powerful GPU.</li>
      </ul>
      <p>The Nsight Perf SDK is intended to be used for performance profiling, to assist developers and artists in improving the performance of their code, art assets, and GPU shaders.</p>
      <p>See the bundled NVIDIA Developer Tools License document for additional details.</p>
    </div>


  </body>

</html>
)";
    }

    inline std::string GetSchedulingInfoHtml()
    {
        return R"ImADelimiter(
<html>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>

  <head>
    <title>Metrics Scheduling Information</title>

    <style id="ReportStyle">

      table {
        font-size: 14px;
        margin: 2 auto;
        border-collapse: collapse;
        border: 1px solid ;
      }

      table th {
        margin: 0 auto;
        border-collapse: collapse;
        border: 1px solid ;
        background: #F8F8F8;
      }

      table td {
        margin: 0 auto;
        border-collapse: collapse;
        border: 1px solid ;
      }

      table tbody tr td {
        margin: 0 auto;
        border-collapse: collapse;
        border: 1px solid ;
      }

      .tablename {
        color: DarkGreen;
        border-color: Black;
        background: #F8F8F8;
        font-weight: bold;
      }

      .subhdr {
        background: #F8F8F8;
      }

      .ca {
        text-align: center;
      }

      .la {
        text-align: left;
      }

      .ra {
        text-align: right;
      }

      .ww {
        word-wrap: break-word;
      }

      .full_name {
        min-width: 150px;
        width: 33vw;
        max-width: calc(92vw - 920px);
      }

      .base {
        font-size: 8px;
        color: #606060;
        border-color: Black;
      }

      .comp {
        font-size: 8px;
        color: steelblue;
        border-color: Black;
      }

      .titlearea {
        display: flex;
        align-items: center;
        color: white;
        font-family: verdana;
      }

      .titlebar {
        margin-left: 0;
        margin-right: auto;
      }

      .global_settings {
        margin-left: auto;
        margin-right: 0;
      }

      .title {
        font-size: 28px;
        margin-left: 10px;
      }

      .section {
        border-radius: 15px;
        padding: 10px;
        background: #FFFFFF;
        margin: 10px;
        min-width: calc(100% - 40px);
        width: max-content;
      }

      .section_title {
        font-family: verdana;
        font-weight: bold;
        color: black;
      }

      .workflow {
        width: 960px;
        max-width: 90vw;
      }

    </style>


    <script type="text/JavaScript">

      function escapeHtml(input) {
        if (typeof input == 'string' || input instanceof String) {
          return input
            .replace(/&/g, "&amp;") // make it first
            .replace(/ /g, "&nbsp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&apos;");
        }
        return input;
      }

      function addCellAttr(trow, attributes, text, passThrough = false) {
        let td = document.createElement('td');
        for (const attr in attributes) {
          td.setAttribute(attr, attributes[attr]);
        }

        if (passThrough) {
          td.innerHTML = text;
        } else {
          td.innerHTML = escapeHtml(text);
        }
        trow.appendChild(td);
      }

      function addCellSimple(trow, classes, text, passThrough = false, tooltip = "") {
        addCellAttr(trow, { "class": classes, "title": tooltip }, text, passThrough);
      }

      function tbody_MetricsSchedulingInformation(tbody) {
        let perRawCounterPassIndex = {}; // rawCounter : passIndex
        for (const [passIndex, rawCounters] of Object.entries(g_rawCounterDistribution)) {
          for (const rawCounter of rawCounters) {
            perRawCounterPassIndex[rawCounter] = passIndex;
          }
        }

        let perMetricSchedulingData = {}; // metric : { passIndex : [Counters] }
        for (const metric in g_metricRawCounters) {
          const rawCounters = g_metricRawCounters[metric];
          perMetricSchedulingData[metric] = {};
          for (const rawCounter of rawCounters) {
            const passIndex = perRawCounterPassIndex[rawCounter];
            if (!perMetricSchedulingData[metric].hasOwnProperty(passIndex)) {
              perMetricSchedulingData[metric][passIndex] = [];
            }
            perMetricSchedulingData[metric][passIndex].push(rawCounter);
          }
        }

        // sort the dict
        let sortedMetrics = Object.keys(perMetricSchedulingData).sort();
        let sortedPerMetricSchedulingData = {};
        // for dict, since ES6, insertion order is preserved
        for (const metric of sortedMetrics) {
          sortedPerMetricSchedulingData[metric] = {};
          let sortedPassIndices = Object.keys(perMetricSchedulingData[metric]).sort((a, b) => parseInt(a) - parseInt(b));
          for (const passIndex of sortedPassIndices) {
            sortedPerMetricSchedulingData[metric][passIndex] = perMetricSchedulingData[metric][passIndex].sort();
          }
        }
        perMetricSchedulingData = sortedPerMetricSchedulingData;

        for (const [metric, schedulingData] of Object.entries(perMetricSchedulingData)) {
          const sortedPassIndices = Object.keys(schedulingData).sort((a, b) => parseInt(a) - parseInt(b));
          const rowspan = sortedPassIndices.length;

          for (let ii = 0; ii < rowspan; ii++) {
            const trow = document.createElement('tr');
            if (ii === 0) {
              addCellAttr(trow, { "class": "la", 'rowspan' : rowspan.toString() }, metric);
            }

            const passIndex = sortedPassIndices[ii];
            const passLink = `<a href="#Pass_${passIndex}">${passIndex}</a>`;
            addCellAttr(trow, { "class": "ca" }, passLink, true);
            addCellSimple(trow, "la", schedulingData[passIndex].join(', '));
            tbody.appendChild(trow);
          }
        }
      }

      function tbody_PassInformation(tbody) {
        const charLimitPerRow = 300;
        for (const [passIndex, counters] of Object.entries(g_rawCounterDistribution)) {
          let rows = [];
          let currentRow = [];
          let currentCharCount = 0;

          counters.sort();
          for (const counter of counters) {
            if (currentCharCount + counter.length > charLimitPerRow) {
              rows.push(currentRow);
              currentRow = [];
              currentCharCount = 0;
            }
            currentRow.push(counter);
            currentCharCount += counter.length;
          }

          if (currentRow.length > 0) {
            rows.push(currentRow);
          }

          for (let ii = 0; ii < rows.length; ii++) {
            const trow = document.createElement('tr');
            if (ii === 0) {
              trow.id = `Pass_${passIndex}`;
              addCellAttr(trow, { "class": "ca", 'rowspan': rows.length.toString()}, passIndex);
            }

            addCellSimple(trow, "la", rows[ii].join(', '));
            tbody.appendChild(trow);
          }
        }
      }

      function onBodyLoaded() {
        tbody_MetricsSchedulingInformation(document.getElementById('tbody_metrics_scheduling_information'));
        tbody_PassInformation(document.getElementById('tbody_pass_information'));
      }
    </script>

  </head>

  <body onload="onBodyLoaded()" style="background-color:#202020;">
    <noscript>
      <p>Enable javascript to see report contents</span>
    </noscript>

    <div>
    <div class="titlearea">
        <div class="titlebar">
          <svg width="150" height="26" viewBox="0 0 150 26" fill="none" xmlns="http://www.w3.org/2000/svg" class="svelte-gccl40">
            <path d="M140.061 20.8363V20.4478H140.312C140.449 20.4478 140.637 20.4579 140.637 20.624C140.637 20.8045 140.541 20.8363 140.378 20.8363H140.061V20.8363ZM140.061 21.1093H140.229L140.619 21.7896H141.047L140.616 21.0819C140.839 21.066 141.022 20.9605 141.022 20.6615C141.022 20.2918 140.765 20.1719 140.33 20.1719H139.7V21.7882H140.062V21.1079L140.061 21.1093ZM141.894 20.9808C141.894 20.0318 141.15 19.48 140.322 19.48C139.495 19.48 138.747 20.0303 138.747 20.9808C138.747 21.9312 139.489 22.483 140.322 22.483C141.156 22.483 141.894 21.9298 141.894 20.9808ZM141.44 20.9808C141.44 21.6726 140.928 22.1363 140.322 22.1363V22.132C139.7 22.1363 139.198 21.6726 139.198 20.9808C139.198 20.2889 139.701 19.8266 140.322 19.8266C140.944 19.8266 141.44 20.2889 141.44 20.9808Z" fill="white"></path>
            <path d="M84.2106 4.92277V21.9614H89.0584V4.92277H84.2106V4.92277ZM46.0831 4.89966V21.9614H50.9731V8.71733L54.7878 8.73033C56.042 8.73033 56.9106 9.02933 57.5144 9.66921C58.2811 10.481 58.5939 11.7882 58.5939 14.1802V21.9614H63.3311V12.535C63.3311 5.80677 59.0115 4.89966 54.7864 4.89966H46.0831ZM92.0148 4.92277V21.96H99.8757C104.064 21.96 105.431 21.2681 106.91 19.7182C107.955 18.6291 108.63 16.24 108.63 13.6284C108.63 11.2335 108.058 9.09721 107.062 7.76688C105.266 5.38788 102.679 4.92277 98.8166 4.92277H92.0148V4.92277ZM96.8219 8.6321H98.9053C101.929 8.6321 103.884 9.97977 103.884 13.4768C103.884 16.9738 101.929 18.3229 98.9053 18.3229H96.8219V8.6321ZM77.2227 4.92277L73.178 18.4254L69.3021 4.92277H64.0702L69.6062 21.96H76.5912L82.1709 4.92277H77.2241H77.2227ZM110.885 21.96H115.733V4.92277H110.884V21.96H110.885ZM124.473 4.92855L117.704 21.9542H122.484L123.555 18.9454H131.564L132.578 21.9542H137.766L130.947 4.9271H124.473V4.92855ZM127.618 8.03555L130.554 16.0118H124.589L127.618 8.03555Z" fill="white"></path>
            <path d="M14.8242 7.76243V5.41809C15.054 5.4022 15.2854 5.3892 15.5211 5.38198C21.9809 5.17976 26.2191 10.8925 26.2191 10.8925C26.2191 10.8925 21.6419 17.2048 16.7345 17.2048C16.0274 17.2048 15.3945 17.0921 14.8242 16.9014V9.79042C17.3397 10.0923 17.8446 11.1944 19.3562 13.6962L22.7185 10.881C22.7185 10.881 20.2641 7.68442 16.1263 7.68442C15.6767 7.68442 15.2461 7.7162 14.8227 7.76098L14.8242 7.76243ZM14.8227 0.0158691V3.51865C15.054 3.49987 15.2868 3.48543 15.5196 3.47676C24.5023 3.17631 30.3554 10.7914 30.3554 10.7914C30.3554 10.7914 23.6337 18.9063 16.6297 18.9063C15.9881 18.9063 15.3872 18.8471 14.8227 18.7489V20.9141C15.3057 20.9748 15.8062 21.0109 16.3271 21.0109C22.8437 21.0109 27.5576 17.7074 32.1217 13.7959C32.8782 14.3968 35.9758 15.86 36.613 16.5013C32.273 20.1081 22.1613 23.0143 16.4275 23.0143C15.8746 23.0143 15.3436 22.9811 14.8227 22.932V25.974H39.5927V0.0173139H14.8242L14.8227 0.0158691ZM14.8227 16.9V18.7489C8.79499 17.6814 7.12183 11.4616 7.12183 11.4616C7.12183 11.4616 10.0157 8.27809 14.8227 7.76243V9.79042C14.8227 9.79042 14.8169 9.79042 14.814 9.79042C12.2912 9.48998 10.3212 11.83 10.3212 11.83C10.3212 11.83 11.4255 15.769 14.8242 16.9029L14.8227 16.9ZM4.11888 11.1944C4.11888 11.1944 7.6907 5.9612 14.8242 5.41954V3.52154C6.92396 4.15131 0.0814819 10.7943 0.0814819 10.7943C0.0814819 10.7943 3.95593 21.9165 14.8242 22.9349V20.917C6.84831 19.9203 4.11888 11.1944 4.11888 11.1944V11.1944Z" fill="#74B71B"></path>
          </svg>
          <span class="title" id="titlebar_text">Nsight Perf SDK Profiler Report</span>
        </div>
    </div>

    <div class="section">
        <table style="display: inline-block; border: 1px solid;">
            <thead>
                <tr>
                    <th colspan="3" class="tablename">Metrics Scheduling Information</th>
                </tr>
                <tr>
                    <th class="la">Metric</th>
                    <th class="la">Pass Index</th>
                    <th class="la">Raw Counters</th>
                </tr>
            </thead>
            <tbody id="tbody_metrics_scheduling_information">
            </tbody>
        </table>
    </div>

    <div class="section">
        <table style="display: inline-block; border: 1px solid;">
            <thead>
                <tr>
                    <th colspan="3" class="tablename">Pass Information</th>
                </tr>
                <tr>
                    <th class="la">Pass Index</th>
                    <th class="la">Raw Counters</th>
                </tr>
            </thead>
            <tbody id="tbody_pass_information">
            </tbody>
        </table>
    </div>

    </div>

    <script>
      g_json = {
        /***JSON_DATA_HERE***/
      };

      g_metricRawCounters = g_json.metricRawCounterDependencies || {};
      g_rawCounterDistribution = g_json.rawCounterDistribution || {}
    </script>

  </body>
</html>

)ImADelimiter";
    }
}}
