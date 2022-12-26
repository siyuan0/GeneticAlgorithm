#ifndef INCLUDE_GA_CORE
#define INCLUDE_GA_CORE

#include <vector>
#include <memory>
#include <ctime>
#include <cstdlib>
#include "solution.hpp"
#include <unordered_map>
#include <thread>
#include "utils.hpp"

static int unique_id = 0;

int generateThreadID()
{
    unique_id += 1;
    return unique_id;
}

template <typename T>
struct ProblemCtx
{   // problem specific methods
    std::vector<T> (*getRandomSolutions)(int, std::unordered_map<std::string, float>);
};

struct GA_policy
{
    int numParents;
    int numPopulationReplaced;
};

template <typename T>
class GA
{
private:
    std::vector<T> _population;
    std::unordered_map<std::string, float> _parameters;
    GA_policy _policy; // tracks current algorithm state
    ProblemCtx<T> _problemCtx;

public:
    GA(ProblemCtx<T> problemCtx,
       std::unordered_map<std::string, float> parameters)
    {
        std::srand(std::time(NULL)); // seed the random number generator using current time
        _problemCtx = problemCtx;
        _parameters = parameters;
        _policy = {
            static_cast<int>(_parameters["initialParentCount"]),
            static_cast<int>(_parameters["initialPopulationReplaced"])
        };
    };

    void print()
    {   // print curr population
        for(int i=0; i<_population.size(); i++) std::cout << _population[i] << '\n';
    }

    void generateInitialPopulation()
    {
        std::cout << "generateInitialPopulation: unimplemented\n";
        _population.clear();
        _population = _problemCtx.getRandomSolutions(_parameters["population size"], _parameters);
    }


    void evalPopulation(std::shared_ptr<std::vector<T>> population)
    {
        std::cout << "evalPopulation: unimplemented\n";
    }

    std::vector<int> getParents(int numParents, int rangeStart, int rangeEnd)
    {
        std::cout << "getParents: " << numParents << " unimplemented\n";
        std::vector<int> nilreturn;
        return nilreturn;
    }

    std::vector<T> getNewPopulation(int numNewPopulation)
    {
        std::cout << "getNewPopulation: unimplemented\n";
        std::vector<T> nilreturn;
        return nilreturn;
    }

    void updatePopulation(std::vector<T> newPopulation)
    {
        std::cout << "updatePopulation: unimplemented\n";
    }

    bool terminateSearch()
    {
        std::cout << "terminateSearch: unimplemented\n";
        return true;
    }

    void optimiseThread(int maxIter, int rangeStart, int rangeEnd, GA_policy* policy)
    {   // perform optimisation on a subset of population
        // ie. _population[rangeStart:rangeEnd]
        int threadID = generateThreadID();
        THREADPRINT("thread " << threadID << " started\n")
        int iter = 0;
        while(iter < maxIter)
        {
            iter += 1;

            // perform population search
            std::vector<int> parentIdx = getParents(policy->numParents, rangeStart, rangeEnd);
            std::vector<T> newPopulation = getNewPopulation(policy->numPopulationReplaced);
            updatePopulation(newPopulation);
            if(terminateSearch()) break;
        }
        THREADPRINT("thread " << threadID << " ended\n")
    }

    void optimise()
    {
        generateInitialPopulation();
        std::vector<std::thread> threadList;
        std::cout << "number of available processors = " << std::thread::hardware_concurrency() << '\n';
        for(int i=0; i<_parameters["number of Threads"]; i++)
        {
            threadList.push_back(std::thread(
                        &GA<T>::optimiseThread, 
                        this,
                         _parameters["max_iterations"], 
                         0, 
                         _population.size(), 
                         &_policy)); // starts a thread
        }
        
        for(int i=0; i<threadList.size(); i++) threadList[i].join(); // wait for all threads to complete
        std::cout << "all threads completed\n";
        
    }
};

#endif //INCLUDE_GA_CORE