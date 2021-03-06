/*
 *  Multi2Sim
 *  Copyright (C) 2011  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <cpuarch.h>



/*
 * Global variables
 */


/* Help message */

char *cpu_config_help =
	"The CPU configuration file is a plain text file with the IniFile format, defining\n"
	"the parameters of the CPU model used for a detailed (architectural) simulation.\n"
	"This configuration file is passed to Multi2Sim with option '--cpu-config <file>,\n"
	"which must be accompanied by option '--cpu-sim detailed'.\n"
	"\n"
	"The following is a list of the sections allowed in the CPU configuration file,\n"
	"along with the list of variables for each section.\n"
	"\n"
	"Section '[ General ]':\n"
	"\n"
	"  Cores = <num_cores> (Default = 1)\n"
	"      Number of cores.\n"
	"  Threads = <num_threads> (Default = 1)\n"
	"      Number of hardware threads per core. The total number of computing nodes\n"
	"      in the CPU model is equals to Cores * Threads.\n"
	"  FastForward = <num_inst> (Default = 0)\n"
	"      Number of x86 instructions to run with a fast functional simulation before\n"
	"      the architectural simulation starts.\n"
	"  ContextSwitch = {t|f} (Default = t)\n"
	"      Allow context switches in computing nodes. If this option is set to false,\n"
	"      the maximum number of contexts that can be run is limited by the number of\n"
	"      computing nodes; if a contexts spawns a child that cannot be allocated to\n"
	"      a free hardware thread, the simulation will stop with an error message.\n"
	"  ContextQuantum = <cycles> (Default = 100k)\n"
	"      If ContextSwitch is true, maximum number of cycles that a context can occupy\n"
	"      a CPU hardware thread before it is replaced by other pending context.\n"
	"  ThreadQuantum = <cycles> (Default = 1k)\n"
	"      For multithreaded processors (Threads > 1) configured as coarse-grain multi-\n"
	"      threading (FetchKind = SwitchOnEvent), number of cycles in which instructions\n"
	"      are fetched from the same thread before switching.\n"
	"  ThreadSwitchPenalty = <cycles> (Default = 0)\n"
	"      For coarse-grain multithreaded processors (FetchKind = SwitchOnEvent), number\n"
	"      of cycles that the fetch stage stalls after a thread switch.\n"
	"  RecoverKind = {Writeback|Commit} (Default = Writeback)\n"
	"      On branch misprediction, stage in the execution of the mispredicted branch\n"
	"      when processor recovery is triggered.\n"
	"  RecoverPenalty = <cycles> (Default = 0)\n"
	"      Number of cycles that the fetch stage gets stalled after a branch\n"
	"      misprediction.\n"
	"  PageSize = <size> (Default = 4kB)\n"
	"      Memory page size in bytes.\n"
	"  DataCachePerfect = {t|f} (Default = False)\n"
	"  InstructionCachePerfect = {t|f} (Default = False)\n"
	"      Set these options to true to simulate a perfect data/instruction caches,\n"
	"      respectively, where every access results in a hit. If set to false, the\n"
	"      parameters of the caches are given in the cache system configuration file\n"
	"      (option --cpu-cache-config <file>).\n"
	"\n"
	"Section '[ Pipeline ]':\n"
	"\n"
	"  FetchKind = {Shared|TimeSlice|SwitchOnEvent} (Default = TimeSlice)\n"
	"      Policy for fetching instruction from different threads. A shared fetch stage\n"
	"      fetches instructions from different threads in the same cycle; a time-slice\n"
	"      fetch switches between threads in a round-robin fashion; option SwitchOnEvent\n"
	"      switches thread fetch on long-latency operations or thread quantum expiration.\n"
	"  DecodeWidth = <num_inst> (Default = 4)\n"
	"      Number of x86 instructions decoded per cycle.\n"
	"  DipatchKind = {Shared|TimeSlice} (Default = TimeSlice)\n"
	"      Policy for dispatching instructions from different threads. If shared,\n"
	"      instructions from different threads are dispatched in the same cycle. Otherwise,\n"
	"      instruction dispatching is done in a round-robin fashion at a cycle granularity.\n"
	"  DispatchWidth = <num_inst> (Default = 4)\n"
	"      Number of microinstructions dispatched per cycle.\n"
	"  IssueKind = {Shared|TimeSlice} (Default = TimeSlice)\n"
	"      Policy for issuing instructions from different threads. If shared, instructions\n"
	"      from different threads are issued in the same cycle; otherwise, instruction issue\n"
	"      is done round-robin at a cycle granularity.\n"
	"  IssueWidth = <num_inst> (Default = 4)\n"
	"      Number of microinstructions issued per cycle.\n"
	"  CommitKind = {Shared|TimeSlice} (Default = Shared)\n"
	"      Policy for committing instructions from different threads. If shared,\n"
	"      instructions from different threads are committed in the same cycle; otherwise,\n"
	"      they commit in a round-robin fashion.\n"
	"  CommitWidth = <num_inst> (Default = 4)\n"
	"      Number of microinstructions committed per cycle.\n"
	"  OccupancyStats = {t|f} (Default = False)\n"
	"      Calculate structures occupancy statistics. Since this computation requires\n"
	"      additional overhead, the option needs to be enabled explicitly. These statistics\n"
	"      will be attached to the CPU report.\n"
	"\n"
	"Section '[ Queues ]':\n"
	"\n"
	"  FetchQueueSize = <bytes> (Default = 64)\n"
	"      Size of the fetch queue given in bytes.\n"
	"  UopQueueSize = <num_uops> (Default = 32)\n"
	"      Size of the uop queue size, given in number of uops.\n"
	"  RobKind = {Private|Shared} (Default = Private)\n"
	"      Reorder buffer sharing among hardware threads.\n"
	"  RobSize = <num_uops> (Default = 64)\n"
	"      Reorder buffer size in number of microinstructions (if private, per-thread size).\n"
	"  IqKind = {Private|Shared} (Default = Private)\n"
	"      Instruction queue sharing among threads.\n"
	"  IqSize = <num_uops> (Default = 40)\n"
	"      Instruction queue size in number of uops (if private, per-thread IQ size).\n"
	"  LsqKind = {Private|Shared} (Default = 20)\n"
	"      Load-store queue sharing among threads.\n"
	"  LsqSize = <num_uops> (Default = 20)\n"
	"      Load-store queue size in number of uops (if private, per-thread LSQ size).\n"
	"  RfKind = {Private|Shared} (Default = Private)\n"
	"      Register file sharing among threads.\n"
	"  RfIntSize = <entries> (Default = 80)\n"
	"      Number of integer physical register (if private, per-thread).\n"
	"  RfFpSize = <entries> (Default = 40)\n"
	"      Number of floating-point physical registers (if private, per-thread).\n"
	"\n"
	"Section '[ TraceCache ]':\n"
	"\n"
	"  Present = {t|f} (Default = False)\n"
	"      If true, a trace cache is included in the model. If false, the rest of the\n"
	"      options in this section are ignored.\n"
	"  Sets = <num_sets> (Default = 64)\n"
	"      Number of sets in the trace cache.\n"
	"  Assoc = <num_ways> (Default = 4)\n"
	"      Associativity of the trace cache. The product Sets * Assoc is the total\n"
	"      number of traces that can be stored in the trace cache.\n"
	"  TraceSize = <num_uops> (Default = 16)\n"
	"      Maximum size of a trace of uops.\n"
	"  BranchMax = <num_branches> (Default = 3)\n"
	"      Maximum number of branches contained in a trace.\n"
	"  QueueSize = <num_uops> (Default = 32)\n"
	"      Size of the trace queue size in uops.\n"
	"\n"
	"Section '[ FunctionalUnits ]':\n"
	"\n"
	"  The possible variables in this section follow the format\n"
	"      <func_unit>.<field> = <value>\n"
	"  where <func_unit> refers to a functional unit type, and <field> refers to a\n"
	"  property of it. Possible values for <func_unit> are:\n"
	"      IntAdd      Integer adder\n"
	"      IntMult     Integer multiplier\n"
	"      IntDiv      Integer divider\n"
	"      EffAddr     Operator for effective address computations\n"
	"      Logic       Operator for logic operations\n"
	"      FpSimple    Simple floating-point operations\n"
	"      FpAdd       Floating-point adder\n"
	"      FpMult      Floating-point multiplier\n"
	"      FpDiv       Floating-point divider\n"
	"      FpComplex   Operator for complex floating-point computations\n"
	"  Possible values for <field> are:\n"
	"      Count       Number of functional units of a given kind.\n"
	"      OpLat       Latency of the operator.\n"
	"      IssueLat    Latency since an instruction was issued until the functional\n"
	"                  unit is available for the next use. For pipelined operators,\n"
	"                  IssueLat is smaller than OpLat.\n"
	"\n"
	"Section '[ BranchPredictor ]':\n"
	"\n"
	"  Kind = {Perfect|Taken|NotTaken|Bimodal|TwoLevel|Combined} (Default = TwoLevel)\n"
	"      Branch predictor type.\n"
	"  BTB.Sets = <num_sets> (Default = 256)\n"
	"      Number of sets in the BTB.\n"
	"  BTB.Assoc = <num_ways) (Default = 4)\n"
	"      BTB associativity.\n"
	"  Bimod.Size = <entries> (Default = 1024)\n"
	"      Number of entries of the bimodal branch predictor.\n"
	"  Choice.Size = <entries> (Default = 1024)\n"
	"      Number of entries for the choice predictor.\n"
	"  RAS.Size = <entries> (Default = 32)\n"
	"      Number of entries of the return address stack (RAS).\n"
	"  TwoLevel.L1Size = <entries> (Default = 1)\n"
	"      For the two-level adaptive predictor, level 1 size.\n"
	"  TwoLevel.L2Size = <entries> (Default = 1024)\n"
	"      For the two-level adaptive predictor, level 2 size.\n"
	"  TwoLevel.HistorySize = <size> (Default = 8)\n"
	"      For the two-level adaptive predictor, level 2 history size.\n"
	"\n";


