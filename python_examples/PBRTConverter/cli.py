import argparse
import sys
import os

from nori_parser import parse_nori_scene
from pbrt_writer import write_pbrt
from mitsuba_writer import write_mitsuba


def main():
    parser = argparse.ArgumentParser(
        description="Convert Nori2 scene files to PBRTv3 / Mitsuba 0.6 format"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    nori2pbrt = subparsers.add_parser(
        "nori2pbrt", help="Convert Nori2 XML scene to PBRTv3 (.pbrt)"
    )
    nori2pbrt.add_argument("input", help="Input Nori XML scene file")
    nori2pbrt.add_argument("output", help="Output PBRT scene file")

    nori2mitsuba = subparsers.add_parser(
        "nori2mitsuba", help="Convert Nori2 XML scene to Mitsuba 0.6 (.xml)"
    )
    nori2mitsuba.add_argument("input", help="Input Nori XML scene file")
    nori2mitsuba.add_argument("output", help="Output Mitsuba scene file")

    args = parser.parse_args()

    if args.command in ("nori2pbrt", "nori2mitsuba"):
        if not os.path.exists(args.input):
            print(f"Error: input file '{args.input}' not found", file=sys.stderr)
            return 1
        scene = parse_nori_scene(args.input)
        if args.command == "nori2pbrt":
            write_pbrt(scene, args.output)
        else:
            write_mitsuba(scene, args.output)
        print(f"Converted: {args.input} -> {args.output}")
    return 0
