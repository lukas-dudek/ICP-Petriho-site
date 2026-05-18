// variable.h

#ifndef PETS_VARIABLE_H
#define PETS_VARIABLE_H

#include <string>

namespace pets {

// @brief Reprezentuje interní proměnnou interpretované Petriho sítě.
// 
// Proměnné mohou být libovolného typu podporovaného inskripčním jazykem
// (standardně C/C++). Hodnota je uložena jako string a přetypovává se
// při použití.
class Variable {
public:
    
    std::string type;          ///< Typ proměnné (např. "int", "float", "const char*")
    std::string name;          ///< Jméno proměnné
    std::string initial_value; ///< Inicializační výraz (např. "5000", "0.0")

    
    std::string current_value; ///< Aktuální hodnota (uložená jako string)

    
    Variable() = default;
    Variable(std::string type_, std::string name_, std::string init_val = {})
        : type(std::move(type_))
        , name(std::move(name_))
        , initial_value(std::move(init_val))
        , current_value(initial_value)
    {}

    
    void reset() { current_value = initial_value; }
};

} // namespace pets

#endif // PETS_VARIABLE_H