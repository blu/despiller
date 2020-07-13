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

void print(FILE* f, const reg::Registry& registry, const bb::Address address)
{
	fprintf(f, "\033[38;5;13m%08x\033[0m\n", address);

	if (registry.begin() == registry.end()) {
		fprintf(f, "empty\n");
		return;
	}

	using namespace reg;
	using namespace isa;
	Register last = reg_invalid;

	for (const auto it : registry) {
		if (last != it.first) {
			fprintf(f, reg_invalid == last ? "%04x { " : "}\n%04x { ", it.first);
			last = it.first;
		}
		if (word_invalid != it.second) {
			fprintf(f, "0x%08x ", it.second);
		}
		else {
			fprintf(f, "unknown ");
		}
	}
	fprintf(f, "}\n");
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

	using namespace cfg;
	ControlFlowGraph graph; // full-program CFG

	// compose 'int main()' of two basic blocks..
	const Address addrMain_0 = 0x7000;
	// first basic block -- invoke a callee in our turn
	{
		BasicBlock block(addrMain_0);
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
	const Address addrMain_1 = 0x7004;
	// second basic block -- once our callee is done we return to our caller
	{
		BasicBlock block(addrMain_1);
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

	// print out the BBs
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

	// perform CFG analysis (BBs and their order are preset as no automated CFG traversal yet)
	using namespace reg;

	graph.stackClear();
	// set up at-entry registry for 'int main()' and compute at-exit registry
	{
		Registry reg;
		reg.addUnknown(127); // our main takes just an LR as an arg
		bool success = graph.setRegistry(addrMain_0, std::move(reg));
		assert(success);
		success = graph.calcRegistry(addrMain_0);
		assert(success);
	}
	// set up at-entry registry for 'int foo()' and compute at-exit registry
	{
		Registry reg(*graph.getRegistry(addrMain_0, REG_EXIT));
		bool success = graph.setRegistry(addrFoo, std::move(reg));
		assert(success);
		success = graph.calcRegistry(addrFoo);
		assert(success);
	}
	// set up at-entry registry for 'int main():past-callee' and compute at-exit registry
	{
		Registry reg(*graph.getRegistry(addrFoo, REG_EXIT));
		bool success = graph.setRegistry(addrMain_1, std::move(reg));
		assert(success);
		success = graph.calcRegistry(addrMain_1);
		assert(success);
	}

	putc('\n', stdout);
	print(stdout, *graph.getRegistry(addrMain_0, REG_ENTRY), addrMain_0);
	print(stdout, *graph.getRegistry(addrMain_0, REG_EXIT), addrMain_0 + graph.getBasicBlock(addrMain_0)->getSequence().size() - 1);

	print(stdout, *graph.getRegistry(addrFoo, REG_ENTRY), addrFoo);
	print(stdout, *graph.getRegistry(addrFoo, REG_EXIT), addrFoo + graph.getBasicBlock(addrFoo)->getSequence().size() - 1);

	print(stdout, *graph.getRegistry(addrMain_1, REG_ENTRY), addrMain_1);
	print(stdout, *graph.getRegistry(addrMain_1, REG_EXIT), addrMain_1 + graph.getBasicBlock(addrMain_1)->getSequence().size() - 1);

	return 0;
}
