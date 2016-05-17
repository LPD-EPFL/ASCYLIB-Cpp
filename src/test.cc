#include<assert.h>
#include<getopt.h>
#include<pthread.h>
#include"search.h"
#include"common.h"
#include"utils.h"
#include"rapl_read.h"
#include"ssmem.h"
#include"array_map_optik.h"
#include"linkedlist_lazy.h"
//#include"linkedlist_lazy_cache.h"
#include"linkedlist_copy.h"
#include"linkedlist_coupling.h"
#include"linkedlist_harris.h"
#include"linkedlist_harris_opt.h"
#include"linkedlist_optik.h"
#include"linkedlist_optik_gl.h"
#include"linkedlist_pugh.h"
#include"linkedlist_seq.h"
#include"hashtable_harris.h"
#include"hashtable_copy.h"
#include"hashtable_java.h"
#include"hashtable_optik.h"
#include"hashtable_optik_gl.h"
#include"hashtable_optik_arraymap.h"
#include"hashtable_pugh.h"
#include"skiplist_fraser.h"
#include"skiplist_herlihy_lb.h"
#include"skiplist_herlihy_lf.h"
#include"skiplist_optik.h"
#include"skiplist_seq.h"

#define ASSERT_SIZE 1

// When adding here, also add in the help section
// and argument processing
// Line 366
enum algorithms {
	AM_OPTIK,
	LL_LAZY, LL_COPY, LL_COUPLING, LL_HARRIS, LL_HARRIS_OPT,
	LL_OPTIK, LL_OPTIK_GL, LL_PUGH, LL_SEQ,
	HT_HARRIS, HT_COPY, HT_JAVA, HT_OPTIK, HT_OPTIK_GL, HT_OPTIK_AM,
	HT_PUGH,
	SL_FRASER, SL_HERLIHY_LB, SL_HERLIHY_LF, SL_OPTIK, SL_SEQ
};
algorithms algorithm;

__thread ssmem_allocator_t* alloc;

RETRY_STATS_VARS_GLOBAL;

size_t array_ll_fixed_size = DEFAULT_RANGE;
unsigned int maxhtlength, maxbulength;
unsigned int levelmax;

size_t initial = DEFAULT_INITIAL;
size_t concurrency = CHM_NUM_SEGMENTS;
size_t range = DEFAULT_RANGE;
size_t load_factor = DEFAULT_LOAD;
size_t update = DEFAULT_UPDATE;
size_t num_threads = DEFAULT_NB_THREADS;
size_t duration = DEFAULT_DURATION;
int test_verbose = 0;
int workload = 0;

size_t print_vals_num = 100;
size_t pf_vals_num = 1023;
size_t put, put_explicit = false;
double update_rate, put_rate, get_rate;

size_t size_after = 0;
int seed = 0;
__thread unsigned long *seeds;
uint32_t rand_max;
#define rand_min 1
//#define sval_t int
//#define skey_t int
#define sval_t int*
#define skey_t int

static volatile int stop;
volatile int phase_put = 0;
volatile uint32_t phase_put_threshold_start = 0.99999 * UINT_MAX;
volatile uint32_t phase_put_threshold_stop  = 0.9999999 * UINT_MAX;
__thread volatile ticks phase_start, phase_stop;
ZIPF_RAND_DECLARATIONS();

volatile ticks *putting_succ;
volatile ticks *putting_fail;
volatile ticks *getting_succ;
volatile ticks *getting_fail;
volatile ticks *removing_succ;
volatile ticks *removing_fail;
volatile ticks *putting_count;
volatile ticks *putting_count_succ;
volatile ticks *getting_count;
volatile ticks *getting_count_succ;
volatile ticks *removing_count;
volatile ticks *removing_count_succ;
volatile ticks *total;

/* ################################################################### *
* LOCALS
* ################################################################### */

#ifdef DEBUG
extern __thread uint32_t put_num_restarts;
extern __thread uint32_t put_num_failed_expand;
extern __thread uint32_t put_num_failed_on_new;
#endif

barrier_t barrier, barrier_global;

typedef struct thread_data
{
	uint32_t id;
	Search<skey_t, sval_t> *set;
	//Search<intptr_t, intptr_t> *set;
	//Search<int *, int *> *set;
	//DS_TYPE* set;
} thread_data_t;

