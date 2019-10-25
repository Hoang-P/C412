#include "Branch_Predictor.h"

const unsigned instShiftAmt = 2; // Number of bits to shift a PC by

// You can play around with these settings.
const unsigned localPredictorSize = 65536;
const unsigned localCounterBits = 2;
const unsigned localHistoryTableSize = 8192; 
const unsigned globalPredictorSize = 16384;
const unsigned globalCounterBits = 2;
const unsigned choicePredictorSize = 16384; // Keep this the same as globalPredictorSize.
const unsigned choiceCounterBits = 2;
const unsigned gshareCounterBits = 2; //Do not change this
const unsigned gsharePredictorSize = 65536;
const float theta = 1.93*n + 14;

Branch_Predictor *initBranchPredictor()
{
    Branch_Predictor *branch_predictor = (Branch_Predictor *)malloc(sizeof(Branch_Predictor));

    #ifdef TWO_BIT_LOCAL
    branch_predictor->local_predictor_sets = localPredictorSize;
    assert(checkPowerofTwo(branch_predictor->local_predictor_sets));

    branch_predictor->index_mask = branch_predictor->local_predictor_sets - 1;

    // Initialize sat counters
    branch_predictor->local_counters =
        (Sat_Counter *)malloc(branch_predictor->local_predictor_sets * sizeof(Sat_Counter));

    int i = 0;
    for (i; i < branch_predictor->local_predictor_sets; i++)
    {
        initSatCounter(&(branch_predictor->local_counters[i]), localCounterBits);
    }
    #endif
	
	#ifdef TOURNAMENT
    assert(checkPowerofTwo(localPredictorSize));
    assert(checkPowerofTwo(localHistoryTableSize));
    assert(checkPowerofTwo(globalPredictorSize));
    assert(checkPowerofTwo(choicePredictorSize));
    assert(globalPredictorSize == choicePredictorSize);

    branch_predictor->local_predictor_size = localPredictorSize;
    branch_predictor->local_history_table_size = localHistoryTableSize;
    branch_predictor->global_predictor_size = globalPredictorSize;
    branch_predictor->choice_predictor_size = choicePredictorSize;
   
    // Initialize local counters 
    branch_predictor->local_counters =
        (Sat_Counter *)malloc(localPredictorSize * sizeof(Sat_Counter));

    int i = 0;
    for (i; i < localPredictorSize; i++)
    {
        initSatCounter(&(branch_predictor->local_counters[i]), localCounterBits);
    }

    branch_predictor->local_predictor_mask = localPredictorSize - 1;

    // Initialize local history table
    branch_predictor->local_history_table = 
        (unsigned *)malloc(localHistoryTableSize * sizeof(unsigned));

    for (i = 0; i < localHistoryTableSize; i++)
    {
        branch_predictor->local_history_table[i] = 0;
    }

    branch_predictor->local_history_table_mask = localHistoryTableSize - 1;

    // Initialize global counters
    branch_predictor->global_counters = 
        (Sat_Counter *)malloc(globalPredictorSize * sizeof(Sat_Counter));

    for (i = 0; i < globalPredictorSize; i++)
    {
        initSatCounter(&(branch_predictor->global_counters[i]), globalCounterBits);
    }

    branch_predictor->global_history_mask = globalPredictorSize - 1;

    // Initialize choice counters
    branch_predictor->choice_counters = 
        (Sat_Counter *)malloc(choicePredictorSize * sizeof(Sat_Counter));

    for (i = 0; i < choicePredictorSize; i++)
    {
        initSatCounter(&(branch_predictor->choice_counters[i]), choiceCounterBits);
    }

    branch_predictor->choice_history_mask = choicePredictorSize - 1;

    // global history register
    branch_predictor->global_history = 0;

    // We assume choice predictor size is always equal to global predictor size.
    branch_predictor->history_register_mask = choicePredictorSize - 1;
    #endif

	#ifdef GSHARE
    assert(checkPowerofTwo(gsharePredictorSize));
	
    branch_predictor->gshare_counters = 
        (Sat_Counter *)malloc(gsharePredictorSize * sizeof(Sat_Counter));
	
	int i = 0;
    for (i; i < gsharePredictorSize; i++)
    {
        initSatCounter(&(branch_predictor->gshare_counters[i]), gshareCounterBits);
    }

    branch_predictor->global_history_mask = gsharePredictorSize - 1;
    
	branch_predictor->global_history = 0;
    #endif

	#ifdef perceptron
	
	assert(checkPowerofTwo(p_size));
	branch_predictor->p_mask = p_size - 1;

	int i;
	int j;

	for (i = 0;i < n;i++) {
		branch_predictor->global_history[i] = 0;
	}

	for (i = 0;i < p_size; i++) {
		for (j = 0;j < n;j++) {
			branch_predictor->P[i][j] = 0;
		}
	}

	#endif

    return branch_predictor;
}

// sat counter functions
inline void initSatCounter(Sat_Counter *sat_counter, unsigned counter_bits)
{
    sat_counter->counter_bits = counter_bits;
    sat_counter->counter = 0;
    sat_counter->max_val = (1 << counter_bits) - 1;
}

inline void incrementCounter(Sat_Counter *sat_counter)
{
    if (sat_counter->counter < sat_counter->max_val)
    {
        ++sat_counter->counter;
    }
}

inline void decrementCounter(Sat_Counter *sat_counter)
{
    if (sat_counter->counter > 0) 
    {
        --sat_counter->counter;
    }
}