/* Main Processor Global Variable */
struct cpu_t *cpu;


/* Configuration file and parameters */

char *cpu_config_file_name = "";
char *cpu_report_file_name = "";

int cpu_cores = 1;
int cpu_threads = 1;

long long cpu_fast_forward_count;

int cpu_context_quantum;
int cpu_context_switch;

int cpu_thread_quantum;
int cpu_thread_switch_penalty;

char *cpu_recover_kind_map[] = { "Writeback", "Commit" };
enum cpu_recover_kind_t cpu_recover_kind;
int cpu_recover_penalty;

char *cpu_fetch_kind_map[] = { "Shared", "TimeSlice", "SwitchOnEvent" };
enum cpu_fetch_kind_t cpu_fetch_kind;

int cpu_decode_width;

char *cpu_dispatch_kind_map[] = { "Shared", "TimeSlice" };
enum cpu_dispatch_kind_t cpu_dispatch_kind;
int cpu_dispatch_width;

char *cpu_issue_kind_map[] = { "Shared", "TimeSlice" };
enum cpu_issue_kind_t cpu_issue_kind;
int cpu_issue_width;

char *cpu_commit_kind_map[] = { "Shared", "TimeSlice" };
enum cpu_commit_kind_t cpu_commit_kind;
int cpu_commit_width;

int cpu_occupancy_stats;




/*
 * Public Functions
 */


