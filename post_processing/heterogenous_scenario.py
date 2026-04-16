import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import re
import glob
import os
import math
import scipy.stats as st
import tikzplotlib
from matplotlib.patches import Patch

plt.rcParams.update({
    "figure.facecolor": "white",
    "axes.facecolor": "white",
    "savefig.facecolor": "white",
    "savefig.transparent": False,
    "font.size": 8,
    "axes.labelsize": 8,
    "axes.titlesize": 8,
    "legend.fontsize": 8,
    "xtick.labelsize": 8,
    "ytick.labelsize": 8,
    "axes.linewidth": 0.8,
    "grid.linewidth": 0.5,
    "grid.alpha": 0.5,
    "grid.linestyle": "--",
    "hatch.linewidth": 0.8,
    "legend.frameon": True,
    "legend.fancybox": False,
    "legend.framealpha": 1.0,
    "figure.dpi": 300,
    "savefig.dpi": 300,
})


def add_custom_legend(ax, scenarios, colors, hatches, labels, ncol=2,
                      bbox_to_anchor=(0.5, 1.22), loc='upper center',
                      fontsize=8, columnspacing=0.9, handletextpad=0.5,
                      borderpad=0.3, labelspacing=0.4):
    handles = []
    for i, _ in enumerate(scenarios):
        handles.append(
            Patch(
                facecolor='white',
                edgecolor=colors[i % len(colors)],
                hatch=hatches[i % len(hatches)],
                linewidth=1.0
            )
        )

    legend = ax.legend(
        handles,
        labels,
        ncol=ncol,
        loc=loc,
        bbox_to_anchor=bbox_to_anchor,
        frameon=True,
        fancybox=False,
        edgecolor='gray',
        columnspacing=columnspacing,
        handletextpad=handletextpad,
        borderpad=borderpad,
        labelspacing=labelspacing,
        fontsize=fontsize,
    )
    return legend


def simplify_name(full_name):
    match = re.match(r'(\w+):vector.*? (.+?) \(#', full_name)
    if match:
        vector_name = match.group(1)
        module_name = match.group(2)
        return f"{module_name}.{vector_name}"
    else:
        return full_name


def read_in_data(scenarios, number_of_runs, number_of_apps, base_path):
    scenario_data = {}

    for scenario in scenarios:
        print(scenario)

        scenario_data[scenario] = dict()

        for run in range(number_of_runs):
            scenario_data[scenario][run] = dict()

            file_path = os.path.join(base_path, f"{scenario}-#{run}.csv")
            df = pd.read_csv(file_path)

            values_only = df.iloc[:, 1::2]
            values_only.columns = [df.iloc[:, j].name for j in range(0, len(df.columns), 2)]
            values_only.columns = [simplify_name(c) for c in values_only.columns]

            for app_id in range(number_of_apps):
                scenario_data[scenario][run][app_id] = {
                    "delay": [],
                    "received": 0,
                    "sent": 0,
                    "per": 0,
                    "throughput": [],
                    "throughput_long_term": []
                }

                col = [c for c in values_only.columns if f"Listener.app[{app_id}].endToEndDelay" in c]
                if col:
                    scenario_data[scenario][run][app_id]["delay"].extend(
                        (values_only[col[0]].dropna() * 1000).round(3).tolist()
                    )

                col = [c for c in values_only.columns if f"Listener.app[{app_id}].throughput" in c]
                if col:
                    scenario_data[scenario][run][app_id]["throughput"].extend(
                        (values_only[col[0]].dropna() / 1000).round(3).tolist()
                    )

                    tp_series = values_only[col[0]].dropna()
                    bin_size = 10
                    avg_tp_bins = [
                        tp_series[i:i + bin_size].mean() / 1000
                        for i in range(0, len(tp_series), bin_size)
                    ]
                    scenario_data[scenario][run][app_id]["throughput_long_term"].extend(
                        [round(x, 3) for x in avg_tp_bins]
                    )

                col = [c for c in values_only.columns if f"Listener.app[{app_id}].packetReceived" in c]
                if col:
                    scenario_data[scenario][run][app_id]["received"] = len(
                        values_only[col[0]].dropna().tolist()
                    )

                col = [c for c in values_only.columns if f"Talker0.app[{app_id}].packetSent" in c]
                if col:
                    scenario_data[scenario][run][app_id]["sent"] = len(
                        values_only[col[0]].dropna().tolist()
                    )

                if scenario_data[scenario][run][app_id]["sent"] > 0:
                    if scenario_data[scenario][run][app_id]["received"] > scenario_data[scenario][run][app_id]["sent"]:
                        scenario_data[scenario][run][app_id]["per"] = 0
                    else:
                        scenario_data[scenario][run][app_id]["per"] = 100 * (
                            scenario_data[scenario][run][app_id]["sent"]
                            - scenario_data[scenario][run][app_id]["received"]
                        ) / scenario_data[scenario][run][app_id]["sent"]
                else:
                    print("no pkts sent" + str(scenario) + str(run) + str(app_id))

    return scenario_data


