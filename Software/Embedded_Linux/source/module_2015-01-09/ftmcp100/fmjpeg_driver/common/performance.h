
#ifdef EVALUATION_PERFORMANCE
#define INTERVAL	5
#define TIME_INTERVAL 1000000*INTERVAL
typedef struct {
	unsigned int start;
	unsigned int stop;
	unsigned int hw_start;
	unsigned int hw_stop;
	unsigned int ap_start;
	unsigned int ap_stop;
} FRAME_TIME;

typedef struct {
	unsigned int sw_total;
	unsigned int hw_total;
	unsigned int ap_total;
	unsigned int total;
	unsigned int count;
} TOTAL_TIME;

typedef unsigned long long uint64;
#endif