/* Check CPU configuration file */
void cpu_config_check(void)
{
	struct config_t *config;
	int err;
	char *section;

	/* Open file */
	config = config_create(cpu_config_file_name);
	err = config_load(config);
	if (!err && cpu_config_file_name[0])
		fatal("%s: cannot load CPU configuration file", cpu_config_file_name);

	
	/* General configuration */

	section = "General";

	cpu_cores = config_read_int(config, section, "Cores", cpu_cores);
	cpu_threads = config_read_int(config, section, "Threads", cpu_threads);

	cpu_fast_forward_count = config_read_llint(config, section, "FastForward", 0);

	cpu_context_switch = config_read_bool(config, section, "ContextSwitch", 1);
	cpu_context_quantum = config_read_int(config, section, "ContextQuantum", 100000);

	cpu_thread_quantum = config_read_int(config, section, "ThreadQuantum", 1000);
	cpu_thread_switch_penalty = config_read_int(config, section, "ThreadSwitchPenalty", 0);

	cpu_recover_kind = config_read_enum(config, section, "RecoverKind", cpu_recover_kind_writeback, cpu_recover_kind_map, 2);
	cpu_recover_penalty = config_read_int(config, section, "RecoverPenalty", 0);

	mmu_page_size = config_read_int(config, section, "PageSize", 4096);


	/* Section '[ Pipeline ]' */

	section = "Pipeline";

	cpu_fetch_kind = config_read_enum(config, section, "FetchKind", cpu_fetch_kind_timeslice, cpu_fetch_kind_map, 3);

	cpu_decode_width = config_read_int(config, section, "DecodeWidth", 4);

	cpu_dispatch_kind = config_read_enum(config, section, "DispatchKind", cpu_dispatch_kind_timeslice, cpu_dispatch_kind_map, 2);
	cpu_dispatch_width = config_read_int(config, section, "DispatchWidth", 4);

	cpu_issue_kind = config_read_enum(config, section, "IssueKind", cpu_issue_kind_timeslice, cpu_issue_kind_map, 2);
	cpu_issue_width = config_read_int(config, section, "IssueWidth", 4);

	cpu_commit_kind = config_read_enum(config, section, "CommitKind", cpu_commit_kind_shared, cpu_commit_kind_map, 2);
	cpu_commit_width = config_read_int(config, section, "CommitWidth", 4);

	cpu_occupancy_stats = config_read_bool(config, section, "OccupancyStats", 0);


	/* Section '[ Queues ]' */

	section = "Queues";

	fetchq_size = config_read_int(config, section, "FetchQueueSize", 64);

	uopq_size = config_read_int(config, section, "UopQueueSize", 32);

	rob_kind = config_read_enum(config, section, "RobKind", rob_kind_private, rob_kind_map, 2);
	rob_size = config_read_int(config, section, "RobSize", 64);

	iq_kind = config_read_enum(config, section, "IqKind", iq_kind_private, iq_kind_map, 2);
	iq_size = config_read_int(config, section, "IqSize", 40);

	lsq_kind = config_read_enum(config, section, "LsqKind", lsq_kind_private, lsq_kind_map, 2);
	lsq_size = config_read_int(config, section, "LsqSize", 20);

	rf_kind = config_read_enum(config, section, "RfKind", rf_kind_private, rf_kind_map, 2);
	rf_int_size = config_read_int(config, section, "RfIntSize", 80);
	rf_fp_size = config_read_int(config, section, "RfFpSize", 40);


	/* Section '[ TraceCache ]' */

	section = "TraceCache";

	trace_cache_present = config_read_bool(config, section, "Present", 0);
	trace_cache_num_sets = config_read_int(config, section, "Sets", 64);
	trace_cache_assoc = config_read_int(config, section, "Assoc", 4);
	trace_cache_trace_size = config_read_int(config, section, "TraceSize", 16);
	trace_cache_branch_max = config_read_int(config, section, "BranchMax", 3);
	trace_cache_queue_size = config_read_int(config, section, "QueueSize", 32);

	
	/* Functional Units */

	section = "FunctionalUnits";

	fu_res_pool[fu_intadd].count = config_read_int(config, section, "IntAdd.Count", 4);
	fu_res_pool[fu_intadd].oplat = config_read_int(config, section, "IntAdd.OpLat", 2);
	fu_res_pool[fu_intadd].issuelat = config_read_int(config, section, "IntAdd.IssueLat", 1);

	fu_res_pool[fu_intmult].count = config_read_int(config, section, "IntMult.Count", 1);
	fu_res_pool[fu_intmult].oplat = config_read_int(config, section, "IntMult.OpLat", 3);
	fu_res_pool[fu_intmult].issuelat = config_read_int(config, section, "IntMult.IssueLat", 3);

	fu_res_pool[fu_intdiv].count = config_read_int(config, section, "IntDiv.Count", 1);
	fu_res_pool[fu_intdiv].oplat = config_read_int(config, section, "IntDiv.OpLat", 20);
	fu_res_pool[fu_intdiv].issuelat = config_read_int(config, section, "IntDiv.IssueLat", 20);

	fu_res_pool[fu_effaddr].count = config_read_int(config, section, "EffAddr.Count", 4);
	fu_res_pool[fu_effaddr].oplat = config_read_int(config, section, "EffAddr.OpLat", 2);
	fu_res_pool[fu_effaddr].issuelat = config_read_int(config, section, "EffAddr.IssueLat", 1);

	fu_res_pool[fu_logic].count = config_read_int(config, section, "Logic.Count", 4);
	fu_res_pool[fu_logic].oplat = config_read_int(config, section, "Logic.OpLat", 1);
	fu_res_pool[fu_logic].issuelat = config_read_int(config, section, "Logic.IssueLat", 1);

	fu_res_pool[fu_fpsimple].count = config_read_int(config, section, "FpSimple.Count", 2);
	fu_res_pool[fu_fpsimple].oplat = config_read_int(config, section, "FpSimple.OpLat", 2);
	fu_res_pool[fu_fpsimple].issuelat = config_read_int(config, section, "FpSimple.IssueLat", 2);

	fu_res_pool[fu_fpadd].count = config_read_int(config, section, "FpAdd.Count", 2);
	fu_res_pool[fu_fpadd].oplat = config_read_int(config, section, "FpAdd.OpLat", 5);
	fu_res_pool[fu_fpadd].issuelat = config_read_int(config, section, "FpAdd.IssueLat", 5);

	fu_res_pool[fu_fpmult].count = config_read_int(config, section, "FpMult.Count", 1);
	fu_res_pool[fu_fpmult].oplat = config_read_int(config, section, "FpMult.OpLat", 10);
	fu_res_pool[fu_fpmult].issuelat = config_read_int(config, section, "FpMult.IssueLat", 10);

	fu_res_pool[fu_fpdiv].count = config_read_int(config, section, "FpDiv.Count", 1);
	fu_res_pool[fu_fpdiv].oplat = config_read_int(config, section, "FpDiv.OpLat", 20);
	fu_res_pool[fu_fpdiv].issuelat = config_read_int(config, section, "FpDiv.IssueLat", 20);

	fu_res_pool[fu_fpcomplex].count = config_read_int(config, section, "FpComplex.Count", 1);
	fu_res_pool[fu_fpcomplex].oplat = config_read_int(config, section, "FpComplex.OpLat", 40);
	fu_res_pool[fu_fpcomplex].issuelat = config_read_int(config, section, "FpComplex.IssueLat", 40);


	/* Branch Predictor */

	section = "BranchPredictor";

	bpred_kind = config_read_enum(config, section, "Kind", bpred_kind_twolevel, bpred_kind_map, 6);
	bpred_btb_sets = config_read_int(config, section, "BTB.Sets", 256);
	bpred_btb_assoc = config_read_int(config, section, "BTB.Assoc", 4);
	bpred_bimod_size = config_read_int(config, section, "Bimod.Size", 1024);
	bpred_choice_size = config_read_int(config, section, "Choice.Size", 1024);
	bpred_ras_size = config_read_int(config, section, "RAS.Size", 32);
	bpred_twolevel_l1size = config_read_int(config, section, "TwoLevel.L1Size", 1);
	bpred_twolevel_l2size = config_read_int(config, section, "TwoLevel.L2Size", 1024);
	bpred_twolevel_hist_size = config_read_int(config, section, "TwoLevel.HistorySize", 8);

	/* Close file */
	config_check(config);
	config_free(config);
}


