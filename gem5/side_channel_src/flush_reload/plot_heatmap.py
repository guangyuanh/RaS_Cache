import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm, SymLogNorm, Normalize
import sys
import os
import scipy.stats

def security_metric(plot_data, key_msb):
    n_input = plot_data.shape[0]
    n_block = plot_data.shape[1]
    assert n_input == 256
    scores = np.zeros((n_block+3, 2))
    for i in range(n_block):
        merge_time = np.zeros(16)
        for j in range(16):
            merge_time[j] = np.mean(plot_data[j*16:(j+1)*16, i])
        truth = np.zeros(16)
        truth[key_msb ^ i] = -1.0
        scores[i, 0], scores[i, 1] = scipy.stats.pearsonr(merge_time, truth)
    scores[n_block, 0] = np.mean(scores[:n_block, 0])
    scores[n_block, 1] = np.mean(scores[:n_block, 1])
    scores[n_block+1, 0] = np.amin(scores[:n_block, 0])
    scores[n_block+1, 1] = np.amin(scores[:n_block, 1])
    scores[n_block+2, 0] = np.amax(scores[:n_block, 0])
    scores[n_block+2, 1] = np.amax(scores[:n_block, 1])
    return scores

if __name__ == '__main__':
    fname = sys.argv[1]
    os.system('./exe_process_fr_d '+fname)
    fname = fname + '.csv'

    data = np.loadtxt(fname)
    assert data.shape[0] == 4096
    n_set = data.shape[1]
    print('Number of cache lines: ', n_set)

    yticks = range(256)
    keptticks = yticks[::16]
    yticks = ['' for y in yticks]
    yticks[::16] = keptticks

    xticks = range(n_set)
    keptticks = xticks[::int(len(xticks)/8)]
    xticks = ['' for x in xticks]
    xticks[::int(len(xticks)/8)] = keptticks

    key_msb = [ \
        0,  1,  13, 6, \
        14, 13, 0,  5, \
        15, 9,  1,  0, \
        4,  4,  15, 11]

    # first 16 lines are affected by secret-dependent accesses
    n_score_block = 16
    save_score = np.zeros((n_score_block+3, 33))
    # init for finding min, max
    save_score[2,0] = 100.0
    save_score[3,0] = 100.0
    save_score[4,0] = -100.0
    save_score[5,0] = -100.0

    assert fname[-4:] == '.csv'
    fig_dir = 'fr_dcache_fig/'+fname[:-4]+'/'
    if not os.path.exists(fig_dir):
        print('Making diretory: ', fig_dir)
        os.makedirs(fig_dir)
    for i in range(16):
        plot_data = data[i*256:(i+1)*256]
        sec_scores = security_metric(plot_data[:, :n_score_block], key_msb[i])
        save_score[:, i*2+1:i*2+3] = sec_scores
        plt.figure(figsize = (8,6))
        ax = sns.heatmap(plot_data,
                #norm = SymLogNorm(linthresh=0.03),
                norm = LogNorm(vmin = 20,vmax = 200),
                #cmap = "Greens",
                yticklabels=yticks, xticklabels=xticks,#)
                vmin = 20,
                vmax = 200)
        #cbar = ax.collections[0].colorbar
        #cbar.ax.tick_params(labelsize=20)
        #cbar_ax = ax.figure.axes[-1]
        #cbar_ax.tick_params(labelsize = 20)
        plt.xlabel('Block of Shared Memory', fontsize = 24)
        plt.ylabel('Input Byte', fontsize = 24)
        plt.xticks(fontsize = 24, rotation = 0)
        plt.yticks(fontsize = 24, rotation = 0)
        plt.savefig(fig_dir+'Byte'+str(i)+'.jpg', bbox_inches='tight')
        #plt.show()
        plt.close()
        if i % 4 == 0:
            print('Byte '+str(i)+\
              ' score: '+str(sec_scores[n_score_block, 0])+\
              ' pval: ' +str(sec_scores[n_score_block, 1]) )
            save_score[0,0] = save_score[0,0] + sec_scores[n_score_block, 0]/4
            save_score[1,0] = save_score[1,0] + sec_scores[n_score_block, 1]/4
            save_score[2,0] = np.amin((save_score[2,0], sec_scores[n_score_block, 0]))
            save_score[3,0] = np.amin((save_score[3,0], sec_scores[n_score_block, 1]))
            save_score[4,0] = np.amax((save_score[4,0], sec_scores[n_score_block, 0]))
            save_score[5,0] = np.amax((save_score[5,0], sec_scores[n_score_block, 1]))
    print('mean of score: ', save_score[0,0])
    print('mean of pval: ',  save_score[1,0])
    print('min  of score: ', save_score[2,0])
    print('min  of pval: ',  save_score[3,0])
    print('max  of score: ', save_score[4,0])
    print('max  of pval: ',  save_score[5,0])
    np.savetxt(fig_dir+'pearson_scores.csv', save_score, delimiter = ',')#, fmt = '%.4f')