def e2e_delay_percentiles(scenarios, tc_selected, tc_groups, scenario_data,
                          number_of_runs, filename, number_of_apps, style_map):

    data_delay = {scenario: {tc: [] for tc in tc_groups.keys()} for scenario in scenarios}

    for scenario in scenarios:
        for run in range(number_of_runs):
            for app_id in range(number_of_apps):
                listener_delay = scenario_data[scenario][run][app_id]["delay"]

                for tc, app_ids in tc_groups.items():
                    if app_id in app_ids:
                        data_delay[scenario][tc].extend(listener_delay)
                        break

    plot_labels = ["Mean", "99th Percentile", "99.9th Percentile"]
    plot_data = []

    for scenario in scenarios:
        combined_data = []
        combined_data.extend(data_delay[scenario][tc_selected])

        metrics = [
            np.mean(combined_data),
            np.percentile(combined_data, 99),
            np.percentile(combined_data, 99.9)
        ]
        plot_data.append(metrics)

    plot_data = np.array(plot_data)

    x = np.arange(len(plot_labels))
    width = 0.1
    spacing_factor = 1.2

    fig, ax = plt.subplots(figsize=(7.2, 4.6))

    for i, scenario in enumerate(scenarios):
        ax.bar(
            x + (i - len(plot_data) / 2) * width * spacing_factor + width / 2,
            plot_data[i],
            width,
            label=scenario,
            color='white',
            edgecolor=style_map[scenario]["color"],
            hatch=style_map[scenario]["hatch"],
            linewidth=0.8
        )

    ax.set_ylabel("End-to-End Delay [ms]", fontsize=8)
    ax.set_xticks(x)
    ax.set_xticklabels(plot_labels, rotation=25, ha='right', fontsize=7)
    ax.set_yscale('log')
    ax.set_ylim(0.1, 1000)
    ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.6)
    ax.hlines(50, xmin=-0.5, xmax=2.5, colors='red', linestyles='--', linewidth=1.5)

    legend_colors = [style_map[s]["color"] for s in scenarios]
    legend_hatches = [style_map[s]["hatch"] for s in scenarios]
    legend_labels = [
        "GF, adaptive MCS + Dyn. Priority (MDBV)",
        "GF, adaptive MCS + Dyn. MAX C/I (MDBV)",
        "GF, adaptive MCS + Dyn. PF (MDBV)",
        "GF, adaptive MCS + Dyn. Priority",
        "GF, adaptive MCS + Dyn. MAX C/I",
        "GF, adaptive MCS + Dyn. PF"
    ]
    
    add_custom_legend(
        ax,
        scenarios,
        legend_colors,
        legend_hatches,
        legend_labels,
        ncol=2,
        bbox_to_anchor=(0.5, 1.34),
        fontsize=8,
        columnspacing=1.0,
        handletextpad=0.6
    )

    fig.subplots_adjust(top=0.72, bottom=0.22, left=0.12, right=0.98)
    tikzplotlib.save(filename)
    plt.savefig(filename.replace(".tex", ".png"), dpi=300)


