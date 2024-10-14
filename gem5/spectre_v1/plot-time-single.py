import numpy as np
import matplotlib.pyplot as plt
import sys

if __name__ == '__main__':

  f_ary_time = sys.argv[1]
  assert f_ary_time[-4:] == '.csv'
  fig_name = f_ary_time[:-4]+'.jpg'

  ary_time = np.loadtxt(f_ary_time)
  print('arary time size:', ary_time.shape)

  n_trial = ary_time.shape[0]
  n_time = ary_time.shape[1]

  print('max single trial time: ', np.amax(ary_time))
  print('max single trial time location: ', int(np.argmax(ary_time)/256), \
          np.argmax(ary_time)%256)

  avg_time = np.mean(ary_time, axis = 0)
  assert avg_time.shape[0] == 256

  print('max avg time: ', np.amax(avg_time))
  print('max avg time: location: ', np.argmax(avg_time))

  fig1, ax1 = plt.subplots(figsize = (5, 3.5))
  ax1.plot(avg_time)
  ax1.set_xlim((-1, 256))
  #ax1.set_ylim((-1, np.amax(avg_time)*1.1))
  ax1.set_ylim((30, 200))
  #fig1.show()
  plt.xlabel('Cache Line', fontsize = 18)
  plt.ylabel('Reload Time (cycles)', fontsize = 18)
  plt.xticks(fontsize = 18)
  plt.yticks(fontsize = 18)
  plt.savefig(fig_name, bbox_inches='tight')
  plt.show()
