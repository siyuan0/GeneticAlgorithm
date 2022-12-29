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
#include <shared_mutex>

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
    std::unordered_map<std::string, float> _parameters;
    GA_policy _policy; // tracks current algorithm state
    ProblemCtx<T> _problemCtx;
    bool _terminateFlag;
    int _numOptimiseThreads;
    std::shared_timed_mutex _populationGuard; // to ensure thread-safe modifications to population
    std::shared_timed_mutex _allThreadGuard; // to pause all threads (general purpose)
    std::shared_timed_mutex _printGuard; // to pause threads for printing
    std::unique_lock<std::shared_timed_mutex> _printLock{_printGuard, std::defer_lock};
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
        std::shared_lock<std::shared_timed_mutex> locallock{_populationGuard, std::defer_lock};
        locallock.lock();
        _problemCtx.updatePopulation(_population, children, sortedIdx, _parameters);
        locallock.unlock();
    }

    void printThread()
    {
        std::unique_lock populock{_populationGuard, std::defer_lock};
        std::unique_lock printlock{_printGuard, std::defer_lock};
        int checkpoint_print = _parameters["print every"];
        THREADPRINT("started printer thread\n")
        // perform printing to file
        while(_threadProgress.empty())
        {
            // wait for other threads to start
            THREADPRINT("printer thread: waiting...\n")
        } 
        THREADPRINT("printer thread: wait is over!\n")

        while(!_terminateFlag)
        {
            if(_threadProgress.empty() | (checkpoint_print > _parameters["max_iterations"]))
            {
                break;
            }
            for(auto it=_threadProgress.begin(); it!=_threadProgress.end(); it++)
            {
                int showidx = it->second;
                if(showidx >= checkpoint_print)
                {
                    populock.lock();
                    std::stringstream ss;
                    ss << "Results/iter" << _threadProgress.begin()->second << ".txt";
                    printToFile(ss.str());
                    populock.unlock();
                    checkpoint_print += _parameters["print every"];        
                }
                break;
            }
        }
        THREADPRINT("printer thread: no more threads\n")
    }

    void watcherThread()
    {   // thread that performs concurrent tasks alongside the optimisation threads
        int checkpoint_terminate = 0;
        int checkpoint_swapPopulation = 0;
        bool check;
        THREADPRINT("thread terminateSearch started\n")
        std::unique_lock populock{_populationGuard, std::defer_lock};
        std::unique_lock allThreadLock{_allThreadGuard, std::defer_lock};
        while(!_terminateFlag)
        {
            if(_threadProgress.empty()){
                _terminateFlag=true; 
                THREADPRINT("watcherThread: no more threads\n")
                break;
            }
            // perform terminateSearch
            check = true;
            for(auto it=_threadProgress.begin(); it!=_threadProgress.end(); it++) check = check && (it->second > checkpoint_terminate);
            if(check)
            {
                populock.lock();
                if(_problemCtx.endSearch(_population))
                {
                    _terminateFlag = true;
                    populock.unlock();
                    break;
                }
                populock.unlock();
                checkpoint_terminate += _parameters["check termination every"];
            }

            // perform population swapping
            if(_parameters["number of Threads"] > 1)
            {
                check = true;
                for(auto it=_threadProgress.begin(); it!=_threadProgress.end(); it++) check = check && (it->second > checkpoint_swapPopulation);
                if(check)
                {
                    allThreadLock.lock();
                    int idx1 = std::rand() % _population.size();
                    int idx2 = idx1 + 1;
                    // randomise idx2 until it is likely in a different subpopulation
                    // it is not definite because there is a corner case of threadcount=2 and idx1=populationsize/2 that fails without more 
                    // logic to catch that
                    while(std::abs(idx1 - idx2) < (_population.size() / _parameters["number of Threads"] - 1)) idx2 = std::rand() % _population.size();
                    // swap the two members of the population
                    T tmp = _population[idx1];
                    _population[idx1] = _population[idx2];
                    _population[idx2] = tmp;
                    allThreadLock.unlock();
                    checkpoint_swapPopulation += _parameters["swap population every"];
                }
            }
        }
    }

    void optimiseThread(int maxIter, int rangeStart, int rangeEnd, GA_policy* policy)
    {   // perform GA search on a subset of population
        // ie. _population[rangeStart:rangeEnd]
        int threadID = generateThreadID();
        int checkpoint_print = _parameters["print every"];
        _threadProgress.emplace(threadID, 0);
        _numOptimiseThreads += 1;
        std::shared_lock<std::shared_timed_mutex> threadlock(_allThreadGuard, std::defer_lock);
        // std::unique_lock<> printlock(_printGuard, std::defer_lock);
        THREADPRINT("thread " << threadID << " started handling " << rangeEnd-rangeStart << " solutions\n")

        while(_threadProgress[threadID] < maxIter)
        {
            _threadProgress[threadID] += 1;
            // if(_threadProgress[threadID] >= checkpoint_print)
            // {   // pause for printing
            //     if(!_printLock.owns_lock()) _printLock.lock();
            //     while(_printLock.owns_lock()) // do nothing
            //     checkpoint_print += _parameters["print every"];
            //     // printlock.lock();
            // }
            // THREADPRINT("thread " << threadID << "working on iter " << _threadProgress[threadID] << '\n')
            threadlock.lock();

            // perform GA search
            std::pair<std::vector<int>, std::vector<std::pair<float, int>>> parentIdxAndSortedArr = getParents(rangeStart, rangeEnd);
            std::vector<T> children = getChildren(parentIdxAndSortedArr.first);
            updatePopulation(children, parentIdxAndSortedArr.second);

            threadlock.unlock();
            if(_terminateFlag) break;
        }
        THREADPRINT("thread " << threadID << " ended after " << _threadProgress[threadID] << " iterations\n")
        _threadProgress.erase(threadID);
        _numOptimiseThreads -= 1;
        if(threadlock) threadlock.unlock();
        // if(printlock) printlock.unlock();
    }

    void optimise()
    {
        // initialise the multi-threading environment
        std::vector<std::thread> threadList;
        std::cout << "number of available processors = " << std::thread::hardware_concurrency() << '\n';
        int populationPerThread = _population.size() / _parameters["number of Threads"] + 1;
        _terminateFlag = false;
        // _printLock.lock(); // lock the printGuard so that threads will wait for watcher to take control of printGuard

        // starts printer thread 
        threadList.push_back(std::thread(&GA<T>::printThread, this));

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
        threadList.push_back(std::thread(&GA<T>::watcherThread, this));
        
        
        for(int i=0; i<threadList.size(); i++) threadList[i].join(); // wait for all threads to complete
        THREADPRINT("all threads completed\n")
    }
};

#endif //INCLUDE_GA_CORE