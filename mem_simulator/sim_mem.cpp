#include "sim_mem.h"
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Initialization of the main_memory
char main_memory[MEMORY_SIZE] = { 0 };

sim_mem::sim_mem(std::string exe_file_name, std::string swap_file_name, int text_size, int data_size, int bss_size, int heap_stack_size, int page_size, int num_of_pages)
{
	// Assign the variables
	this->exe_file_name = exe_file_name;
	this->swap_file_name = swap_file_name;
	this->text_size = text_size;
	this->data_size = data_size;
	this->bss_size = bss_size;
	this->heap_stack_size = heap_stack_size;
	this->page_size = page_size;
	this->num_of_pages = num_of_pages;

	// Open the exe file
	exe_file.open(exe_file_name.c_str(), std::ios::binary | std::ios::in);
	if (!exe_file.is_open()) {
		std::cerr << "Failed to open the exe file: " << exe_file_name << "\n";
		exit(1);
	}

	// Open the swap file and initialize it to all zeros
	swap_file.open(swap_file_name.c_str(), std::ios::binary | std::ios::in | std::ios::out);
	if (!swap_file.is_open()) {
		std::cout << "Swap file not found. Creating new one." << std::endl;
		swap_file.open(swap_file_name.c_str(), std::ios::binary | std::ios::out);
		if (!swap_file.is_open()) {
			std::cerr << "Error: Failed to Create swap file!" << std::endl;
			exit(1);
		}
	}

	// Initialize the page_table with default values
	for (int i = 0; i < num_of_pages; i++) {
		page_descriptor pd = { false, -1, false, -1, 0 };
		page_table.push_back(pd);
	}

	// Initialize swap file with 0 by the logical memory size
	//int swap_size = data_size + bss_size + heap_stack_size;
	//int num_of_swap_page = swap_size / page_size + (swap_size % page_size ? 1 : 0);
	char* buffer = (char*)malloc(page_size);
	memset(buffer, 0, page_size);	
	for (int i = 0; i < num_of_pages; i++) {
		swap_file.write(buffer, page_size);
	}
	swap_file.flush();

	free(buffer);

	// Initialize main memory
	memset(main_memory, 0, MEMORY_SIZE);

	// Initialize status of frames
	frame_count = MEMORY_SIZE / page_size;
	frames_status = (int*)malloc(frame_count * sizeof(int));
	memset(frames_status, -1, frame_count * sizeof(int));
}

sim_mem::~sim_mem()
{
	exe_file.close();
	swap_file.close();

	free(frames_status);
}

char sim_mem::load(int address)
{
	if (address < 0 || address >= MEMORY_SIZE) {
		std::cerr << "Error: Invalid memory access!" << std::endl;
		return 0;
	}

	// Implement your logic here...
	int page_num = address / page_size;
	int offset = address % page_size;

	if (!page_table[page_num].valid) {
		load_page(page_num, true);
	}

	// Map the virtual address to the physical address
	int frame_num = page_table[page_num].frame;
	int phys_addr = (frame_num * page_size) + offset;

	// Update LRU
	page_table[page_num].ref_time = clock();

	return main_memory[phys_addr];
}

void sim_mem::store(int address, char value)
{
	if (address < 0 || address >= MEMORY_SIZE) {
		std::cerr << "Error: Invalid memory access!" << std::endl;
		return;
	}

	// Implement your logic here...
	int page_num = address / page_size;
	int offset = address % page_size;

	if (!page_table[page_num].valid) {
		load_page(page_num, false);
	}

	// Map the virtual address to the physical address
	int frame_num = page_table[page_num].frame;
	int phys_addr = (frame_num * page_size) + offset;

	// Store the value at the physical address
	main_memory[phys_addr] = value;

	// Mark the page as dirty
	page_table[page_num].dirty = true;

	// Update LRU
	page_table[page_num].ref_time = clock();

}

// You'll likely need helper functions for manipulating memory and pages.
void sim_mem::load_page(int index, bool is_load)
{
	// Implement your logic here...

	// Check if this page has read only permissions(text) or write permissions(data + bss + heap stack)
	int permissions = (index < text_size / page_size) ? O_RDONLY : O_RDWR;

	bool copy_from_exe = false;
	bool init_new_page = false;

	if (permissions == O_RDONLY) {
		copy_from_exe = true;
	}
	else {
		if (!page_table[index].dirty) {
			// bss, heap stack
			if (index >= ((text_size + data_size) / page_size)) {
				init_new_page = true;
			}
			else {
				copy_from_exe = true;
			}
		}
	}

	if (init_new_page && is_load) {
		// heap stack
		if (index >= ((text_size + data_size + bss_size) / page_size)) {
			std::cerr << "Error: impossible to init heap/stack page in memory!" << std::endl;
			return;
		} 
	}

	// Find a free frame in main memory to load the page into
	int free_index = -1;
	for (int i = 0; i < frame_count; i++) {
		if (frames_status[i] == -1) {
			free_index = i;
			break;
		}
	}

	// If memory is full, execute swapping.
	if (free_index == -1) {
		// If no free frame is available, swap out a page to make room
		// and then load the new page into the freed frame.
		int victim_page = -1;

		// First, look for a non-dirty page that can be swapped out
		for (int i = 0; i < num_of_pages; i++) {
			if (page_table[i].valid && !page_table[i].dirty) {
				victim_page = i;
				break;
			}
		}

		if (victim_page == -1) {
			// If no non-dirty page is available, pick a dirty page to swap out
			// and write its contents to the swap file before freeing its frame.
			long lru = LONG_MAX;
			for (int i = 0; i < num_of_pages; i++) {
				if (page_table[i].valid && lru > page_table[i].ref_time) {
					victim_page = i;
					lru = page_table[i].ref_time;
				}
			}
		}

		// Load the new page into the freed frame
		free_index = page_table[victim_page].frame;

		// Unload page
		unload_page(victim_page);
	}
	
	// Read the page from the executable or swap file into a temporary buffer
	char* buffer = (char*)malloc(page_size);
	if (init_new_page) {
		memset(buffer, 0, page_size);
	}
	else {
		std::fstream& file = copy_from_exe ? exe_file : swap_file;
		// Seek to the start byte of the page in the file
		file.seekg(index * page_size, std::ios::beg);
		file.read(buffer, page_size);
	}

	// Copy the contents of the temporary buffer into main memory
	memcpy(&main_memory[free_index * page_size], buffer, page_size);
	// Free the temporary buffer
	delete[] buffer;

	// Update the page table entry for the loaded page
	page_table[index].valid = true;
	page_table[index].frame = free_index;
	page_table[index].dirty = false;
	page_table[index].swap_index = -1;

	// Update the status of selected frame in main memory
	frames_status[free_index] = index;
}

