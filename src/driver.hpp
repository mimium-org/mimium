#pragma once
#include <string>
#include <memory>
#include "mimium_parser.hpp"
#include "scanner.hpp"

namespace mmmpsr{

    class MimiumDriver{
        public:
        MimiumDriver() = default;
        virtual ~MimiumDriver();
        void parse(std::string& str);
        private:
           std::unique_ptr<mmmpsr::MimiumParser>  parser  = nullptr;
           std::unique_ptr<mmmpsr::MimiumScanner> scanner = nullptr;
    };
};