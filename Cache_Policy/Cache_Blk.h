#ifndef __CACHE_BLK_H__
#define __CACHE_BLK_H__

#define __STDC_FORMAT_MACROS
#include <inttypes.h> // uint64_t

#include <stdbool.h>

// saturating counter
typedef struct Sat_Counter
{
    unsigned counter_bits;
    uint8_t max_val;
    uint8_t counter;
}Sat_Counter;

typedef struct Cache_Block
{
    uint64_t tag;

    bool valid; // Is this block valid?
    bool dirty; // Has this block been modified?

    uint64_t when_touched; // The last time this block is referenced.
    uint64_t frequency; // How many times this block is referenced.

    uint32_t set; // Which set this block belongs to?
    uint32_t way; // Which way (within this set) belongs to?

    // Advanced Features
    uint64_t PC; // Which instruction that brings in this block?
    int core_id; // Which core the instruction is running on.
	
	// SHiP stuff
	unsigned sig;
	bool outcome;
	unsigned RRPV_idx;
	Sat_Counter RRPV;
	
}Cache_Block;

// Counter functions
void initSatCounter(Sat_Counter *sat_counter, unsigned counter_bits);
void incrementCounter(Sat_Counter *sat_counter);
void decrementCounter(Sat_Counter *sat_counter);
void setTwoCounter(Sat_Counter *sat_counter);
bool checkZero(Sat_Counter *sat_counter);
bool checkThree(Sat_Counter *sat_counter);

#endif
