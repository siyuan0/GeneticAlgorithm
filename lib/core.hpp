#include <vector>
#include <memory>
#include <ctime>
#include <cstdlib>
#include "solution.hpp"
#include <unordered_map>
#include <string>

class GA
{
private:
    std::shared_ptr<std::vector<solution>> _population;
    std::unordered_map<std::string, float> _parameters;
    float (*_evalSolution)(solution);
    solution (*_getRandomSolution)();
public:
    GA(float (*evalSolution)(solution),
       solution (*getRandomSolution)(),
       std::unordered_map<std::string, float> parameters)
    {
        std::srand(std::time(NULL)); // seed the random number generator using current time
        _evalSolution = evalSolution;
        _getRandomSolution = getRandomSolution;
        _parameters = parameters;
    };

    void generateInitialPopulation();

    void evalPopulation(std::shared_ptr<std::vector<solution>> population);

    void getParents();

    void getNewPopulation(); // breed and mutate

    void updatePopulation(); // using curr and new population

    void optimise(int max_iterations, int terminateEvalValue, std::shared_ptr<std::vector<solution>> population);
};