def cv_throughput(scenarios, tcs, tc_groups, scenario_data, number_of_runs, filename):
    data_cv = {scenario: {tc: [] for tc in tc_groups} for scenario in scenarios}

    for scenario in scenarios:
        for run in range(number_of_runs):
            for tc, ue_ids in tc_groups.items():
                if tc in ["TC7", "TC1", "TC0"]:
                    for ue_idx in ue_ids:
                        ue_tp_series = scenario_data[scenario][run][ue_idx]["throughput_long_term"]
                        if len(ue_tp_series) > 0:
                            mean_tp = np.mean(ue_tp_series)
                            std_tp = np.std(ue_tp_series)
                            cv_tp = std_tp / mean_tp if mean_tp > 0 else 0
                        else:
                            cv_tp = 0
                        data_cv[scenario][tc].append(cv_tp)

    colors = ["orange", "blue", "green"]
    hatches = ["/", "\\", "x"]

    stats = []
    positions = []
    pos = 1

    for tc in ["TC7", "TC1", "TC0"]:
        for i, scenario in enumerate(scenarios):
            d = np.array(data_cv[scenario][tc])
            if len(d) == 0:
                d = np.array([0])

            q1, med, q3 = np.percentile(d, [25, 50, 75])
            iqr = q3 - q1
            whislo = np.min(d[d >= q1 - 1.5 * iqr]) if np.any(d >= q1 - 1.5 * iqr) else np.min(d)
            whishi = np.max(d[d <= q3 + 1.5 * iqr]) if np.any(d <= q3 + 1.5 * iqr) else np.max(d)
            outliers = d[(d < whislo) | (d > whishi)]
            grouped_outliers = [np.max(outliers)] if len(outliers) > 0 else []

            stats.append({
                "med": med,
                "q1": q1,
                "q3": q3,
                "whislo": whislo,
                "whishi": whishi,
                "fliers": grouped_outliers,
                "edgecolor": colors[i],
                "hatch": hatches[i],
            })
            positions.append(pos)
            pos += 0.7
        pos += 1.5

    fig, ax = plt.subplots(figsize=(5.6, 3.2))

    for i, s in enumerate(stats):
        ax.bxp(
            [s],
            positions=[positions[i]],
            patch_artist=True,
            showfliers=True,
            flierprops=dict(
                marker='*',
                color='black',
                markersize=5,
                alpha=1,
                markerfacecolor='black',
                markeredgecolor='black',
            ),
            boxprops=dict(facecolor='white', edgecolor=s["edgecolor"], hatch=s["hatch"]),
            medianprops=dict(color='black'),
            widths=0.6
        )

    for i, s in enumerate(stats):
        ax.plot(positions[i], s["med"], marker='x', color='black', markersize=5, mew=1.2)

    mid_positions = []
    for i in range(len(["TC7", "TC1", "TC0"])):
        start_pos = i * (len(scenarios) * 0.7 + 1.5)
        end_pos = start_pos + (len(scenarios) - 1) * 0.7
        mid_positions.append((start_pos + end_pos) / 2 + 1)

    ax.set_xticks(mid_positions)
    ax.set_xticklabels(["TC7", "TC1", "TC0"], fontsize=8)

    legend_labels = [
        "GF, adaptive MCS + Dyn. Priority (MDBV)",
        "GF, adaptive MCS + Dyn. MAX C/I (MDBV)",
        "GF, adaptive MCS + Dyn. PF (MDBV)",
    ]

    add_custom_legend(
        ax,
        scenarios,
        colors,
        hatches,
        legend_labels,
        ncol=1,
        bbox_to_anchor=(0.5, 1.30),
        fontsize=8
    )

    ax.set_ylabel("Throughput CV", fontsize=8)
    ax.grid(True, linestyle='--', alpha=0.5)
    ax.set_yscale('log')

    fig.subplots_adjust(top=0.72, bottom=0.18, left=0.12, right=0.98)
    tikzplotlib.save(filename)
    plt.savefig(filename.replace(".tex", ".png"), dpi=300)


