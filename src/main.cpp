#include <iostream>
#include <cstdint>
#include <cstring>
#include <string>
#define private public
#include "mmu.h"
#include "pagetable.h"
#undef private
#include <sstream>
#include <vector>
#include <algorithm>

// 64 MB (64 * 1024 * 1024)
#define PHYSICAL_MEMORY 67108864

void printStartMessage(int page_size);
void createProcess(int text_size, int data_size, Mmu *mmu, PageTable *page_table);
void allocateVariable(uint32_t pid, std::string var_name, DataType type, uint32_t num_elements, Mmu *mmu, PageTable *page_table);
void setVariable(uint32_t pid, std::string var_name, uint32_t offset, void *value, Mmu *mmu, PageTable *page_table, uint8_t *memory);
void freeVariable(uint32_t pid, std::string var_name, Mmu *mmu, PageTable *page_table);
void terminateProcess(uint32_t pid, Mmu *mmu, PageTable *page_table);
uint32_t getTypeSize(DataType type);

int main(int argc, char **argv)
{
    // Ensure user specified page size as a command line parameter
    if (argc < 2)
    {
        std::cerr << "Error: you must specify the page size" << std::endl;
        return 1;
    }

    // Print opening instuction message
    int page_size = std::stoi(argv[1]);
    printStartMessage(page_size);

    // Create physical 'memory' (raw array of bytes)
    uint8_t *memory = new uint8_t[PHYSICAL_MEMORY];

    // Create MMU and Page Table
    Mmu *mmu = new Mmu(PHYSICAL_MEMORY);
    PageTable *page_table = new PageTable(page_size);

    // Prompt loop
    std::string command;
    std::cout << "> ";
    std::getline(std::cin, command);
    while (command != "exit")
    {
        // Handle command
        // TODO: implement this!
        std::stringstream ss(command);
        std::string action;
        ss >> action;

        if (action == "create")
        {
            int text_size;
            int data_size;
            ss >> text_size >> data_size;

            createProcess(text_size, data_size, mmu, page_table);
        }
        else if (action == "allocate")
        {
            uint32_t pid;
            std::string var_name;
            std::string type_string;
            uint32_t num_elements;

            ss >> pid >> var_name >> type_string >> num_elements;

            DataType type;

            if (type_string == "char")
                type = DataType::Char;
            else if (type_string == "short")
                type = DataType::Short;
            else if (type_string == "int")
                type = DataType::Int;
            else if (type_string == "long")
                type = DataType::Long;
            else if (type_string == "float")
                type = DataType::Float;
            else if (type_string == "double")
                type = DataType::Double;
            else
            {
                std::cout << "error: command not recognized" << std::endl;
                std::cout << "> ";
                std::getline(std::cin, command);
                continue;
            }

            allocateVariable(pid, var_name, type, num_elements, mmu, page_table);
        }
        else if (action == "set")
        {
            uint32_t pid;
            std::string var_name;
            uint32_t offset;

            ss >> pid >> var_name >> offset;

            std::string value;
            uint32_t current_offset = offset;

            while (ss >> value)
            {
                setVariable(pid, var_name, current_offset, &value, mmu, page_table, memory);
                current_offset++;
            }
        }
        else if (action == "free")
        {
            uint32_t pid;
            std::string var_name;

            ss >> pid >> var_name;

            freeVariable(pid, var_name, mmu, page_table);
        }
        else if (action == "terminate")
        {
            uint32_t pid;

            ss >> pid;

            terminateProcess(pid, mmu, page_table);
        }
     else if (action == "print")
    {
        std::string object;
        ss >> object;

        if (object == "mmu")
        {
            mmu->print();
        }
        else if (object == "page")
        {
            page_table->print();
        }
        else if (object == "processes")
        {
            for (Process *p : mmu->_processes)
            {
                std::cout << p->pid << std::endl;
            }
        }
        else
        {
            size_t colon = object.find(":");

            if (colon == std::string::npos)
            {
                std::cout << "error: command not recognized" << std::endl;
                continue;
            }

            uint32_t pid = std::stoi(object.substr(0, colon));
            std::string var_name = object.substr(colon + 1);

            Process *proc = NULL;

            for (Process *p : mmu->_processes)
            {
                if (p->pid == pid)
                {
                    proc = p;
                    break;
                }
            }

            if (proc == NULL)
            {
                std::cout << "error: process not found" << std::endl;
                continue;
            }

            Variable *var = NULL;

            for (Variable *v : proc->variables)
            {
                if (v->name == var_name && v->type != DataType::FreeSpace)
                {
                    var = v;
                    break;
                }
            }

            if (var == NULL)
            {
                std::cout << "error: variable not found" << std::endl;
                continue;
            }

            uint32_t type_size = getTypeSize(var->type);
            uint32_t count = var->size / type_size;

            uint32_t to_print = std::min(count, (uint32_t)4);

            for (uint32_t i = 0; i < to_print; i++)
            {
                uint32_t virtual_address = var->virtual_address + i * type_size;
                int physical_address = page_table->getPhysicalAddress(pid, virtual_address);

                if (var->type == DataType::Char)
                {
                    char v;
                    memcpy(&v, memory + physical_address, sizeof(char));
                    std::cout << v;
                }
                else if (var->type == DataType::Short)
                {
                    short v;
                    memcpy(&v, memory + physical_address, sizeof(short));
                    std::cout << v;
                }
                else if (var->type == DataType::Int)
                {
                    int v;
                    memcpy(&v, memory + physical_address, sizeof(int));
                    std::cout << v;
                }
                else if (var->type == DataType::Long)
                {
                    long v;
                    memcpy(&v, memory + physical_address, sizeof(long));
                    std::cout << v;
                }
                else if (var->type == DataType::Float)
                {
                    float v;
                    memcpy(&v, memory + physical_address, sizeof(float));
                    std::cout << v;
                }
                else if (var->type == DataType::Double)
                {
                    double v;
                    memcpy(&v, memory + physical_address, sizeof(double));
                    std::cout << v;
                }

                if (i < to_print - 1)
                    std::cout << ", ";
            }

            if (count > 4)
            {
                std::cout << ", ... [" << count << " items]";
            }

            std::cout << std::endl;        
        }
    }
        else
        {
            std::cout << "error: command not recognized" << std::endl;
        }
        // Get next command
        std::cout << "> ";
        std::getline(std::cin, command);
    }

    // Cean up
    delete[] memory;
    delete mmu;
    delete page_table;

    return 0;
}

