import numpy as np
import matplotlib.pyplot as plt
import sys
import os
import scipy.stats

def security_metric(plot_data, key_msb):
    assert len(key_msb) == 2
    assert plot_data.ndim == 1
    n_input = plot_data.shape[0]
    assert n_input == 16
    scores = np.zeros(2)
    truth = np.zeros(16)
    truth[key_msb[0] ^ key_msb[1]] = -1.0
    scores[0], scores[1] = scipy.stats.pearsonr(plot_data, truth)
    return scores

if __name__ == '__main__':

  f_name = sys.argv[1]
  os.system('./exe_process_cache_collision '+f_name)
  f_ary_time = f_name+'.csv'

  ary_time = np.loadtxt(f_ary_time)
  print('arary time size:', ary_time.shape)

  n_pair = ary_time.shape[0]
  assert n_pair == 15*16
  n_time = ary_time.shape[1]
  #assert n_time == 16

  print('max single trial time: ', np.amax(ary_time))
  print('max single trial time location: ', int(np.argmax(ary_time)/16), \
          np.argmax(ary_time)%16)

  key_msb = [ \
        0,  1,  13, 6, \
        14, 13, 0,  5, \
        15, 9,  1,  0, \
        4,  4,  15, 11]

  save_score = np.zeros((6, 12*3*2+1))
  # init for finding min, max
  save_score[2,0] = 100.0
  save_score[3,0] = 100.0
  save_score[4,0] = -100.0
  save_score[5,0] = -100.0

  fig_dir = 'ccfig/'+f_ary_time[:-4]+'/'
  if os.path.exists(fig_dir):
      os.system('rm -r '+fig_dir)
  print('Making diretory: ', fig_dir)
  os.makedirs(fig_dir)
  for i in range(16):
    for k in range(i+1, 16):
      if abs(k-i) % 4 == 0:
        j = k - 1
        fig1, ax1 = plt.subplots(figsize = (8,4))
        plot_time = ary_time[i*15+j,:]
        plot_time = plot_time - np.mean(plot_time)
        sec_scores = security_metric(plot_time, (key_msb[i], key_msb[k]))
        save_idx = i*3 + int((k-i-4)/4)
        save_score[0, save_idx*2+1:save_idx*2+3] = sec_scores
        # 4 * C(4, 2) = 24 valid pairs
        save_score[0,0] = save_score[0,0] + sec_scores[0]/24
        save_score[1,0] = save_score[1,0] + sec_scores[1]/24
        save_score[2,0] = np.amin((save_score[2,0], sec_scores[0]))
        save_score[3,0] = np.amin((save_score[3,0], sec_scores[1]))
        save_score[4,0] = np.amax((save_score[4,0], sec_scores[0]))
        save_score[5,0] = np.amax((save_score[5,0], sec_scores[1]))
        print('Byte '+str(i)+' and '+str(k)+\
              ' score: '+str(sec_scores[0])+\
              ' pval: ' +str(sec_scores[1]) )

        ax1.plot(plot_time)
        ax1.set_xlim((-1, n_time))
        #ax1.set_ylim((-1, np.amax(avg_time)*1.1))
        #ax1.set_ylim((-3, 2))
        plt.xlabel('MSBs of D'+str(i+1)+' XOR D'+str(k+1)+'', fontsize = 20)
        plt.ylabel('Execution Time\n(cycles)', fontsize = 20)
        plt.xticks(fontsize = 20)
        plt.yticks(fontsize = 20)
        plt.savefig(fig_dir+str(i)+'_'+str(k)+'.jpg', bbox_inches='tight')
        #if j < 3:
        #  plt.show()
        plt.close()
  print('mean of score: ', save_score[0,0])
  print('mean of pval: ',  save_score[1,0])
  print('min  of score: ', save_score[2,0])
  print('min  of pval: ',  save_score[3,0])
  print('max  of score: ', save_score[4,0])
  print('max  of pval: ',  save_score[5,0])
  np.savetxt(fig_dir+'pearson_scores.csv', save_score, delimiter = ',')#, fmt = '%.4f')