def e2e_delay_distribution(scenarios, tcs, tc_groups, scenario_data,
                           number_of_runs, number_of_apps, filename):

    data_delay = {scenario: {tc: [] for tc in tc_groups.keys()} for scenario in scenarios}

    for scenario in scenarios:
        for run in range(number_of_runs):
            for app_id in range(number_of_apps):
                listener_delay = scenario_data[scenario][run][app_id]["delay"]

                for tc, app_ids in tc_groups.items():
                    if app_id in app_ids:
                        data_delay[scenario][tc].extend(listener_delay)
                        break

    colors = ['orange', 'blue', 'green']
    hatch_patterns = ['/', '\\', 'x']

    stats = []
    positions = []
    pos = 1

    for tc in tcs:
        for i, scenario in enumerate(scenarios):
            d = np.array(data_delay[scenario][tc])
            if len(d) == 0:
                d = np.array([0])

            q1, med, q3 = np.percentile(d, [25, 50, 75])
            iqr = q3 - q1
            whislo = np.min(d[d >= q1 - 1.5 * iqr]) if np.any(d >= q1 - 1.5 * iqr) else np.min(d)
            whishi = np.max(d[d <= q3 + 1.5 * iqr]) if np.any(d <= q3 + 1.5 * iqr) else np.max(d)
            outliers = d[(d < whislo) | (d > whishi)]
            grouped_outliers = [np.max(outliers)] if len(outliers) > 0 else []

            stats.append({
                "med": med,
                "q1": q1,
                "q3": q3,
                "whislo": whislo,
                "whishi": whishi,
                "fliers": grouped_outliers,
                "edgecolor": colors[i % len(colors)],
                "hatch": hatch_patterns[i % len(hatch_patterns)]
            })
            positions.append(pos)
            pos += 0.7
        pos += 1.5

    fig, ax = plt.subplots(figsize=(3.2, 3))

    for i, s in enumerate(stats):
        ax.bxp(
            [s],
            positions=[positions[i]],
            patch_artist=True,
            showfliers=True,
            boxprops=dict(facecolor='white', edgecolor=s["edgecolor"], hatch=s.get("hatch", '')),
            medianprops=dict(color='black'),
            flierprops=dict(
                marker='*',
                color='black',
                markersize=5,
                alpha=1,
                markerfacecolor='black',
                markeredgecolor='black',
            ),
            widths=0.6,
            manage_ticks=True
        )

    for i, s in enumerate(stats):
        ax.plot(positions[i], s["med"], marker='x', color='black', markersize=6, mew=1.5)

    mid_positions = []
    start = 1
    for i in range(len(tcs)):
        start_pos = start + i * (len(scenarios) * 0.7 + 1.5)
        end_pos = start_pos + (len(scenarios) - 1) * 0.7
        mid_positions.append((start_pos + end_pos) / 2)

    ax.set_xticks(mid_positions)
    ax.set_xticklabels(tcs, fontsize=8)
    ax.set_ylim(0.1, 500)

    legend_labels = [
        "GF, adaptive MCS + Dyn. Priority (MDBV)",
        "GF, adaptive MCS + Dyn. MAX C/I (MDBV)",
        "GF, adaptive MCS + Dyn. PF (MDBV)",
    ]

    add_custom_legend(
        ax,
        scenarios,
        colors,
        hatch_patterns,
        legend_labels,
        ncol=1,
        bbox_to_anchor=(0.5, 1.28),
        fontsize=8
    )

    ax.set_ylabel("End-to-End Delay [ms]", fontsize=8)
    ax.set_yscale("log")
    ax.grid(True, linestyle='--', alpha=0.5)

    fig.subplots_adjust(top=0.74, bottom=0.14, left=0.16, right=0.98)
    tikzplotlib.save(filename)
    plt.savefig(filename.replace(".tex", ".png"), dpi=300)


def main():
    tc_groups = {
        "TC7": list(range(0, 8)),
        "TC6": list(range(8, 16)),
        "TC5": list(range(16, 24)),
        "TC4": list(range(24, 32)),
        "TC1": list(range(32, 40)),
        "TC0": list(range(40, 48))
    }

    scenario_data = read_in_data(
        scenarios=["Priority_MDBV", "MAXCI_MDBV", "PF_MDBV", "Priority", "MAXCI", "PF"],
        number_of_runs=10,
        number_of_apps=48,
        base_path="./results/heterogenous"
    )
    print("Data read and aggregated for all scenarios.")

    e2e_delay_percentiles(
        scenarios=["Priority_MDBV", "MAXCI_MDBV", "PF_MDBV", "Priority", "MAXCI", "PF"],
        tc_selected="TC4",
        tc_groups=tc_groups,
        scenario_data=scenario_data,
        number_of_runs=10,
        filename="plots/heterogenous/Fig5b_e2e_percentile.tex",
        number_of_apps=48,
        style_map={
            "Priority_MDBV": {"color": "orange", "hatch": "/"},
            "MAXCI_MDBV": {"color": "blue", "hatch": "\\"},
            "PF_MDBV": {"color": "green", "hatch": "x"},
            "Priority": {"color": "red", "hatch": "/"},
            "MAXCI": {"color": "gray", "hatch": "\\"},
            "PF": {"color": "violet", "hatch": "x"}
        }
    )

    cv_throughput(
        scenarios=["Priority_MDBV", "MAXCI_MDBV", "PF_MDBV"],
        tcs=["TC6", "TC5", "TC4"],
        tc_groups=tc_groups,
        scenario_data=scenario_data,
        number_of_runs=10,
        filename="plots/heterogenous/Fig5c_cv_tp.tex"
    )

    e2e_delay_distribution(
        scenarios=["Priority_MDBV", "MAXCI_MDBV", "PF_MDBV"],
        tcs=["TC6", "TC5", "TC4"],
        tc_groups=tc_groups,
        scenario_data=scenario_data,
        number_of_runs=10,
        number_of_apps=48,
        filename="plots/heterogenous/Fig5a_e2e_distribution.tex"
    )


if __name__ == "__main__":
    main()