void printStartMessage(int page_size)
{
    std::cout << "Welcome to the Memory Allocation Simulator! Using a page size of " << page_size << " bytes." << std:: endl;
    std::cout << "Commands:" << std:: endl;
    std::cout << "  * create <text_size> <data_size> (initializes a new process)" << std:: endl;
    std::cout << "  * allocate <PID> <var_name> <data_type> <number_of_elements> (allocated memory on the heap)" << std:: endl;
    std::cout << "  * set <PID> <var_name> <offset> <value_0> <value_1> <value_2> ... <value_N> (set the value for a variable)" << std:: endl;
    std::cout << "  * free <PID> <var_name> (deallocate memory on the heap that is associated with <var_name>)" << std:: endl;
    std::cout << "  * terminate <PID> (kill the specified process)" << std:: endl;
    std::cout << "  * print <object> (prints data)" << std:: endl;
    std::cout << "    * If <object> is \"mmu\", print the MMU memory table" << std:: endl;
    std::cout << "    * if <object> is \"page\", print the page table" << std:: endl;
    std::cout << "    * if <object> is \"processes\", print a list of PIDs for processes that are still running" << std:: endl;
    std::cout << "    * if <object> is a \"<PID>:<var_name>\", print the value of the variable for that process" << std:: endl;
    std::cout << std::endl;
}

void createProcess(int text_size, int data_size, Mmu *mmu, PageTable *page_table)
{
    uint32_t pid = mmu->createProcess();

    uint32_t text_addr = 0;
    uint32_t globals_addr = text_addr + text_size;
    uint32_t stack_addr = globals_addr + data_size;

    // Add variables to MMU
    mmu->addVariableToProcess(pid, "<TEXT>", DataType::Char, text_size, text_addr);
    mmu->addVariableToProcess(pid, "<GLOBALS>", DataType::Char, data_size, globals_addr);
    mmu->addVariableToProcess(pid, "<STACK>", DataType::Char, 65536, stack_addr);

    // Add pages to page table
    uint32_t total_size = text_size + data_size + 65536;

    int first_page = 0;
    int last_page = (total_size - 1) / page_table->_page_size;

    for (int page = first_page; page <= last_page; page++)
    {
        page_table->addEntry(pid, page);
    }

    std::cout << pid << std::endl;
}

void allocateVariable(uint32_t pid, std::string var_name, DataType type, uint32_t num_elements, Mmu *mmu, PageTable *page_table)
{
    Process *proc = NULL;

    for (Process *p : mmu->_processes)
    {
        if (p->pid == pid)
        {
            proc = p;
            break;
        }
    }

    if (proc == NULL)
    {
        std::cout << "error: process not found" << std::endl;
        return;
    }

    for (Variable *v : proc->variables)
    {
        if (v->name == var_name)
        {
            std::cout << "error: variable already exists" << std::endl;
            return;
        }
    }

    uint32_t size = getTypeSize(type) * num_elements;

    uint32_t address = 0;

    for (Variable *v : proc->variables)
    {
        if (v->type != DataType::FreeSpace)
        {
            uint32_t end = v->virtual_address + v->size;
            if (end > address)
            {
                address = end;
            }
        }
    }

    int first_page = address / page_table->_page_size;
    int last_page = (address + size - 1) / page_table->_page_size;

    for (int page = first_page; page <= last_page; page++)
    {
        std::string key = std::to_string(pid) + "|" + std::to_string(page);

        if (page_table->_table.count(key) == 0)
        {
            page_table->addEntry(pid, page);
        }
    }

    mmu->addVariableToProcess(pid, var_name, type, size, address);

    std::cout << address << std::endl;
}

