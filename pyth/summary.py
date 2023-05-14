"""
SUMMARY.PY
==========
Call summary.py with NO arguments.  It will automatically detect all of the
run ids within the ../output folder.  It will also automatically break these
run ids into groups based on (a) number of direction requests and (b) annealing
method.
"""

from collections import defaultdict

import matplotlib.pyplot as plt
import pandas as pd

from setup import *

SUMMARY_FOLDER = '../output-summary/'
OUTPUT_FULL    = '../output/'
OUTPUT_FORMAT  = '../output/run-%d/'
INPUT_FILE_FORMAT = '../input/directions-%d.txt'

##################################################

def list_state_files(run_id, without_second_rep):
    folder = (OUTPUT_FORMAT % run_id) + (
                    'no-second-rep/' if without_second_rep
                    else '')
    state_files = [ f for f in os.listdir(folder)
                            if (f.startswith('simanneal-') and
                                f.endswith('.txt')
                                and f != 'simanneal-full-log.txt') ]
    return state_files

def pull_epoch(f):
    # Pulls the epoch, ***, from a file name "simanneal-***.txt"
    while ('/' in f):
        f = f[1:]
    try:
        return int(f.replace('simanneal-', '').replace('.txt', ''))
    except:
        raise Exception('Asked to pull epoch from file name "%s"' % f)

def list_epochs_and_files(run_id, without_second_rep):
    state_files = list_state_files(run_id, without_second_rep)
    return list(zip( map(pull_epoch, state_files),
                     state_files))

def list_epochs(run_id, without_second_rep):
    return [e for (e,f) in list_epochs_and_files(run_id,
                                        without_second_rep)]

##################################################

def read_sa_settings(run_id):
    output_folder = OUTPUT_FORMAT % run_id
    state_files = list_state_files(run_id, False)
    with open(output_folder + state_files[0], 'r') as f:
        cooling_method = ''
        while f.readline().strip() != 'Cooling method description:':
            continue
        while (the_line := f.readline()).strip() != '-'*50:
            cooling_method += the_line
        f.close()
    # Also must keep track of the number of locations:
    input_file = INPUT_FILE_FORMAT % run_id
    with open(input_file, 'r') as f:
        f.readline() # Throw away the run id
        num_locs = int(f.readline().split(': ')[-1].strip())
    # Also must keep track of the last epoch saved, which will
    # be the same both with and without the second representations.
    last_epochs = max(list_epochs(run_id, False))
    return (num_locs, last_epochs, cooling_method)

##################################################

