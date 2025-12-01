/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include"interrupts_101302923_101303908.hpp"

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

// Sort ready queue based on priority level
void prioritize(std::vector<PCB> &ready_queue) {
    std::sort(ready_queue.begin(), ready_queue.end(),
            [](const PCB &first, const PCB &second) {
                return (first.priority > second.priority);
            });
}

// Sorts wait queue based on ready times
void sort_wait(std::vector<PCB> &wait_queue) {
  std::sort(wait_queue.begin(), wait_queue.end(),
            [](const PCB &first, const PCB &second) {
              return (first.ready_time > second.ready_time);
            });
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
        for(size_t i = 0; i < wait_queue.size(); ++i) {
            PCB &proc = wait_queue[i];
            if(proc.ready_time <= current_time) {
                proc.state = READY;
                ready_queue.push_back(proc);
                execution_status += print_exec_status(current_time, proc.PID, WAITING, READY);
                wait_queue.erase(wait_queue.begin() + i);
                --i;
            }
        }
        
        sort_wait(wait_queue);

        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        static bool processing_other = false;
        static int time_in_quantum = 0;

        if(!ready_queue.empty()) {
            if(!processing_other) {
                // EP: pick the highest priority process
                prioritize(ready_queue);

                // Pick first process from ready queue
                run_process(running, job_list, ready_queue, current_time);
                execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
                processing_other = true;
                time_in_quantum = 0;
            }

            // Determine slice: remaining in quantum, remaining CPU, next IO
            int slice = std::min({int(QUANTUM - time_in_quantum), int(running.remaining_time), int(running.next_io)});

            // Advance time
            current_time += slice;
            running.remaining_time -= slice;
            running.next_io -= slice;
            time_in_quantum += slice;

            // Check IO
            if(running.next_io == 0 && running.remaining_time > 0) {
                running.state = WAITING;
                running.ready_time = current_time + running.io_duration;
                running.next_io = running.io_freq;
                wait_queue.push_back(running);
                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
                processing_other = false;
                time_in_quantum = 0;
            }
            // Check quantum preemption
            else if(time_in_quantum == QUANTUM && running.remaining_time > 0) {
                running.state = READY;
                ready_queue.push_back(running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                processing_other = false;
                time_in_quantum = 0;
            }
            // Check termination
            else if(running.remaining_time <= 0) {
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
                terminate_process(running, job_list);
                processing_other = false;
                time_in_quantum = 0;
            }
        }

        /////////////////////////////////////////////////////////////////

    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}