/* Dump the CPU configuration */
void cpu_config_dump(FILE *f)
{
	/* General configuration */
	fprintf(f, "[ Config.General ]\n");
	fprintf(f, "Cores = %d\n", cpu_cores);
	fprintf(f, "Threads = %d\n", cpu_threads);
	fprintf(f, "FastForward = %lld\n", cpu_fast_forward_count);
	fprintf(f, "ContextSwitch = %s\n", cpu_context_switch ? "True" : "False");
	fprintf(f, "ContextQuantum = %d\n", cpu_context_quantum);
	fprintf(f, "ThreadQuantum = %d\n", cpu_thread_quantum);
	fprintf(f, "ThreadSwitchPenalty = %d\n", cpu_thread_switch_penalty);
	fprintf(f, "RecoverKind = %s\n", cpu_recover_kind_map[cpu_recover_kind]);
	fprintf(f, "RecoverPenalty = %d\n", cpu_recover_penalty);
	fprintf(f, "PageSize = %d\n", mmu_page_size);
	fprintf(f, "\n");

	/* Pipeline */
	fprintf(f, "[ Config.Pipeline ]\n");
	fprintf(f, "FetchKind = %s\n", cpu_fetch_kind_map[cpu_fetch_kind]);
	fprintf(f, "DecodeWidth = %d\n", cpu_decode_width);
	fprintf(f, "DispatchKind = %s\n", cpu_dispatch_kind_map[cpu_dispatch_kind]);
	fprintf(f, "DispatchWidth = %d\n", cpu_dispatch_width);
	fprintf(f, "IssueKind = %s\n", cpu_issue_kind_map[cpu_issue_kind]);
	fprintf(f, "IssueWidth = %d\n", cpu_issue_width);
	fprintf(f, "CommitKind = %s\n", cpu_commit_kind_map[cpu_commit_kind]);
	fprintf(f, "CommitWidth = %d\n", cpu_commit_width);
	fprintf(f, "OccupancyStats = %s\n", cpu_occupancy_stats ? "True" : "False");
	fprintf(f, "\n");

	/* Queues */
	fprintf(f, "[ Config.Queues ]\n");
	fprintf(f, "FetchQueueSize = %d\n", fetchq_size);
	fprintf(f, "UopQueueSize = %d\n", uopq_size);
	fprintf(f, "RobKind = %s\n", rob_kind_map[rob_kind]);
	fprintf(f, "RobSize = %d\n", rob_size);
	fprintf(f, "IqKind = %s\n", iq_kind_map[iq_kind]);
	fprintf(f, "IqSize = %d\n", iq_size);
	fprintf(f, "LsqKind = %s\n", lsq_kind_map[lsq_kind]);
	fprintf(f, "LsqSize = %d\n", lsq_size);
	fprintf(f, "RfKind = %s\n", rf_kind_map[rf_kind]);
	fprintf(f, "RfIntSize = %d\n", rf_int_size);
	fprintf(f, "RfFpSize = %d\n", rf_fp_size);
	fprintf(f, "\n");

	/* Trace Cache */
	fprintf(f, "[ Config.TraceCache ]\n");
	fprintf(f, "Present = %s\n", trace_cache_present ? "True" : "False");
	fprintf(f, "Sets = %d\n", trace_cache_num_sets);
	fprintf(f, "Assoc = %d\n", trace_cache_assoc);
	fprintf(f, "TraceSize = %d\n", trace_cache_trace_size);
	fprintf(f, "BranchMax = %d\n", trace_cache_branch_max);
	fprintf(f, "QueueSize = %d\n", trace_cache_queue_size);
	fprintf(f, "\n");

	/* Functional units */
	fprintf(f, "[ Config.FunctionalUnits ]\n");

	fprintf(f, "IntAdd.Count = %d\n", fu_res_pool[fu_intadd].count);
	fprintf(f, "IntAdd.OpLat = %d\n", fu_res_pool[fu_intadd].oplat);
	fprintf(f, "IntAdd.IssueLat = %d\n", fu_res_pool[fu_intadd].issuelat);

	fprintf(f, "IntMult.Count = %d\n", fu_res_pool[fu_intmult].count);
	fprintf(f, "IntMult.OpLat = %d\n", fu_res_pool[fu_intmult].oplat);
	fprintf(f, "IntMult.IssueLat = %d\n", fu_res_pool[fu_intmult].issuelat);

	fprintf(f, "IntDiv.Count = %d\n", fu_res_pool[fu_intdiv].count);
	fprintf(f, "IntDiv.OpLat = %d\n", fu_res_pool[fu_intdiv].oplat);
	fprintf(f, "IntDiv.IssueLat = %d\n", fu_res_pool[fu_intdiv].issuelat);

	fprintf(f, "EffAddr.Count = %d\n", fu_res_pool[fu_effaddr].count);
	fprintf(f, "EffAddr.OpLat = %d\n", fu_res_pool[fu_effaddr].oplat);
	fprintf(f, "EffAddr.IssueLat = %d\n", fu_res_pool[fu_effaddr].issuelat);

	fprintf(f, "Logic.Count = %d\n", fu_res_pool[fu_logic].count);
	fprintf(f, "Logic.OpLat = %d\n", fu_res_pool[fu_logic].oplat);
	fprintf(f, "Logic.IssueLat = %d\n", fu_res_pool[fu_logic].issuelat);

	fprintf(f, "FpSimple.Count = %d\n", fu_res_pool[fu_fpsimple].count);
	fprintf(f, "FpSimple.OpLat = %d\n", fu_res_pool[fu_fpsimple].oplat);
	fprintf(f, "FpSimple.IssueLat = %d\n", fu_res_pool[fu_fpsimple].issuelat);

	fprintf(f, "FpAdd.Count = %d\n", fu_res_pool[fu_fpadd].count);
	fprintf(f, "FpAdd.OpLat = %d\n", fu_res_pool[fu_fpadd].oplat);
	fprintf(f, "FpAdd.IssueLat = %d\n", fu_res_pool[fu_fpadd].issuelat);

	fprintf(f, "FpMult.Count = %d\n", fu_res_pool[fu_fpmult].count);
	fprintf(f, "FpMult.OpLat = %d\n", fu_res_pool[fu_fpmult].oplat);
	fprintf(f, "FpMult.IssueLat = %d\n", fu_res_pool[fu_fpmult].issuelat);

	fprintf(f, "FpDiv.Count = %d\n", fu_res_pool[fu_fpdiv].count);
	fprintf(f, "FpDiv.OpLat = %d\n", fu_res_pool[fu_fpdiv].oplat);
	fprintf(f, "FpDiv.IssueLat = %d\n", fu_res_pool[fu_fpdiv].issuelat);

	fprintf(f, "FpComplex.Count = %d\n", fu_res_pool[fu_fpcomplex].count);
	fprintf(f, "FpComplex.OpLat = %d\n", fu_res_pool[fu_fpcomplex].oplat);
	fprintf(f, "FpComplex.IssueLat = %d\n", fu_res_pool[fu_fpcomplex].issuelat);

	fprintf(f, "\n");

	/* Branch Predictor */
	fprintf(f, "[ Config.BranchPredictor ]\n");
	fprintf(f, "Kind = %s\n", bpred_kind_map[bpred_kind]);
	fprintf(f, "BTB.Sets = %d\n", bpred_btb_sets);
	fprintf(f, "BTB.Assoc = %d\n", bpred_btb_assoc);
	fprintf(f, "Bimod.Size = %d\n", bpred_bimod_size);
	fprintf(f, "Choice.Size = %d\n", bpred_choice_size);
	fprintf(f, "RAS.Size = %d\n", bpred_ras_size);
	fprintf(f, "TwoLevel.L1Size = %d\n", bpred_twolevel_l1size);
	fprintf(f, "TwoLevel.L2Size = %d\n", bpred_twolevel_l2size);
	fprintf(f, "TwoLevel.HistorySize = %d\n", bpred_twolevel_hist_size);
	fprintf(f, "\n");

	/* End of configuration */
	fprintf(f, "\n");

}


