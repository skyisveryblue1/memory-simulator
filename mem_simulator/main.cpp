
#include "sim_mem.h"

int main()
{
	char val;
	sim_mem mem_sm("C:\\Windows\\System32\\notepad.exe", "swap_file", 16, 16, 32, 32, 16, 8);
	mem_sm.store(98, 'X');
	val = mem_sm.load(8);
	mem_sm.print_memory();
	mem_sm.print_swap();

	return 0;
}