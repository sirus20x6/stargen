#!/bin/bash
# Automated test script to compare parallel vs sequential output
# Tests that parallel generation produces identical output to sequential

set -e  # Exit on error

STARGEN="./stargen"
SYSTEMS=5
THREADS=4
SEED=42
MASS=1.0

echo "================================"
echo "Parallel vs Sequential Test"
echo "================================"
echo "Systems: $SYSTEMS"
echo "Threads: $THREADS"
echo "Seed: $SEED"
echo ""

# Test TEXT output
echo "[1/3] Testing TEXT output..."
$STARGEN -t -c$SYSTEMS -s$SEED -m$MASS > /tmp/stargen_seq.txt 2>/tmp/stargen_seq_err.txt
$STARGEN -t -c$SYSTEMS -j$THREADS -s$SEED -m$MASS > /tmp/stargen_par.txt 2>/tmp/stargen_par_err.txt

if diff -q /tmp/stargen_seq.txt /tmp/stargen_par.txt > /dev/null; then
    echo "  ✓ TEXT output matches"
else
    echo "  ✗ TEXT output differs"
    echo "  Run: diff /tmp/stargen_seq.txt /tmp/stargen_par.txt"
    exit 1
fi

# Test with different system counts
echo "[2/3] Testing with larger system count..."
SYSTEMS2=20
$STARGEN -t -c$SYSTEMS2 -s$SEED -m$MASS > /tmp/stargen_seq2.txt 2>/tmp/stargen_seq2_err.txt
$STARGEN -t -c$SYSTEMS2 -j$THREADS -s$SEED -m$MASS > /tmp/stargen_par2.txt 2>/tmp/stargen_par2_err.txt

if diff -q /tmp/stargen_seq2.txt /tmp/stargen_par2.txt > /dev/null; then
    echo "  ✓ TEXT output matches (20 systems)"
else
    echo "  ✗ TEXT output differs (20 systems)"
    echo "  Run: diff /tmp/stargen_seq2.txt /tmp/stargen_par2.txt"
    exit 1
fi

# Test with different seed
echo "[3/3] Testing with different seed..."
SEED2=100
$STARGEN -t -c$SYSTEMS -s$SEED2 -m$MASS > /tmp/stargen_seq3.txt 2>/tmp/stargen_seq3_err.txt
$STARGEN -t -c$SYSTEMS -j$THREADS -s$SEED2 -m$MASS > /tmp/stargen_par3.txt 2>/tmp/stargen_par3_err.txt

if diff -q /tmp/stargen_seq3.txt /tmp/stargen_par3.txt > /dev/null; then
    echo "  ✓ TEXT output matches (seed=$SEED2)"
else
    echo "  ✗ TEXT output differs (seed=$SEED2)"
    echo "  Run: diff /tmp/stargen_seq3.txt /tmp/stargen_par3.txt"
    exit 1
fi

echo ""
echo "================================"
echo "All tests passed! ✓"
echo "================================"
echo ""
echo "Parallel and sequential modes produce identical output."
echo ""

# Cleanup
rm -f /tmp/stargen_*.txt /tmp/stargen_*_err.txt

exit 0
