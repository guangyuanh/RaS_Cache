# path_to_gem5
GEM5_DIR=../

# name of evaluated system
# consistant with run_checkpoint.sh
COMMENTS_RESTORE="ras_spec_w4_sa"

cd ${GEM5_DIR}/result_benchmark/dir_checkpoint/
sh grep_ras.sh $COMMENTS_RESTORE
cd ${GEM5_DIR}/result_benchmark/
python plot-ras-perf.py
python plot-ras-nofillmshr.py

print "Figure and data are saved to gem5/result_benchmark/results"
