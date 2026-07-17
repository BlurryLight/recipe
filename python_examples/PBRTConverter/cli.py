import argparse
import sys
import os

from .nori_parser import parse_nori_scene
from .pbrt_writer import write_pbrt


def main():
    parser = argparse.ArgumentParser(
        description="Convert Nori2 scene files to PBRTv3 format"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    nori2pbrt = subparsers.add_parser(
        "nori2pbrt", help="Convert Nori2 XML scene to PBRTv3 (.pbrt)"
    )
    nori2pbrt.add_argument("input", help="Input Nori XML scene file")
    nori2pbrt.add_argument("output", help="Output PBRT scene file")

    args = parser.parse_args()

    if args.command == "nori2pbrt":
        if not os.path.exists(args.input):
            print(f"Error: input file '{args.input}' not found", file=sys.stderr)
            return 1
        scene = parse_nori_scene(args.input)
        write_pbrt(scene, args.output)
        print(f"Converted: {args.input} -> {args.output}")
    return 0
