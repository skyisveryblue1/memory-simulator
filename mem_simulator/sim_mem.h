#ifndef SIM_MEM_H
#define SIM_MEM_H

#include <string>
#include <fstream>
#include <vector>

#define MEMORY_SIZE 200

typedef struct page_descriptor {
    bool valid;
    int frame;
    bool dirty;
    int swap_index;
    long ref_time;
} page_descriptor;

class sim_mem {
public:
    sim_mem(std::string exe_file_name, std::string swap_file_name,
            int text_size, int data_size, int bss_size, 
            int heap_stack_size, int page_size, int num_of_pages);
    ~sim_mem();

    char load(int address);
    void store(int address, char value);
    void print_memory();
    void print_swap();
    void print_page_table();

private:
    // Private member variables
    std::string exe_file_name;
    std::string swap_file_name;
    std::fstream exe_file;
    std::fstream swap_file;
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int page_size;
    int num_of_pages;

    // You may need a data structure to keep track of your pages and their statuses
    // For example, you might have a struct for pages and a vector of these structs
    // to represent your page table.
    std::vector<page_descriptor> page_table;

    // Helper functions to manipulate pages and the memory
    // They're not specified in the task description, but you'll likely need them
    void load_page(int index, bool is_load);
    void unload_page(int index);
    void swap_page(int index);

    // Status which frames were used in main memory
    int frame_count;
    int* frames_status;
};

#endif  // SIM_MEM_H
