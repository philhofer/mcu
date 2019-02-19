#!/bin/sh

# this script generates a vector table
# for armv6-m cpus, and perhaps other ones

cat <<EOF
    .cpu cortex-m0
    .arch armv6-m
    .thumb
    .pushsection .vector_table
    .word @STACK_TOP@
    .word reset_entry+1
    .word nmi_entry+1
    .word hardfault_entry+1
EOF

for i in $(seq 4 10); do
    cat <<EOF
    .word fatal0+1
EOF
done

cat <<EOF
    .word svcall_entry+1
EOF

for i in $(seq 12 13); do
    cat <<EOF
    .word fatal0+1
EOF
done

cat <<EOF
    .word pendsv_entry+1
    .word systick_entry+1
EOF

for i in $(seq 0 $(echo "$NUM_IRQ-1" | bc)); do
    cat<<EOF
    .word @IRQ${i}@+1
EOF
done

cat <<EOF
    .popsection
fatal0:
    b .
EOF
