#include "../include/Errors.hpp"


std::string_view error_to_string(Error err) {
    switch (err) {
    case Error::InvalidInput:       return "Некоректний ввід.";
    case Error::InvalidGenerator:   return "Порядок alpha < p - 1.";
    case Error::NoRelationsFound:   return "Не вдалося зібрати достатньо гладких співвідношень.";
    case Error::SystemUnsolvable:   return "Система лінійних рівнянь не має розв'язку.";
    case Error::LogarithmNotFound:  return "Дискретний логарифм не знайдено.";
    default:                        return "Невідома помилка.";
    }
}