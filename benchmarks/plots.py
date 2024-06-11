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
    type=Path,
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
                if line.startswith("num_parsers_paths"):
                    num_parsers_paths = int(line.split(":")[1].strip())

        benchmark_file = program_data_file.with_suffix(".csv")
        if not benchmark_file.exists():
            print(f"Could not find benchmark file for {program_name}")
            return None
        benchmark_data = pd.read_csv(benchmark_file)
        total_time = int(benchmark_data["Total Time"][0])
        analysis_time = int(
            benchmark_data.loc[benchmark_data['Timer'] == 'Dataplaneanalysis']['Total Time'].iloc[0]
        )
        if cyclomatic_complexity == 0:
            print(f'warning: ignoring {program_name} because cyclomatic complexity is 0')
            continue
        if num_parsers_paths == 0:
            print(f'warning: ignoring {program_name} because num_parsers_paths is 0')
            continue
        if analysis_time == 0:
            print(f'warning: ignoring {program_name} because analysis time is 0')
            continue
        complexity_data.append(
            (
                program_name,
                cyclomatic_complexity,
                statement_count_before,
                statement_count_after,
                num_parsers_paths,
                total_time,
                analysis_time,
            )
        )
    df = pd.DataFrame(
        complexity_data,
        columns=[
            "Program Name",
            "Cyclomatic Complexity",
            "Statement Count Before",
            "Statement Count After",
            "Number of Parser Paths",
            "Total Time (ms)",
            "Analysis Time (ms)",
        ],
    )
    df.sort_values("Analysis Time (ms)", inplace=True, ascending=False)
    return df


def plot_data(output_directory: Path, data: pd.DataFrame) -> None:
    print("Plotting data...")
    # Compute
    data["log(Cyclomatic)"] = np.log(data["Cyclomatic Complexity"])
    data["Cyclomatic + #of Parser Paths"] = (
        data["Cyclomatic Complexity"] + data["Number of Parser Paths"]
    )
    data["log(Cyclomatic + #of Parser Paths)"] = np.log(
        data["Cyclomatic Complexity"] + data["Number of Parser Paths"]
    )
    data["log(Total Time (ms))"] = np.log(data["Total Time (ms)"])
    data["log(Analysis Time (ms))"] = np.log(data["Analysis Time (ms)"])

    def plot_one(x, y, output_name):
        plt.gcf().clear()
        sns.regplot(
            data=data,
            x=x,
            y=y,
            scatter=True,
            robust=True,
        )
        outdir = output_directory.joinpath(output_name)
        plt.savefig(outdir.with_suffix(".png"), bbox_inches="tight")
        plt.savefig(outdir.with_suffix(".pdf"), bbox_inches="tight")
        plt.gcf().clear()

    plot_one(
        "log(Cyclomatic)",
        "log(Total Time (ms))",
        "flay_regression_plot_log_cyclomatic__vs__log_total_time",
    )
    plot_one(
        "log(Cyclomatic + #of Parser Paths)",
        "log(Total Time (ms))",
        "flay_regression_plot_log_cyclomatic_and_path__vs__log_total_time",
    )
    plot_one(
        "log(Cyclomatic)",
        "log(Analysis Time (ms))",
        "flay_regression_plot_log_cyclomatic__vs__log_analysis_time",
    )
    plot_one(
        "log(Cyclomatic + #of Parser Paths)",
        "log(Analysis Time (ms))",
        "flay_regression_plot_log_cyclomatic_and_path__vs__log_analysis_time",
    )


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
