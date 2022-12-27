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
#include <algorithm>

static int unique_id = 0;

int generateThreadID()
{
    unique_id += 1;
    return unique_id;
}

template <typename T>
struct ProblemCtx
{   // problem specific methods
    std::vector<T> (*getRandomSolutions)(int, std::unordered_map<std::string, float>&);
    std::vector<int> (*getParentIdx)(std::vector<T>&, int, int, std::unordered_map<std::string, float>&);
    std::vector<T> (*getChildren)(std::vector<T>&, std::vector<int>&, std::unordered_map<std::string, float>&);
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
    std::timed_mutex _populationGuard; // to ensure thread-safe modifications to population
    std::unordered_map<std::string, float> _parameters;
    GA_policy _policy; // tracks current algorithm state
    ProblemCtx<T> _problemCtx;
    // T _bestSolution;

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
        std::cout << "\tgenerateInitialPopulation: unimplemented\n";
        _population.clear();
        _population = _problemCtx.getRandomSolutions(_parameters["population size"], _parameters);
    }

    std::vector<int> getParents(int rangeStart, int rangeEnd)
    {
        std::vector<int> nilreturn = _problemCtx.getParentIdx(_population, rangeStart, rangeEnd, _parameters);
        return nilreturn;
    }

    std::vector<T> getChildren(std::vector<int> parentIdx)
    {
        return _problemCtx.getChildren(_population, parentIdx, _parameters);
    }

    void updatePopulation(std::vector<T> newPopulation)
    {
        THREADPRINT("\tupdatePopulation: unimplemented\n")
    }

    bool terminateSearch()
    {
        THREADPRINT("\tterminateSearch: unimplemented\n")
        return true;
    }

    void optimiseThread(int maxIter, int rangeStart, int rangeEnd, GA_policy* policy)
    {   // perform GA search on a subset of population
        // ie. _population[rangeStart:rangeEnd]
        int threadID = generateThreadID();
        THREADPRINT("thread " << threadID << " started handling " << rangeEnd-rangeStart << " solutions\n")
        int iter = 0;

        while(iter < maxIter)
        {
            iter += 1;

            // perform GA search
            std::vector<int> parentIdx = getParents(rangeStart, rangeEnd);
            std::vector<T> children = getChildren(parentIdx);
            updatePopulation(children);
            if(terminateSearch()) break;
        }
        THREADPRINT("thread " << threadID << " ended\n")
    }

    void optimise()
    {
        generateInitialPopulation();
        std::vector<std::thread> threadList;
        std::cout << "number of available processors = " << std::thread::hardware_concurrency() << '\n';
        int populationPerThread = _population.size() / _parameters["number of Threads"] + 1;
        for(int i=0; i<_parameters["number of Threads"]; i++)
        {
            threadList.push_back(std::thread(
                        &GA<T>::optimiseThread, 
                        this,
                         _parameters["max_iterations"], 
                         i * populationPerThread, 
                         std::min<int>((i+1) * populationPerThread, _population.size()), 
                         &_policy)); // starts a thread
        }
        
        for(int i=0; i<threadList.size(); i++) threadList[i].join(); // wait for all threads to complete
        std::cout << "all threads completed\n";
        
    }
};

#endif //INCLUDE_GA_CORE