void sim_mem::unload_page(int index)
{
	// Implement your logic here...
	// If the page is dirty, write its contents to the swap file
	if (page_table[index].dirty) {
		swap_file.seekg(index * page_size, SEEK_SET);
		swap_file.write(&main_memory[page_table[index].frame * page_size], page_size);
	}

	// Mark the page as no longer valid, and free its frame
	page_table[index].valid = false;
	page_table[index].frame = -1;
	page_table[index].dirty = false;
	page_table[index].swap_index = -1;

	// Update the status of selected frame in main memory
	frames_status[index] = -1;
}

void sim_mem::swap_page(int index)
{
	// Implement your logic here...
	if (page_table[index].valid) {
		std::cerr << "Error: Page already in memory!" << std::endl;
		return;
	}

	char* buffer = new char[page_size];
	// Seek to the beginning of the page in the swap file
	swap_file.seekg(index * page_size, std::ios::beg);

	// Read the page from the swap file to a buffer
	swap_file.read(buffer, page_size);

	// Allocate a free frame in memory and copy the page from the buffer to memory
	memcpy(&main_memory[page_table[index].frame * page_size], buffer, page_size);

	delete[] buffer;

	// Update the page table entry for the loaded page
	page_table[index].valid = true;
	page_table[index].frame = index;
	page_table[index].dirty = false;
	page_table[index].swap_index = index;
}


void sim_mem::print_memory()
{
	std::cout << "\n Physical memory\n";
	for (int i = 0; i < MEMORY_SIZE; i++) {
		std::cout << "[" << main_memory[i] << "]\n";
	}
}

void sim_mem::print_swap()
{
	char* buffer = (char*)malloc(page_size);
	printf("\n Swap memory\n");
	swap_file.seekg(0, std::ios::beg); // go to the start of the file
	while (!swap_file.eof()) {
		swap_file.read(buffer, this->page_size);
		std::streamsize read_count = swap_file.gcount();
		for (int i = 0; i < read_count; i++) {
			printf("%d - [%c]\t", i, buffer[i]);
		}
		printf("\n");
	}
	free(buffer);
}

void sim_mem::print_page_table()
{
	int i;
	int num_of_txt_pages = text_size / page_size;
	int num_of_data_pages = data_size / page_size;
	int num_of_bss_pages = bss_size / page_size;
	int num_of_stack_heap_pages = heap_stack_size / page_size;
	printf("Valid\t Dirty\t Frame\t Swap index\n");
	for (i = 0; i < num_of_txt_pages; i++) {
		printf("[%d]\t[%d]\t[%d]\t[%d]\n",
			page_table[i].valid,
			page_table[i].dirty,
			page_table[i].frame,
			page_table[i].swap_index);
	}
	printf("Valid\t Dirty\t Frame\t Swap index\n");
	for (i = num_of_txt_pages; i < num_of_txt_pages + num_of_data_pages; i++) {
		printf("[%d]\t[%d]\t[%d]\t[%d]\n",
			page_table[i].valid,
			page_table[i].dirty,
			page_table[i].frame,
			page_table[i].swap_index);
	}
	printf("Valid\t Dirty\t Frame\t Swap index\n");
	for (i = num_of_txt_pages + num_of_data_pages; i < num_of_txt_pages + num_of_data_pages + num_of_bss_pages; i++) {
		printf("[%d]\t[%d]\t[%d]\t[%d]\n",
			page_table[i].valid,
			page_table[i].dirty,
			page_table[i].frame,
			page_table[i].swap_index);
	}
	printf("Valid\t Dirty\t Frame\t Swap index\n");
	for (i = num_of_txt_pages + num_of_data_pages + num_of_bss_pages; i < num_of_pages; i++) {
		printf("[%d]\t[%d]\t[%d]\t[%d]\n",
			page_table[i].valid,
			page_table[i].dirty,
			page_table[i].frame,
			page_table[i].swap_index);
	}
}
