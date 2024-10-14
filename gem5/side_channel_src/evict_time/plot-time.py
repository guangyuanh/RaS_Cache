import numpy as np
import matplotlib.pyplot as plt
import sys
import os
import scipy.stats

def security_metric(plot_data, key_msb):
    assert plot_data.ndim == 1
    n_input = plot_data.shape[0]
    assert n_input == 256
    scores = np.zeros(2)
    merge_time = np.zeros(16)
    for j in range(16):
        merge_time[j] = np.mean(plot_data[j*16:(j+1)*16])
    truth = np.zeros(16)
    # we observe that set 0 is evicted so Din = key_msb should give max time
    truth[key_msb] = 1.0
    scores[0], scores[1] = scipy.stats.pearsonr(merge_time, truth)
    return scores

if __name__ == '__main__':

  f_timing = sys.argv[1]
  os.system('./calculation '+f_timing)

  f_ary_time = f_timing + '.csv'

  ary_time = np.loadtxt(f_ary_time)
  print('arary time size:', ary_time.shape)

  n_byte = ary_time.shape[0]
  assert n_byte == 16
  n_time = ary_time.shape[1]
  #assert n_time == 16

  key_msb = [ \
        0,  1,  13, 6, \
        14, 13, 0,  5, \
        15, 9,  1,  0, \
        4,  4,  15, 11]

  # 1 avg + (score, pval) for each of 16 bytes
  save_score = np.zeros((6, 33))
  # init for finding min, max
  save_score[2,0] = 100.0
  save_score[3,0] = 100.0
  save_score[4,0] = -100.0
  save_score[5,0] = -100.0

  fig_dir = 'etfig/'+f_ary_time[:-4]+'/'
  if not os.path.exists(fig_dir):
      print('Making diretory: ', fig_dir)
      os.makedirs(fig_dir)
  for i in range(16):
      fig1, ax1 = plt.subplots(figsize = (8,3.5))
      plot_data = ary_time[i, :]
      sec_scores = security_metric(plot_data, key_msb[i])
      save_score[0, i*2+1:i*2+3] = sec_scores
      ax1.plot(plot_data)
      ax1.set_xlim((-1, n_time))
      #ax1.set_ylim((-1, np.amax(avg_time)*1.1))
#      ax1.set_ylim((-5, 5))
      plt.xlabel('D'+str(i+1), fontsize = 20)
      plt.ylabel('Execution Time\n(cycles)', fontsize = 20)
      plt.xticks(fontsize = 20)
      plt.yticks(fontsize = 20)
      plt.savefig(fig_dir+str(i)+'.jpg', bbox_inches='tight')
      #if j < 3:
      #  plt.show()
      plt.close()
      print('Byte '+str(i)+\
            ' score: '+str(sec_scores[0])+\
            ' pval: ' +str(sec_scores[1]) )
      save_score[0,0] = save_score[0,0] + sec_scores[0]/16
      save_score[1,0] = save_score[1,0] + sec_scores[1]/16
      save_score[2,0] = np.amin((save_score[2,0], sec_scores[0]))
      save_score[3,0] = np.amin((save_score[3,0], sec_scores[1]))
      save_score[4,0] = np.amax((save_score[4,0], sec_scores[0]))
      save_score[5,0] = np.amax((save_score[5,0], sec_scores[1]))
  print('mean of score: ', save_score[0,0])
  print('mean of pval: ',  save_score[1,0])
  print('min  of score: ', save_score[2,0])
  print('min  of pval: ',  save_score[3,0])
  print('max  of score: ', save_score[4,0])
  print('max  of pval: ',  save_score[5,0])
  np.savetxt(fig_dir+'pearson_scores.csv', save_score, delimiter = ',')#, fmt = '%.4f')
