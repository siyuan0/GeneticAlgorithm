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
#include <fstream>
#include <atomic>

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
    std::pair<std::vector<int>, std::vector<std::pair<float, int>>> 
        (*getParentIdx)(std::vector<T>&, int, int, std::unordered_map<std::string, float>&);
    std::vector<T> (*getChildren)(std::vector<T>&, std::vector<int>&, std::unordered_map<std::string, float>&);
    void (*updatePopulation)(std::vector<T>&, std::vector<T>&, std::vector<std::pair<float, int>>, std::unordered_map<std::string, float>&);
    bool (*endSearch)(std::vector<T>);
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
    std::mutex _populationGuard; // to ensure thread-safe modifications to population
    std::unordered_map<std::string, float> _parameters;
    GA_policy _policy; // tracks current algorithm state
    ProblemCtx<T> _problemCtx;
    bool _terminateFlag;
    int _numOptimiseThreads;
    std::unordered_map<int, std::atomic<int>> _threadProgress;

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
        _terminateFlag = false;
        _numOptimiseThreads = 0;
    };

    void print()
    {   // print curr population
        for(int i=0; i<_population.size(); i++) std::cout << _population[i] << '\n';
    }

    void printToFile(const std::string fileName)
    {   // save the current population to fileName in the format for each line: x1, x2,... xn, f
        std::ofstream outfile;
        outfile.open(fileName, std::ios::out|std::ios::trunc);
        for(T s : _population)
        {
            outfile << s << '\n';
        }
        outfile.close();
    }

    void generateInitialPopulation()
    {
        _population.clear();
        _population = _problemCtx.getRandomSolutions(_parameters["population size"], _parameters);
    }

    std::pair<std::vector<int>, std::vector<std::pair<float, int>>>  getParents(int rangeStart, int rangeEnd)
    {
        auto idxAndSortedArr  = _problemCtx.getParentIdx(_population, rangeStart, rangeEnd, _parameters);
        return idxAndSortedArr;
    }

    std::vector<T> getChildren(std::vector<int>& parentIdx)
    {
        return _problemCtx.getChildren(_population, parentIdx, _parameters);
    }

    void updatePopulation(std::vector<T>& children, std::vector<std::pair<float, int>>& sortedIdx)
    {
        std::unique_lock locallock{_populationGuard, std::defer_lock};
        locallock.lock();
        _problemCtx.updatePopulation(_population, children, sortedIdx, _parameters);
        locallock.unlock();
    }

    void terminateSearchThread()
    {
        int checkpoint = 10;
        bool check = false;
        THREADPRINT("thread terminateSearch started\n")
        std::unique_lock locallock{_populationGuard, std::defer_lock};
        while(!_terminateFlag)
        {
            if(_threadProgress.empty()){
                THREADPRINT("terminateSearch: no more threads\n")
                _terminateFlag=true; 
                break;
            }
            check = true;
            // THREADPRINT("progress: " << _threadProgress[1] << '\n')
            for(auto it=_threadProgress.begin(); it!=_threadProgress.end(); it++)
            {
                check = check && (it->second > checkpoint);
            }
            if(check)
            {
                THREADPRINT("checking for termination\n")
                locallock.lock();
                if(_problemCtx.endSearch(_population))
                {
                    _terminateFlag = true;
                    locallock.unlock();
                    break;
                }
                locallock.unlock();
                checkpoint += _parameters["check termination every"];
            }
        }
    }

    void optimiseThread(int maxIter, int rangeStart, int rangeEnd, GA_policy* policy)
    {   // perform GA search on a subset of population
        // ie. _population[rangeStart:rangeEnd]
        int threadID = generateThreadID();
        _threadProgress.emplace(threadID, 0);
        _numOptimiseThreads += 1;
        THREADPRINT("thread " << threadID << " started handling " << rangeEnd-rangeStart << " solutions\n")

        while(_threadProgress[threadID] < maxIter)
        {
            _threadProgress[threadID] += 1;

            // perform GA search
            std::pair<std::vector<int>, std::vector<std::pair<float, int>>> parentIdxAndSortedArr = getParents(rangeStart, rangeEnd);
            std::vector<T> children = getChildren(parentIdxAndSortedArr.first);
            if(_terminateFlag) break;
            updatePopulation(children, parentIdxAndSortedArr.second);
        }
        THREADPRINT("thread " << threadID << " ended after " << _threadProgress[threadID] << " iterations\n")
        _threadProgress.erase(threadID);
        _numOptimiseThreads -= 1;
    }

    void optimise()
    {
        generateInitialPopulation();
        std::vector<std::thread> threadList;
        std::cout << "number of available processors = " << std::thread::hardware_concurrency() << '\n';
        int populationPerThread = _population.size() / _parameters["number of Threads"] + 1;
        _terminateFlag = false;

        // create the optimisation threads
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
        // starts terminate checker thread
        threadList.push_back(std::thread(&GA<T>::terminateSearchThread, this)); 
        
        for(int i=0; i<threadList.size(); i++) threadList[i].join(); // wait for all threads to complete
        THREADPRINT("all threads completed\n")
    }
};

#endif //INCLUDE_GA_CORE