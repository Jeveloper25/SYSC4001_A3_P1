/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 */

#include"interrupts.hpp"
#include <string>

std::tuple<std::string, std::string, int> simulate_trace(std::vector<std::string> trace_file, int time, std::vector<std::string> vectors, std::vector<int> delays, std::vector<external_file> external_files, PCB current, std::vector<PCB> wait_queue) {

    std::string trace;      //!< string to store single line of trace file
    std::string execution = "";  //!< string to accumulate the execution output
    std::string system_status = "";  //!< string to accumulate the system status output
    int current_time = time;
	unsigned int line_number;

    //parse each line of the input trace file. 'for' loop to keep track of indices.
    for(size_t i = 0; i < trace_file.size(); i++) {
	auto trace = trace_file[i];

	auto [activity, duration_intr, program_name] = parse_trace(trace);

	if(activity == "CPU") { //As per Assignment 1
	    execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", CPU Burst\n";
	    current_time += duration_intr;

	} else if(activity == "SYSCALL") { //As per Assignment 1
	int device_number = duration_intr;
		if (device_number >= std::min(delays.size(), vectors.size()) || device_number< 0) {
			std::cout << "Line "<< line_number << "\nInvalid device number: " << device_number
			<< "\nDevice number must be between 0 and " << std::min(delays.size(), vectors.size()) << std::endl;
		}
		    auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
		    execution += intr;
		    current_time = time;
			int isr_duration = delays.at(device_number);
			execution += std::to_string(current_time) + ", " + std::to_string(ISR_ACTIVITY_TIME) + ", " + "SYSCALL: run the ISR (device driver)\n";
			current_time += ISR_ACTIVITY_TIME;
			execution += std::to_string(current_time) + ", " + std::to_string(ISR_ACTIVITY_TIME) + ", " + "transfer data from device to memory\n";
			current_time += ISR_ACTIVITY_TIME;
		if (3 * ISR_ACTIVITY_TIME < isr_duration) {
			int remaining_time = (isr_duration - 2 * ISR_ACTIVITY_TIME);
			execution += std::to_string(current_time) + ", " + std::to_string(remaining_time) + ", " + "check for errors\n";
			current_time += remaining_time;
		} else {
			execution += std::to_string(current_time) + ", " + std::to_string(ISR_ACTIVITY_TIME) + ", " + "check for errors\n";
			current_time += ISR_ACTIVITY_TIME;
		}
		    execution +=  std::to_string(current_time) + ", 1, IRET\n";
		    current_time += 1;

	} else if(activity == "END_IO") {
	int device_number = duration_intr;
		if (device_number >= std::min(delays.size(), vectors.size()) || device_number < 0) {
			std::cout << "Line "<< line_number << "\nInvalid device number: " << device_number
			<< "\nDevice number must be between 0 and " << std::min(delays.size(), vectors.size()) << std::endl;
		}
	    auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
	    current_time = time;
	    execution += intr;
		int isr_duration = delays.at(device_number);
		execution += std::to_string(current_time) + ", " + std::to_string(ISR_ACTIVITY_TIME) + ", " + "ENDIO: run the ISR (device driver)\n";
		current_time += ISR_ACTIVITY_TIME;
		if (ISR_ACTIVITY_TIME * 2 < isr_duration) {
			int remaining_time = (isr_duration - ISR_ACTIVITY_TIME);
			execution += std::to_string(current_time) + ", " + std::to_string(remaining_time) + ", " + "check device status\n";
			current_time += remaining_time;
		} else {
			execution += std::to_string(current_time) + ", " + std::to_string(ISR_ACTIVITY_TIME) + ", " + "check device status\n";
			current_time += ISR_ACTIVITY_TIME;
		}
	    execution +=  std::to_string(current_time) + ", 1, IRET\n";
	    current_time += 1;

	} else if(activity == "FORK") {
	    auto [intr, time] = intr_boilerplate(current_time, 2, 10, vectors);
	    execution += intr;
	    current_time = time;

	    ///////////////////////////////////////////////////////////////////////////////////////////
	    //Add your FORK output here
		execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", cloning the PCB\n";
		current_time += duration_intr;
		execution += std::to_string(current_time) + ", 0, scheduler called\n";
		execution += std::to_string(current_time) + ", 1, IRET\n";
		current_time++;

	    ///////////////////////////////////////////////////////////////////////////////////////////

	    //The following loop helps you do 2 things:
	    // * Collect the trace of the chile (and only the child, skip parent)
	    // * Get the index of where the parent is supposed to start executing from
	    std::vector<std::string> child_trace;
	    bool skip = true;
	    bool exec_flag = false;
	    int parent_index = 0;

	    for(size_t j = i; j < trace_file.size(); j++) {
		auto [_activity, _duration, _pn] = parse_trace(trace_file[j]);
		if(skip && _activity == "IF_CHILD") {
		    skip = false;
		    continue;
		} else if(_activity == "IF_PARENT"){
		    skip = true;
		    parent_index = j;
		    if(exec_flag) {
			break;
		    }
		} else if(skip && _activity == "ENDIF") {
		    skip = false;
		    continue;
		} else if(!skip && _activity == "EXEC") {
		    skip = true;
		    child_trace.push_back(trace_file[j]);
		    exec_flag = true;
		}

		if(!skip) {
		    child_trace.push_back(trace_file[j]);
		}
	    }
	    i = parent_index;

	    ///////////////////////////////////////////////////////////////////////////////////////////
	    //With the child's trace, run the child (HINT: think recursion)
		PCB child_pcb = current;
		child_pcb.PPID = child_pcb.PID;
		child_pcb.PID++;
		if (!allocate_memory(&child_pcb)) {
			execution += std::to_string(current_time) + ", 1, ERROR: no partition available for child (size: " + std::to_string(child_pcb.size) + "MB)\n";
			current_time += 1;
			}
		wait_queue.push_back(current);
		system_status += "time: " + std::to_string(current_time) + "; current trace: FORK, " + std::to_string(duration_intr) + "\n";
		system_status += print_PCB(child_pcb, wait_queue);
		auto [child_exec, child_status, child_time] = simulate_trace(child_trace, current_time, vectors, delays, external_files, child_pcb, wait_queue);
		execution += child_exec;
		system_status += child_status;
		current_time = child_time;
		wait_queue.pop_back();
		free_memory(&child_pcb);

	    ///////////////////////////////////////////////////////////////////////////////////////////


	} else if(activity == "EXEC") {
		// E requirement
		auto [intr, time] = intr_boilerplate(current_time, 3, 10, vectors);
	    current_time = time;
	    execution += intr;

	    ///////////////////////////////////////////////////////////////////////////////////////////
	    //Add your EXEC output here
		
        // search for the program in external_files
		// F requirement
        unsigned int program_size = 0;
        bool program_found = false;
        for (const auto& file : external_files) {
            if (file.program_name == program_name) {
                // program found
                program_size = file.size;
                program_found = true;
                break;
            }
        }

        if (!program_found) {
            // output for program not found
            execution += std::to_string(current_time) + ", 1, ERROR: Program " + program_name + " not found in external files\n";
            current_time += 1;
        } else {
            // exec output logic for program when found
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", Program is " + std::to_string(program_size) + " MB large\n";
            current_time += duration_intr;

            // freeing current partition
            free_memory(&current);
            execution += std::to_string(current_time) + ", 1, free current partition\n";
            current_time += 1;

            // update PCB
            current.program_name = program_name;
            current.size = program_size;
            current.partition_number = -1;

            // allocate memory
            // G requirement
            if (allocate_memory(&current)) {
                execution += std::to_string(current_time) + ", 1, allocate partition " + std::to_string(current.partition_number) + " for " + program_name + "\n";
                current_time += 1;

                // H requirement
		int load_time = program_size * 15; // assumption in assignment (15ms for every MB)
                execution += std::to_string(current_time) + ", " + std::to_string(load_time) + ", loading " + program_name + " into memory\n";
                current_time += load_time;

                // I requirement
                execution += std::to_string(current_time) + ", 1, partition " + std::to_string(current.partition_number) + " marked as occupied\n";
                current_time += 1;

                // J requirement
                execution += std::to_string(current_time) + ", 6, update PCB with new program info\n";
                current_time += 6;

                // K requirment
                execution += std::to_string(current_time) + ", 0, " + "scheduler called\n";
                execution += std::to_string(current_time) + ", 1, IRET\n";
                current_time += 1;  

            } else {
                // no partition found
                execution += std::to_string(current_time) + ", 1, ERROR: no partition available for " + program_name + " (size: " + std::to_string(program_size) + "MB)\n";
                current_time += 1;
            }
        }

	    ///////////////////////////////////////////////////////////////////////////////////////////

		//FROM KEON: Added the relative path of the program, to use the program of a different test case just change the number after test.
	    
		std::ifstream exec_trace_file("input_files/test5/" + program_name + ".txt");

	    std::vector<std::string> exec_traces;
	    std::string exec_trace;
	    while(std::getline(exec_trace_file, exec_trace)) {
			exec_traces.push_back(exec_trace);
	    }

		exec_trace_file.close();

	    ///////////////////////////////////////////////////////////////////////////////////////////
	    //With the exec's trace (i.e. trace of external program), run the exec (HINT: think recursion)
		
		current.program_name = program_name;
		system_status += "time: " + std::to_string(current_time) + "; current trace: EXEC, " + program_name + "\n";
		system_status += print_PCB(current, wait_queue);
		
		// I requirment
		auto [exec_exec, exec_status, exec_time] = simulate_trace(exec_traces, current_time, vectors, delays, external_files, current, wait_queue);

		execution += exec_exec;
        system_status += exec_status;
		current_time = exec_time;

	    ///////////////////////////////////////////////////////////////////////////////////////////

	    break; //Why is this important? (answer in report)

	}
	line_number++;
    }

    return {execution, system_status, current_time};
}

