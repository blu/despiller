#include <stdio.h>
#include <alloca.h>
#include <utility>
#include "isa.h"
#include "bb.h"
#include "cfg.h"

enum AddressColor : uint8_t {
	addrcolor_err,
	addrcolor_one,
	addrcolor_two
};

void print(FILE* f, const bb::BasicBlock& block, const AddressColor addrcolor = addrcolor_err)
{
	using namespace bb;

	char buffer[128];
	constexpr size_t bufferSize = sizeof(buffer);
	Address addr = block.getStartAddress();

	if (isAddressValid(addr)) {
		const char* format_color[] = { // all entries must contain an %x followed by an %s
			"\033[38;5;13m%08x\033[0m\t%s\n",
			"\033[38;5;8m%08x\033[0m\t%s\n",
			"\033[38;5;15m%08x\033[0m\t%s\n"
		};
		const char* const format = format_color[uint8_t(addrcolor)];

		for (const auto it : block.getSequence()) {
			const size_t checkSize = strFromInstr(it, buffer, bufferSize);
			assert(checkSize <= bufferSize);
			fprintf(f, format, addr++, buffer);
		}
	}
	else
		fprintf(f, "invalid basic block\n");
}

int main(int, char **)
{
	using namespace bb;

	// get a basic block of all opcodes -- naturally invalid
	{
		using namespace isa;
		BasicBlock block(0x7f00); // for immediates' sake, keep addresses in 16-bit range
		{
			Instr instr(op_nop);
			instr.setOperand(0, reg_invalid, true);
			block.addInstr(instr);
		}
		{
			Instr instr(op_li);
			instr.setOperand(0, 42);
			instr.setOperand(1, 0xff);
			instr.setOperand(2, 0x7f);
			block.addInstr(instr);
		}
		{
			Instr instr(op_push);
			instr.setOperand(0, 42, true);
			block.addInstr(instr);
		}
		{
			Instr instr(op_pop);
			instr.setOperand(0, 42, true);
			block.addInstr(instr);
		}
		{
			Instr instr(op_br);
			instr.setOperand(0, 42, true);
			block.addInstr(instr);
		}
		{
			Instr instr(op_cbr);
			instr.setOperand(0, 42);
			instr.setOperand(1, 43);
			instr.setOperand(2, 44);
			block.addInstr(instr);
		}
		{
			Instr instr(op_op2);
			instr.setOperand(0, 42);
			instr.setOperand(1, 43, true);
			block.addInstr(instr);
		}
		{
			Instr instr(op_op3);
			instr.setOperand(0, 42);
			instr.setOperand(1, 43);
			instr.setOperand(2, 44);
			block.addInstr(instr);
		}
		print(stdout, block); // print pre-validation state
		block.validate();
		print(stdout, block); // verify invalidity
		fputc('\n', stdout);
	}

	using namespace cfg;

	ControlFlowGraph graph; // full-program CFG

	// compose 'main' of two basic blocks..
	// first basic block -- invoke a callee in our turn
	{
		BasicBlock block(0x7000);
		using namespace isa;
		{
			Instr instr(op_push); // push link to caller
			instr.setOperand(0, 127, true);
			block.addInstr(instr);
		}
		{
			Instr instr(op_li); // load branch target
			instr.setOperand(0, 42);
			instr.setOperand(1, 0x00);
			instr.setOperand(2, 0x7f);
			block.addInstr(instr);
		}
		{
			Instr instr(op_li); // load link target
			instr.setOperand(0, 127);
			instr.setOperand(1, 0x04);
			instr.setOperand(2, 0x70);
			block.addInstr(instr);
		}
		{
			Instr instr(op_br); // call branch target
			instr.setOperand(0, 42, true);
			block.addInstr(instr);
		}
		block.validate();
		graph.addBasicBlock(std::move(block));
	}

	// second basic block -- once our callee is done we return to our caller
	{
		BasicBlock block(0x7004);
		using namespace isa;
		{
			Instr instr(op_pop); // pop link to caller
			instr.setOperand(0, 127, true);
			block.addInstr(instr);
		}
		{
			Instr instr(op_br); // branch to caller, i.e. return
			instr.setOperand(0, 127, true);
			block.addInstr(instr);
		}
		block.validate();
		graph.addBasicBlock(std::move(block));
	}

	const AddressColor color[] = {
		addrcolor_one,
		addrcolor_two
	};
	size_t colorAlt = 0;

	for (const auto it : graph) {
		print(stdout, it.second, color[colorAlt]);
		colorAlt ^= 1;
	}

	return 0;
}