void setVariable(uint32_t pid, std::string var_name, uint32_t offset, void *value, Mmu *mmu, PageTable *page_table, uint8_t *memory)
{
    Process *proc = NULL;

    for (Process *p : mmu->_processes)
    {
        if (p->pid == pid)
        {
            proc = p;
            break;
        }
    }

    if (proc == NULL)
    {
        std::cout << "error: process not found" << std::endl;
        return;
    }

    Variable *var = NULL;

    for (Variable *v : proc->variables)
    {
        if (v->name == var_name && v->type != DataType::FreeSpace)
        {
            var = v;
            break;
        }
    }

    if (var == NULL)
    {
        std::cout << "error: variable not found" << std::endl;
        return;
    }

    uint32_t type_size = getTypeSize(var->type);

    if ((offset + 1) * type_size > var->size)
    {
        std::cout << "error: index out of range" << std::endl;
        return;
    }

    std::string text_value = *((std::string*)value);

    uint32_t virtual_address = var->virtual_address + offset * type_size;

    int physical_address = page_table->getPhysicalAddress(pid, virtual_address);

    if (var->type == DataType::Char)
    {
        char v = text_value[0];
        memcpy(memory + physical_address, &v, sizeof(char));
    }
    else if (var->type == DataType::Short)
    {
        short v = std::stoi(text_value);
        memcpy(memory + physical_address, &v, sizeof(short));
    }
    else if (var->type == DataType::Int)
    {
        int v = std::stoi(text_value);
        memcpy(memory + physical_address, &v, sizeof(int));
    }
    else if (var->type == DataType::Long)
    {
        long v = std::stol(text_value);
        memcpy(memory + physical_address, &v, sizeof(long));
    }
    else if (var->type == DataType::Float)
    {
        float v = std::stof(text_value);
        memcpy(memory + physical_address, &v, sizeof(float));
    }
    else if (var->type == DataType::Double)
    {
        double v = std::stod(text_value);
        memcpy(memory + physical_address, &v, sizeof(double));
    }
}

void freeVariable(uint32_t pid, std::string var_name, Mmu *mmu, PageTable *page_table)
{
    Process *proc = NULL;

    for (Process *p : mmu->_processes)
    {
        if (p->pid == pid)
        {
            proc = p;
            break;
        }
    }

    if (proc == NULL)
    {
        std::cout << "error: process not found" << std::endl;
        return;
    }

    Variable *var = NULL;

    for (Variable *v : proc->variables)
    {
        if (v->name == var_name && v->type != DataType::FreeSpace)
        {
            var = v;
            break;
        }
    }

    if (var == NULL)
    {
        std::cout << "error: variable not found" << std::endl;
        return;
    }

    proc->variables.erase(
        std::remove(proc->variables.begin(), proc->variables.end(), var),
        proc->variables.end()
    );

    delete var;
}

void terminateProcess(uint32_t pid, Mmu *mmu, PageTable *page_table)
{
    Process *proc = NULL;

    for (Process *p : mmu->_processes)
    {
        if (p->pid == pid)
        {
            proc = p;
            break;
        }
    }

    if (proc == NULL)
    {
        std::cout << "error: process not found" << std::endl;
        return;
    }

    for (Variable *v : proc->variables)
    {
        delete v;
    }

    proc->variables.clear();

    mmu->_processes.erase(
        std::remove(mmu->_processes.begin(), mmu->_processes.end(), proc),
        mmu->_processes.end()
    );

    delete proc;

    for (auto it = page_table->_table.begin(); it != page_table->_table.end(); )
    {
        std::string key = it->first;
        std::string pid_text = key.substr(0, key.find("|"));

        if ((uint32_t)std::stoi(pid_text) == pid)
        {
            it = page_table->_table.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
uint32_t getTypeSize(DataType type)
{
    if (type == DataType::Char) return 1;
    if (type == DataType::Short) return 2;
    if (type == DataType::Int) return 4;
    if (type == DataType::Float) return 4;
    if (type == DataType::Long) return 8;
    if (type == DataType::Double) return 8;

    return 0;
}