if __name__ == '__main__':
    folders = [f for f in os.listdir(OUTPUT_FULL)
                if f.startswith('run-')]
    RUN_IDS = [int(f.split('run-')[-1]) for f in folders]

    os.makedirs(SUMMARY_FOLDER, exist_ok=True)

    # First, read all of the different group settings and place
    # each run_id into a list with its group.
    GROUPS = defaultdict(lambda: [])
    for run_id in RUN_IDS:
        settings = read_sa_settings(run_id)
        GROUPS[settings].append(run_id)

    # Now, for each group, generate plots and summary files.
    for (group_num, (settings, run_id_list)) \
                        in enumerate(sorted(GROUPS.items()), start=1):
        GROUP_FOLDER = SUMMARY_FOLDER + ('group-%d/' % group_num)
        os.makedirs(GROUP_FOLDER, exist_ok=True)

        with open(GROUP_FOLDER + 'group-simanneal-settings.txt', 'w') as f:
            (num_locs, last_epoch, cooling_method) = settings
            f.write('Rounds in this group all have\n')
            f.write('  num_locs = %d\n' % num_locs)
            f.write('location requests.\n')
            f.write('-'*50 + '\n')
            f.write('Cooling method for this group was:\n')
            f.write(cooling_method)
            f.write('Run for %d epochs.\n' % last_epoch)
            f.write('-'*50 + '\n')
            f.write('This group contains %d rounds, round numbers:\n'
                            % len(run_id_list))
            f.write(', '.join(map(str, run_id_list)) + '\n')
            f.close()

        ##################################################
        ## BEGIN LOADING DATA FOR THIS GROUP
        ##################################################

        # Next, load all of the data for each run_id in this group.
        # The key to each dictionary is the parameter
        # without_second_rep.
        greedy_scores  = {False: {}, True: {}}
        greedy_times   = {False: {}, True: {}}
        simanneal_data = {False: {}, True: {}}

        for run_id in run_id_list:
            for no_sr in [False, True]:
                ## Note:  no_sr = no second rep = without_second_rep
                run_id_folder = (OUTPUT_FORMAT % run_id) + (
                                    'no-second-rep/' if no_sr
                                    else '')
                ## Record the greedy algorithm distance score and time
                ## to compute:
                with open(run_id_folder + 'greedy-solution.txt','r') \
                        as f:
                    f.readline() # Skip the line "Run id: ...."
                    score = float(f.readline().split(': ')[-1].strip())
                    nanos = float(f.readline().split(': ')[-1].strip())
                    greedy_scores[no_sr][run_id] = score
                    greedy_times[no_sr][run_id] = nanos
                    f.close()

                ## Record simulated annealing improvements vs. time.
                simanneal_data[no_sr][run_id] = \
                    pd.read_csv(run_id_folder + 'simanneal-full-log.txt',
                                        skiprows=3,
                                        header=0,
                                        sep=', ',
                                        engine='python')

        ##################################################
        ## DONE LOADING DATA FOR THE GROUP
        ##
        ## BEGIN PLOTTING HERE
        ##################################################

        FIGSIZE = (3.5, 2.8)
        DPI = 300
        legendparams = dict(
            loc='lower right',
            bbox_to_anchor=(1,1)
        )

        grp_size = len(run_id_list)

        ##################################################

        for no_sr in [False, True]:
            plt.figure(
                figsize=FIGSIZE,
                dpi=DPI
            )
            plt.xlabel('Simulated Annealing Epoch')
            plt.ylabel('Total Distance')

            num_simanneal_better = sum(
                1 for run_id in run_id_list
                if (simanneal_data[no_sr][run_id]['Best Objective'].iloc[-1]
                        < greedy_scores[no_sr][run_id])
            )
            num_greedy_better = grp_size - num_simanneal_better
            for run_id in run_id_list:
                sad = simanneal_data[no_sr][run_id]
                plt.plot(sad['Epoch'], sad['Best Objective'],
                        drawstyle='steps-post',
                        color='blue',
                        alpha=0.5,
                        linewidth=1)
                # Now highlight the portion where the simulated annealing solution
                # is better:
                gr_score = greedy_scores[no_sr][run_id]
                subset = sad.loc[sad['Best Objective'] < gr_score]
                plt.plot(subset['Epoch'], subset['Best Objective'],
                        drawstyle='steps-post',
                        color='green',
                        alpha=0.75,
                        linewidth=2)
            plt.legend([
                'Greedy better (%d/%d)' % (num_greedy_better, grp_size),
                'Annealing better (%d/%d)' % (num_simanneal_better, grp_size)
                ],
                **legendparams)
            fname = GROUP_FOLDER + (
                'nosr-' if no_sr
                else ''
            ) + 'distance-by-epoch-best.png'
            plt.tight_layout()
            plt.savefig(fname)
            plt.close()

            ##################################################

            plt.figure(
                figsize=FIGSIZE,
                dpi=DPI
            )
            match_times = {}
            no_match = []
            for run_id in run_id_list:
                gr_score = greedy_scores[no_sr][run_id]
                anneal_better = (simanneal_data[no_sr][run_id]['Best Objective']
                                    < gr_score)
                if anneal_better.any():
                    idx = anneal_better.argmax() # First true
                    match_times[run_id] = \
                            simanneal_data[no_sr][run_id]['Wall Time (ns)'].iloc[idx]
                else:
                    no_match.append(run_id)

            ## Convert from nanoseconds to milliseconds, just for better
            ## readability:
            times_greedy = [gt / 1_000_000.0 for gt in greedy_times[no_sr].values()]
            times_match  = [mt / 1_000_000.0 for mt in match_times.values()]
            time_units = 'milliseconds'

            all_values = times_greedy + times_match
            common_bins = np.linspace(0.9*min(all_values), 1.1*max(all_values), 30)
            ax = plt.subplot(1,1,1)
            ax.hist(times_greedy,
                        bins=common_bins,
                        color='blue',
                        label='Greedy Times',
                        alpha=0.5)
            ax.hist(times_match,
                        bins=common_bins,
                        color='green',
                        label='Annealing Times to Match (%d/%d)'
                                    % (grp_size - len(no_match), grp_size),
                        alpha=0.5)
            ax.legend(**legendparams)
            ax.set_ylabel('Count')
            ax.set_xlabel('Time (%s)' % time_units)

            fname = GROUP_FOLDER + (
                'nosr-' if no_sr
                else ''
            ) + 'speed-comparison.png'
            plt.tight_layout()
            plt.savefig(fname)
            plt.close()

            ##################################################

            plt.figure(
                figsize=FIGSIZE,
                dpi=DPI
            )
            # plt.title('Improvement by Annealing')
            improvement = np.array([
                greedy_scores[no_sr][run_id]
                    - simanneal_data[no_sr][run_id]['Best Objective'].iloc[-1]
                for run_id in run_id_list
            ])
            M = np.abs(improvement).max()
            nbins = 21 ## Need an *odd* number so that 0 is a split point
            common_bins = np.linspace(-1.1*M, 1.1*M, nbins)
            ax = plt.subplot(1,1,1)
            ax.hist([i for i in improvement if i < 0],
                        bins=common_bins,
                        color='blue',
                        label='Greedy better (%d/%d)'
                                    % ((improvement < 0).sum(), grp_size))
            ax.hist([i for i in improvement if i > 0],
                        bins=common_bins,
                        color='green',
                        label='Annealing better (%d/%d)'
                                    % ((improvement > 0).sum(), grp_size))
            ax.legend(**legendparams)
            ax.set_ylabel('Count')
            ax.set_xlabel('Improvement in total distance')

            fname = GROUP_FOLDER + (
                'nosr-' if no_sr
                else ''
            ) + 'score-comparison.png'
            plt.tight_layout()
            plt.savefig(fname)
            plt.close()

        ##################################################
        # End of separate plots for no_sr True / False
        #
        # Now, plot comparisons between results with and
        # without the second representations.
        ##################################################

        marker_size = 5
        marker_shape = '.'

        for kind in ['Greedy', 'Annealing']:
            plt.figure(
                figsize=(3,3),
                dpi=DPI
            )
            plt.axes(aspect='equal')
            prevent_both_rep = kind + ' score, not allowing any\nalternative representations'
            allow_both_reps  = kind + ' score, allowing\nboth representations'
            plt.xlabel(prevent_both_rep)
            plt.ylabel(allow_both_reps)
            if kind == 'Greedy':
                df = pd.DataFrame(
                    [[greedy_scores[nsr][run_id]
                        for nsr in [False, True]]
                            for run_id in run_id_list],
                    columns=[allow_both_reps, prevent_both_rep]
                )
            else:
                df = pd.DataFrame(
                    [[simanneal_data[nsr][run_id].iloc[-1]['Best Objective']
                        for nsr in [False, True]]
                            for run_id in run_id_list],
                    columns=[allow_both_reps, prevent_both_rep]
                )

            v = df.mean().mean()
            plt.axline([v,v], slope=1,
                         color='black',
                         linewidth=1)

            subset = df.loc[df[allow_both_reps] < df[prevent_both_rep], :]
            descr  = 'Second representation improved score'
            plt.plot(subset[prevent_both_rep],
                        subset[allow_both_reps],
                        color='green',
                        alpha=1,
                        markersize=marker_size,
                        marker=marker_shape,
                        linewidth=0,
                        label=descr)

            subset = df.loc[df[allow_both_reps] >= df[prevent_both_rep], :]
            descr  = 'No improvement found'
            plt.plot(subset[prevent_both_rep],
                        subset[allow_both_reps],
                        color='red',
                        alpha=1,
                        markersize=marker_size,
                        marker=marker_shape,
                        linewidth=0,
                        label=descr)
            # plt.legend(**legendparams)
            fname = GROUP_FOLDER + kind + '-two-rep-improvement.png'
            plt.tight_layout()
            plt.savefig(fname)
            plt.close()

        ##################################################
