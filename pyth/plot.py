from setup import *

import matplotlib.pyplot as plt

########################################

def plot_on_axes(dir_schedule, ax):
    marker_size = 4
    title = 'Total distance: %.5f' % total_distance_dirs(dir_schedule)
    ax.set_title(title)
    ax.set_rticks([sin(pi/6.), sin(pi/3.)],
                labels=['$30^{\\circ}$', '$60^{\\circ}$'])

    coords = [d.radial_plot_coords() for d in dir_schedule]
    THETAS = [t for (t,p) in coords]
    PHIS   = [p for (t,p) in coords]
    ax.plot(
        THETAS,
        PHIS,
        marker='o',
        markersize=marker_size,
        linewidth=1,
        color='blue',
        zorder=0
    )
    coords = [d.radial_plot_coords() for d in dir_schedule if d.is_other_rep()]
    THETAS_OTHER_REP = [t for (t,p) in coords]
    PHIS_OTHER_REP   = [p for (t,p) in coords]
    plt.plot(
        THETAS_OTHER_REP,
        PHIS_OTHER_REP,
        marker='o',
        markersize=marker_size,
        color='orange',
        linewidth=0,
        zorder=1
    )
    # Starting point:
    ax.plot(
        [THETAS[0]],
        [PHIS[0]],
        marker='o',
        markersize=marker_size,
        color='green',
        zorder=2
    )

def plot_schedule_dir(dir_schedule, save_file, figuresize=(4,4)):
    assert(save_file.endswith('.pdf') or save_file.endswith('.png'))

    # Now generate a polar coordinates plot, with radius = sin(phi).
    # For reference, see:
    # https://matplotlib.org/stable/gallery/pie_and_polar_charts/polar_scatter.html
    f = plt.figure(
        figsize=figuresize
    )

    ax = f.add_subplot(projection='polar')
    plot_on_axes(dir_schedule, ax)
    plt.tight_layout()
    plt.savefig(save_file, dpi=300)
    plt.close()

########################################

def read_best_schedule_in(filename):
    schedule = []
    with open(filename, 'r') as f:
        while f.readline().strip() != 'Best State:':
            continue
        while (line := f.readline().strip()) != '-'*50:
            idstr, thetastr, phistr = line.split(', ')
            id = int(idstr.split('=')[1])
            theta = float(thetastr.split('=')[1])
            phi = float(phistr.split('=')[1][:-1]) # drop the ending ')'
            schedule.append(Direction(id, theta, phi))
        f.close()
    return schedule

def read_greedy_schedule_in(filename):
    schedule = []
    with open(filename, 'r') as f:
        while f.readline().strip() != 'Greedy solution:':
            continue
        while (line := f.readline().strip()) != '-'*50:
            idstr, thetastr, phistr = line.split(', ')
            id = int(idstr.split('=')[1])
            theta = float(thetastr.split('=')[1])
            phi = float(phistr.split('=')[1][:-1]) # drop the ending ')'
            schedule.append(Direction(id, theta, phi))
        f.close()
    return schedule

########################################

if __name__ == '__main__':
    if len(sys.argv) <= 1:
        exit()
    if sys.argv[1] == '--all':
        only_plot_last = False
        RUN_IDS = sys.argv[2:]
    else:
        only_plot_last = True
        RUN_IDS = sys.argv[1:]

    RUN_IDS = list(map(int, RUN_IDS))
    for run_id in RUN_IDS:
        folder = '../output/run-%d/' % run_id

        ## Given a file name "simanneal-***.txt" where *** is the epoch saved,
        ## this pulls the epoch number.
        pull_epoch_from_filename = \
            lambda filename: int( filename.split('-')[-1].split('.')[0] )

        state_files = [ (pull_epoch_from_filename(f), f)
                                for f in os.listdir(folder)
                                    if f.startswith('simanneal-')
                                    and f.endswith('.txt')
                                    and f != 'simanneal-full-log.txt' ]
        if only_plot_last:
            state_files = [ max(state_files) ]

        for epoch, f in state_files:
            schedule = read_best_schedule_in(folder + f)
            plot_schedule_dir(schedule, folder + 'simanneal-best-%d.png' % epoch)

        # Also plot the greedy solution:
        schedule = read_greedy_schedule_in(folder + 'greedy-solution.txt')
        plot_schedule_dir(schedule, folder + 'greedy-schedule.png')

        print('Done plotting run_id =', run_id)
