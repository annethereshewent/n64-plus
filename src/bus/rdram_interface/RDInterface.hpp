#include <cstdint>
#include "Config.hpp"
#include "RDSelect.hpp"
#include "Mode.hpp"

class RDInterface {
public:
    RDSelect select;
    Config config;
    Mode mode;

    bool init = false;

    uint32_t currentLoad = 0;
};