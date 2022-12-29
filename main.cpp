#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <chrono>
#include "lib/core.hpp"
#include "third_party/nlohmann/json.hpp"
#include "Example/SchwefelFunction/problem.hpp"

int main(int argc, 
         char *argv[]) {
    if(argc<=1)
    {
        std::cout << "missing paramters.json file\n";
    }else if(argc==2)
    {
        std::ifstream f(argv[1]);
        nlohmann::json data = nlohmann::json::parse(f);
        auto jmap = data.get<std::unordered_map<std::string, float>>();

        GA<Schwefel::soln> GAinst(Schwefel::problemCtx, jmap);
        GAinst.generateInitialPopulation();
        GAinst.printToFile("populationInitial.txt");
        auto start = std::chrono::high_resolution_clock::now();
        GAinst.optimise();
        auto finish = std::chrono::high_resolution_clock::now();
        std::cout << "Optimisation took " << 
                std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count() << "ms\n";
        GAinst.printToFile("populationEnd.txt");
        std::cout << "number of function evaluations: " << Schwefel::num_of_evaluations << '\n';
    }else
    {
        std::cout << "too many arguments\n";
    }

    return 0;
}