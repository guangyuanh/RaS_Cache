# path_to_gem5
GEM5_DIR=../
BUILD_DIR=${GEM5_DIR}

YEAR=2006

# path that contains the benchmark executable and input files
SPEC_DIR=$2

# name of the benchmark executable
EXE_NAME=$3

COMMENTS_SAVE=""
GEM5_OPTIONS="--l1d_assoc=8"
COMMENTS_RESTORE="salru"

#GEM5_OPTIONS="--l1d_assoc=8 --ras-spec"
#COMMENTS_RESTORE="ras_spec"

#GEM5_OPTIONS="--l1d_assoc=8 --ras-plus"
#COMMENTS_RESTORE="ras_plus"

# instruction count in millions
N_SAVE=10000
N_RUN=500

CHECKPOINT_DIR=$GEM5_DIR/result_benchmark/dir_checkpoint/${YEAR}_${EXE_NAME}/save_${COMMENTS_SAVE}_${N_SAVE}M
OUT_DIR=$GEM5_DIR/result_benchmark/dir_checkpoint/${YEAR}_${EXE_NAME}/restore_${COMMENTS_SAVE}_${N_SAVE}M_${COMMENTS_RESTORE}_${N_RUN}M

rm -r $OUT_DIR

cd $SPEC_DIR
echo "Current working diretory: "
pwd
$BUILD_DIR/build/X86/gem5.opt --outdir=$OUT_DIR \
	$GEM5_DIR/configs/example/se.py \
	--num-cpus=1 --cpu-type=DerivO3CPU \
	--mem-type=SimpleMemory --mem-size=4GB \
	--caches --l2cache --l1d_size=32kB \
	--l1i_size=32kB --l1i_assoc=4 \
	--l2_size=2MB --l2_assoc=16 --cacheline_size=64 \
	--num-dirs=1 \
	--checkpoint-restore="${N_SAVE}000000" --at-instruction \
	--maxinsts="${N_RUN}000000" \
	--restore-with-cpu=DerivO3CPU \
	--checkpoint-dir=$CHECKPOINT_DIR \
	--benchmark=$EXE_NAME \
	$GEM5_OPTIONS
