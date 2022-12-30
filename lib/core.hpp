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
#include <filesystem>

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
    void (*setRandomGenerator)(std::mt19937);
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
    std::vector<T> _sharedPool;
    std::unordered_map<std::string, float> _parameters;
    GA_policy _policy; // tracks current algorithm state
    ProblemCtx<T> _problemCtx;
    std::shared_timed_mutex _populationGuard; // to ensure thread-safe modifications to population
    std::shared_timed_mutex _sharedPoolGaurd; // to ensure thread-safe mod to _sharedPool
    std::vector<std::vector<T>> _printerQueue;

public:
    GA(ProblemCtx<T> problemCtx,
       std::unordered_map<std::string, float> parameters)
    {
        std::srand(std::time(NULL)); // seed the random number generator using current time
        _problemCtx = problemCtx;
        _parameters = parameters;
        _policy = {};
    };

    void print()
    {   // print curr population to iostream
        for(int i=0; i<_population.size(); i++) std::cout << _population[i] << '\n';
    }

    void printToFile(const std::vector<T>& population, const std::string fileName)
    {   // save the current population to fileName in the format for each line: x1, x2,... xn, f
        std::ofstream outfile;
        outfile.open(fileName, std::ios::out|std::ios::trunc);
        for(T s : population)
        {
            outfile << s << '\n';
        }
        outfile.close();
    }

    void printToFile(const std::string fileName)
    {   // save the current population to fileName in the format for each line: x1, x2,... xn, f
        printToFile(_population, fileName);
    }

    void sendToPrinterQueue(std::vector<T>& localPopulation, int rangeStart, int rangeEnd, int printIdx)
    {   // updates the _printQueue with this thread's localPopulation
        if((printIdx > _printerQueue.size()-1) | _printerQueue.empty())
        {
            _printerQueue.push_back(std::vector<T>(_population.size()));
        }
        int localCounter = 0;
        for(int i=rangeStart; i<rangeEnd; i++)
        {
            _printerQueue[printIdx][i] = localPopulation[localCounter];
            localCounter += 1;
        }
    }

    void clearPrinterQueue()
    {   // print everything in _printerQueue
        for(int i=0; i<_printerQueue.size(); i++)
        {
            std::stringstream ss;
            ss << "Results/iter" << (i+1)*_parameters["print every"] << ".txt";
            printToFile(_printerQueue[i], ss.str());
        }
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

    std::pair<std::vector<int>, std::vector<std::pair<float, int>>>  getParents(
        std::vector<T>& population,
        ProblemCtx<T>& problemCtx, 
        std::unordered_map<std::string, float>& parameters)
    {   // use the problem specific population parent selection method
        auto idxAndSortedArr  = problemCtx.getParentIdx(population, 0, population.size(), parameters);
        return idxAndSortedArr;
    }

    std::vector<T> getChildren(
        std::vector<int>& parentIdx, 
        std::vector<T>& population, 
        ProblemCtx<T>& ProblemCtx,
        std::unordered_map<std::string, float>& parameters)
    {   // use the problem specific breeding+mutation method
        return ProblemCtx.getChildren(population, parentIdx, parameters);
    }

    void updatePopulation(std::vector<T>& children, std::vector<std::pair<float, int>>& sortedIdx)
    {   // use the problem specific population update method
        std::shared_lock<std::shared_timed_mutex> locallock{_populationGuard, std::defer_lock};
        locallock.lock();
        _problemCtx.updatePopulation(_population, children, sortedIdx, _parameters);
        locallock.unlock();
    }

    void updateLocalPopulation(
        std::vector<T>& population, 
        std::vector<T>& children, 
        std::vector<std::pair<float, int>>& sortedIdx,
        ProblemCtx<T>& ProblemCtx,
        std::unordered_map<std::string, float>& parameters)
    {   // use the problem specific population update method
        ProblemCtx.updatePopulation(population, children, sortedIdx, parameters);
    }

    void optimiseThread(int maxIter, int rangeStart, int rangeEnd)
    {   // perform GA search on a subset of population
        // ie. _population[rangeStart:rangeEnd]
        int threadID = generateThreadID();
        int progressCounter = 0;
        int printIdx = 0;

        // create a copy of managed population locally managed by this thread
        std::vector<T> localPopulation;
        for(int i=rangeStart; i<rangeEnd; i++) localPopulation.push_back(_population[i]);

        // create a copy of resources for this thread
        auto parameters = _parameters;
        auto problemCtx = _problemCtx;
        std::mt19937 randomGenerator;
        randomGenerator.seed(std::time(NULL));
        problemCtx.setRandomGenerator(randomGenerator);

        std::unique_lock<std::shared_timed_mutex> sharedPoolLock{_sharedPoolGaurd, std::defer_lock};
        std::shared_lock<std::shared_timed_mutex> popuLock{_populationGuard, std::defer_lock};
        auto start = std::chrono::high_resolution_clock::now();
        THREADPRINT("--thread " << threadID << " started handling " << rangeEnd-rangeStart << " solutions\n")

        while(progressCounter < maxIter)
        {
            progressCounter += 1;

            // perform GA search
            std::pair<std::vector<int>, std::vector<std::pair<float, int>>> 
                parentIdxAndSortedArr = getParents(localPopulation, problemCtx, parameters);
            std::vector<T> children = getChildren(parentIdxAndSortedArr.first, localPopulation, problemCtx, parameters);
            updateLocalPopulation(localPopulation, children, parentIdxAndSortedArr.second, problemCtx, parameters);

            // send candidates to sharedPool for inter-thread swapping
            if(progressCounter % static_cast<int>(parameters["swap population every"]) == 0)
            {
                int outgoing = intRand(0, localPopulation.size()-1, randomGenerator);
                sharedPoolLock.lock();
                if(_sharedPool.empty())
                {
                    _sharedPool.push_back(localPopulation[outgoing]);
                }else
                {
                    int incoming = intRand(0, _sharedPool.size()-1, randomGenerator);
                    auto tmp = localPopulation[outgoing];
                    localPopulation[outgoing] = _sharedPool[incoming];
                    _sharedPool[incoming] = tmp;
                }
                sharedPoolLock.unlock();
            }

            // printing
            if(progressCounter % static_cast<int>(parameters["print every"]) == 0)
            {
                sendToPrinterQueue(localPopulation, rangeStart, rangeEnd, printIdx);
                printIdx += 1;
            }
        }
        auto finish = std::chrono::high_resolution_clock::now();
        THREADPRINT("--thread " << threadID << " ended after " << progressCounter << " iterations, taking "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count() << "ms\n")

        // update the copy of population shared across threads
        popuLock.lock();
        int localCounter=0;
        for(int i=rangeStart; i<rangeEnd; i++)
        {
            _population[i] = localPopulation[localCounter];
            localCounter++;
        }
        popuLock.unlock();

        // unlock all locks in case any are still locked
        if(popuLock) popuLock.unlock();
        if(sharedPoolLock) sharedPoolLock.unlock();
    }

    void optimise()
    {
        // initialise the multi-threading environment
        std::vector<std::thread> threadList;
        std::cout << "--number of available processors = " << std::thread::hardware_concurrency() << '\n';
        int populationPerThread = _population.size() / _parameters["number of Threads"] + 1;

        // create the optimisation threads
        for(int i=0; i<_parameters["number of Threads"]; i++)
        {
            threadList.push_back(std::thread(
                        &GA<T>::optimiseThread, 
                        this,
                         _parameters["max_iterations"], 
                         i * populationPerThread, 
                         std::min<int>((i+1) * populationPerThread, _population.size()))); // starts a thread
        }
        
        for(int i=0; i<threadList.size(); i++) threadList[i].join(); // wait for all threads to complete
        THREADPRINT("--all threads completed, printing results...\n")
        clearPrinterQueue();
        THREADPRINT("--intermediate results saved to " << std::filesystem::current_path().string() << "/Results/\n")
    }
};

#endif //INCLUDE_GA_CORE