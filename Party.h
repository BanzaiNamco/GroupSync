#ifndef PARTY_H
#define PARTY_H

#include <cstdint>

struct Party {
    uint32_t partyId;
    Party(uint32_t id) : partyId(id) {}
};

#endif