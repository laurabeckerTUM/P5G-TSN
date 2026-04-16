import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import re
import glob
import os
import math
import tikzplotlib
import scipy.stats as st

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

def simplify_name(full_name):
    match = re.match(r'(\w+):vector.*? (.+?) \(#', full_name)
    if match:
        vector_name = match.group(1)
        module_name = match.group(2)
        return f"{module_name}.{vector_name}"
    else:
        return full_name

def read_in_data(scenarios, number_of_runs, number_of_apps, base_path, deadlines):
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
                    "per_delay": 0
                }
                col = [c for c in values_only.columns if f"Listener.app[{app_id}].endToEndDelay" in c]
                if col:
                    scenario_data[scenario][run][app_id]["delay"].extend((values_only[col[0]].dropna() * 1000).round(3).tolist())
                    
                col = [c for c in values_only.columns if f"Listener.app[{app_id}].packetReceived" in c]
                if col:
                    scenario_data[scenario][run][app_id]["received"]=(len(values_only[col[0]].dropna().tolist()))
                    
                col = [c for c in values_only.columns if f"Talker0.app[{app_id}].packetSent" in c]
                if col:
                    scenario_data[scenario][run][app_id]["sent"]=(len(values_only[col[0]].dropna().tolist()))
                    
                if scenario_data[scenario][run][app_id]["sent"] > 0:
                    if (scenario_data[scenario][run][app_id]["received"]>scenario_data[scenario][run][app_id]["sent"]):
                        scenario_data[scenario][run][app_id]["per"] = 0
                        scenario_data[scenario][run][app_id]["per_delay"] = 0
                    else:
                        sent = scenario_data[scenario][run][app_id]["sent"]
                        received = scenario_data[scenario][run][app_id]["received"]
                        scenario_data[scenario][run][app_id]["per"] = 100*(sent-received)/sent
                        
                        if 0 <= app_id <= 13:
                            deadline = deadlines[1] 
                        elif 14 <= app_id <= 27:
                            deadline = deadlines[0] 
                        else:
                            raise ValueError(f"Unexpected app_id: {app_id}")
                        delays = scenario_data[scenario][run][app_id]["delay"]
                        received_in_time = sum(1 for d in delays if d <= deadline)         
                        scenario_data[scenario][run][app_id]["per_delay"] = 100 * (sent - received_in_time) / sent
                else:
                    print("no pkts sent"+ str(scenario) + str(run) + str(app_id))
                             
            col = [c for c in values_only.columns if f"gnb.cellularNic.mac.avgServedBlocksUl" in c]
            if col:
                scenario_data[scenario][run]["rb"]=(values_only[col[0]].dropna()).tolist()
                
    print("Data read and aggregated for all scenarios.")
    return scenario_data