enum algorithms parse_algorithm(char *algorithm_string)
{
	printf("Using %s\n", algorithm_string);
	if (!strncmp(optarg,"LL_COPY",8)) {
		return LL_COPY;
	} else if (!strncmp(optarg,"LL_COUPLING",12)) {
		return LL_COUPLING;
	} else if (!strncmp(optarg,"LL_HARRIS_OPT",14)) {
		return LL_HARRIS_OPT;
	} else if (!strncmp(optarg,"LL_HARRIS",10)) {
		return LL_HARRIS;
	} else if (!strncmp(optarg,"LL_OPTIK_GL",12)) {
		return LL_OPTIK_GL;
	} else if (!strncmp(optarg,"LL_OPTIK",9)) {
		return LL_OPTIK;
	} else if (!strncmp(optarg,"LL_PUGH",8)) {
		return LL_PUGH;
	} else if (!strncmp(optarg,"LL_SEQ",7)) {
		return LL_SEQ;
	} else if (!strncmp(optarg,"HT_HARRIS",10)) {
		return HT_HARRIS;
	} else if (!strncmp(optarg,"HT_COPY",8)) {
		return HT_COPY;
	} else if (!strncmp(optarg,"HT_JAVA",8)) {
		return HT_JAVA;
	} else if (!strncmp(optarg,"HT_OPTIK_GL",12)) {
		return HT_OPTIK_GL;
	} else if (!strncmp(optarg,"HT_OPTIK",9)) {
		return HT_OPTIK;
	} else if (!strncmp(optarg,"AM_OPTIK",9)) {
		return AM_OPTIK;
	} else if (!strncmp(optarg,"HT_OPTIK_AM",12)) {
		return HT_OPTIK_AM;
	} else if (!strncmp(optarg,"HT_PUGH",8)) {
		return HT_PUGH;
	} else if (!strncmp(optarg,"SL_FRASER",10)) {
		return SL_FRASER;
	} else if (!strncmp(optarg,"SL_HERLIHY_LB",14)) {
		return SL_HERLIHY_LB;
	} else if (!strncmp(optarg,"SL_HERLIHY_LF",14)) {
		return SL_HERLIHY_LF;
	} else if (!strncmp(optarg,"SL_OPTIK",9)) {
		return SL_OPTIK;
	} else if (!strncmp(optarg,"SL_SEQ",7)) {
		return SL_SEQ;
	}
	return LL_LAZY;
}

void normal_workload_test_loop(Search<skey_t,sval_t> *set,
			uint64_t &my_putting_count,
			uint64_t &my_putting_count_succ,
			uint64_t &my_getting_count,
			uint64_t &my_getting_count_succ,
			uint64_t &my_removing_count,
			uint64_t &my_removing_count_succ)
{
	uint64_t key;
	uint32_t scale_rem = (uint32_t) (update_rate * UINT_MAX);
	uint32_t scale_put = (uint32_t) (put_rate * UINT_MAX);
	unsigned int c = 0;
	//sval_t val = 0;
	int number = 15;
	sval_t val = &number;
	while (stop == 0) {
		c = (uint32_t)(my_random(&(seeds[0]),&(seeds[1]),&(seeds[2])));
		key = (c & rand_max) + rand_min;

		if (unlikely(c <= scale_put)) {
			int res;
			START_TS(1);
			res = set->insert(key,val);
			if (res) {
				END_TS(1, my_putting_count_succ);
				ADD_DUR(my_putting_succ);
				my_putting_count_succ++;
			}
			END_TS_ELSE(4, my_putting_count - my_putting_count_succ,
				my_putting_fail);
			my_putting_count++;
		} else if (unlikely(c <= scale_rem)) {
			sval_t removed;
			START_TS(2);
			removed = set->remove(key);
			if (removed != 0) {
				END_TS(2, my_removing_count_succ);
				ADD_DUR(my_removing_succ);
				my_removing_count_succ++;
			}
			END_TS_ELSE(5, my_removing_count - my_removing_count_succ,
				my_removing_fail);
			my_removing_count++;
		} else {
			sval_t res;
			START_TS(0);
			res = (sval_t) set->search(key);
			if (res != 0) {
				END_TS(0, my_getting_count_succ);
				ADD_DUR(my_getting_succ);
				my_getting_count_succ++;
			}
			END_TS_ELSE(3, my_getting_count - my_getting_count_succ,
				my_getting_fail);
			my_getting_count++;
		}
	}
}

