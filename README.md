Memory Simulator
=====================
### Memory Simulator developed by C++

# Introduction
In this project we will implement a simulation of cpu access to memory. 
We will use segment page table mechanism that allows running programs where only part of them are in the memory. 
The program memory (also called virtual memory) is split to pages which are load to the main memory if needed.
We will implement a computer virtual memory with max one program that can run in parallel. 

# Rules for memory management using page table
The main function of the project will be built from sequence of "load" and "store" calls (random), this functions simulate the read/write of the CPU (we will focus only on read/write operations for this simulation). 
The main part of the project is to implement load and store functions using a page table that maps the logical pages to physical.
You should write it in c++. Except for constructor and destructor, the main functions are:
- Load function:
Gets a logical address that it has to access to read data. Mainly, this function makes sure the relevant page of the wanted process is in the main memory.
- Store function:
Gets an address it has to access for writing data. Similar to load, you have to make sure the page of the wanted address for the given process is in the memory.
We will use in this project segment page table with 2 levels. First levelswill have 4 entries for each page type (heap_stack, bss, data, text data) and second level is regular page table. The size of each type of the sub inner tables will be derivate from the sizes that will be sent to the constructor (details below)

Diagram explained, the explanation is only relevant for one process:
For each virtual address that we get to store and load functions we firstly have to convert from virtual address to physical address via the two levels table:

1. Virtual address is built from the page number in the address that is represented by 12 bits, 2 bits will define the entry in the out table, and the other bits will be defined by the page size that will be sent to the constructor.
2. In case you get virtual address smaller than 12 bits, you have to pad with zeros (0) from the left.
3. Given a virtual address we would like to firstly identify to which page the address is related and what is the hist.
4. At the end, the page table will return us the page we found, and the matching frame.
5. If the page already in the main memory, we check in the page table if we can access
the matching frame in the main memory and progress in it by the hist for writing or reading.
6. If the page is not in the main memory, we have to grab it from the correct place.

Here there are multiple options:1. Check if this page has read only permissions (text) or write (data+bss+stack_heap)
1. In case there is no write permission
    1. An its load operation, then the page is in the executable file.
    2. If its store operation, the function will print matching error and return (the program continues).
2. In case there is writing permission, it means it is a page of type (data+bss+stack_heap) so we have to check:
- If the page is dirty – means some stuff written on in and some changes made on it (in other words, there was already a reading with store to this page):
    1. If yes, the file is in the swap file
    2. If not, you have to check if it’s a page of (data+bss+stack_heap)
        1. If bss, heap_stack, we allocate new empty page – which is all 0.
        2. Else, address data will be read from the file.

There is one occurrence wher you have to differ between heap_Stack and bss in case of load:
1. If we accessed a page of type heap_stack, we cannot do on him a first time load - other wise it will be an error! In other words, when we load to a page of type heap_stack, it must be in state V=1 or D=1 – else it will be error!.
2. BSS – similar from the file same as data in the case of load on first time

# Additional notes for implementation:
1. When initialize the system, do not load any of the pages from the file to memory.
The page loads will be in "lazy" way, (demand paging).2. Note that on text pages you cannot write! We will never have to backup changes in them, meaning we do not have to write them to swap, and we always can read from them again from the executable file if needed.
3. Pages written to swap by "first-fit" in the first empty space. Where a page loaded from the swap to memory. You have to delete (reset) his content in the swap files to zeros.
4. Choosing a page to free from the memory when the memory is full – using LRU algorithm.
5. Cannot change the function signature.
6. This system runs single process. This process will be "loaded" to the system from the executable file that his name passed as argument to the program, and saved in a variable of type sim_mem
7. For ease, this type will not be standard executable file that contains machine code commands – but we will relate to him like it is.

# files
- main.cpp : has Main function
- sim_mem.h :  Definition of sim_mem class
- sim_mem.cpp : Declaration of sim_mem class
- README.md : me


    
