import sys
import pandas as pd

########################################

if __name__ == '__main__':
    if len(sys.argv) == 3:
        remove_old_files = False
        infile = sys.argv[1]
        outfile = sys.argv[2]
    elif len(sys.argv) == 4:
        remove_old_files = (sys.argv[1] in ('--remove', '-r'))
        infile = sys.argv[2]
        outfile = sys.argv[3]
    else:
        exit()

    if remove_old_files:
        print('WARNING: the instructions in temp.sh include commands'
            + '\nthat remove any previous output files.  Input "okay"'
            + '\nto proceed or anything else to stop.')
        typed = input()
        if typed != 'okay':
            with open(outfile, 'w') as out:
                out.write('echo "Program was cancelled. File \'%s\' will be removed."\n' % outfile)
            exit()

    data = pd.read_csv(infile,
                        comment='#',
                        sep='\\s*,\\s*',
                        engine='python',
                        header=0,
                        dtype={
                            'NumThreads':int,
                            'PlotOnly':str,
                            'CountPlotAll':int,
                            'CountAnimate':int,
                            'FirstIdx':int,
                            'Count':int,
                            'NumLocs':int,
                            'VbEvery':int,
                            'CoolFlatEpochs':int
                        })

    seen_run_ids = set()
    with open(outfile, 'w') as out:
        out.write('# --------------------------------------------------\n')
        out.write('# This temporary file was automatically generated\n')
        out.write('# and should be deleted.\n')
        out.write('# --------------------------------------------------\n')
        out.write('\n')
        for row in data.itertuples(index=True, name=None):
            (idx, Ignore, NumThreads, PlotOnly, CountPlotAll, CountAnimate,
                FirstIdx, Count, NumLocs, NumEpochs, VbEvery,
                CoolInit, CoolBase, CoolFlatEpochs) = row

            out.write('# Row Begins ----------------------------------------\n')
            if Count <= 0:
                raise Exception('Row ' + str(idx) + ' has Count < 0.')

            new_ids = set(range(FirstIdx, FirstIdx + Count))
            if new_ids.isdisjoint(seen_run_ids):
                seen_run_ids = seen_run_ids.union(new_ids)
            else:
                raise Exception('Row ' + str(idx)
                + ' contains a duplicate run id.  Duplicate run ids: '
                + ''.join(map(str, sorted(list(new_ids.intersection(seen_run_ids))))))

            new_ids = list(new_ids)
            new_ids.sort()

            # **************************************************
            if (not pd.isna(PlotOnly)
                and PlotOnly.lower() in ('y', 'yes')):
                pass
            else:
                for run_id in new_ids:
                    out.write('rm -f ./input/directions-%d.txt\n' % run_id)
                    out.write('rm -rf ./output/run-%d\n' % run_id)
                for run_id in new_ids:
                    out.write('cd ./pyth ; python3 setup.py %d %d ; cd ../ ;\n'
                                            % (run_id, NumLocs))
                out.write('./Release/telsimanneal '
                            + ' '.join(map(str,
                                    [NumThreads, NumEpochs, VbEvery,
                                     CoolInit, CoolBase,
                                     CoolFlatEpochs]))
                            + ' '
                            + ' '.join(map(str, new_ids)) + ';\n')

            # **************************************************
            # Automatically plot everything and animate the first run id,
            # but for any other run ids, this will only plot the last
            # epoch and the greedy solution.
            for run_id in new_ids:
                out.write('rm -f ./output/run-%d/greedy-schedule.p*\n' % run_id)
                out.write('rm -f ./output/run-%d/simanneal-best-*.p*\n' % run_id)
                out.write('rm -f ./output/run-%d/simanneal-animation*.p*\n' % run_id)
            new_ids = list(map(str, new_ids))
            if CountPlotAll > Count:
                out.write("# WARNING:  Requested more plots than run ids.\n")
                CountPlotAll = Count
            if CountAnimate > Count:
                out.write("# WARNING:  Requested more animations than run ids.\n")
                CountAnimate = Count
            out.write('cd ./pyth\n')
            if CountPlotAll > 0:
                out.write('python3 plot.py --all ' + ' '.join(new_ids[:CountPlotAll]) + '\n')
            if CountAnimate > 0:
                out.write('python3 animate.py ' + ' '.join(new_ids[:CountAnimate]) + '\n')
            if CountPlotAll < Count:
                out.write('python3 plot.py ' + ' '.join(new_ids[CountPlotAll:]) + '\n')
            out.write('cd ../\n')

        # End of loop through rows of run details.
        # **************************************************
        # Finally, generate all summary plots.
        out.write('rm -rf ./output-summary\n')
        out.write('cd ./pyth\n')
        out.write('python3 summary.py\n')
        out.write('cd ../\n')
        out.close()
