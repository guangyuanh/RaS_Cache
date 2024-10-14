NAME=$1
GREPDIR="greps/ras/"
CHECKPOINT="restore_10000M"
SIMCYCLE="500M"
STATS_FILE=2006_*/${CHECKPOINT}_${NAME}_$SIMCYCLE/stats.txt

mkdir -p $GREPDIR

grep -i siminsts 2006_*/${CHECKPOINT}_${NAME}_$SIMCYCLE/stats.txt > $GREPDIR/siminsts_${SIMCYCLE}_${NAME}.grep
grep -i cpu.numcycles 2006_*/${CHECKPOINT}_${NAME}_$SIMCYCLE/stats.txt > $GREPDIR/perf_${SIMCYCLE}_${NAME}.grep

grep -i system.cpu.dcache.demandHits::cpu.data $STATS_FILE > $GREPDIR/l1dhit_cputdata_${SIMCYCLE}_${NAME}.grep
grep -i system.cpu.dcache.demandMisses::cpu.data $STATS_FILE > $GREPDIR/l1dmiss_cputdata_${SIMCYCLE}_${NAME}.grep

grep system.cpu.dcache.shbMshrCanFill     $STATS_FILE > $GREPDIR/rasl1d_shbMshrCanFill_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.cacheFills         $STATS_FILE > $GREPDIR/rasl1d_cacheFills_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.nofillMshrCreated  $STATS_FILE > $GREPDIR/rasl1d_nofillMshrCreated_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.nofillMshrServiced $STATS_FILE > $GREPDIR/rasl1d_nofillMshrServiced_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.noFillLowerMshr    $STATS_FILE > $GREPDIR/rasl1d_noFillLowerMshr_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.nofillHitNofill    $STATS_FILE > $GREPDIR/rasl1d_nofillHitNofill_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.nofillHitFill      $STATS_FILE > $GREPDIR/rasl1d_nofillHitFill_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.demandAccessHit    $STATS_FILE > $GREPDIR/rasl1d_demandAccessHit_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.demandMshrCreated  $STATS_FILE > $GREPDIR/rasl1d_demandMshrCreated_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.demandHitFill      $STATS_FILE > $GREPDIR/rasl1d_demandHitFill_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.demandHitNofill    $STATS_FILE > $GREPDIR/rasl1d_demandHitNofill_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.demandAckHitNofill $STATS_FILE > $GREPDIR/rasl1d_demandAckHitNofill_${SIMCYCLE}_${NAME}.grep
grep system.cpu.dcache.demandFillHitNofill    $STATS_FILE > $GREPDIR/rasl1d_demandFillHitNofill_${SIMCYCLE}_${NAME}.grep

grep system.l2.shbMshrCanFill     $STATS_FILE > $GREPDIR/rasl2_shbMshrCanFill_${SIMCYCLE}_${NAME}.grep
grep system.l2.cacheFills         $STATS_FILE > $GREPDIR/rasl2_cacheFills_${SIMCYCLE}_${NAME}.grep
grep system.l2.nofillMshrCreated  $STATS_FILE > $GREPDIR/rasl2_nofillMshrCreated_${SIMCYCLE}_${NAME}.grep
grep system.l2.nofillMshrServiced $STATS_FILE > $GREPDIR/rasl2_nofillMshrServiced_${SIMCYCLE}_${NAME}.grep
grep system.l2.noFillLowerMshr    $STATS_FILE > $GREPDIR/rasl2_noFillLowerMshr_${SIMCYCLE}_${NAME}.grep
grep system.l2.nofillHitNofill    $STATS_FILE > $GREPDIR/rasl2_nofillHitNofill_${SIMCYCLE}_${NAME}.grep
grep system.l2.nofillHitFill      $STATS_FILE > $GREPDIR/rasl2_nofillHitFill_${SIMCYCLE}_${NAME}.grep
grep system.l2.demandAccessHit    $STATS_FILE > $GREPDIR/rasl2_demandAccessHit_${SIMCYCLE}_${NAME}.grep
grep system.l2.demandMshrCreated  $STATS_FILE > $GREPDIR/rasl2_demandMshrCreated_${SIMCYCLE}_${NAME}.grep
grep system.l2.demandHitFill      $STATS_FILE > $GREPDIR/rasl2_demandHitFill_${SIMCYCLE}_${NAME}.grep
grep system.l2.demandHitNofill    $STATS_FILE > $GREPDIR/rasl2_demandHitNofill_${SIMCYCLE}_${NAME}.grep
grep system.l2.demandAckHitNofill $STATS_FILE > $GREPDIR/rasl2_demandAckHitNofill_${SIMCYCLE}_${NAME}.grep
grep system.l2.demandFillHitNofill    $STATS_FILE > $GREPDIR/rasl2_demandFillHitNofill_${SIMCYCLE}_${NAME}.grep

