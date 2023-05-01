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

def list_state_files(run_id):
    state_files = [ f for f in os.listdir(OUTPUT_FORMAT % run_id)
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

def list_epochs_and_files(run_id):
    state_files = list_state_files(run_id)
    return list(zip( map(pull_epoch, state_files),
                     state_files))

def list_epochs(run_id):
    return [e for (e,f) in list_epochs_and_files(run_id)]

##################################################

def read_sa_settings(run_id):
    output_folder = OUTPUT_FORMAT % run_id
    state_files = list_state_files(run_id)
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
    # Also must keep track of the last epoch saved:
    last_epochs = max(list_epochs(run_id))
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

        # Next, load all of the data for each run_id in this group.

        greedy_scores  = {}
        greedy_times   = {}
        simanneal_data = {}

        for run_id in run_id_list:
            run_id_folder = OUTPUT_FORMAT % run_id
            ## Record the greedy algorithm distance score and time to compute:
            with open(run_id_folder + 'greedy-solution.txt', 'r') as f:
                f.readline() # Skip the line "Run id: ...."
                score = float(f.readline().split(': ')[-1].strip())
                nanos = float(f.readline().split(': ')[-1].strip())
                greedy_scores[run_id] = score
                greedy_times[run_id] = nanos
                f.close()

            ## Record the simulated annealing improvements versus time.
            simanneal_data[run_id] = \
                pd.read_csv(run_id_folder + 'simanneal-full-log.txt',
                                    skiprows=3,
                                    header=0,
                                    sep=', ',
                                    engine='python')

        ##################################################
        ##
        ## BEGIN PLOTTING HERE
        ##
        ##################################################

        FIGSIZE = (3.5, 2.8)
        DPI = 300
        legendparams = dict(
            loc='lower left',
            bbox_to_anchor=(0,1)
        )

        grp_size = len(run_id_list)

        ##################################################

        plt.figure(
            figsize=FIGSIZE,
            dpi=DPI
        )
        # plt.title('Best Total Distance by Epoch')
        plt.xlabel('Simulated Annealing Epoch')
        plt.ylabel('Total Distance')

        num_simanneal_better = sum(
            1 for run_id in run_id_list
            if (simanneal_data[run_id]['Best Objective'].iloc[-1]
                    < greedy_scores[run_id])
        )
        num_greedy_better = grp_size - num_simanneal_better
        for run_id in run_id_list:
            sad = simanneal_data[run_id]
            plt.plot(sad['Epoch'], sad['Best Objective'],
                    drawstyle='steps-post',
                    color='blue',
                    alpha=0.5,
                    linewidth=1)
            # Now highlight the portion where the simulated annealing solution
            # is better:
            gr_score = greedy_scores[run_id]
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
        fname = GROUP_FOLDER + 'distance-by-epoch-best.png'
        plt.tight_layout()
        plt.savefig(fname)
        plt.close()

        ##################################################

        plt.figure(
            figsize=FIGSIZE,
            dpi=DPI
        )
        # plt.title('Greedy Speed vs. Time for\nAnnealing to Match')

        match_times = {}
        no_match = []
        for run_id in run_id_list:
            gr_score = greedy_scores[run_id]
            anneal_better = (simanneal_data[run_id]['Best Objective']
                                < gr_score)
            if anneal_better.any():
                idx = anneal_better.argmax() # First true
                match_times[run_id] = \
                        simanneal_data[run_id]['Wall Time (ns)'].iloc[idx]
            else:
                no_match.append(run_id)

        ## Convert from nanoseconds to microseconds, just for better
        ## readability:
        times_greedy = [gt / 1000.0 for gt in greedy_times.values()]
        times_match  = [mt / 1000.0 for mt in match_times.values()]
        time_units = 'microseconds'

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
                    label='Annealing Times (%d/%d)'
                                % (grp_size - len(no_match), grp_size),
                    alpha=0.5)
        ax.legend(**legendparams)
        ax.set_ylabel('Count')
        ax.set_xlabel('Time (%s)' % time_units)

        fname = GROUP_FOLDER + 'speed-comparison.png'
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
            greedy_scores[run_id]
                - simanneal_data[run_id]['Best Objective'].iloc[-1]
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

        fname = GROUP_FOLDER + 'score-comparison.png'
        plt.tight_layout()
        plt.savefig(fname)
        plt.close()