void cpu_dump_uop_report(FILE *f, long long *uop_stats, char *prefix, int peak_ipc)
{
	long long uinst_int_count = 0;
	long long uinst_logic_count = 0;
	long long uinst_fp_count = 0;
	long long uinst_mem_count = 0;
	long long uinst_ctrl_count = 0;
	long long uinst_total = 0;

	char *name;
	enum x86_uinst_flag_t flags;
	int i;

	for (i = 0; i < x86_uinst_opcode_count; i++)
	{
		name = x86_uinst_info[i].name;
		flags = x86_uinst_info[i].flags;

		fprintf(f, "%s.Uop.%s = %lld\n", prefix, name, uop_stats[i]);
		if (flags & X86_UINST_INT)
			uinst_int_count += uop_stats[i];
		if (flags & X86_UINST_LOGIC)
			uinst_logic_count += uop_stats[i];
		if (flags & X86_UINST_FP)
			uinst_fp_count += uop_stats[i];
		if (flags & X86_UINST_MEM)
			uinst_mem_count += uop_stats[i];
		if (flags & X86_UINST_CTRL)
			uinst_ctrl_count += uop_stats[i];
		uinst_total += uop_stats[i];
	}
	fprintf(f, "%s.Integer = %lld\n", prefix, uinst_int_count);
	fprintf(f, "%s.Logic = %lld\n", prefix, uinst_logic_count);
	fprintf(f, "%s.FloatingPoint = %lld\n", prefix, uinst_fp_count);
	fprintf(f, "%s.Memory = %lld\n", prefix, uinst_mem_count);
	fprintf(f, "%s.Ctrl = %lld\n", prefix, uinst_ctrl_count);
	fprintf(f, "%s.WndSwitch = %lld\n", prefix, uop_stats[x86_uinst_call] + uop_stats[x86_uinst_ret]);
	fprintf(f, "%s.Total = %lld\n", prefix, uinst_total);
	fprintf(f, "%s.IPC = %.4g\n", prefix, cpu->cycle ? (double) uinst_total / cpu->cycle : 0.0);
	fprintf(f, "%s.DutyCycle = %.4g\n", prefix, cpu->cycle && peak_ipc ?
		(double) uinst_total / cpu->cycle / peak_ipc : 0.0);
	fprintf(f, "\n");
}


