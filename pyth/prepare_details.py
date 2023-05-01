import sys
import pandas as pd

########################################

if __name__ == '__main__':
    if len(sys.argv) != 2:
        exit()
    file = sys.argv[1]

    data = pd.read_csv('../run_details.csv',
                        comment='#',
                        sep='\\s*,\\s*',
                        engine='python',
                        header=0,
                        dtype={
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
    print('# This temporary file was automatically generated')
    print('# and should be deleted.')
    print('# --------------------------------------------------')
    print('\n')
    for row in data.itertuples(index=True, name=None):
        (idx, Ignore, PlotOnly, CountPlotAll, CountAnimate,
            FirstIdx, Count, NumLocs, NumEpochs, VbEvery,
            CoolInit, CoolBase, CoolFlatEpochs) = row

        print('# Row Begins ----------------------------------------')
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
                print('rm -f ./input/directions-%d.txt' % run_id)
                print('rm -rf ./output/run-%d' % run_id)
            for run_id in new_ids:
                print('cd ./pyth ; python3 setup.py %d %d ; cd ../ ;'
                                        % (run_id, NumLocs))
                print('./Release/telsimanneal ', run_id, NumEpochs, VbEvery,
                    CoolInit, CoolBase, CoolFlatEpochs, ';')

        # **************************************************
        # Automatically plot everything and animate the first run id,
        # but for any other run ids, this will only plot the last
        # epoch and the greedy solution.
        for run_id in new_ids:
            print('rm -f ./output/run-%d/greedy-schedule.p*' % run_id)
            print('rm -f ./output/run-%d/simanneal-best-*.p*' % run_id)
            print('rm -f ./output/run-%d/simanneal-animation*.p*' % run_id)
        new_ids = list(map(str, new_ids))
        if CountPlotAll > Count:
            print("# WARNING:  Requested more plots than run ids.")
            CountPlotAll = Count
        if CountAnimate > Count:
            print("# WARNING:  Requested more animations than run ids.")
            CountAnimate = Count
        print('cd ./pyth')
        if CountPlotAll > 0:
            print('python3 plot.py --all ' + ' '.join(new_ids[:CountPlotAll]))
        if CountAnimate > 0:
            print('python3 animate.py ' + ' '.join(new_ids[:CountAnimate]))
        if CountPlotAll < Count:
            print('python3 plot.py ' + ' '.join(new_ids[CountPlotAll:]))
        print('cd ../')

    # End of loop through rows of run details.
    # **************************************************
    # Finally, generate all summary plots.
    print('rm -rf ./output-summary')
    print('cd ./pyth')
    print('python3 summary.py')
    print('cd ../')
