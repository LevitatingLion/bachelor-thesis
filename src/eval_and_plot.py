#!/usr/bin/env python3

# calculate metrics from collected data, generate plots and tables

from locale import LC_ALL, setlocale
from math import log10
from os import listdir, makedirs
from statistics import mean, stdev

import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# use correct number format for locale
mpl.rcParams["axes.formatter.use_locale"] = True

# colors of the uni logo
color_blue = "#2056ae"
color_yellow = "#fab20b"
color_grey = "#858270"


def main():
    """Calculate metrics from collected data, generate plots and tables."""
    # metrics and plots for sidechannels
    fr = print_sidechannel(
        "reload",
        start=0,
        end=250,
        step=5,
        threshold=100,
        direction="-",
        clusters=[(0, 49), (150, 200), (600, 800)],
    )
    ff = print_sidechannel(
        "flush",
        start=110,
        end=140,
        step=1,
        threshold=124,
        direction="+",
        clusters=[(100, 200)],
    )
    plot_sidechannel(fr, ff)
    print()

    # metrics for attacks
    calc_metrics_prefix("misprediction")
    calc_metrics_prefix("ridl")
    calc_metrics_prefix("wtf")
    calc_metrics_prefix("zombieload")
    calc_metrics_prefix("storetoleak")

    # tables for attacks, as markdown tables to be copied to the documentation
    print("Tables:")
    print()
    print_table_attack("ridl")
    print_table_attack("wtf")
    print_table_attack("zombieload")


def print_sidechannel(name, start, end, step, threshold, direction, clusters):
    """Print metrics for the given sidechannel."""
    name = f"sidechannel_{name}"
    print(f"{name}:")

    # read data

    csv = pd.read_csv(f"data/{name}.csv")
    cached = np.asarray(csv["cached"])
    uncached = np.asarray(csv["uncached"])
    assert len(cached) == len(uncached)

    # calculate ratio of samples in clusters

    for a, b in clusters:
        ratio_cached = sum(1 for x in cached if a <= x <= b) / len(cached)
        ratio_uncached = sum(1 for x in uncached if a <= x <= b) / len(uncached)
        print(f"    [{a}, {b}]: {ratio_cached:.4g} cached, {ratio_uncached:.4g} uncached")

    # calculate true-positive/negative rates

    def classify(x):
        assert direction in "+-"
        if direction == "+":
            return x >= threshold
        else:
            return x < threshold

    # true-positive: rate of cached samples that are classified as cached
    rate_tp = sum(1 for x in cached if classify(x)) / len(cached)
    # true-negative: rate of uncached samples that are classified as uncached
    rate_tn = sum(1 for x in uncached if not classify(x)) / len(uncached)

    print(f"    true-positives:    {rate_tp:.4g}")
    print(f"    true-negatives:    {rate_tn:.4g}")

    return cached, uncached, start, end, step, threshold


def plot_sidechannel(fr, ff):
    """Plot histograms for given sidechannel data."""

    def plot_hist(ax, cached, uncached, start, end, step, threshold):
        """Plot a single histogram."""
        bins = np.arange(start, end + 1, step)

        ax.hist(cached, bins, alpha=0.7, label="im Cache", density=True, color=color_blue)
        ax.hist(uncached, bins, alpha=0.7, label="nicht im Cache", density=True, color=color_yellow)

        ax.axvline(threshold, color="k", linewidth=1, label="Threshold")

    # create subplots
    fig, (ax_fr, ax_ff) = plt.subplots(nrows=2, figsize=[6, 4], tight_layout=True)

    # plot both histograms
    plot_hist(ax_fr, *fr)
    plot_hist(ax_ff, *ff)

    # set titles and labels, enable legends
    ax_fr.set_title("Flush+Reload")
    ax_ff.set_title("Flush+Flush")
    for ax in ax_fr, ax_ff:
        ax.set_xlabel("Latenz (Zyklen)")
        ax.set_ylabel("Anteil")
        ax.legend()

    # set correct locale and export plot as pdf and png
    setlocale(LC_ALL, "de_DE.utf8")
    makedirs("plots", exist_ok=True)
    fig.savefig("plots/sidechannel.pdf")
    fig.savefig("plots/sidechannel.png")


def calc_metrics_prefix(prefix):
    """Print metrics for all data files with the given prefix."""
    for file in listdir("data"):
        if file.startswith(prefix):
            calc_metrics(file.rsplit(".", 1)[0])
    print()


def calc_metrics(name):
    """Print metrics for data file with the given name."""
    csv = pd.read_csv(f"data/{name}.csv")

    print(f"{name}:")
    for measure in csv:
        samples = csv[measure]
        # print mean and stdev for each metric
        print(f"    {measure:20}: {mean(samples):10.4g} ± {stdev(samples):.4g}")


def print_table_attack(attack):
    """Print markdown table containing the metrics for the given attack's variants."""
    variants = {
        "default": "Basis",
        "ff": "Flush+Flush",
        "signal": "Signal",
        "transient": "Transient",
        "load": "Load",
        "lp": "Load Port",
        "kernel": "Kernel",
    }

    def adjust_data(name, data):
        """Add new columns to the given data, calculated from existing columns."""
        name = name.rsplit(".", 1)[0]
        # hits -> percentage
        data["percent_hits"] = data["rate_hits"] * 100
        # time per leak -> data rate
        data["rate_data"] = 8 / data["total_time"]
        return name, data

    def mean_stdev(data):
        """Calculate mean and stdev for each column and format them for latex output."""
        res = {}
        for key in data:
            res[key] = f"${fmt(mean(data[key]))} \\pm {fmt(stdev(data[key]))}$"
        return res

    def fmt(num):
        """Format a number to 4 significant figures, for latex output."""
        precision = 4
        assert num >= 0
        assert num < 100000

        # hardcoded case for 0
        if num == 0:
            return "0{,}000"
        digits = int(log10(num)) + 1
        if digits >= precision:
            # example: 54321.0 -> "54,320"
            r = format(int(round(num, precision - digits)), ",d")
        else:
            # example: 54.321 -> "54.32"
            r = format(num, f",.{precision-digits}f")
        # change decimal and thousand seperator
        return r.replace(",", "\\,").replace(".", "{,}")

    # get all variants of the given attack
    rows = (f for f in listdir("data") if f.startswith(attack))
    # read data for every variant and add computed columns
    rows = [adjust_data(f, pd.read_csv(f"data/{f}")) for f in rows]
    # sort by variant name
    rows.sort()

    # print markdown table
    print(f"{attack}:")
    print()
    print("Variante|Erfolgsrate (%)|Datenrate (B/s)|Kodieren (Zyklen)|Dekodieren (Zyklen)")
    print("---|---|---|---|---")
    for name, data in rows:
        _, variant = name.split("_")
        name = variants[variant]
        d = mean_stdev(data)
        print(
            f"{name}|{d['percent_hits']}|{d['rate_data']}|{d['cycles_leak']}|{d['cycles_decode']}"
        )
    print()


if __name__ == "__main__":
    main()
