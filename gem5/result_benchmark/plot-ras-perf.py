import numpy as np
import matplotlib.pyplot as plt
import scipy.stats.mstats
import os
from plot_utils import print_mmm_1d, plot_metric

if __name__ == '__main__':
    metric_dir  = 'ras/'
    metric_name = 'perf_500M_'
    figure_title = 'Execution Time'
    f_prefix = 'dir_checkpoint/greps/'+metric_dir+metric_name

    scale_factor = 1.0

    methods = [
               'orig_sa',
               'ras_spec_w1_sa',
               'ras_spec_w4_sa',
               'ras_spec_w16_sa',
            ]

    ylim = None#(0.5, 2.0)
    labels = [
            'Baseline',
            'RaS-Spec-W1',
            'RaS-Spec-W4',
            'RaS-Spec-W16',
            ]
    assert len(labels) == len(methods)

    benchmarks = [
        'GemsFDTD', 'astar',  'bwaves', 'bzip2',    'cactusADM',
        'calculix', 'gamess', 'gcc',    'gobmk',    'gromacs',
        'h264ref',  'hmmer',  'lbm',    'leslie3d', 'libquantum',
        'mcf',      'milc',   'namd',   'omnetpp',  'povray',
        'sjeng',    'sphinx3','wrf',    'zeusmp']
    benchmarks_g = benchmarks + ['gmean']
    n_m = len(methods)
    n_b = len(benchmarks)
    data      = np.zeros((n_b+1, n_m))
    rdata     = np.zeros((n_b+3, n_m)) # relative data
    print('n_m: ', n_m, 'n_b: ', n_b)

    for i in range(n_m):
        f_name = f_prefix+methods[i]+'.grep'
        with open(f_name, 'r') as f:
            lines = f.readlines()
            assert len(lines) == n_b, f_name+' n_line: '+str(len(lines))
            for j in range(n_b):
                line = lines[j].split()
                assert line[0][5:5+len(benchmarks[j])] == benchmarks[j],\
                    'Wrong benchmark name: '+\
                    f_name+':line '+str(j)+' '+line[0][5:5+len(benchmarks[j])]+\
                    ' vs '+benchmarks[j]
                #assert line[1].isnumeric(), f_name+':line '+str(j)
                data[j, i] = float(line[1])

    data = data*scale_factor
    for i in range(n_b):
        rdata[i, :] = data[i, :]/data[i, 0]
    for i in range(n_m):
        data[n_b, i]  = scipy.stats.mstats.gmean(data[:n_b, i])
        rdata[n_b, i] = scipy.stats.mstats.gmean(rdata[:n_b, i])
        rdata[n_b+1, i] = np.amin(rdata[:n_b, i])
        rdata[n_b+2, i] = np.amax(rdata[:n_b, i])

    if not os.path.exists('results/'):
        os.makedirs('results/')
    with open('results/'+metric_name+'methods.txt', 'w') as f:
        for m in methods:
            f.write(m+'\n')
    np.savetxt('results/'+metric_name+'relative.csv', rdata*100.0, delimiter=',', fmt='%.3f')
    print('Figure Title: ', figure_title)
    print('Save Name: ', 'results/'+metric_name)
    for i in range(n_m):
        rdata_single = rdata[:n_b, i]
        print_mmm_1d(rdata_single*100.0, methods[i], benchmarks, 2, avg_alg = 'gmean')

    benchmarks_g = benchmarks + ['gmean']
    for i in range(len(benchmarks_g)):
        if i%3 == 1:
            benchmarks_g[i] = '\n'+ benchmarks_g[i]
        if i%3 == 2:
            benchmarks_g[i] = '\n\n'+ benchmarks_g[i]
    plot_metric(rdata[:n_b+1, :], '', benchmarks_g, labels, 'results/'+metric_name+'relative.jpg', ylim, legend_cols = len(labels))
