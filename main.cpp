#include <stdio.h>
#include <alloca.h>
#include "isa.h"

int main(int, char **)
{
	{
		using namespace isa;
		Instr instr(op_nop);
		instr.setOperand(0, reg_invalid, true);

		const size_t buffSize = strFromInstr(instr, nullptr, 0);
		char* buffer = reinterpret_cast< char* >(alloca(buffSize));

		const size_t checkSize = strFromInstr(instr, buffer, buffSize);
		assert(checkSize == buffSize);

		printf("%s\n", buffer);
	}
	{
		using namespace isa;
		Instr instr(op_li);
		instr.setOperand(0, 42);
		instr.setOperand(1, 0x80);
		instr.setOperand(2, 0xff);

		const size_t buffSize = strFromInstr(instr, nullptr, 0);
		char* buffer = reinterpret_cast< char* >(alloca(buffSize));

		const size_t checkSize = strFromInstr(instr, buffer, buffSize);
		assert(checkSize == buffSize);

		printf("%s\n", buffer);
	}

	{
		using namespace isa;
		Instr instr(op_push);
		instr.setOperand(0, 42, true);

		const size_t buffSize = strFromInstr(instr, nullptr, 0);
		char* buffer = reinterpret_cast< char* >(alloca(buffSize));

		const size_t checkSize = strFromInstr(instr, buffer, buffSize);
		assert(checkSize == buffSize);

		printf("%s\n", buffer);
	}
	{
		using namespace isa;
		Instr instr(op_pop);
		instr.setOperand(0, 42, true);

		const size_t buffSize = strFromInstr(instr, nullptr, 0);
		char* buffer = reinterpret_cast< char* >(alloca(buffSize));

		const size_t checkSize = strFromInstr(instr, buffer, buffSize);
		assert(checkSize == buffSize);

		printf("%s\n", buffer);
	}
	{
		using namespace isa;
		Instr instr(op_br);
		instr.setOperand(0, 42, true);

		const size_t buffSize = strFromInstr(instr, nullptr, 0);
		char* buffer = reinterpret_cast< char* >(alloca(buffSize));

		const size_t checkSize = strFromInstr(instr, buffer, buffSize);
		assert(checkSize == buffSize);

		printf("%s\n", buffer);
	}
	{
		using namespace isa;
		Instr instr(op_cbr);
		instr.setOperand(0, 42);
		instr.setOperand(1, 43);
		instr.setOperand(2, 44);

		const size_t buffSize = strFromInstr(instr, nullptr, 0);
		char* buffer = reinterpret_cast< char* >(alloca(buffSize));

		const size_t checkSize = strFromInstr(instr, buffer, buffSize);
		assert(checkSize == buffSize);

		printf("%s\n", buffer);
	}
	{
		using namespace isa;
		Instr instr(op_op2);
		instr.setOperand(0, 42);
		instr.setOperand(1, 43, true);

		const size_t buffSize = strFromInstr(instr, nullptr, 0);
		char* buffer = reinterpret_cast< char* >(alloca(buffSize));

		const size_t checkSize = strFromInstr(instr, buffer, buffSize);
		assert(checkSize == buffSize);

		printf("%s\n", buffer);
	}
	{
		using namespace isa;
		Instr instr(op_op3);
		instr.setOperand(0, 42);
		instr.setOperand(1, 43);
		instr.setOperand(2, 44);

		const size_t buffSize = strFromInstr(instr, nullptr, 0);
		char* buffer = reinterpret_cast< char* >(alloca(buffSize));

		const size_t checkSize = strFromInstr(instr, buffer, buffSize);
		assert(checkSize == buffSize);

		printf("%s\n", buffer);
	}
	return 0;
}