void skewed_workload_test_loop(Search<skey_t,sval_t> *set,
			uint64_t &my_putting_count,
			uint64_t &my_putting_count_succ,
			uint64_t &my_getting_count,
			uint64_t &my_getting_count_succ,
			uint64_t &my_removing_count,
			uint64_t &my_removing_count_succ)
{
	uint64_t key;
	uint32_t scale_rem = (uint32_t) (update_rate * UINT_MAX);
	uint32_t scale_put = (uint32_t) (put_rate * UINT_MAX);
	unsigned int c = 0;
	int number = 15;
	sval_t val = &number;
	while (stop == 0) {
		c = (uint32_t)(my_random(&(seeds[0]),&(seeds[1]),&(seeds[2])));
		key = rand_max - zipf_get_next(__zipf_arr) + rand_min;
		ZIPF_STATS_DO(__zipf_arr->stats[key]++);

		if (unlikely(c <= scale_put)) {
			int res;
			START_TS(1);
			res = set->insert(key,val);
			if (res) {
				END_TS(1, my_putting_count_succ);
				ADD_DUR(my_putting_succ);
				my_putting_count_succ++;
			}
			END_TS_ELSE(4, my_putting_count - my_putting_count_succ,
				my_putting_fail);
			my_putting_count++;
		} else if (unlikely(c <= scale_rem)) {
			sval_t removed;
			START_TS(2);
			removed = set->remove(key);
			if(removed != 0) {
				END_TS(2, my_removing_count_succ);
				ADD_DUR(my_removing_succ);
				my_removing_count_succ++;
			}
			END_TS_ELSE(5, my_removing_count - my_removing_count_succ,
				my_removing_fail);
			my_removing_count++;
		} else {
			sval_t res;
			START_TS(0);
			res = (sval_t) set->search(key);
			if(res != 0) {
				END_TS(0, my_getting_count_succ);
				ADD_DUR(my_getting_succ);
				my_getting_count_succ++;
			}
			END_TS_ELSE(3, my_getting_count - my_getting_count_succ,
			my_getting_fail);
			my_getting_count++;
		}
	}
}

