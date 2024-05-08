#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path
from typing import Any, Optional

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

OUTPUT_DIR = Path("plots")

PARSER = argparse.ArgumentParser()
PARSER.add_argument(
    "-r",
    "--compiler-root-dir",
    dest="root_dir",
    help="The root directory of the compiler source tree."
    "This is used to import P4C's Python libraries",
)
PARSER.add_argument(
    "-i",
    "--input-dir",
    dest="input_dir",
    help="The folder containing measurement data.",
)
PARSER.add_argument(
    "-o",
    "--out-dir",
    dest="out_dir",
    default=OUTPUT_DIR,
    help="The output folder where all plots are dumped.",
)
PARSER.add_argument(
    "-w",
    "--write-csv",
    dest="csv_file",
    help="Write the data out to the specified csv file.",
)

# Parse options and process argv
ARGS, ARGV = PARSER.parse_known_args()

# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
sys.path.append(str(ARGS.root_dir))
from tools import testutils  # pylint: disable=wrong-import-position

OUTPUT_DIR = Path("plots")


def get_data(input_dir: Path) -> Optional[pd.DataFrame]:
    input_dir = Path(testutils.check_if_dir(input_dir))

    # benchmarking_data = list(input_dir.glob("*.csv"))
    program_data_files = input_dir.glob("*.ref")

    complexity_data = []
    for program_data_file in program_data_files:
        program_data_file = Path(program_data_file)
        program_name = program_data_file.stem
        cyclomatic_complexity = 0
        with program_data_file.open("r", encoding="utf8") as program_data_file_handle:
            for line in program_data_file_handle.readlines():
                if line.startswith("cyclomatic_complexity"):
                    cyclomatic_complexity = int(line.split(":")[1].strip())
                if line.startswith("statement_count_before"):
                    statement_count_before = int(line.split(":")[1].strip())
                if line.startswith("statement_count_after"):
                    statement_count_after = int(line.split(":")[1].strip())

        benchmark_file = program_data_file.with_suffix(".csv")
        if not benchmark_file.exists():
            print(f"Could not find benchmark file for {program_name}")
            return None
        benchmark_data = pd.read_csv(benchmark_file)
        total_time = int(benchmark_data["Total Time"][0]) / 1000
        complexity_data.append(
            (
                program_name,
                cyclomatic_complexity,
                statement_count_before,
                statement_count_after,
                total_time,
            )
        )
    df = pd.DataFrame(
        complexity_data,
        columns=[
            "Program Name",
            "Cyclomatic Complexity",
            "Statement Count Before",
            "Statement Count After",
            "Time (Seconds)",
        ],
    )
    df.sort_values("Time (Seconds)", inplace=True, ascending=False)
    return df


def plot_data(output_directory: Path, data: pd.DataFrame) -> None:
    sns.regplot(
        data=data,
        x="Cyclomatic Complexity",
        y="Time (Seconds)",
        scatter=True,
        robust=True,
    )
    outdir = output_directory.joinpath("flay_regression_plot")
    plt.savefig(outdir.with_suffix(".png"), bbox_inches="tight")
    plt.savefig(outdir.with_suffix(".pdf"), bbox_inches="tight")
    plt.gcf().clear()


def main(args: Any, extra_args: Any) -> None:
    sns.set_theme(
        context="paper",
        style="ticks",
        rc={
            "lines.linewidth": 1,
            "hatch.linewidth": 0.3,
            "axes.spines.right": False,
            "axes.spines.top": False,
            "lines.markeredgewidth": 0.1,
            "axes.labelsize": 7,
            "font.size": 7,
            "xtick.labelsize": 7,
            "ytick.labelsize": 7,
            "legend.fontsize": 7,
            "font.family": "serif",
            "font.serif": ["Computer Modern Roman"] + plt.rcParams["font.serif"],
        },
    )
    plt.rcParams["pdf.fonttype"] = 42
    plt.rcParams["ps.fonttype"] = 42

    testutils.check_and_create_dir(args.out_dir)
    if args.input_dir:
        data = get_data(args.input_dir)
        if data is None:
            return
        if args.csv_file:
            data.to_csv(args.csv_file)
        plot_data(args.out_dir, data)
    else:
        print("No input directory provided. Not plotting coverage data.")


if __name__ == "__main__":
    main(ARGS, ARGV)
