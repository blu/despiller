#include <stdio.h>
#include <alloca.h>
#include "isa.h"
#include "bb.h"

void print(FILE* f, const bb::BasicBlock& block)
{
	using namespace bb;

	char buffer[128];
	constexpr size_t bufferSize = sizeof(buffer);
	Address addr = block.getStartAddress();

	if (isAddressValid(addr)) {
		for (const auto it : block.getSequence()) {
			const size_t checkSize = strFromInstr(it, buffer, bufferSize);
			assert(checkSize <= bufferSize);
			fprintf(f, "%08x\t%s\n", addr++, buffer);
		}
	}
	else
		fprintf(f, "invalid basic block\n");
}

int main(int, char **)
{
	using namespace bb;

	BasicBlock block(0x7f00); // for immediates' sake, keep addresses in 16-bit range
	{
		using namespace isa;
		Instr instr(op_nop);
		instr.setOperand(0, reg_invalid, true);
		block.addInstr(instr);
	}
	{
		using namespace isa;
		Instr instr(op_li);
		instr.setOperand(0, 42);
		instr.setOperand(1, 0xff);
		instr.setOperand(2, 0x7f);
		block.addInstr(instr);
	}
	{
		using namespace isa;
		Instr instr(op_push);
		instr.setOperand(0, 42, true);
		block.addInstr(instr);
	}
	{
		using namespace isa;
		Instr instr(op_pop);
		instr.setOperand(0, 42, true);
		block.addInstr(instr);
	}
	{
		using namespace isa;
		Instr instr(op_br);
		instr.setOperand(0, 42, true);
		block.addInstr(instr);
	}
	{
		using namespace isa;
		Instr instr(op_cbr);
		instr.setOperand(0, 42);
		instr.setOperand(1, 43);
		instr.setOperand(2, 44);
		block.addInstr(instr);
	}
	{
		using namespace isa;
		Instr instr(op_op2);
		instr.setOperand(0, 42);
		instr.setOperand(1, 43, true);
		block.addInstr(instr);
	}
	{
		using namespace isa;
		Instr instr(op_op3);
		instr.setOperand(0, 42);
		instr.setOperand(1, 43);
		instr.setOperand(2, 44);
		block.addInstr(instr);
	}
	print(stdout, block);
	block.validate();
	print(stdout, block);

	return 0;
}