void* test(void* thread)
{
	thread_data_t* td = (thread_data_t*) thread;
	uint32_t ID = td->id;
	set_cpu(ID);
	//ssalloc_init();

	Search<skey_t,sval_t>* set = td->set;

	//THREAD_INIT(ID);  expanded below -----v
	if (workload == 2) {
		if (!ID) {
			printf("- Creating zipf random numbers array\n");
		}
		__zipf_arr = zipf_get_rand_array(ZIPF_ALPHA, 0,
				rand_max + 1, ID);
		barrier_cross(&barrier);
		if (!ID) {
			printf("- Done\n");
		}
	}
	PF_INIT(3, SSPFD_NUM_ENTRIES, ID);

#if defined(COMPUTE_LATENCY)
	volatile ticks my_putting_succ = 0;
	volatile ticks my_putting_fail = 0;
	volatile ticks my_getting_succ = 0;
	volatile ticks my_getting_fail = 0;
	volatile ticks my_removing_succ = 0;
	volatile ticks my_removing_fail = 0;
#endif
	uint64_t my_putting_count = 0;
	uint64_t my_getting_count = 0;
	uint64_t my_removing_count = 0;

	uint64_t my_putting_count_succ = 0;
	uint64_t my_getting_count_succ = 0;
	uint64_t my_removing_count_succ = 0;

#if defined(COMPUTE_LATENCY) && PFD_TYPE == 0
	volatile ticks start_acq, end_acq;
	volatile ticks correction = getticks_correction_calc();
#endif

	seeds = seed_rand();

#if defined(HTICKET)
	init_thread_htlocks(the_cores[d->id]);
#elif defined(CLH)
	init_clh_thread(&clh_local_p);
#endif

#if GC == 1
	alloc = (ssmem_allocator_t*) malloc(sizeof(ssmem_allocator_t));
	assert(alloc != NULL);
	ssmem_alloc_init_fs_size(alloc, SSMEM_DEFAULT_MEM_SIZE, SSMEM_GC_FREE_SET_SIZE, ID);
#endif
	barrier_cross(&barrier);

	uint64_t key;

	uint32_t i;
	uint32_t num_elems_thread = (uint32_t) (initial / num_threads);
	uint32_t missing = initial % num_threads; //(uint32_t) initial - (num_elems_thread * num_threads);
	if (ID < missing) {
		num_elems_thread++;
	}

	/* do not initialize from one for llcopy */
	/* otherwise GC does not work properly */
	/* #if INITIALIZE_FROM_ONE == 1 */
	/*   num_elems_thread = (ID == 0) * initial; */
	/*   key = range; */
	/* #endif */

	int one = 1;
	for(i = 0; i < num_elems_thread; i++) {
		key = (my_random(&(seeds[0]), &(seeds[1]), &(seeds[2]))
			% (rand_max + 1)) + rand_min;
		//if(set->insert(key, NULL) == false) {
		//if(set->insert(key, 1) == false) {
		if(set->insert(key, &one) == false) {
			i--;
		} else if (test_verbose && ID == 0 && (i & 1023) == 0) {
			size_t ni = i * num_threads;
			double perc = 100.0 * ni / initial;
			printf(" #inserted = %-10zu ~%.2f%%\n", ni, perc);
		}
	}
	MEM_BARRIER;
	barrier_cross(&barrier);

	if (!ID) {
		printf("#BEFORE size is: %zu\n", (size_t) set->length());
	}

	RETRY_STATS_ZERO();

	barrier_cross(&barrier_global);

	RR_START_SIMPLE();

	if (workload == 2) {
		skewed_workload_test_loop(set,
			my_putting_count, my_putting_count_succ,
			my_getting_count, my_getting_count_succ,
			my_removing_count, my_removing_count_succ);
	} else {
		normal_workload_test_loop(set,
			my_putting_count, my_putting_count_succ,
			my_getting_count, my_getting_count_succ,
			my_removing_count, my_removing_count_succ);
	}

	barrier_cross(&barrier);
	RR_STOP_SIMPLE();

	if (!ID) {
		size_after = set->length();
		printf("#AFTER  size is: %zu\n", size_after);
	}

	barrier_cross(&barrier);

#if defined(COMPUTE_LATENCY)
	putting_succ[ID] += my_putting_succ;
	putting_fail[ID] += my_putting_fail;
	getting_succ[ID] += my_getting_succ;
	getting_fail[ID] += my_getting_fail;
	removing_succ[ID] += my_removing_succ;
	removing_fail[ID] += my_removing_fail;
#endif
	putting_count[ID] += my_putting_count;
	getting_count[ID] += my_getting_count;
	removing_count[ID]+= my_removing_count;

	putting_count_succ[ID] += my_putting_count_succ;
	getting_count_succ[ID] += my_getting_count_succ;
	removing_count_succ[ID]+= my_removing_count_succ;

	EXEC_IN_DEC_ID_ORDER(ID, num_threads)
	{
		print_latency_stats(ID, SSPFD_NUM_ENTRIES, print_vals_num);
	}
	EXEC_IN_DEC_ID_ORDER_END(&barrier);

	SSPFDTERM();
#if GC == 1
	ssmem_term();
	free(alloc);
#endif
	if (workload == 2) {
		ZIPF_STATS_DO(
			if (id == 0) {
				zipf_print_stats(__zipf_arr);
			}
			free(__zipf_arr->stats);
		);
		free(__zipf_arr);
	}
	//THREAD_END();
	pthread_exit(NULL);
}

int floor_log_2(unsigned int n)
{
	int pos = 0;
	if (n >= 1<<16) { n >>= 16; pos += 16; }
	if (n >= 1<< 8) { n >>=  8; pos +=  8; }
	if (n >= 1<< 4) { n >>=  4; pos +=  4; }
	if (n >= 1<< 2) { n >>=  2; pos +=  2; }
	if (n >= 1<< 1) {           pos +=  1; }
	return ((n == 0) ? (-1) : pos);
}

