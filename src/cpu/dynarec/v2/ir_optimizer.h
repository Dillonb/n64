#ifndef N64_IR_OPTIMIZER_H
#define N64_IR_OPTIMIZER_H

void ir_optimize_constant_propagation();
void ir_optimize_eliminate_dead_code();

#endif //N64_IR_OPTIMIZER_H
