import numpy as np
import matplotlib.pyplot as plt
import scipy.stats.mstats
import os
from plot_utils import *

if __name__ == '__main__':
    methods = [
            'orig_sa',
            'ras_spec_w1_sa',
            'ras_spec_w4_sa',
            'ras_spec_w16_sa',
            ]

    labels = ['Baseline-Hit',
            'Baseline-Miss', 'Baseline-MatchF']
    labels_miss = ['Baseline-Miss', 'Baseline-MatchF']
    labels_split = []
    labels_missrate = ['Baseline']
    n_baseline = len(labels)
    system = 'RaS-Spec'
    params = [
            'W1',
            'W4', 
            'W16',
            ]
    stats  = ['Hit',
            'NF', 'ClearedPF', 'ClearedDF', 'F', 'MatchF', 'MatchNF']
    stats_miss = stats[1:]
    stats_split  = ['Remain', 'ClearedPF', 'ClearedDF']
    for param in params:
        labels_missrate.append(system+'-'+param)
        for stat in stats:
            labels.append(system+'-'+param+'-'+stat)
        for stat in stats_miss:
            labels_miss.append(system+'-'+param+'-'+stat)
        for stat in stats_split:
            labels_split.append(system+'-'+param+'-'+stat)
    assert len(methods) == len(labels_missrate)
    benchmarks = ['GemsFDTD', 'astar', 'bwaves', 'bzip2',
        'cactusADM', 'calculix', 'gamess', 'gcc', 'gobmk',
        'gromacs', 'h264ref', 'hmmer', 'lbm',
        'leslie3d', 'libquantum', 'mcf', 'milc', 'namd',
        'omnetpp', 'povray', 'sjeng', 'sphinx3', 'wrf',
        'zeusmp']
    n_b = len(benchmarks)
    n_m = len(methods)
    print('n_b: ', n_b, 'n_m', n_m)
    print_decimal = 2
    if not os.path.exists('results/'):
        os.makedirs('results/')
    for level in ['l1d', 'l2']:
        data_split = []
        data_missrate = np.zeros((n_b+3, 2*n_m-1))
        data_missrate_nodiff = np.zeros((n_b+3, n_m))
        missrate_column = []
        for i in range(n_m):
            method = methods[i]
            print(method)
            name = [
                    'ras'+level+'_demandAccessHit_500M',
                    'ras'+level+'_demandMshrCreated_500M',
                    'ras'+level+'_shbMshrCanFill_500M',
                    'ras'+level+'_nofillMshrServiced_500M',

                    'ras'+level+'_nofillHitFill_500M',
                    'ras'+level+'_nofillHitNofill_500M',
                    'ras'+level+'_demandHitFill_500M',
                    'ras'+level+'_demandHitNofill_500M',
                    'ras'+level+'_demandFillHitNofill_500M',
                    'ras'+level+'_demandAckHitNofill_500M',
                    ]
            demandhit = readstats_single(name[0], method, benchmarks)
            demandmshr= readstats_single(name[1], method, benchmarks)
            cleared_pf= readstats_single(name[2], method, benchmarks)
            remain    = readstats_single(name[3], method, benchmarks)
            nfhitf    = readstats_single(name[4], method, benchmarks)
            nfhitnf   = readstats_single(name[5], method, benchmarks)
            dhitf     = readstats_single(name[6], method, benchmarks)
            dhitnf    = readstats_single(name[7], method, benchmarks)
            dfhitnf   = readstats_single(name[8], method, benchmarks)
            dackhitnf = readstats_single(name[9], method, benchmarks)
            demandmis = demandmshr + dhitf + dhitnf
            miss_rate = np.divide(demandmis*100.0, demandhit + demandmis)
            if i == 0:
                data_missrate[:n_b,  0] = miss_rate
                data_missrate[ n_b,  0] = np.mean(miss_rate)
                data_missrate[n_b+1, 0] = np.amin(miss_rate)
                data_missrate[n_b+2, 0] = np.amax(miss_rate)
                missrate_column.append(method)
            else:
                data_missrate[:n_b,  2*i-1] = miss_rate
                data_missrate[ n_b,  2*i-1] = np.mean(miss_rate)
                data_missrate[n_b+1, 2*i-1] = np.amin(miss_rate)
                data_missrate[n_b+2, 2*i-1] = np.amax(miss_rate)
                diff_missrate = miss_rate - data_missrate[:n_b, 0]
                data_missrate[:n_b,  2*i] = diff_missrate
                data_missrate[ n_b,  2*i] = np.mean(diff_missrate)
                data_missrate[n_b+1, 2*i] = np.amin(diff_missrate)
                data_missrate[n_b+2, 2*i] = np.amax(diff_missrate)
                missrate_column.append(method)
                missrate_column.append(method+' - '+missrate_column[0])
            data_missrate_nodiff[:n_b,  i] = miss_rate
            data_missrate_nodiff[ n_b,  i] = np.mean(miss_rate)
            data_missrate_nodiff[n_b+1, i] = np.amin(miss_rate)
            data_missrate_nodiff[n_b+2, i] = np.amax(miss_rate)

            if level == 'l1d':
                hit_cpudata = readstats_single('l1dhit_cputdata_500M',
                        method, benchmarks)
                mis_cpudata = readstats_single('l1dmiss_cputdata_500M',
                        method, benchmarks)
                check_equal(hit_cpudata, demandhit)
                check_equal(mis_cpudata, demandmis)
                check_equal(dackhitnf)
                cleared_df = dfhitnf
                fig_level = 'L1D'
            else:
                assert level == 'l2', level
                check_equal(dfhitnf)
                cleared_df = dackhitnf
                fig_level = 'L2'

            if 'orig' in method:
                assert np.amax(np.abs(cleared_pf)) == 0, argmax(np.abs(cleared_pf))
                assert np.amax(np.abs(remain))  == 0, argmax(np.abs(remain))
                check_equal(nfhitf)
                check_equal(nfhitnf)
                check_equal(dhitnf)
                print_mmm_1d(dhitf, 'DemandHitOnFill '+level, benchmarks, 0)
            else:
                if 'plus' in method:
                    check_equal(nfhitf, dhitf, 'nfhitf dhitf different', benchmarks)
                    check_equal(nfhitnf, dhitnf, 'nfhitnf dhitnf different', benchmarks)
                    check_equal(cleared_df)

                total_nofillmshr = remain + cleared_pf + cleared_df
                split_remain     = np.divide(remain,  total_nofillmshr)*100.0
                split_cleared_pf = np.divide(cleared_pf, total_nofillmshr)*100.0
                split_cleared_df = np.divide(cleared_df, total_nofillmshr)*100.0
                print_mmm_1d(split_remain,  '% of '+level+' Remain',
                    benchmarks, print_decimal)
                print_mmm_1d(split_cleared_pf, '% of '+level+' Cleared by Prefetch',
                    benchmarks, print_decimal)
                print_mmm_1d(split_cleared_df, '% of '+level+' Cleared by Demand Fetch',
                    benchmarks, print_decimal)

                split_avg = []
                for s in [split_remain, split_cleared_pf, split_cleared_df]:
                    s_avg = np.zeros(n_b+1)
                    s_avg[:n_b] = s
                    s_avg[ n_b] = np.mean(s)
                    split_avg.append(s_avg)
                data_split.append(split_avg)

        save_dir = 'results/'
        if not os.path.exists(save_dir):
            os.makedirs(save_dir)
        np.savetxt(save_dir+'ras_'+level+'_missrate.csv', data_missrate,
                delimiter = ',', fmt='%0.2f')
        with open(save_dir+'ras_'+level+'_missrate_columns.txt', 'w') as f:
            for col in missrate_column:
                f.write(col+'\n')

        benchmarks_avg = benchmarks + ['mean']
        plot_metric_split_multi(data_split, labels_split, benchmarks_avg,
            'Portion of '+fig_level+' MSHRs (%)',
            'results/ras_'+level+'_mshr_multi_split.jpg', legend_ncol = 3)
        for i in range(len(benchmarks_avg)):
            if i%3 == 1:
                benchmarks_avg[i] = '\n'+ benchmarks_avg[i]
            if i%3 == 2:
                benchmarks_avg[i] = '\n\n'+ benchmarks_avg[i]
        plot_metric(data_missrate_nodiff[:n_b+1, :], 'Miss Rate(%)', benchmarks_avg, 
            labels_missrate, 'results/ras_'+level+'_missrate.jpg', (0.0, 50.0),
            colorlst = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red'],
            markerlst = ['o', 'P', '*', 'X'], fig_type = 'line',
            fig_size = (16, 2), bbox_to_anchor_p = (0.23, 1.05))
