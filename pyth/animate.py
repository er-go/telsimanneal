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
            if frame >= num_anim_frames:
                # If beyond the last frame, then in the "pause" before
                # the animation repeats.  (The pause is added manually
                # because it is not automatically included in the
                # animated PNG file.)
                frame = num_anim_frames-1
            ax.cla()
            plot_on_axes(read_best_schedule_in(state_files[frame]), ax)

        # Finally, build the animation and save it. For a reference about
        # how to do this, see:
        # https://matplotlib.org/stable/api/animation_api.html
        import matplotlib.animation

        end_pause_seconds = 3
        for time_seconds in [20, 15, 10, 5]:
            ms_per_frame = (time_seconds * 1000) // num_anim_frames

            animation = matplotlib.animation.FuncAnimation(
                                fig,
                                draw,
                                fargs=None,
                                interval = ms_per_frame,
                                frames = ( num_anim_frames
                                    + ((end_pause_seconds * 1000)
                                                // ms_per_frame) ),
                                repeat=True,
                                repeat_delay=0
                                )
            animation.save(folder + 'simanneal-animation-'
                                + str(time_seconds) + 's.png')
