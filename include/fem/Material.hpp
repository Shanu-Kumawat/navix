#pragma once
#include <string>
#include <cstdint>

namespace Core {
namespace FEM {

struct Material {
    uint32_t id;
    std::string name;
    double youngsModulus; // in Pa
    double poissonsRatio; // unitless
    double density;       // in kg/m^3
    double yieldStrength; // in Pa

    Material() 
        : id(0), name("Unknown"), youngsModulus(200e9), poissonsRatio(0.3), density(7850.0), yieldStrength(250e6) {}
        
    Material(uint32_t id, const std::string& name, double E, double nu, double rho, double Y)
        : id(id), name(name), youngsModulus(E), poissonsRatio(nu), density(rho), yieldStrength(Y) {}
};

} // namespace FEM
} // namespace Core