// Branch Predictor functions
bool predict(Branch_Predictor *branch_predictor, Instruction *instr)
{
    uint64_t branch_address = instr->PC;

    #ifdef TWO_BIT_LOCAL    
    // Step one, get prediction
    unsigned local_index = getIndex(branch_address, 
                                    branch_predictor->index_mask);

    bool prediction = getPrediction(&(branch_predictor->local_counters[local_index]));

    // Step two, update counter
    if (instr->taken)
    {
        // printf("Correct: %u -> ", branch_predictor->local_counters[local_index].counter);
        incrementCounter(&(branch_predictor->local_counters[local_index]));
        // printf("%u\n", branch_predictor->local_counters[local_index].counter);
    }
    else
    {
        // printf("Incorrect: %u -> ", branch_predictor->local_counters[local_index].counter);
        decrementCounter(&(branch_predictor->local_counters[local_index]));
        // printf("%u\n", branch_predictor->local_counters[local_index].counter);
    }

    return prediction == instr->taken;
    #endif

    #ifdef TOURNAMENT
    // Step one, get local prediction.
    unsigned local_history_table_idx = getIndex(branch_address,
                                           branch_predictor->local_history_table_mask);
    
    unsigned local_predictor_idx = 
        branch_predictor->local_history_table[local_history_table_idx] & 
        branch_predictor->local_predictor_mask;

    bool local_prediction = 
        getPrediction(&(branch_predictor->local_counters[local_predictor_idx]));

    // Step two, get global prediction.
    unsigned global_predictor_idx = 
        branch_predictor->global_history & branch_predictor->global_history_mask;

    bool global_prediction = 
        getPrediction(&(branch_predictor->global_counters[global_predictor_idx]));

    // Step three, get choice prediction.
    unsigned choice_predictor_idx = 
        branch_predictor->global_history & branch_predictor->choice_history_mask;

    bool choice_prediction = 
        getPrediction(&(branch_predictor->choice_counters[choice_predictor_idx]));


    // Step four, final prediction.
    bool final_prediction;
    if (choice_prediction)
    {
        final_prediction = global_prediction;
    }
    else
    {
        final_prediction = local_prediction;
    }

    bool prediction_correct = final_prediction == instr->taken;
    // Step five, update counters
    if (local_prediction != global_prediction)
    {
        if (local_prediction == instr->taken)
        {
            // Should be more favorable towards local predictor.
            decrementCounter(&(branch_predictor->choice_counters[choice_predictor_idx]));
        }
        else if (global_prediction == instr->taken)
        {
            // Should be more favorable towards global predictor.
            incrementCounter(&(branch_predictor->choice_counters[choice_predictor_idx]));
        }
    }

    if (instr->taken)
    {
        incrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));
        incrementCounter(&(branch_predictor->local_counters[local_predictor_idx]));
    }
    else
    {
        decrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));
        decrementCounter(&(branch_predictor->local_counters[local_predictor_idx]));
    }

    // Step six, update global history register
    branch_predictor->global_history = branch_predictor->global_history << 1 | instr->taken;
    // exit(0);
    //
    return prediction_correct;
    #endif

	#ifdef GSHARE
	unsigned branch_idx = branch_address & branch_predictor->global_history_mask;
	unsigned gh_idx = branch_predictor->global_history & branch_predictor->global_history_mask;
	unsigned xor_bit = branch_idx ^ gh_idx;
	
	bool xor_prediction = getPrediction(&(branch_predictor->gshare_counters[xor_bit]));
	bool prediction_correct = xor_prediction == instr->taken;
	
	if (instr->taken)
    {
        incrementCounter(&(branch_predictor->gshare_counters[xor_bit]));
    }
    else
    {
        decrementCounter(&(branch_predictor->gshare_counters[xor_bit]));
    }
	// update global history register
	branch_predictor->global_history = branch_predictor->global_history << 1 | instr->taken;
	return prediction_correct;
	#endif

	#ifdef perceptron
	int i;
	bool res;
	bool prediction_correct;
	int sign;
	unsigned hash;
	hash = getIndex(branch_address, branch_predictor->p_mask);

	float y = branch_predictor->P[hash][0];
	for (i = 1; i < n; i++) {
		y += branch_predictor->P[hash][i]*branch_predictor->global_history[i];
	}

	if (y < 0) {
		res = false;
	}
	else {
		res = true;
	}

	if (instr->taken) {
		sign = 1;
	}
	else {
		sign = -1;
	}
	
	prediction_correct = res == instr->taken;

	if (!prediction_correct || (fabs(y) <= theta)) {
		branch_predictor->P[hash][0] += sign;
		for (i = 0; i < n; i++) {
			branch_predictor->P[hash][i] = branch_predictor->P[hash][i] + sign*branch_predictor->global_history[i];
		}
	}

	for (i = n-1; i >= 0; i--) {
		branch_predictor->global_history[i+1] = branch_predictor->global_history[i];
	}

	if (instr->taken) {
		branch_predictor->global_history[0] = 1;
	}
	else {
		branch_predictor->global_history[0] = -1;
	}

	return prediction_correct;
	
	#endif
}

inline unsigned getIndex(uint64_t branch_addr, unsigned index_mask)
{
    return (branch_addr >> instShiftAmt) & index_mask;
}

inline bool getPrediction(Sat_Counter *sat_counter)
{
    uint8_t counter = sat_counter->counter;
    unsigned counter_bits = sat_counter->counter_bits;

    // MSB determins the direction
    return (counter >> (counter_bits - 1));
}

int checkPowerofTwo(unsigned x)
{
    //checks whether a number is zero or not
    if (x == 0)
    {
        return 0;
    }

    //true till x is not equal to 1
    while( x != 1)
    {
        //checks whether a number is divisible by 2
        if(x % 2 != 0)
        {
            return 0;
        }
        x /= 2;
    }
    return 1;
}
