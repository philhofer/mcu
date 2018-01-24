#!/bin/sh

# this script generates a vector table
# for armv6-m cpus, and perhaps other ones

cat <<EOF
    .cpu cortex-m0
    .arch armv6-m
    .thumb
    .pushsection vector_table
    .word 0x20000000+@STACK_SIZE@
    .word reset_entry
    .word nmi_entry
    .word hardfault_entry
EOF

for i in $(seq 5 10); do
    cat <<EOF
    .word fatal0
EOF
done

cat <<EOF
    .word svcall_entry
EOF

for i in $(seq 12 13); do
    cat <<EOF
    .word fatal0
EOF
done

cat <<EOF
    .word pendsv_entry
    .word systick_entry
EOF

for i in $(seq $NUM_IRQ); do
    cat<<EOF
    .word @IRQ${i}@
EOF
done

cat <<EOF
    .popsection
fatal0:
    b .
EOF