def e2e_delay_distribution(scenarios, scenario_data, number_of_runs, number_of_apps, filename, deadlines):
    delays = {scenario: {"app0": [], "app1": []} for scenario in scenarios}

    for scenario in scenarios:
        delays_app0 = []
        delays_app1 = []

        for run in range(number_of_runs):
            for app_id in range(number_of_apps):
                listener_delay = scenario_data[scenario][run][app_id]["delay"]
                if app_id < 14:
                    delays_app0.extend(listener_delay)  
                else:
                    delays_app1.extend(listener_delay)  

        delays[scenario]["app0"] = delays_app0
        delays[scenario]["app1"] = delays_app1


    apps = ["app1", "app0"]

    group_spacing = 2
    hatch_patterns = ['/', '\\', 'x', '/', '\\']
    colors = ['orange', 'blue', 'green', 'red', 'gray']

    scenario_labels = {
        "Preallocation_adaptiveMCS": "GF, adaptive MCS",
        "Preallocation_CG5": "GF, MCS: 5",
        "Preallocation_CG11": "GF, MCS: 11",
        "Preallocation_CG20": "GF, MCS: 20",
        "Priority": r"Dyn. PDB"
    }

    stats = []
    positions = []
    pos = 1

    for app in apps:
        for i, s in enumerate(scenarios):
            d = np.array(delays[s][app])
            if len(d) == 0:
                d = np.array([0.0])

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
                "hatch": hatch_patterns[i % len(hatch_patterns)],
            })
            positions.append(pos)
            pos += 0.7
        pos += group_spacing - 1

    fig, ax = plt.subplots(figsize=(5.8/2.54, 5/2.54))

    for i, s in enumerate(stats):
        ax.bxp(
            [s],
            positions=[positions[i]],
            patch_artist=True,
            showfliers=True,
            boxprops=dict(
                facecolor='white',
                edgecolor=s["edgecolor"],
                hatch=s["hatch"]
            ),
            medianprops=dict(color='black'),
            flierprops=dict(
                marker='*',
                color='black',
                markersize=8,  
                alpha=1,
                markerfacecolor='black',
                markeredgecolor='black',
            ),
            widths=0.6,
            manage_ticks=True,
        )

    for i, s in enumerate(stats):
        ax.plot(positions[i], s["med"], marker='x', color='black', markersize=6, mew=1.5)

    mid_first = (positions[0] + positions[len(scenarios) - 1]) / 2
    mid_second = (positions[len(scenarios)] + positions[-1]) / 2
    ax.set_xticks([mid_first, mid_second])

    ax.set_xticklabels(["TC 6", "TC 5"], fontsize=8)

    legend_handles = []
    for i, s in enumerate(scenarios):
        handle = plt.Rectangle(
            (0, 0), 1, 1,
            facecolor='white',
            edgecolor=colors[i % len(colors)],
            hatch=hatch_patterns[i % len(hatch_patterns)],
            linewidth=1.0
        )
        legend_handles.append(handle)

    deadline_handle = plt.Line2D(
        [0], [0],
        color='red',
        linestyle='--',
        linewidth=1.5
    )

    legend_labels = [scenario_labels.get(s, s) for s in scenarios] + ["max. path delay"]

    ax.legend(
        legend_handles + [deadline_handle],
        legend_labels,
        ncol=3,
        fontsize=8,
        loc='upper center',
        bbox_to_anchor=(0.5, 1.32),
        framealpha=0.8
    )

    ax.set_ylabel("End-to-End Delay [ms]", fontsize=8)
    ax.set_yscale("log")
    ax.set_ylim(0.5, 400)
    ax.set_xlim(-0.86, 8.8)
    ax.grid(True, linestyle='--', alpha=0.5)

    ax.hlines(deadlines[0], xmin=0, xmax=4.0, colors='red', linestyles='--', linewidth=1.5)
    ax.hlines(deadlines[1],xmin=5, xmax=9.0, colors='red', linestyles='--', linewidth=1.5)

    plt.tight_layout(pad=0.3)

    tikzplotlib.save(filename, axis_width="5.8cm", axis_height="5cm")
    plt.savefig(filename.replace(".tex", ".png"), dpi=600, bbox_inches="tight", facecolor="white")


