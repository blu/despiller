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

	if (!block.isValid() && addrcolor_err != addrcolor) {
		fprintf(f, "invalid basic block\n");
		return;
	}

	char buffer[128];
	constexpr size_t bufferSize = sizeof(buffer);

	const char* format_color[] = { // all entries must contain an %x followed by an %s
		"\033[38;5;13m%08x\033[0m\t%s\n",
		"\033[0;34;40m%08x\033[0m\t%s\n",
		"\033[0;35;40m%08x\033[0m\t%s\n"
	};
	const char* const format = format_color[uint8_t(addrcolor)];
	Address addr = block.getStartAddress();

	for (const auto it : block.getSequence()) {
		const size_t checkSize = strFromInstr(it, buffer, bufferSize);
		assert(checkSize <= bufferSize);
		fprintf(f, format, addr++, buffer);
	}
}

int main(int, char **)
{
	fprintf(stdout, "sizeof(Instr): %lu\nsizeof(BasicBlock): %lu\nsizeof(ControlFlowGraph): %lu\nsizeof(Registry): %lu\n\n",
		sizeof(isa::Instr),
		sizeof(bb::BasicBlock),
		sizeof(cfg::ControlFlowGraph),
		sizeof(reg::Registry));

	using namespace bb;

	// get a basic block of all opcodes -- naturally invalid
	{
		BasicBlock block(0x7f00); // for immediates' sake, keep addresses in 16-bit range
		using namespace isa;
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
		const bool valid = block.validate();
		assert(!valid);
		print(stdout, block);
	}

	using namespace reg;
	using namespace cfg;
	ControlFlowGraph graph; // full-program CFG

	// compose 'int main()' of two basic blocks..
	const Address addrMain = 0x7000;
	// first basic block -- invoke a callee in our turn
	{
		BasicBlock block(addrMain);
		using namespace isa;
		{
			Instr instr(op_push); // push link to caller
			instr.setOperand(0, 127, true);
			block.addInstr(instr);
		}
		{
			Instr instr(op_li); // load branch target -- foo
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
		const bool valid = block.validate();
		assert(valid);
		const bool success = graph.addBasicBlock(std::move(block));
		assert(success);
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
			Instr instr(op_br); // branch to caller -- return
			instr.setOperand(0, 127, true);
			block.addInstr(instr);
		}
		const bool valid = block.validate();
		assert(valid);
		const bool success = graph.addBasicBlock(std::move(block));
		assert(success);
	}
	// set up at-entry registry for 'int main()'
	{
		Registry reg;
		reg.addUnknown(0x7f); // LR
		const bool success = graph.addRegistry(addrMain, std::move(reg));
		assert(success);
	}

	// compose 'int foo()' of one basic block
	const Address addrFoo = 0x7f00;
	{
		BasicBlock block(addrFoo);
		using namespace isa;
		{
			Instr instr(op_push); // push link to caller
			instr.setOperand(0, 127, true);
			block.addInstr(instr);
		}
		{
			union {
				const int16_t imm = -42;
				uint8_t imm_arr[2];
			};
			Instr instr(op_li); // load result from foo
			instr.setOperand(0, 0);
			instr.setOperand(1, imm_arr[0]);
			instr.setOperand(2, imm_arr[1]);
			block.addInstr(instr);
		}
		{
			Instr instr(op_pop); // pop link to caller
			instr.setOperand(0, 127, true);
			block.addInstr(instr);
		}
		{
			Instr instr(op_br); // branch to caller -- return
			instr.setOperand(0, 127, true);
			block.addInstr(instr);
		}
		const bool valid = block.validate();
		assert(valid);
		const bool success = graph.addBasicBlock(std::move(block));
		assert(success);
	}
	// set up at-entry registry for 'int foo()'
	{
		Registry reg;
		reg.addUnknown(0x7f); // LR
		const bool success = graph.addRegistry(addrFoo, std::move(reg));
		assert(success);
	}

	const AddressColor color[] = {
		addrcolor_one,
		addrcolor_two
	};

	size_t colorAlt = 0;

	Address lastEnd = addr_invalid;
	for (const auto it : graph) {
		// print a gap at each address discontinuity
		const Address bbStart = it.getStartAddress();
		if (bbStart != lastEnd)
			fputc('\n', stdout);
		lastEnd = bbStart + Address(it.getSequence().size());
		print(stdout, it, color[colorAlt]);
		colorAlt ^= 1;
	}

	return 0;
}
