a:
addi    s0, zero, 0x100     # a base address - 0x100
b:
addi    s1, zero, 0x400
addi    s1, s1, 0x500       # b base address - 0x100 + M * K * 1 = 0x900
c:
addi    s2, zero, 0
addi    s2, s2, 0x600
addi    s2, s2, 0x600
addi    s2, s2, 0x600
addi    s2, s2, 0x600       # c base address - 0x900 + K * N * 2 = 0x1800
mmul:
addi    a5, s0, 0           # int8_t* pa = a
addi    a3, s2, 0           # int32_t* pc = c
addi    t0, zero, 0         # int y = 0
addi    t1, zero, 64        # int temp1 = M
y_for_begin:
addi    t2, zero, 0         # int x = 0
addi    t3, zero, 60        # int temp2 = N
x_for_begin:
addi    a4, s1, 0           # int16_t* pb = b
addi    t6, zero, 0         # int32_t s = 0
addi    t4, zero, 0         # int k = 0
addi    t5, zero, 32        # int temp3 = K
k_for_begin:
slli    s3, t2, 1           # = x * sizeof(int16_t) = byte offset
add     s3, s3, a4          # address of pb[x]
add     s4, t4, a5          # address of pa[k]
lb      a0, 0, s4           # = pa[k]
lh      a1, 0, s3           # = pb[x]
mul     a7, a0, a1          # = pa[k] * pb[x]
add     t6, t6, a7          # s += pa[k] * pb[x]
addi    a4, a4, 120         # pb += N * sizeof(int16_t)
addi    t4, t4, 1           # k += 1
blt     t4, t5, k_for_begin # while k != K
k_for_end:
slli    s5, t2, 2           # = x * sizeof(int32_t) = byte offset
add     s5, s5, a3          # address of pc[x]
sw      t6, 0, s5           # pc[x] = s
addi    t2, t2, 1
blt     t2, t3, x_for_begin # while x != N
x_for_end:
addi    a5, a5, 32          # pa += K * sizeof(int8_t)
addi    a3, a3, 240         # pc += N * sizeof(int32_t)
addi    t0, t0, 1           # y += 1
blt     t0, t1, y_for_begin # while y != M
y_for_end:
jalr    zero, 0, ra         # return

int log2(int64_t num) {
    int res = 0, pw = 0;
    for(int i = 32; i > 0; i --) {
        res += i;
        if(((1LL << res) - 1) & num) {
            res -= i;
        }
    }
    return res;
}

binlog:
addi    t0, zero, 0                     # int res = 0
addi    t1, zero, 0                     # int pw = 0
addi    t2, zero, 0                     # int i = 0
lui     s0, zero, 0xfffff000
addi    s0, s0, 0x00000fff              # s0 = UINT32_MAX
binlog_for_begin:
slli    t3, t2, 1                       # 1LL << i
and     t3, t3, a0                      # (1LL << i) & num
beq     t3, zero, binlog_for_else
add     s0, zero, t2                    # i
addi    s0, s0, 1                       # i + 1
binlog_for_else:
addi    t2, t2, 1                       # i += 1
blt     t2, 32, binlog_for_begin
jalr    zero, 0, ra
