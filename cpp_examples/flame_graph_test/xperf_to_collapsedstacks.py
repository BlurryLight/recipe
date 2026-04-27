# Copyright 2015 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
For more information see http://randomascii.wordpress.com/2013/03/26/summarizing-xperf-cpu-usage-with-flame-graphs/

This script converts sampling profiler call stacks from an xperf trace file
to collapsed call stack files of the type used as input to flamegraph.pl.
The initial xperf conversion is done using wpaexporter
	"wpaexporter.EXE trace.etl -profile ExportCPUUsageSampled.wpaProfile -symbols"
or:
	"wpaexporter.EXE trace.etl -range 1.0s 2.0s -profile ExportCPUUsageSampled.wpaProfile -symbols"
1.0s and 2.0s represent a time range - time values (without the 's' suffix) can be optionally
passed to this script.
See https://randomascii.wordpress.com/2013/11/04/exporting-arbitrary-data-from-xperf-etl-files/ for
details on wpaexporter.

The output from wpaexporter goes to CPU_Usage_(Sampled)_Randomascii_Export.csv. The first line is
column labels and the rest is the fields from the profile - process, threadID, and '/' separated
stack.
"""
import sys
import os
import time
import tempfile
import subprocess
import argparse
import webbrowser
import csv

if sys.version_info < (3, 11):
	print("Python 3.11 or higher is required.")
	print("Python version is %s" % sys.version)
	sys.exit(1)

parser = argparse.ArgumentParser(description='Process xperf ETL file and generate flamegraph(s).')
parser.add_argument('etlFilename', metavar='FILE', type=str,
                    help='Path to ETL file')
parser.add_argument('-p', '--processlist', help='List of process names to generate flamegraph for. Comma separated.', type=str)
parser.add_argument('-b', '--begin', help='Time range. Begin from the specified value. In seconds.', type=float)
parser.add_argument('-e', '--end', help='Time range. End at the specified value. In seconds.', type=float)
parser.add_argument('-o', '--output', help='Path to directory where output will written into. Default is system TEMP directory', type=str)
parser.set_defaults(output=tempfile.gettempdir())
parser.add_argument('-n', '--numshow', help='Number of top processes to generate flame graph for. Default is 1', type=int)
parser.set_defaults(numshow=1)
parser.add_argument('-d', '--dontopen', help='Do not open the generated SVG file automatically. Default is open', action='store_true')
parser.set_defaults(dontopen=False)
parser.add_argument('--perl', help='Path to perl.exe. Defaults to perl in PATH, then common Windows install locations.', type=str)
parser.add_argument('-s', '--symbols', help='Ask wpaexporter to load symbols. This can be very slow if symbol paths or servers are not configured.', action='store_true')
parser.set_defaults(symbols=False)
parser.add_argument('--symbol-path', help='Symbol path used only for wpaexporter. Pass a local PDB directory or a PDB file path to avoid symbol servers.', type=str)
args = parser.parse_args()
args.output = os.path.abspath(args.output)
os.makedirs(args.output, exist_ok=True)

processList = []
if args.processlist:
	processList = [item.lower() for item in args.processlist.split(',')]

# How many threads to create collapsed stacks for.
numToShow = args.numshow

scriptPath = os.path.abspath(sys.argv[0])
scriptDir = os.path.split(scriptPath)[0]
flameGraphCandidates = [
	os.path.join(scriptDir, "flamegraph.pl"),
	os.path.join(scriptDir, "FlameGraph", "flamegraph.pl"),
]
flameGraphPath = next((path for path in flameGraphCandidates if os.path.exists(path)), flameGraphCandidates[0])
if not os.path.exists(flameGraphPath):
	print("Couldn't find \"%s\". Download it from https://github.com/brendangregg/FlameGraph/blob/master/flamegraph.pl" % flameGraphPath)
	sys.exit(0)

def find_executable(name, extra_candidates=None):
	extra_candidates = extra_candidates or []
	if args.perl and name.lower() == "perl":
		if os.path.exists(args.perl):
			return args.perl
		print("Couldn't find perl at \"%s\"." % args.perl)
		sys.exit(1)
	path = shutil.which(name)
	if path:
		return path
	for candidate in extra_candidates:
		if os.path.exists(candidate):
			return candidate
	return None

import shutil
perlPath = find_executable("perl", [
	r"C:\Program Files\Git\usr\bin\perl.exe",
	r"C:\Strawberry\perl\bin\perl.exe",
	r"C:\msys64\usr\bin\perl.exe",
	r"D:\msys64\usr\bin\perl.exe",
])
if not perlPath:
	print("Couldn't find perl.exe. Install Perl or pass --perl C:\\path\\to\\perl.exe.")
	sys.exit(1)

profilePath = os.path.join(scriptDir, "ExportCPUUsageSampled.wpaProfile")
if not os.path.exists(profilePath):
	print("Couldn't find \"%s\". This should be part of the UIforETW repo and releases" % profilePath)
	sys.exit(0)

#if "_NT_SYMBOL_PATH" not in os.environ:
#	print("_NT_SYMBOL_PATH is not set. Exiting.")
#	sys.exit(0)

progFilesx86 = os.environ.get("ProgramFiles(x86)") or os.environ.get("ProgramFiles")
if not progFilesx86:
	print("Couldn't find ProgramFiles environment variable. Make sure WPT 10 is installed.")
	sys.exit(0)
wpaExporterPath = os.path.join(progFilesx86, r"Windows Kits\10\Windows Performance Toolkit\wpaexporter.EXE")
if not os.path.exists(wpaExporterPath):
	print("Couldn't find \"%s\". Make sure WPT 10 is installed." % wpaExporterPath)
	sys.exit(0)

if args.begin is not None and args.end is not None:
	wpaCommand = [
		wpaExporterPath,
		args.etlFilename,
		"-range",
		"%ss" % args.begin,
		"%ss" % args.end,
		"-profile",
		profilePath,
		"-outputfolder",
		args.output,
	]
else:
	wpaCommand = [
		wpaExporterPath,
		args.etlFilename,
		"-profile",
		profilePath,
		"-outputfolder",
		args.output,
	]

if args.symbols:
	wpaCommand.append("-symbols")

csvName = os.path.join(args.output, "CPU_Usage_(Sampled)_Randomascii_Export.csv")
if os.path.exists(csvName):
	os.remove(csvName)

print("> %s" % subprocess.list2cmdline(wpaCommand))
start = time.perf_counter()
wpaEnv = os.environ.copy()
if args.symbol_path:
	symbolPath = os.path.abspath(args.symbol_path)
	if os.path.isfile(symbolPath):
		symbolPath = os.path.dirname(symbolPath)
	if not os.path.isdir(symbolPath):
		print("Symbol path does not exist or is not a directory: %s" % symbolPath)
		sys.exit(1)
	wpaEnv["_NT_SYMBOL_PATH"] = symbolPath
	wpaEnv.pop("_NT_ALT_SYMBOL_PATH", None)
	print("Using _NT_SYMBOL_PATH=%s" % symbolPath)
else:
	print("正在使用系统默认环境变量，可能会导致符号加载缓慢或失败(需要从微软服务器拉取系统符号)。使用 --symbol-path 参数可以指定符号路径以加快加载速度(可能无法采集到系统符号)。")

print(wpaCommand)
completed = subprocess.run(wpaCommand, stderr=subprocess.STDOUT, stdout=subprocess.PIPE, text=True, env=wpaEnv)
print(completed.stdout)
if completed.returncode != 0:
	if os.path.exists(csvName) and os.path.getsize(csvName) > 0:
		print("Warning: wpaexporter exited with code %d after writing %s. Continuing." % (completed.returncode, csvName))
	else:
		sys.exit(completed.returncode)
print("Elapsed time for wpaexporter: %1.3f s" % (time.perf_counter() - start))


# This dictionary of dictionaries accumulates sample data. The first key is
# the process name with the thread ID appended to it, like this:
#     devenv.exe_8872_6148
# The second key is the semi-colon separated call stack.
# The final data field is the count of how many times that exact call stack
# has been hit.
samples = {}

# Process all of the lines in the output of wpaexporter, skipping the first line
# which is just the column names.
if not os.path.exists(csvName):
	print("Couldn't find wpaexporter output \"%s\"." % csvName)
	sys.exit(1)
with open(csvName, encoding="utf-8-sig") as csvFile:
	reader = csv.reader(csvFile)
	headers = next(reader, None)
	rows = list(reader)

if not headers:
	print("No rows were exported by wpaexporter.")
	sys.exit(1)

def find_column(headers, candidates):
	lower_headers = [header.strip().lower() for header in headers]
	for candidate in candidates:
		candidate = candidate.lower()
		if candidate in lower_headers:
			return lower_headers.index(candidate)
	for index, header in enumerate(lower_headers):
		if any(candidate.lower() in header for candidate in candidates):
			return index
	return None

def find_last_column(headers, candidates):
	lower_headers = [header.strip().lower() for header in headers]
	for candidate in candidates:
		candidate = candidate.lower()
		matches = [index for index, header in enumerate(lower_headers) if header == candidate]
		if matches:
			return matches[-1]
	for index in range(len(lower_headers) - 1, -1, -1):
		header = lower_headers[index]
		if any(candidate.lower() in header for candidate in candidates):
			return index
	return None

processColumn = find_column(headers, ["Process", "Process Name"])
threadColumn = find_column(headers, ["Thread ID"])
stackColumn = find_last_column(headers, ["Stack"])

if processColumn is None or threadColumn is None or stackColumn is None:
	print("Expected Process, Thread ID, and Stack columns in %s." % csvName)
	print("Found columns: %s" % ", ".join(headers))
	sys.exit(1)

for row in rows:
	if len(row) <= max(processColumn, threadColumn, stackColumn):
		continue
	process = row[processColumn].strip()
	if processList and process.split(' ')[0].lower() not in processList:
		continue
	threadID = row[threadColumn].strip()
	stackSummary = row[stackColumn].strip()
	if stackSummary == "n/a":
		continue
	# Since we are using semicolon separators we can't have semicolons
	# in the function names.
	stackSummary = stackSummary.replace(";", ":")
	# Spaces seem like a bad idea also.
	stackSummary = stackSummary.replace(" ", "_")
	# Having single-quote characters in the call stacks gives flamegraph.pl heartburn.
	# Replace them with back ticks.
	stackSummary = stackSummary.replace("'", "`")
	# Double-quote characters also cause problems. Remove them.
	stackSummary = stackSummary.replace('"', "")
	# Remove <PDB_not_found> labels
	stackSummary = stackSummary.replace("<PDB_not_found>", "Unknown")
	# Convert the wpaexporter stack separators to flamegraph stack separators
	stackSummary = stackSummary.replace("/", ";")
	#stackSummary = "A;B;C"

	processAndThread = "%s_%s" % (process.replace(" ", "_"), threadID)
	processAndThread = processAndThread.replace("(", "")
	processAndThread = processAndThread.replace(")", "")
	# Add the record to the nested dictionary, and increment the count
	# if it already exists.
	if not processAndThread in samples:
		samples[processAndThread] = {}
	if not stackSummary in samples[processAndThread]:
		samples[processAndThread][stackSummary] = 0
	# Accumulate counts for this call stack.
	samples[processAndThread][stackSummary] += 1

# Now find the threads with the most samples - we'll dump them.
sortedThreads = []
totalSamples = 0
for processAndThread in samples.keys():
	numSamples = 0
	for stack in samples[processAndThread]:
		numSamples += samples[processAndThread][stack]
	sortedThreads.append((numSamples, processAndThread))
	totalSamples += numSamples

sortedThreads.sort() # Put the threads in order by number of samples
sortedThreads.reverse() # Put the thread with the most samples first

print("Found %d samples from %d threads." % (totalSamples, len(samples)))

if len(processList)>0:
	numToShow = len(sortedThreads)

count = 0
for numSamples, processAndThread in sortedThreads[:numToShow]:
	threadSamples = samples[processAndThread]
	outputName = os.path.join(args.output, "collapsed_stacks_%d.txt" % count)
	count += 1
	print("Writing %d samples to temporary file %s" % (numSamples, outputName))
	sortedStacks = []
	for stack in threadSamples:
		sortedStacks.append("%s %d\n" % (stack, threadSamples[stack]))
	sortedStacks.sort()
	# Some versions of perl (the version that ships with Chromium's depot_tools
	# for one) can't handle reading files with CRLF line endings, so write the
	# file as binary to avoid line-ending translation.
	with open(outputName, "w", encoding="utf-8", newline="\n") as out:
		for stack in sortedStacks:
			out.write(stack)

	destPath = os.path.join(args.output, "%s.svg" % processAndThread)
	title = "CPU Usage flame graph of %s" % processAndThread
	perlCommand = [perlPath, flameGraphPath, "--title=%s" % title, outputName]
	print("> %s" % subprocess.list2cmdline(perlCommand))
	svgOutput = subprocess.check_output(perlCommand, text=True)
	if len(svgOutput) > 100:
		with open(destPath, "w", encoding="utf-8") as svgFile:
			svgFile.write(svgOutput)
		if not args.dontopen:
			if hasattr(os, "startfile"):
				os.startfile(destPath)
			else:
				webbrowser.open(destPath)
		print('Results are in "%s" - they should be auto-opened in the default SVG viewer.' % destPath)
	else:
		print("Result size is %d bytes - is perl in your path?" % len(svgOutput))
