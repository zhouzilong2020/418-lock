
# Performance Comparison of Different Locks in Multi-core Processors

## How to run?
```bash
git clone git@github.com:zhouzilong2020/418-lock.git

cd /the/path/to/the/project

chmod +x scripts/exp

chmod +x scripts/fig

scripts/exp # run the experiment, the result will be saved at ./result

scripts/fig # draw figures, will be saved at ./fig
```

## Roadmap
Week of March 28
- Review relevant literature on parallel lock implementations :white_check_mark:
- Refine project goals and deliverables :white_check_mark:
    
Week of April 4
-  Implement spinning lock and measure performance :white_check_mark:
-  Implement reader-writer lock and measure performance :white_check_mark:
    
Week of April 11
- Implement ticket lock and measure performance :white_check_mark:
- Implement array-based lock and measure performance :white_check_mark:
    
Week of April 18
-  Analyze and compare performance results  :white_check_mark:    
    
Week of April 25 :one:
-  refine test drive :white_check_mark:
-  sythesis workload for testing :white_check_mark:
-  automate test process and produce figure :white_check_mark:

Week of April 25 :two:
- basic analysis for different lock performance under different workload

Week of May 2 :one:
- final report and demo scratch

Week of May 2 :two:
-  Finalize poster and demo 
- Practice presenting poster and demo
- Submit final project report and any code/documentation updates by May 4


## Summary
We are aiming to compare the performance of different lock implementations in XV6 in terms of throughput and latency metrics.

## Goals and Deliverables
The main goal of the project is to compare the performance of different lock implementations in a parallel programming environment using the XV6 operating system. Specifically, the project will evaluate the following locks: 
- Spinning Lock (test and set, test and test and set)
- Reader-Writer Lock
- Ticket Lock
- Array Based Lock 
- MCS (maybe) 
- RCU (maybe) 

The project will measure performance based on two metrics: throughput (i.e., the number of operations completed per second) and latency (i.e., the time it takes to complete a single operation).

  

### Plan to achieve
- Implement and compare the performance of different lock implementations in a parallel programming environment.
- Evaluate the performance of different lock implementations based on throughput and latency metrics.
- Produce a report detailing the results of the performance evaluation and analyze the performance characteristics of each lock implementation.

### HOPE TO ACHIEVE
- Implement additional lock implementations beyond the ones listed in the project description and compare their performance.
- Investigate the impact of different workloads on the performance of lock implementations in a parallel programming environment.

### DEMO
The demo will show the results of the performance evaluation, including throughput and latency metrics, for each lock implementation. It will demonstrate the effectiveness of the chosen lock implementations in a parallel programming environment