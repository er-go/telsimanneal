"""
ANIMATE.PY
==========
Takes command line arguments that indicate indicate the run ids for
which to build an animation.  The animation will be saved as an
animated PNG at three speeds.
"""

if __name__ == '__main__':
    from plot import *

    if len(sys.argv) <= 1:
        exit()
    for run_id in map(int, sys.argv[1:]):
        folder = '../output/run-%d/' % run_id

        ## Given a file name "simanneal-***.txt" where *** is the epoch saved,
        ## this pulls the epoch number.
        pull_epoch_from_filename = \
            lambda filename: int( filename.split('-')[-1].split('.')[0] )
        state_files = [ (pull_epoch_from_filename(f), folder + f)
                            for f in os.listdir(folder)
                                    if f.startswith('simanneal-')
                                    and f.endswith('.txt')
                                    and f != 'simanneal-full-log.txt' ]
        state_files.sort()
        state_files = [f for (epoch, f) in state_files]

        num_anim_frames = len(state_files)
        fig = plt.figure(figsize=(4,4))
        ax = fig.add_subplot(projection='polar')

        def draw(frame):
            ax.cla()
            plot_on_axes(read_best_schedule_in(state_files[frame]), ax)

        # Finally, build the animation and save it. For a reference about
        # how to do this, see:
        # https://matplotlib.org/stable/api/animation_api.html
        import matplotlib.animation

        for ms_per_frame, filename_extra in [
                (500, ''),
                (200, '-fast'),
                (100, '-faster'),
            ]:
            animation = matplotlib.animation.FuncAnimation(
                                fig,
                                draw,
                                fargs=None,
                                frames=num_anim_frames,
                                interval=ms_per_frame, # ms per frame
                                repeat=True,
                                repeat_delay=5000
                                )
            animation.save(folder + 'simanneal-animation' + filename_extra + '.png')