def per_rb_utilization(scenarios, scenario_data, number_of_runs, number_of_apps, filename):
    MAX_RB = 106

    run_avg_per = {}
    run_used_rb = {}

    for scenario in scenarios:
        run_avg_per[scenario] = []
        run_used_rb[scenario] = []

        for run in range(number_of_runs):
            per_values_run = []
            for app_id in range(number_of_apps):
                per_values_run.append(scenario_data[scenario][run][app_id]["per_delay"])

            avg_run_per = np.mean(per_values_run)
            run_avg_per[scenario].append(avg_run_per)

            used_rbs = 0
            max_rbs = 0
            for elem in scenario_data[scenario][run]["rb"]:
                used_rbs += elem
                max_rbs += MAX_RB

            run_used_rb[scenario].append(100 * used_rbs / max_rbs)

    means_per, cis_per, means_rb, cis_rb = [], [], [], []

    for scenario in scenarios:
        arr_per = np.array(run_avg_per[scenario])
        if len(arr_per) == 0:
            mean_per, ci_low, ci_high = np.nan, np.nan, np.nan
        elif len(arr_per) == 1:
            mean_per = arr_per[0]
            ci_low = ci_high = mean_per
        else:
            mean_per = np.mean(arr_per)
            ci_low, ci_high = st.t.interval(
                0.95, len(arr_per) - 1, loc=mean_per, scale=st.sem(arr_per)
            )
        means_per.append(mean_per)
        cis_per.append((mean_per - ci_low, ci_high - mean_per))

        arr_rb = np.array(run_used_rb[scenario])
        if len(arr_rb) == 0:
            mean_rb, ci_low, ci_high = np.nan, np.nan, np.nan
        elif len(arr_rb) == 1:
            mean_rb = arr_rb[0]
            ci_low = ci_high = mean_rb
        else:
            mean_rb = np.mean(arr_rb)
            ci_low, ci_high = st.t.interval(
                0.95, len(arr_rb) - 1, loc=mean_rb, scale=st.sem(arr_rb)
            )
        means_rb.append(mean_rb)
        cis_rb.append((mean_rb - ci_low, ci_high - mean_rb))


    custom_labels = [
        "GF\nMCS\nadaptive",
        "GF\nMCS\n5",
        "GF\nMCS\n11",
        "GF\nMCS\n20",
        "Dyn."
    ]

    x = np.arange(len(scenarios))
    width = 0.28
    offset = 0.03

    fig, ax1 = plt.subplots(figsize=(8/2.54, 6.0/2.54))

    ax1.set_ylabel("Average PER [%]", fontsize=8)
    ax1.set_ylim(0, 100)
    ax1.set_xlim(-0.541, 4.541)
    ax1.grid(axis="y", linestyle="--", alpha=0.4)

    ax1.set_xticks(x)
    ax1.set_xticklabels(custom_labels, fontsize=8, ha="center")

    bars1 = ax1.bar(
        x - width / 2 - offset,
        means_per,
        width,
        yerr=np.array(cis_per).T,
        capsize=3,
        label="PER",
        facecolor="white",
        edgecolor="blue",
        hatch="//"
    )

    ax2 = ax1.twinx()
    ax2.set_ylabel("RB Utilization [%]", fontsize=8)
    ax2.set_xlim(-0.541, 4.541)
    ax2.set_ylim(0, 100)

    bars2 = ax2.bar(
        x + width / 2 + offset,
        means_rb,
        width,
        yerr=np.array(cis_rb).T,
        capsize=3,
        label="Used RBs",
        facecolor="white",
        edgecolor="orange",
        hatch="\\\\"
    )

    leg1 = ax1.legend(
        [bars1],
        ["PER"],
        loc="upper left",
        bbox_to_anchor=(0.02, 0.98),
        frameon=True,
        fontsize=8,
        borderaxespad=0.0
    )
    ax1.add_artist(leg1)

    ax2.legend(
        [bars2],
        ["Used RBs"],
        loc="upper left",
        bbox_to_anchor=(0.02, 0.85),
        frameon=True,
        fontsize=8,
        borderaxespad=0.0
    )

    plt.tight_layout(pad=0.3)

    tikzplotlib.save(filename, axis_width="5.8cm", axis_height="5cm")

    plt.savefig(filename.replace(".tex", ".png"), dpi=600, bbox_inches="tight", facecolor="white")


def main():
    scenarios=["Preallocation_adaptiveMCS","Preallocation_CG5", "Preallocation_CG11", "Preallocation_CG20","Priority"]
    number_of_runs=10
    number_of_apps=28

    scenario_data = read_in_data(scenarios, number_of_runs, number_of_apps, "./results/periodic/", deadlines=[0.72548,5.77516])

    e2e_delay_distribution(scenarios, scenario_data, number_of_runs, number_of_apps, "plots/periodic/Fig4a_e2e_delay.tex", deadlines=[0.72548,5.77516])

    per_rb_utilization(scenarios, scenario_data, number_of_runs, number_of_apps, "plots/periodic/Fig4b_per_rb_utilization.tex")


if __name__ == "__main__":
    main()
