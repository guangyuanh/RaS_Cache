import numpy as np
import matplotlib.pyplot as plt
import sys

if __name__ == '__main__':
  f_ary_time = sys.argv[1]
  n_sets  = int(sys.argv[2])
  n_assoc = int(sys.argv[3])
  #miss_th = int(sys.argv[4])
  assert f_ary_time[-4:] == '.csv', f_ary_time[-4:]
  fig_name = f_ary_time[:-4]+'.jpg'
  fig_count_name = f_ary_time[:-4]+'_count.jpg'

  report_th = 1000
  replace_latency = 200
  #assert replace_latency > miss_th

  ary_time = np.loadtxt(f_ary_time)
  print('array time size: ', ary_time.shape)
  print('array max: ', np.amax(ary_time))
  print('array argmax: ', np.unravel_index(np.argmax(ary_time), \
                                           ary_time.shape))
  print('array min: ', np.amin(ary_time))

  n_trial = ary_time.shape[0]
  n_time_read = ary_time.shape[1]
  assert n_time_read == n_sets * n_assoc, 'The size of input array should be \
          (n_trial, n_sets * n_assoc).'
  print('n_trial: ', n_trial)

  ary_set_total = np.zeros((n_trial, n_sets))
  ary_miss_num  = np.zeros(n_sets)
  for i in range(n_trial):
    for j in range(n_sets):
      for k in range(n_assoc):
        t = ary_time[i, j*n_assoc + k]
        if t > report_th:
          print('Timer overflow of ', t, ' at i: ', i, ' j: ', j, ' k: ', k)
          t = replace_latency
          ary_time[i, j*n_assoc + k] = replace_latency
        ary_set_total[i, j] += t
        #if t > miss_th:
        #  ary_miss_num[j] += 1
  avg_time = np.mean(ary_set_total, axis = 0)*1.0/n_assoc
  assert avg_time.shape[0] == n_sets

  fig1, ax1 = plt.subplots(figsize = (5,3.5))
  ax1.plot(range(n_sets), avg_time)
  ax1.set_xlim((-1, n_sets))
  #ax1.set_ylim((-1, np.amax(avg_time)*1.1+1))
  ax1.set_ylim((10, 45))
  plt.xlabel('Index Bits of Cache Line', fontsize = 18)
  plt.ylabel('Access Time (cycles)', fontsize = 18)
  plt.xticks(fontsize = 18, rotation = 0)
  plt.yticks(fontsize = 18, rotation = 0)
  plt.savefig(fig_name, bbox_inches='tight')
  plt.show()

  '''
  fig1, ax1 = plt.subplots()
  ax1.plot(range(n_sets), ary_miss_num)
  ax1.set_xlim((-1, n_sets))
  ax1.set_ylim((-1, np.amax(ary_miss_num)*1.1+1))
  plt.xlabel('Cache Sets')
  plt.ylabel('Number of Hits (th='+str(miss_th)+')')
  plt.savefig(fig_count_name, bbox_inches='tight')
  plt.show()
  '''