int main(int argc, char**argv)
{
#if defined(CLH)
	init_clh_thread(&clh_local_p);
#endif
	set_cpu(0);
	//ssalloc_init();
	seeds = seed_rand();
	algorithm = LL_LAZY;

	RR_INIT_ALL();

	struct option long_options[] = {
		{"help",                      no_argument,       NULL, 'h'},
		{"verbose",                   no_argument,       NULL, 'e'},
		{"duration",                  required_argument, NULL, 'd'},
		{"initial-size",              required_argument, NULL, 'i'},
		{"num-threads",               required_argument, NULL, 'n'},
		{"range",                     required_argument, NULL, 'r'},
		{"update-rate",               required_argument, NULL, 'u'},
		{"num-buckets",               required_argument, NULL, 'b'},
		{"print-vals",                required_argument, NULL, 'v'},
		{"vals-pf",                   required_argument, NULL, 'f'},
		{"load-factor",               required_argument, NULL, 'l'},
		{"concurrency",               required_argument, NULL, 'c'},
		{"workflow",                  required_argument, NULL, 'w'},
		{"algorithm",                 required_argument, NULL, 'a'},
		{NULL, 0, NULL, 0}
	};

	int i,c;
	while(1) {
		i=0;
		c = getopt_long(argc, argv, "hAf:d:i:n:r:s:u:m:el:p:w:b:v:f:c:x:a:",
			long_options, &i);
		if (c==-1) {
			break;
		}
		if (c==0 && long_options[i].flag == 0) {
			c = long_options[i].val;
		}

		switch(c) {
		case 0:
			break;
		case 'h':
		printf("ASCYLIB -- stress test "
			"\n"
			"\n"
			"Usage:\n"
			"  %s [options...]\n"
			"\n"
			"Options:\n"
			"  -h, --help\n"
			"        Print this message\n"
			"  -e, --verbose\n"
			"        Be verbose\n"
			"  -d, --duration <int>\n"
			"        Test duration in milliseconds\n"
			"  -i, --initial-size <int>\n"
			"        Number of elements to insert before test\n"
			"  -n, --num-threads <int>\n"
			"        Number of threads\n"
			"  -r, --range <int>\n"
			"        Range of integer values inserted in set\n"
			"  -u, --update-rate <int>\n"
			"        Percentage of update transactions\n"
			"  -c, --concurrency <int>\n"
			"        Concurrency level (for the Java hash table)\n"
			"  -l, --load-factor <int>\n"
			"        Elements per bucket (only available for hash tables)\n"
			"  -p, --put-rate <int>\n"
			"        Percentage of put update transactions (should be less than percentage of updates)\n"
			"  -b, --num-buckets <int>\n"
			"        Number of initial buckets (stronger than -l)\n"
			"  -v, --print-vals <int>\n"
			"        When using detailed profiling, how many values to print.\n"
			"  -f, --val-pf <int>\n"
			"        When using detailed profiling, how many values to keep track of.\n"
			"  -w, --workload <int>\n"
			"        0  -- normal workload(default)\n"
			"        2  -- skewed workload\n"
			"  -a, --algorithm <name>\n"
			"        What algorithm to use (LL_LAZY by default).\n"
			"        Possible options:\n"
			"        AM_OPTIK,\n"
			"        LL_LAZY, LL_COPY, LL_COUPLING, LL_HARRIS, LL_HARRIS_OPT\n"
			"        LL_OPTIK, LL_OPTIK_GL, LL_PUGH, LL_SEQ\n"
			"        HT_HARRIS, HT_COPY, HT_JAVA, HT_OPTIK, HT_OPTIK_GL\n"
			"        HT_OPTIK_AM, HT_PUGH\n"
			"        SL_FRASER, SL_HERLIHY_LB, SL_HERLIHY_LF, SL_OPTIK, SL_SEQ\n"
			"\n"
			, argv[0]);
			exit(0);
		case 'd':
			duration = atoi(optarg);
			break;
		case 'e':
			test_verbose = 1;
			break;
		case 'i':
			initial = atoi(optarg);
			break;
		case 'c':
			concurrency = pow2roundup(atoi(optarg));
			break;
		case 'n':
			num_threads = atoi(optarg);
			break;
		case 'r':
			range = atol(optarg);
			break;
		case 'u':
			update = atoi(optarg);
			break;
		case 'p':
			put_explicit = 1;
			put = atoi(optarg);
			break;
		case 'l':
			load_factor = atoi(optarg);
			break;
		case 'v':
			print_vals_num = atoi(optarg);
			break;
		case 'f':
			pf_vals_num = pow2roundup(atoi(optarg)) - 1;
			break;
		case 'w':
			workload = atoi(optarg);
			break;
		case 'a':
			algorithm = parse_algorithm(optarg);
			break;
		case '?':
		default:
			printf("Unknown option %c. Use -h or --help for help.\n",c);
			exit(1);
		}
	}

	if (!is_power_of_two(initial)) {
		size_t initial_pow2 = pow2roundup(initial);
		printf("** rounding up initial (to make it power of 2): old %zu / new: %zu\n", initial, initial_pow2);
	}
	if (range < initial) {
		range = 2 * initial;
	}
	printf("## Initial: %zu / Range: %zu / Load factor (for HTs): %zu / Concurrency (for HTs): %zu\n", initial, range, load_factor, concurrency);

	// TODO this calculation is NOT correct
	//double kb = initial * 16/*sizeof(DS_NODE)*/ / 1024.0;
	//double mb = kb / 1024.0;
	//printf("Sizeof initial: %.2f kB = %.2f MB\n", kb, mb);

	if (!is_power_of_two(range)) {
		size_t range_pow2 = pow2roundup(range);
		printf("** rounding up range (to make it power of 2): old: %zu / new: %zu\n", range, range_pow2);
	}

	array_ll_fixed_size = range;

	if (put > update) {
		put = update;
	}
	update_rate = update / 100.0;
	if (put_explicit) {
		put_rate = put / 100.0;
	} else {
		put_rate = update_rate / 2;
	}
	get_rate = 1 - update_rate;

	rand_max = range - 1;

	struct timeval start,end;
	struct timespec timeout;
	timeout.tv_sec = duration / 1000;
	timeout.tv_nsec = (duration % 1000) * 1000000;

	stop = 0;

	levelmax = floor_log_2((unsigned int) initial);
	maxhtlength = (unsigned int) initial / load_factor;
	maxbulength = ((unsigned int) range / initial) * load_factor;
	size_t capacity = initial / load_factor;

	Search<skey_t, sval_t> *set;
	if (algorithm == LL_COPY) {
		set = new LinkedListCopy<skey_t,sval_t>();
	} else if (algorithm == LL_COUPLING) {
		set = new LinkedListCoupling<skey_t,sval_t>();
	} else if (algorithm == LL_HARRIS) {
		set = new LinkedListHarris<skey_t, sval_t>();
	} else if (algorithm == LL_HARRIS_OPT) {
		set = new LinkedListHarrisOpt<skey_t, sval_t>();
	} else if (algorithm == LL_OPTIK) {
		set = new LinkedListOptik<skey_t, sval_t>();
	} else if (algorithm == LL_OPTIK_GL) {
		set = new LinkedListOptikGL<skey_t, sval_t>();
	} else if (algorithm == LL_PUGH) {
		set = new LinkedListPugh<skey_t, sval_t>();
	} else if (algorithm == LL_SEQ) {
		set = new LinkedListSeq<skey_t, sval_t>();
	} else if (algorithm == HT_HARRIS) {
		set = new HashtableHarris<skey_t, sval_t>(maxhtlength);
	} else if (algorithm == HT_COPY) {
		set = new HashtableCopy<skey_t, sval_t>(maxhtlength);
	} else if (algorithm == HT_JAVA) {
		set = new HashtableJavaCHM<skey_t, sval_t>(capacity, concurrency);
	} else if (algorithm == HT_OPTIK) {
		set = new HashtableOptik<skey_t, sval_t>(maxhtlength);
	} else if (algorithm == HT_OPTIK_GL) {
		set = new HashtableOptikGL<skey_t, sval_t>(maxhtlength);
	} else if (algorithm == AM_OPTIK) {
		set = new ArrayMapOptik<skey_t, sval_t>(initial);
	} else if (algorithm == HT_OPTIK_AM) {
		set = new HashtableOptikArrayMap<skey_t, sval_t>
			(maxhtlength, maxbulength);
	} else if (algorithm == HT_PUGH) {
		set = new HashtablePugh<skey_t, sval_t>(maxhtlength);
	} else if (algorithm == SL_FRASER) {
		set = new SkiplistFraser<skey_t, sval_t>(levelmax);
	} else if (algorithm == SL_HERLIHY_LB) {
		set = new SkiplistHerlihyLB<skey_t, sval_t>(levelmax);
	} else if (algorithm == SL_HERLIHY_LF) {
		set = new SkiplistHerlihyLF<skey_t, sval_t>(levelmax);
	} else if (algorithm == SL_OPTIK) {
		set = new SkiplistOptik<skey_t, sval_t>(levelmax);
	} else if (algorithm == SL_SEQ) {
		set = new SkiplistSeq<skey_t, sval_t>(levelmax);
	} else {
		set = new LinkedListLazy<skey_t,sval_t>();
	}
		//new LinkedListLazyCache<skey_t,sval_t>();
	assert( set != NULL );

	/* Initializes the local data */
	putting_succ = (ticks *) calloc(num_threads , sizeof(ticks));
	putting_fail = (ticks *) calloc(num_threads , sizeof(ticks));
	getting_succ = (ticks *) calloc(num_threads , sizeof(ticks));
	getting_fail = (ticks *) calloc(num_threads , sizeof(ticks));
	removing_succ = (ticks *) calloc(num_threads , sizeof(ticks));
	removing_fail = (ticks *) calloc(num_threads , sizeof(ticks));
	putting_count = (ticks *) calloc(num_threads , sizeof(ticks));
	putting_count_succ = (ticks *) calloc(num_threads , sizeof(ticks));
	getting_count = (ticks *) calloc(num_threads , sizeof(ticks));
	getting_count_succ = (ticks *) calloc(num_threads , sizeof(ticks));
	removing_count = (ticks *) calloc(num_threads , sizeof(ticks));
	removing_count_succ = (ticks *) calloc(num_threads , sizeof(ticks));

	pthread_t threads[num_threads];
	pthread_attr_t attr;
	int rc;
	void *status;

	barrier_init(&barrier_global, num_threads + 1);
	barrier_init(&barrier, num_threads);

	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	thread_data_t* tds = (thread_data_t*) malloc(num_threads * sizeof(thread_data_t));

	unsigned long t;
	for(t = 0; t < num_threads; t++) {
		tds[t].id = t;
		tds[t].set = set;
		rc = pthread_create(&threads[t], &attr, test, tds + t);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}

	}

	/* Free attribute and wait for the other threads */
	pthread_attr_destroy(&attr);

	barrier_cross(&barrier_global);
	gettimeofday(&start, NULL);

	RR_START_UNPROTECTED_ALL();
	nanosleep(&timeout, NULL);
	RR_STOP_UNPROTECTED_ALL();

	stop = 1;

	gettimeofday(&end, NULL);
	duration = (end.tv_sec*1000 + end.tv_usec/1000) - (start.tv_sec*1000+start.tv_usec/1000);
	for (t=0; t < num_threads; t++) {
		rc = pthread_join(threads[t], &status);
		if (rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
	}

	free(tds);

	volatile ticks putting_suc_total = 0;
	volatile ticks putting_fal_total = 0;
	volatile ticks getting_suc_total = 0;
	volatile ticks getting_fal_total = 0;
	volatile ticks removing_suc_total = 0;
	volatile ticks removing_fal_total = 0;
	volatile uint64_t putting_count_total = 0;
	volatile uint64_t putting_count_total_succ = 0;
	volatile uint64_t getting_count_total = 0;
	volatile uint64_t getting_count_total_succ = 0;
	volatile uint64_t removing_count_total = 0;
	volatile uint64_t removing_count_total_succ = 0;

	for(t=0; t < num_threads; t++)
	{
		if (test_verbose)
		{
			printf("Thrd: %3lu : srch: %10zu (%10zu) "
				"/ insr: %10zu (%10zu) / rems: %10zu (%10zu)\n",
				t, getting_count[t], getting_count_succ[t],
				putting_count[t], putting_count_succ[t],
				removing_count[t], removing_count_succ[t]);
		}
		PRINT_OPS_PER_THREAD();
		putting_suc_total += putting_succ[t];
		putting_fal_total += putting_fail[t];
		getting_suc_total += getting_succ[t];
		getting_fal_total += getting_fail[t];
		removing_suc_total += removing_succ[t];
		removing_fal_total += removing_fail[t];
		putting_count_total += putting_count[t];
		putting_count_total_succ += putting_count_succ[t];
		getting_count_total += getting_count[t];
		getting_count_total_succ += getting_count_succ[t];
		removing_count_total += removing_count[t];
		removing_count_total_succ += removing_count_succ[t];
	}
#if defined(COMPUTE_LATENCY)
	printf("#thread srch_suc srch_fal insr_suc insr_fal remv_suc remv_fal "
			"  ## latency (in cycles) \n");
	fflush(stdout);
	long unsigned get_suc = (getting_count_total_succ) ?
		getting_suc_total / getting_count_total_succ : 0;
	long unsigned get_fal = (getting_count_total - getting_count_total_succ) ?
		getting_fal_total / (getting_count_total - getting_count_total_succ) : 0;
	long unsigned put_suc = putting_count_total_succ ?
		putting_suc_total / putting_count_total_succ : 0;
	long unsigned put_fal = (putting_count_total - putting_count_total_succ) ?
		putting_fal_total / (putting_count_total - putting_count_total_succ) : 0;
	long unsigned rem_suc = removing_count_total_succ ?
		removing_suc_total / removing_count_total_succ : 0;
	long unsigned rem_fal = (removing_count_total - removing_count_total_succ) ?
		removing_fal_total / (removing_count_total - removing_count_total_succ) : 0;
	printf("%-7zu %-8lu %-8lu %-8lu %-8lu %-8lu %-8lu\n", num_threads,
			get_suc, get_fal, put_suc, put_fal, rem_suc, rem_fal);
#endif

#define LLU long long unsigned int

	int UNUSED pr = (int) (putting_count_total_succ - removing_count_total_succ);
	if (size_after != (initial + pr))
	{
		printf("// WRONG size. %zu + %d != %zu\n", initial, pr, size_after);
#if ASSERT_SIZE == 1
		assert(size_after == (initial + pr));
#endif
	}

	printf("    : %-10s | %-10s | %-11s | %-11s | %s\n", "total", "success", "succ %", "total %", "effective %");
	uint64_t total = putting_count_total + getting_count_total + removing_count_total;
	double putting_perc = 100.0 * (1 - ((double)(total - putting_count_total) / total));
	double putting_perc_succ = (1 - (double) (putting_count_total - putting_count_total_succ) / putting_count_total) * 100;
	double getting_perc = 100.0 * (1 - ((double)(total - getting_count_total) / total));
	double getting_perc_succ = (1 - (double) (getting_count_total - getting_count_total_succ) / getting_count_total) * 100;
	double removing_perc = 100.0 * (1 - ((double)(total - removing_count_total) / total));
	double removing_perc_succ = (1 - (double) (removing_count_total - removing_count_total_succ) / removing_count_total) * 100;

	printf("srch: %-10llu | %-10llu | %10.1f%% | %10.1f%% | \n", (LLU) getting_count_total,
	(LLU) getting_count_total_succ,  getting_perc_succ, getting_perc);
	printf("insr: %-10llu | %-10llu | %10.1f%% | %10.1f%% | %10.1f%%\n", (LLU) putting_count_total,
	(LLU) putting_count_total_succ, putting_perc_succ, putting_perc, (putting_perc * putting_perc_succ) / 100);
	printf("rems: %-10llu | %-10llu | %10.1f%% | %10.1f%% | %10.1f%%\n", (LLU) removing_count_total,
	(LLU) removing_count_total_succ, removing_perc_succ, removing_perc, (removing_perc * removing_perc_succ) / 100);

	double throughput = (putting_count_total + getting_count_total + removing_count_total) * 1000.0 / duration;
	printf("#txs %zu\t(%-10.0f\n", num_threads, throughput);
	printf("#Mops %.3f\n", throughput / 1e6);

	RR_PRINT_UNPROTECTED(RAPL_PRINT_POW);
	RR_PRINT_CORRECTED();
	RETRY_STATS_PRINT(total, putting_count_total, removing_count_total, putting_count_total_succ + removing_count_total_succ);

	pthread_exit(NULL);

	return 0;
}
