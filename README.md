# multicore-programming-final-project
Final project in course multicore programming from NCTU

## Target
Speed up bfs by using multi thread programming with pthread.

## Result
|                |   Original    |   Modified    |    Speedup    | 
| -------------  | ------------- | ------------- | ------------- |
| Execution time | 15.136085 s   |  5.106758 s   |     2.964     |

1st among final projects

## Environment
Ubuntu 14.04

## Report
### First version (total: 11s, build graph: 9s, BFS: 2s)
In this version, we **use multi threads to run BFS**. In the bfs part, after checking a level of nodes, I'll check if there are more than a certain number (4 or more, depends on the hardware) of nodes in next level. If there are, I will create the corresponding amount of threads to deal with each node in the queue. 

### Second optimization (total: 11s, build graph: 6s, BFS: 5s)
In this version, we **rebuild the data structure** of the graph as an array and run with only one thread. In the first version, most of the time is spent on building graph. Therefore, we thought time could be reduced by changing the data structure of the graph. We have tried to store the graph in array and vector. It turns out that the vector in C++ is not as efficient as the array. 
We use array to store each node's neighbors. 
- First, we had to find out the amount of nodes and maximum amount of a node's neighbors. 
- Second, we could build a 2D array, which looks like Array[# of nodes + 1][maximum # of neighbors + 1]. Array[i][0] is used to store the number of neighbors of the node i. For example, if node 532 has four neighbors: 533, 534, 535, and 536, row 532 of the array looks like this: 
```
A[532][0] = 4, A[532][1] = 533, A[532][2] = 534, A[532][3] = 535, A[532][4] = 536
```
- Later, we realized that allocating memory for a 2D array takes a lot of time. So we **use a 1D array** instead.

### Third version (total: 8s, build graph: 6s, BFS: 2s)
In this version, we **combine version 1 with version 2**. Although the data structure is different, we still use the same way to distribute the workload of bfs to each thread and get a similar result. 

### Forth version (total: 5s, build graph: 3s, BFS: 2s)
In this version, we use **multi threads to build the graph with new data structure** and run bfs. 
There are three steps in building the graph: 
1.	Find the number of nodes. 
- We traverse the input arrays (tail and head) to find the biggest node number. It costs 0.05 second and get a similar result when multi-threaded.
2.	Get the maximum amount of neighbors a node may have. 
- We traverse the input array to calculate the largest number of edges of a node. The number is used to construct the array. It costs around 1 second and could be reduced to 0.75 second when multi-threaded.
3.	Store the graph in a 1D array. 
- We traverse the input arrays (tail and head) and store all edge information into a 1D array. It costs around 5 seconds and could be reduced to around 2 seconds when multi-threaded.
