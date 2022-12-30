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
#include <chrono>
#include <shared_mutex>

static int unique_id = 0;

int generateThreadID()
{
    unique_id += 1;
    return unique_id;
}

// problem specific methods (to be defined for each optimisation problem)
template <typename T>
struct ProblemCtx
{   
    std::vector<T> (*getRandomSolutions)(int, std::unordered_map<std::string, float>&);
    std::pair<std::vector<int>, std::vector<std::pair<float, int>>> 
        (*getParentIdx)(std::vector<T>&, int, int, std::unordered_map<std::string, float>&);
    std::vector<T> (*getChildren)(std::vector<T>&, std::vector<int>&, std::unordered_map<std::string, float>&);
    void (*updatePopulation)(std::vector<T>&, std::vector<T>&, std::vector<std::pair<float, int>>, std::unordered_map<std::string, float>&);
    bool (*endSearch)(std::vector<T>);
};

// a struct to allow change of GA runtime hyperparameters
struct GA_policy
{};

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
        _policy = {};
        _terminateFlag = false;
        _numOptimiseThreads = 0;
    };

    void print()
    {   // print curr population to iostream
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

    std::vector<T>* getPopulation()
    {   // returns a pointer to the population
        return &_population;
    }

    void generateInitialPopulation()
    {   // use the problem specific population initialisation method
        _population.clear();
        _population = _problemCtx.getRandomSolutions(_parameters["population size"], _parameters);
    }

    std::pair<std::vector<int>, std::vector<std::pair<float, int>>>  getParents(int rangeStart, int rangeEnd)
    {   // use the problem specific population parent selection method
        auto idxAndSortedArr  = _problemCtx.getParentIdx(_population, rangeStart, rangeEnd, _parameters);
        return idxAndSortedArr;
    }

    std::vector<T> getChildren(std::vector<int>& parentIdx)
    {   // use the problem specific breeding+mutation method
        return _problemCtx.getChildren(_population, parentIdx, _parameters);
    }

    void updatePopulation(std::vector<T>& children, std::vector<std::pair<float, int>>& sortedIdx)
    {   // use the problem specific population update method
        std::shared_lock<std::shared_timed_mutex> locallock{_populationGuard, std::defer_lock};
        locallock.lock();
        _problemCtx.updatePopulation(_population, children, sortedIdx, _parameters);
        locallock.unlock();
    }

    void printThread() // perform printing to file
    {
        std::unique_lock populock{_populationGuard, std::defer_lock};
        std::unique_lock printlock{_printGuard, std::defer_lock};
        int checkpoint_print = _parameters["print every"];
        THREADPRINT("started printer thread\n")
        
        while(_threadProgress.empty())
        {   // wait for other threads to start
            /* since _threadProgress isn't protected by a mutex, we take advantage of the 
               the iostream mutex in THREADPRINT to introduce some mutex waiting in
               this while loop, so that we don't inspect _threadProgress while another thread
               is still modifying _threadProgress, which causes a bug where the printerThread 
               is stuck here indefinitely */
            THREADPRINT("")
        } 

        while(!_terminateFlag)
        {
            if(_threadProgress.empty() | (checkpoint_print > _parameters["max_iterations"])) break;
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
        THREADPRINT("printer thread: ended\n")
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
            if(_threadProgress.empty()){ // we can end all threads now
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
        _threadProgress.emplace(threadID, 0);
        _numOptimiseThreads += 1;
        std::shared_lock<std::shared_timed_mutex> threadlock(_allThreadGuard, std::defer_lock);
        THREADPRINT("thread " << threadID << " started handling " << rangeEnd-rangeStart << " solutions\n")

        while(_threadProgress[threadID] < maxIter)
        {
            _threadProgress[threadID] += 1;
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
    }

    void optimise()
    {
        // initialise the multi-threading environment
        std::vector<std::thread> threadList;
        std::cout << "number of available processors = " << std::thread::hardware_concurrency() << '\n';
        int populationPerThread = _population.size() / _parameters["number of Threads"] + 1;
        _terminateFlag = false;

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