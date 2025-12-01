/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 *
 */

#include "interrupts_101302923_101303908.hpp"
#include <algorithm>
#include <iostream>
#include <ostream>
#include <queue>
#include <string>
#include <vector>

using namespace std;

void FCFS(std::vector<PCB> &ready_queue) {
  std::sort(ready_queue.begin(), ready_queue.end(),
            [](const PCB &first, const PCB &second) {
              return (first.arrival_time > second.arrival_time);
            });
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

std::tuple<std::string /* add std::string for bonus mark */>
run_simulation(std::vector<PCB> list_processes) {

  // Sort the processes based on arrival time,
  // makes handling new arrivals easier
  stable_sort(list_processes.begin(), list_processes.end(),
              [](const PCB &first, const PCB &second) {
                return (first.arrival_time >= second.arrival_time);
              });

  std::priority_queue<PCB, vector<PCB>, Compare>
      ready_queue;             // The ready queue of processes
  std::vector<PCB> wait_queue; // The wait queue of processes
  std::vector<PCB> job_list;
  // A list to keep track of all the processes. This is similar
  // to the "Process, Arrival time, Burst time" table that you
  // see in questions. You don't need to use it, I put it here
  // to make the code easier :).

  unsigned int current_time = 0;
  PCB running;

  // Initialize an empty running process
  idle_CPU(running);

  std::string execution_status;

  // make the output table (the header row)
  execution_status = print_exec_header();

  // Indicates if process arrived/readied while another is running
  bool processing_other = false;
  int next_arrival_time = -1;
  int next_ready_time = -1;
  // Loop while till there are no ready or waiting processes.
  // This is the main reason I have job_list, you don't have to use it.
  while (!all_process_terminated(job_list) || job_list.empty()) {

    // Inside this loop, there are three things you must do:
    //  1) Populate the ready queue with processes as they arrive
    //  2) Manage the wait queue
    //  3) Schedule processes from the ready queue

    // Population of ready queue is given to you as an example.
    // Go through the list of proceeses
    if (!list_processes.empty()) {
      auto &process = list_processes.back();
      // Fast forward to next arrival if there are no ready processes and the
      // waiting processes are further away
      if (ready_queue.empty() && !wait_queue.empty() &&
          wait_queue.back().ready_time > next_arrival_time &&
          next_arrival_time > current_time) {
        current_time = next_arrival_time;
      }
      if (process.arrival_time <= current_time) {
        // check if the AT = current time
        // if so, assign memory and put the process into the ready queue
        do {
          if (assign_memory(process)) {
            process.state = READY;       // Set the process state to READY
            ready_queue.push(process);   // Add the process to the ready queue
            job_list.push_back(process); // Add it to the list of processes
            execution_status +=
                print_exec_status(current_time, process.PID, NEW, READY);
            list_processes.pop_back();
          } else {
            break;
          }
          if (list_processes.empty())
            break;
          process = list_processes.back();
        } while (process.arrival_time <= current_time);
      }
      // Record the time of the next arrival
      if (!list_processes.empty()) {
        next_arrival_time = list_processes.back().arrival_time;
      }
    }

    ///////////////////////MANAGE WAIT QUEUE/////////////////////////
    // This mainly involves keeping track of how long a process must remain in
    // the ready queue

    // Fast forward time to when the IO is complete if there are no ready
    // processes
    if (ready_queue.empty()) {
      current_time = next_ready_time;
    }
    // Move process to ready queue if IO has completed
    if (!wait_queue.empty() && wait_queue.back().ready_time == current_time) {
      auto &process = wait_queue.back();
      process.state = READY;
      ready_queue.push(process);
      wait_queue.pop_back();
      execution_status +=
          print_exec_status(current_time, process.PID, WAITING, READY);
    }
    // Record the time of the next IO completion
    if (!wait_queue.empty()) {
      next_ready_time = wait_queue.back().ready_time;
    }
    /////////////////////////////////////////////////////////////////

    //////////////////////////SCHEDULER//////////////////////////////
    int next = next_arrival_time - current_time; // Time until next arrival
    int ready = next_ready_time - current_time;  // Time until next ready

    if (!ready_queue.empty()) {
      // Run process from ready queue if none other are running
      if (!processing_other) {
        run_process(running, job_list, ready_queue, current_time);
        execution_status +=
            print_exec_status(current_time, running.PID, READY, RUNNING);
      }
      // Check if new arrival must be processed before process
      // IO/termination/preemption
      if (((next < running.next_io || running.next_io == 0) &&
           next < running.remaining_time && next < QUANTUM) &&
          next > 0) {
        processing_other = true;
        current_time += next;
        running.remaining_time -= next;
        running.next_io -= next;
        continue;
      }
      // Check if other process must be readied before process
      // IO/termination/preemption
      if (((ready < running.next_io || running.next_io == 0) &&
           ready < running.remaining_time && ready < QUANTUM) &&
          ready > 0) {
        processing_other = true;
        current_time += ready;
        running.remaining_time -= ready;
        running.next_io -= ready;
        continue;
      }
      // Check if preemption before process IO/termination
      if ((QUANTUM < running.next_io || running.next_io == 0) &&
          QUANTUM < running.remaining_time) {
        running.remaining_time -= QUANTUM;
        current_time += QUANTUM;
        running.next_io -= QUANTUM;
        running.state = READY;
        ready_queue.push(running);
        execution_status +=
            print_exec_status(current_time, running.PID, RUNNING, READY);
        continue;
      }

      // Run the process until IO occurs
      if (running.next_io < running.remaining_time && running.next_io != 0) {
        running.remaining_time -= running.next_io;
        current_time += running.next_io;
        running.state = WAITING;
        running.ready_time = current_time + running.io_duration;
        running.next_io = running.io_freq;
        wait_queue.push_back(running);
        sort_wait(wait_queue);
        sync_queue(job_list, running);
        execution_status +=
            print_exec_status(current_time, running.PID, RUNNING, WAITING);
        processing_other = false;
      } else {
        // Run the process until termination
        current_time += running.remaining_time;
        execution_status +=
            print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
        terminate_process(running, job_list);
        processing_other = false;
      }
    }
    /////////////////////////////////////////////////////////////////
  }

  // Close the output table
  execution_status += print_exec_footer();

  return std::make_tuple(execution_status);
}

int main(int argc, char **argv) {

  // Get the input file from the user
  if (argc != 2) {
    std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1
              << std::endl;
    std::cout << "To run the program, do: ./interrutps <your_input_file.txt>"
              << std::endl;
    return -1;
  }

  // Open the input file
  auto file_name = argv[1];
  std::ifstream input_file;
  input_file.open(file_name);

  // Ensure that the file actually opens
  if (!input_file.is_open()) {
    std::cerr << "Error: Unable to open file: " << file_name << std::endl;
    return -1;
  }

  // Parse the entire input file and populate a vector of PCBs.
  // To do so, the add_process() helper function is used (see include file).
  std::string line;
  std::vector<PCB> list_process;
  while (std::getline(input_file, line)) {
    auto input_tokens = split_delim(line, ", ");
    for (auto token : input_tokens) {
      // std::cout << token << std::endl;
    }
    auto new_process = add_process(input_tokens);
    list_process.push_back(new_process);
  }
  input_file.close();

  // With the list of processes, run the simulation
  auto [exec] = run_simulation(list_process);

  write_output(exec, "execution.txt");

  return 0;
}