int main(int argc, char** argv) {

    //vectors is a C++ std::vector of strings that contain the address of the ISR
    //delays  is a C++ std::vector of ints that contain the delays of each device
    //the index of these elemens is the device number, starting from 0
    //external_files is a C++ std::vector of the struct 'external_file'. Check the struct in 
    //interrupt.hpp to know more.
    auto [vectors, delays, external_files] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    //Just a sanity check to know what files you have
    print_external_files(external_files);

    //Make initial PCB (notice how partition is not assigned yet)
    PCB current(0, -1, "init", 1, -1);
    //Update memory (partition is assigned here, you must implement this function)
    if(!allocate_memory(&current)) {
	std::cerr << "ERROR! Memory allocation failed!" << std::endl;
    }

    std::vector<PCB> wait_queue;

    /******************ADD YOUR VARIABLES HERE*************************/


    /******************************************************************/

    //Converting the trace file into a vector of strings.
    std::vector<std::string> trace_file;
    std::string trace;
    while(std::getline(input_file, trace)) {
	trace_file.push_back(trace);
    }

    auto [execution, system_status, _] = simulate_trace(   trace_file, 
					    0, 
					    vectors, 
					    delays,
					    external_files, 
					    current, 
					    wait_queue);

    input_file.close();

    write_output(execution, "execution.txt");
    write_output(system_status, "system_status.txt");

    return 0;
}
