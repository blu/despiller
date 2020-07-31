# despiller
A proof of concept of a code-optimisation technique, hereby referred to as *de-spilling of registers*, in a fake ISA.

*This is a work in progress, far from feature-complete status. Once the project reaches sufficient maturity this message will be removed.*

# overview
De-spilling is a codegen optimisaiton technique, which addresses post-factum excessive spilling or registers. The latter can come from two sources:

* prior suboptimal codegen stages
* function prologues/epilogues not taking into account the inherent register pressure at function call sites

De-spilling works by tracking register occupancy across the control-flow graph (CFG) of a program. One can think of de-spilling as a whole-program-optimisaion technique, but it can be applied to any well-isolated (i.e. singular entry edge) subgraph in the CFG. Characteristic of de-spilling is that it does not try to re-arrange basic blocks like inlining does, but instead re-arranges individual instructions' register operands with the goal of picking vacant registers as instruction destinations. This enables the elimination (e.g. op -> nop) of spill/restore sequences, which constitutes the actual optimisation. Please note, that a routine that has undergone such optimisation may no longer comply with the standard calling conventions, and hence invoking it outside the original program context for which de-spilling was carried is effectively undefined behavior.

As de-spilling is a very conservative optimisation technique, it could be applied to program binaries post compile.

# ISA
A fictional, non-Turing-complete ISA of little-endian 31-bit machine word is used. The ISA is 3-argument and has an unspecified-size GPR file { R0..Rn }. The ISA also has access to an unlimited storage space 'storage' where regs can be spilled -- i.e. stored to and eventually restored from, in a LIFO manner. The ISA employs the following ops:

* `nop` -- no-op
* `li Rn, imm` -- load immediate to register
* `push Rn` -- store a single register to 'storage'
* `pop Rn` -- restore a single register from 'storage'
* `br Rt` -- unconditional branch to register
* `cbr Rt, Rn, Rm` -- conditional branch to register; branch to Rt if unspecified comparison between Rn and Rm is true
* `op Rn, Rm` -- unspecified op which is not any of the above, taking 2 operands: one destination and one source
* `op Rn, Rm, Rk` -- unspecified op which is not any of the above, taking 3 operands: one destination and two sources

# limitations
This proof of concept does not handle branch targets which cannot be computed via simple static analysis, e.g. targets of virtual or dispatch calls.
