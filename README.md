# GeneticAlgorithm

Most problem parameters are stored in `Example/SchwefelFunction/parameters.json`, and the file can be 
modified freely and the compiled program can be rerun without needing for rebuild.

The exception is changing the DIMENSION of the problem, which needs to be done by modifying the number in 
line 17 in `Example/SchwefelFunction/problem.hpp` to the DIMENSION desired. If this is changed, the program
will need to be rebuilt following the instructions below. This design is because the `soln` class uses
c-style array instead of STL containers for faster execution (eg. avoid the slower heap access in std::vector)

## Debug build
For debug build, run 

`mkdir Debug; cd Debug`

`cmake -DCMAKE_BUILD_TYPE=Debug ..`

`cmake --build .`

To execute
`./GA_run ../Example/SchwefelFunction/parameters.json`

note: Debug build will be slightly slower due to inclusion of debugging symbols

## Release build (the faster version)
For release build, run

`mkdir Release; cd Release`

`cmake -DCMAKE_BUILD_TYPE=Release ..`

`cmake --build .`

To execute
`./GA_run ../Example/SchwefelFunction/parameters.json`