#define DUMP_FU_STAT(NAME, ITEM) { \
	fprintf(f, "fu." #NAME ".Accesses = %lld\n", CORE.fu->accesses[ITEM]); \
	fprintf(f, "fu." #NAME ".Denied = %lld\n", CORE.fu->denied[ITEM]); \
	fprintf(f, "fu." #NAME ".WaitingTime = %.4g\n", CORE.fu->accesses[ITEM] ? \
		(double) CORE.fu->waiting_time[ITEM] / CORE.fu->accesses[ITEM] : 0.0); \
}

#define DUMP_DISPATCH_STAT(NAME) { \
	fprintf(f, "Dispatch.Stall." #NAME " = %lld\n", CORE.di_stall[di_stall_##NAME]); \
}

#define DUMP_CORE_STRUCT_STATS(NAME, ITEM) { \
	fprintf(f, #NAME ".Size = %d\n", (int) ITEM##_size * cpu_threads); \
	if (cpu_occupancy_stats) \
		fprintf(f, #NAME ".Occupancy = %.2f\n", cpu->cycle ? (double) CORE.ITEM##_occupancy / cpu->cycle : 0.0); \
	fprintf(f, #NAME ".Full = %lld\n", CORE.ITEM##_full); \
	fprintf(f, #NAME ".Reads = %lld\n", CORE.ITEM##_reads); \
	fprintf(f, #NAME ".Writes = %lld\n", CORE.ITEM##_writes); \
}

#define DUMP_THREAD_STRUCT_STATS(NAME, ITEM) { \
	fprintf(f, #NAME ".Size = %d\n", (int) ITEM##_size); \
	if (cpu_occupancy_stats) \
		fprintf(f, #NAME ".Occupancy = %.2f\n", cpu->cycle ? (double) THREAD.ITEM##_occupancy / cpu->cycle : 0.0); \
	fprintf(f, #NAME ".Full = %lld\n", THREAD.ITEM##_full); \
	fprintf(f, #NAME ".Reads = %lld\n", THREAD.ITEM##_reads); \
	fprintf(f, #NAME ".Writes = %lld\n", THREAD.ITEM##_writes); \
}

void cpu_dump_report()
{
	FILE *f;
	int core, thread;
	uint64_t now = ke_timer();

	/* Open file */
	f = open_write(cpu_report_file_name);
	if (!f)
		return;
	
	/* Dump CPU configuration */
	fprintf(f, ";\n; CPU Configuration\n;\n\n");
	cpu_config_dump(f);
	
	/* Report for the complete processor */
	fprintf(f, ";\n; Simulation Statistics\n;\n\n");
	fprintf(f, "; Global statistics\n");
	fprintf(f, "[ Global ]\n\n");
	fprintf(f, "Cycles = %lld\n", cpu->cycle);
	fprintf(f, "Time = %.1f\n", (double) now / 1000000);
	fprintf(f, "CyclesPerSecond = %.0f\n", now ? (double) cpu->cycle / now * 1000000 : 0.0);
	fprintf(f, "MemoryUsed = %lu\n", (long) mem_mapped_space);
	fprintf(f, "MemoryUsedMax = %lu\n", (long) mem_max_mapped_space);
	fprintf(f, "\n");

	/* Dispatch stage */
	fprintf(f, "; Dispatch stage\n");
	cpu_dump_uop_report(f, cpu->dispatched, "Dispatch", cpu_dispatch_width);

	/* Issue stage */
	fprintf(f, "; Issue stage\n");
	cpu_dump_uop_report(f, cpu->issued, "Issue", cpu_issue_width);

	/* Commit stage */
	fprintf(f, "; Commit stage\n");
	cpu_dump_uop_report(f, cpu->committed, "Commit", cpu_commit_width);

	/* Committed branches */
	fprintf(f, "; Committed branches\n");
	fprintf(f, ";    Branches - Number of committed control uops\n");
	fprintf(f, ";    Squashed - Number of mispredicted uops squashed from the ROB\n");
	fprintf(f, ";    Mispred - Number of mispredicted branches in the correct path\n");
	fprintf(f, ";    PredAcc - Prediction accuracy\n");
	fprintf(f, "Commit.Branches = %lld\n", cpu->branches);
	fprintf(f, "Commit.Squashed = %lld\n", cpu->squashed);
	fprintf(f, "Commit.Mispred = %lld\n", cpu->mispred);
	fprintf(f, "Commit.PredAcc = %.4g\n", cpu->branches ?
		(double) (cpu->branches - cpu->mispred) / cpu->branches : 0.0);
	fprintf(f, "\n");
	
	/* Report for each core */
	FOREACH_CORE {
		
		/* Core */
		fprintf(f, "\n; Statistics for core %d\n", core);
		fprintf(f, "[ c%d ]\n\n", core);

		/* Functional units */
		fprintf(f, "; Functional unit pool\n");
		fprintf(f, ";    Accesses - Number of uops issued to a f.u.\n");
		fprintf(f, ";    Denied - Number of requests denied due to busy f.u.\n");
		fprintf(f, ";    WaitingTime - Average number of waiting cycles to reserve f.u.\n");
		DUMP_FU_STAT(IntAdd, fu_intadd);
		DUMP_FU_STAT(IntMult, fu_intmult);
		DUMP_FU_STAT(IntDiv, fu_intdiv);
		DUMP_FU_STAT(Effaddr, fu_effaddr);
		DUMP_FU_STAT(Logic, fu_logic);
		DUMP_FU_STAT(FPSimple, fu_fpsimple);
		DUMP_FU_STAT(FPAdd, fu_fpadd);
		DUMP_FU_STAT(FPMult, fu_fpmult);
		DUMP_FU_STAT(FPDiv, fu_fpdiv);
		DUMP_FU_STAT(FPComplex, fu_fpcomplex);
		fprintf(f, "\n");

		/* Dispatch slots */
		if (cpu_dispatch_kind == cpu_dispatch_kind_timeslice) {
			fprintf(f, "; Dispatch slots usage (sum = cycles * dispatch width)\n");
			fprintf(f, ";    used - dispatch slot was used by a non-spec uop\n");
			fprintf(f, ";    spec - used by a mispeculated uop\n");
			fprintf(f, ";    ctx - no context allocated to thread\n");
			fprintf(f, ";    uopq,rob,iq,lsq,rename - no space in structure\n");
			DUMP_DISPATCH_STAT(used);
			DUMP_DISPATCH_STAT(spec);
			DUMP_DISPATCH_STAT(uopq);
			DUMP_DISPATCH_STAT(rob);
			DUMP_DISPATCH_STAT(iq);
			DUMP_DISPATCH_STAT(lsq);
			DUMP_DISPATCH_STAT(rename);
			DUMP_DISPATCH_STAT(ctx);
			fprintf(f, "\n");
		}

		/* Dispatch stage */
		fprintf(f, "; Dispatch stage\n");
		cpu_dump_uop_report(f, CORE.dispatched, "Dispatch", cpu_dispatch_width);

		/* Issue stage */
		fprintf(f, "; Issue stage\n");
		cpu_dump_uop_report(f, CORE.issued, "Issue", cpu_issue_width);

		/* Commit stage */
		fprintf(f, "; Commit stage\n");
		cpu_dump_uop_report(f, CORE.committed, "Commit", cpu_commit_width);

		/* Committed branches */
		fprintf(f, "; Committed branches\n");
		fprintf(f, "Commit.Branches = %lld\n", CORE.branches);
		fprintf(f, "Commit.Squashed = %lld\n", CORE.squashed);
		fprintf(f, "Commit.Mispred = %lld\n", CORE.mispred);
		fprintf(f, "Commit.PredAcc = %.4g\n", CORE.branches ?
			(double) (CORE.branches - CORE.mispred) / CORE.branches : 0.0);
		fprintf(f, "\n");

		/* Occupancy stats */
		fprintf(f, "; Structure statistics (reorder buffer, instruction queue,\n");
		fprintf(f, "; load-store queue, and integer/floating-point register file)\n");
		fprintf(f, ";    Size - Available size\n");
		fprintf(f, ";    Occupancy - Average number of occupied entries\n");
		fprintf(f, ";    Full - Number of cycles when the structure was full\n");
		fprintf(f, ";    Reads, Writes - Accesses to the structure\n");
		if (rob_kind == rob_kind_shared)
			DUMP_CORE_STRUCT_STATS(ROB, rob);
		if (iq_kind == iq_kind_shared) {
			DUMP_CORE_STRUCT_STATS(IQ, iq);
			fprintf(f, "IQ.WakeupAccesses = %lld\n", CORE.iq_wakeup_accesses);
		}
		if (lsq_kind == lsq_kind_shared)
			DUMP_CORE_STRUCT_STATS(LSQ, lsq);
		if (rf_kind == rf_kind_shared) {
			DUMP_CORE_STRUCT_STATS(RF_Int, rf_int);
			DUMP_CORE_STRUCT_STATS(RF_Fp, rf_fp);
		}
		fprintf(f, "\n");

		/* Report for each thread */
		FOREACH_THREAD {
			fprintf(f, "\n; Statistics for core %d - thread %d\n", core, thread);
			fprintf(f, "[ c%dt%d ]\n\n", core, thread);

			/* Dispatch stage */
			fprintf(f, "; Dispatch stage\n");
			cpu_dump_uop_report(f, THREAD.dispatched, "Dispatch", cpu_dispatch_width);

			/* Issue stage */
			fprintf(f, "; Issue stage\n");
			cpu_dump_uop_report(f, THREAD.issued, "Issue", cpu_issue_width);

			/* Commit stage */
			fprintf(f, "; Commit stage\n");
			cpu_dump_uop_report(f, THREAD.committed, "Commit", cpu_commit_width);

			/* Committed branches */
			fprintf(f, "; Committed branches\n");
			fprintf(f, "Commit.Branches = %lld\n", THREAD.branches);
			fprintf(f, "Commit.Squashed = %lld\n", THREAD.squashed);
			fprintf(f, "Commit.Mispred = %lld\n", THREAD.mispred);
			fprintf(f, "Commit.PredAcc = %.4g\n", THREAD.branches ?
				(double) (THREAD.branches - THREAD.mispred) / THREAD.branches : 0.0);
			fprintf(f, "\n");

			/* Occupancy stats */
			fprintf(f, "; Structure statistics (reorder buffer, instruction queue, load-store queue,\n");
			fprintf(f, "; integer/floating-point register file, and renaming table)\n");
			if (rob_kind == rob_kind_private)
				DUMP_THREAD_STRUCT_STATS(ROB, rob);
			if (iq_kind == iq_kind_private) {
				DUMP_THREAD_STRUCT_STATS(IQ, iq);
				fprintf(f, "IQ.WakeupAccesses = %lld\n", THREAD.iq_wakeup_accesses);
			}
			if (lsq_kind == lsq_kind_private)
				DUMP_THREAD_STRUCT_STATS(LSQ, lsq);
			if (rf_kind == rf_kind_private) {
				DUMP_THREAD_STRUCT_STATS(RF_Int, rf_int);
				DUMP_THREAD_STRUCT_STATS(RF_Fp, rf_fp);
			}
			fprintf(f, "RAT.IntReads = %lld\n", THREAD.rat_int_reads);
			fprintf(f, "RAT.IntWrites = %lld\n", THREAD.rat_int_writes);
			fprintf(f, "RAT.FpReads = %lld\n", THREAD.rat_fp_reads);
			fprintf(f, "RAT.FpWrites = %lld\n", THREAD.rat_fp_writes);
			fprintf(f, "BTB.Reads = %lld\n", THREAD.btb_reads);
			fprintf(f, "BTB.Writes = %lld\n", THREAD.btb_writes);
			fprintf(f, "\n");

			/* Trace cache stats */
			if (THREAD.trace_cache)
				trace_cache_dump_report(THREAD.trace_cache, f);
		}
	}

	/* Close */
	fclose(f);
}


void cpu_thread_init(int core, int thread)
{
}


void cpu_core_init(int core)
{
	int thread;
	CORE.thread = calloc(cpu_threads, sizeof(struct cpu_thread_t));
	FOREACH_THREAD
		cpu_thread_init(core, thread);
}


/* Initialization */
void cpu_init()
{
	int core;

	/* Analyze CPU configuration file */
	cpu_config_check();

	/* Create processor structure and allocate cores/threads */
	cpu = calloc(1, sizeof(struct cpu_t));
	cpu->core = calloc(cpu_cores, sizeof(struct cpu_core_t));
	FOREACH_CORE
		cpu_core_init(core);

	rf_init();
	bpred_init();
	trace_cache_init();
	fetchq_init();
	uopq_init();
	rob_init();
	iq_init();
	lsq_init();
	eventq_init();
	fu_init();
}


/* Finalization */
void cpu_done()
{
	int core;

	/* Finalize structures */
	fetchq_done();
	uopq_done();
	rob_done();
	iq_done();
	lsq_done();
	eventq_done();
	bpred_done();
	trace_cache_done();
	rf_done();
	fu_done();

	/* Free processor */
	FOREACH_CORE
		free(CORE.thread);
	free(cpu->core);
	free(cpu);
}


/* Load programs to different contexts from a configuration text file or
 * from arguments */
void cpu_load_progs(int argc, char **argv, char *ctxfile)
{
	if (argc > 1)
		ld_load_prog_from_cmdline(argc - 1, argv + 1);
	if (*ctxfile)
		ld_load_prog_from_ctxconfig(ctxfile);
}


void cpu_dump(FILE *f)
{
	int core, thread;
	
	/* General information */
	fprintf(f, "\n");
	fprintf(f, "sim.last_dump  %lld  # Cycle of last dump\n", cpu->last_dump);
	fprintf(f, "sim.ipc_last_dump  %.4g  # IPC since last dump\n", cpu->cycle - cpu->last_dump > 0 ?
		(double) (cpu->inst - cpu->last_committed) / (cpu->cycle - cpu->last_dump) : 0);
	fprintf(f, "\n");

	/* Cores */
	FOREACH_CORE {
		fprintf(f, "Core %d:\n", core);
		
		fprintf(f, "eventq:\n");
		uop_lnlist_dump(CORE.eventq, f);
		fprintf(f, "rob:\n");
		rob_dump(core, f);

		FOREACH_THREAD {
			fprintf(f, "Thread %d:\n", thread);
			
			fprintf(f, "fetch queue:\n");
			uop_list_dump(THREAD.fetchq, f);
			fprintf(f, "uop queue:\n");
			uop_list_dump(THREAD.uopq, f);
			fprintf(f, "iq:\n");
			uop_lnlist_dump(THREAD.iq, f);
			fprintf(f, "lq:\n");
			uop_lnlist_dump(THREAD.lq, f);
			fprintf(f, "sq:\n");
			uop_lnlist_dump(THREAD.sq, f);
			rf_dump(core, thread, f);
			if (THREAD.ctx) {
				fprintf(f, "mapped context: %d\n", THREAD.ctx->pid);
				ctx_dump(THREAD.ctx, f);
			}
			
			fprintf(f, "\n");
		}
	}

	/* Register last dump */
	cpu->last_dump = cpu->cycle;
	cpu->last_committed = cpu->inst;
}


#define UPDATE_THREAD_OCCUPANCY_STATS(ITEM) { \
	THREAD.ITEM##_occupancy += THREAD.ITEM##_count; \
	if (THREAD.ITEM##_count == ITEM##_size) \
		THREAD.ITEM##_full++; \
}


#define UPDATE_CORE_OCCUPANCY_STATS(ITEM) { \
	CORE.ITEM##_occupancy += CORE.ITEM##_count; \
	if (CORE.ITEM##_count == ITEM##_size * cpu_threads) \
		CORE.ITEM##_full++; \
}


void cpu_update_occupancy_stats()
{
	int core, thread;

	FOREACH_CORE {

		/* Update occupancy stats for shared structures */
		if (rob_kind == rob_kind_shared)
			UPDATE_CORE_OCCUPANCY_STATS(rob);
		if (iq_kind == iq_kind_shared)
			UPDATE_CORE_OCCUPANCY_STATS(iq);
		if (lsq_kind == lsq_kind_shared)
			UPDATE_CORE_OCCUPANCY_STATS(lsq);
		if (rf_kind == rf_kind_shared) {
			UPDATE_CORE_OCCUPANCY_STATS(rf_int);
			UPDATE_CORE_OCCUPANCY_STATS(rf_fp);
		}

		/* Occupancy stats for private structures */
		FOREACH_THREAD {
			if (rob_kind == rob_kind_private)
				UPDATE_THREAD_OCCUPANCY_STATS(rob);
			if (iq_kind == iq_kind_private)
				UPDATE_THREAD_OCCUPANCY_STATS(iq);
			if (lsq_kind == lsq_kind_private)
				UPDATE_THREAD_OCCUPANCY_STATS(lsq);
			if (rf_kind == rf_kind_private) {
				UPDATE_THREAD_OCCUPANCY_STATS(rf_int);
				UPDATE_THREAD_OCCUPANCY_STATS(rf_fp);
			}
		}
	}
}


void cpu_stages()
{
	/* Static scheduler called after any context changed status other than 'sepcmode' */
	if (!cpu_context_switch && ke->context_reschedule) {
		cpu_static_schedule();
		ke->context_reschedule = 0;
	}

	/* Dynamic scheduler called after any context changed status other than 'specmode',
	 * or quantum of the oldest context expired, and no context is being evicted. */
	if (cpu_context_switch && !cpu->ctx_dealloc_signals &&
		(ke->context_reschedule || cpu->ctx_alloc_oldest + cpu_context_quantum <= cpu->cycle))
	{
		cpu_dynamic_schedule();
		ke->context_reschedule = 0;
	}

	/* Stages */
	cpu_commit();
	cpu_writeback();
	cpu_issue();
	cpu_dispatch();
	cpu_decode();
	cpu_fetch();

	/* Update stats for structures occupancy */
	if (cpu_occupancy_stats)
		cpu_update_occupancy_stats();
}


/* return an address that combines the context and the virtual address page;
 * this address will be univocal for all generated
 * addresses and is a kind of hash function to index tlbs */
uint32_t cpu_tlb_address(int ctx, uint32_t vaddr)
{
	assert(ctx >= 0 && ctx < cpu_cores * cpu_threads);
	return (vaddr >> MEM_LOG_PAGE_SIZE) * cpu_cores * cpu_threads + ctx;
}




/*
 * Simulation loop
 */

static int sigusr_received = 0;


/* Signal handlers */
static void cpu_signal_handler(int signum)
{
	switch (signum) {
	
	case SIGINT:
		if (ke_sim_finish)
			abort();
		ke_sim_finish = ke_sim_finish_signal;
		cpu_dump(stderr);
		fprintf(stderr, "SIGINT received\n");
		break;
		
	case SIGABRT:
		signal(SIGABRT, SIG_DFL);
		if (debug_status(error_debug_category))
			cpu_dump(debug_file(error_debug_category));
		exit(1);
		break;
	
	case SIGUSR2:
		sigusr_received = 1;
	
	}
}


/* Dump a log of current status in a file */
static void sim_dump_log()
{
	FILE *f;
	char name[100];
	
	/* Dump log into file */
	sprintf(name, "m2s.%d.%lld", (int) getpid(), cpu->cycle);
	f = fopen(name, "wt");
	if (f) {
		cpu_dump(f);
		fclose(f);
	}

	/* Ready to receive new SIGUSR signals */
	sigusr_received = 0;
}


/* Fast forward simulation */
static void cpu_fast_forward(long long max_inst)
{
	struct ctx_t *ctx;
	uint64_t inst = 0;

	/* Intro message */
	fprintf(stderr, "\n");
	fprintf(stderr, "; Fast-forward simulation (%lld x86 instructions)\n", max_inst);
	fprintf(stderr, "\n");

	/* Functional simulation */
	for (;;) {

		/* Check finished contexts */
		if (ke->finished_list_count >= ke->context_list_count)
			ke_sim_finish = ke_sim_finish_ctx;
		
		/* Stop if any previous reason met */
		if (ke_sim_finish)
			break;

		/* Stop if maximum number of fast-forward instructions met */
		if (inst >= max_inst)
			break;

		/* Run an instruction from every running process */
		inst += ke->running_list_count;
		for (ctx = ke->running_list_head; ctx; ctx = ctx->running_list_next)
			ctx_execute_inst(ctx);
	
		/* Free finished contexts */
		while (ke->finished_list_head)
			ctx_free(ke->finished_list_head);
	
		/* Process list of suspended contexts */
		ke_process_events();
	}

	/* End message */
	fprintf(stderr, "\n");
	if (ke_sim_finish)
		fprintf(stderr, "; Simulation finished during fast-forward simulation.\n");
	else
		fprintf(stderr, "; Fast-forward simulation complete - continuing detailed simulation.\n");
	fprintf(stderr, "\n");
}


/* Run simulation loop */
void cpu_run()
{
	
	/* Install signal handlers */
	signal(SIGINT, &cpu_signal_handler);
	signal(SIGABRT, &cpu_signal_handler);
	signal(SIGUSR1, &cpu_signal_handler);
	signal(SIGUSR2, &cpu_signal_handler);

	/* Fast forward instructions */
	if (cpu_fast_forward_count)
		cpu_fast_forward(cpu_fast_forward_count);
	
	/* Detailed simulation loop */
	for (;;) {

		/* Stop if all contexts finished */
		if (ke->finished_list_count >= ke->context_list_count)
			ke_sim_finish = ke_sim_finish_ctx;

		/* Stop if maximum number of CPU instructions exceeded */
		if (ke_max_inst && cpu->inst >= ke_max_inst)
			ke_sim_finish = ke_sim_finish_max_cpu_inst;

		/* Stop if maximum number of cycles exceeded */
		if (ke_max_cycles && cpu->cycle >= ke_max_cycles)
			ke_sim_finish = ke_sim_finish_max_cpu_cycles;

		/* Stop if maximum time exceeded (check only every 10k cycles) */
		if (ke_max_time && !(cpu->cycle % 10000) && ke_timer() > ke_max_time * 1000000)
			ke_sim_finish = ke_sim_finish_max_time;

		/* Stop if any previous reason met */
		if (ke_sim_finish)
			break;

		/* Next cycle */
		cpu->cycle++;

		/* Processor stages */
		cpu_stages();

		/* Process host threads generating events */
		ke_process_events();

		/* Event-driven module */
		esim_process_events();
		
		/* Dump log */
		if (sigusr_received)
			sim_dump_log();
	}

	/* Restore signal handlers */
	signal(SIGINT, SIG_DFL);
	signal(SIGABRT, SIG_DFL);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	signal(SIGALRM, SIG_IGN);

	/* CPU report */
	cpu_dump_report();
}

