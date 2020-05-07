# despiller
A proof of concept of a code-optimisation technique, hereby referred to as *de-spilling of registers*, in a fake ISA.

# overview
De-spilling is a codegen optimisaiton technique, which addresses post-factum excessive spilling or registers. The latter can come from two sources:

* suboptimal previous codegen stages
* function prologues/epilogues not taking into account the register-pressure picture at function call sites

De-spilling works by tracking register occupancy across the control-flow graph (CFG) of a program. One can think of de-spilling as a whole-program-optimisaion technique, but it can be applied to any well-isolated (singular entry edge) subgraph in the CFG. Characteristic of de-spilling is that it does not try to re-arrange basic blocks (like inlining does), but instead re-arranges instruction register operands with the goal of fitting instruction targets into vacant registers, enabling the elimination (op -> nop) of spill/restore sequences, which constitutes the actual optimisation.

As de-spilling is a very conservative optimisation technique, it could be applied to program binaries post compile.

# limitations
This proof of concept does not handle non-deterministic branch targets -- i.e. targets which cannot be computed via static analysis, e.g. targets of virtual or dispatch calls.
