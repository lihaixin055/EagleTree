/*
 * block_management.h
 *
 *  Created on: Feb 17, 2013
 *      Author: niv
 */

#ifndef BLOCK_MANAGEMENT_H_
#define BLOCK_MANAGEMENT_H_

#include "ssd.h"
#include "scheduler.h"
//#include "mtrand.h"
namespace ssd {

class Migrator {
public:
	Migrator();
	//Migrator(Migrator&);
	~Migrator();
	void init(IOScheduler*, Block_manager_parent*, Garbage_Collector*, Wear_Leveling_Strategy*, FtlParent*, Ssd*);
	void schedule_gc(double time, int package, int die, int block, int klass);
	vector<deque<Event*> > migrate(Event * gc_event);
	void update_structures(Address const& a);
	void print_pending_migrations();
	deque<Event*> trigger_next_migration(Event * gc_read);
	bool more_migrations(Event * gc_read);
	void register_event_completion(Event* event);
	void register_ECC_check_on(uint logical_address);
	uint how_many_gc_operations_are_scheduled() const;
	void set_block_manager(Block_manager_parent* b) { bm = b; }
	Garbage_Collector* get_garbage_collector() { return gc; }
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & scheduler;
    	ar & bm;
    	ar & ssd;
    	ar & ftl;
    	ar & gc;
    	ar & wl;
    }
private:
	bool copy_back_allowed_on(long logical_address);
	void register_copy_back_operation_on(uint logical_address);
	void handle_erase_completion(Event* event);
	void handle_trim_completion(Event* event);
	void issue_erase(Address ra, double time);
	IOScheduler *scheduler;
	Block_manager_parent* bm;
	Ssd* ssd;
	FtlParent* ftl;
	Garbage_Collector* gc;
	Wear_Leveling_Strategy* wl;
	map<long, uint> page_copy_back_count; // Pages that have experienced a copy-back, mapped to a count of the number of copy-backs
	vector<vector<uint> > num_blocks_being_garbaged_collected_per_LUN;
	map<int, int> blocks_being_garbage_collected;
	vector<queue<Event*> > erase_queue;
	vector<int> num_erases_scheduled_per_package;
	map<long, vector<deque<Event*> > > dependent_gc;
	map<Block*, double> gc_time_stat;
};

class Block_manager_parent {
public:
	Block_manager_parent(int classes = 1);
	virtual ~Block_manager_parent();
	virtual void init(Ssd*, FtlParent*, IOScheduler*, Garbage_Collector*, Wear_Leveling_Strategy*, Migrator*);
	virtual void register_write_outcome(Event const& event, enum status status);
	virtual void register_read_command_outcome(Event const& event, enum status status);
	virtual void register_read_transfer_outcome(Event const& event, enum status status);
	virtual void register_erase_outcome(Event const& event, enum status status);
	virtual void register_register_cleared();
	virtual Address choose_write_address(Event& write);
	Address choose_flexible_read_address(Flexible_Read_Event* fr);
	virtual void register_write_arrival(Event const& write);
	virtual void trim(Event const& write);
	virtual void receive_message(Event const& message) {}
	double in_how_long_can_this_event_be_scheduled(Address const& die_address, double current_time, event_type type = NOT_VALID) const;
	double in_how_long_can_this_write_be_scheduled(double current_time) const;
	vector<deque<Event*> > migrate(Event * gc_event);
	bool Copy_backs_in_progress(Address const& address);
	bool can_schedule_on_die(Address const& address, event_type type, uint app_io_id) const;
	bool is_die_register_busy(Address const& addr) const;
	void register_trim_making_gc_redundant(Event* trim);
	Address choose_copbyback_address(Event const& write);
	void schedule_gc(double time, int package_id, int die_id, int block, int klass);
	virtual void check_if_should_trigger_more_GC(Event const& event);
	double get_average_migrations_per_gc() const;
	int get_num_age_classes() const { return num_age_classes; }
	int get_num_pages_available_for_new_writes() const { return num_available_pages_for_new_writes; }
	void subtract_from_available_for_new_writes(int num) {
		num_available_pages_for_new_writes -= num;
		//printf("%d   %d\n", num_available_pages_for_new_writes, num_free_pages);
	}
	vector<Block*> const& get_all_blocks() const { return all_blocks; }
	uint sort_into_age_class(Address const& address) const;
	void copy_state(Block_manager_parent* bm);
	virtual bool bm(Block* block, double current_time) { return true; }
	virtual bool may_garbage_collect_this_block(Block* block, double current_time) { return true;}
	static Block_manager_parent* get_new_instance();
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & ssd;
    	ar & ftl;
    	ar & scheduler;
    	ar & free_block_pointers;

    	ar & free_blocks;
    	ar & all_blocks;
    	ar & num_age_classes;
    	ar & num_free_pages;
    	ar & num_available_pages_for_new_writes;
    	ar & free_block_pointers;

    	ar & wl;
    	ar & gc;
    	ar & migrator;
    }
    Address find_free_unused_block(double time);
	Address find_free_unused_block(uint package_id, uint die_id, enum age age, double time);
	Address find_free_unused_block(uint package_id, uint die_id, double time);
	Address find_free_unused_block(uint package_id, double time);
	Address find_free_unused_block(enum age age, double time);
	pair<bool, pair<int, int> > get_free_block_pointer_with_shortest_IO_queue(vector<vector<Address> > const& dies) const;

protected:
	virtual Address choose_best_address(Event& write) = 0;
	virtual Address choose_any_address(Event const& write) = 0;
	void increment_pointer(Address& pointer);
	bool can_schedule_write_immediately(Address const& prospective_dest, double current_time);
	bool can_write(Event const& write) const;



	void return_unfilled_block(Address block_address, double current_time);

	Address get_free_block_pointer_with_shortest_IO_queue();

	inline bool has_free_pages(Address const& address) const { return address.valid == PAGE && address.page < BLOCK_SIZE; }

	Ssd* ssd;
	FtlParent* ftl;
	IOScheduler *scheduler;
	vector<vector<Address> > free_block_pointers;
	Migrator* migrator;
	vector<vector<vector<vector<Address> > > > free_blocks;  // package -> die -> class -> list of such free blocks
	int get_num_free_blocks() const;
	int get_num_free_blocks(int package, int die) const;
	int get_num_pointers_with_free_space() const;
private:
	Address find_free_unused_block(uint package_id, uint die_id, uint age_class, double time);
	void issue_erase(Address a, double time);


	bool copy_back_allowed_on(long logical_address);
	void register_copy_back_operation_on(uint logical_address);
	void register_ECC_check_on(uint logical_address);
	bool schedule_queued_erase(Address location);

	vector<Block*> all_blocks;

	// The num_age_classes variable controls into how many age classes we divide blocks.
	// In every LUN, the block manager tries to keep num_age_classes free blocks.
	// This allows doing efficient dynamic wear-leveling by putting pages of a certain temperature in blocks of a certain age.
	int num_age_classes;

	uint num_free_pages;
	uint num_available_pages_for_new_writes;

	pair<bool, pair<int, int> > last_get_free_block_pointer_with_shortest_IO_queue_result;
	bool IO_has_completed_since_last_shortest_queue_search;

	vector<queue<Event*> > erase_queue;
	vector<int> num_erases_scheduled_per_package;
	Wear_Leveling_Strategy* wl;
	Garbage_Collector* gc;

};



class Wear_Leveling_Strategy {
public:
	Wear_Leveling_Strategy();
	//Wear_Leveling_Strategy(Wear_Leveling_Strategy&);
	Wear_Leveling_Strategy(Ssd* ssd, Migrator*);
	~Wear_Leveling_Strategy() {};
	void register_erase_completion(Event const& event);
	bool schedule_wear_leveling_op(Block* block);
	double get_normalised_age(uint age) const;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & age_distribution;
    	ar & all_blocks;
    	ar & num_erases_up_to_date;
    	ar & ssd;
    	ar & average_erase_cycle_time;
    	ar & blocks_being_wl;
    	ar & blocks_to_wl;
    	ar & migrator;

    	ar & max_age;
    	ar & block_data;
    }
private:
    void init();
	double get_min_age() const;
	//void update_blocks_with_min_age(uint min_age);
	void find_wl_candidates(double current_time);
	//set<Block*> blocks_with_min_age;
	map<int, int> age_distribution;  // maps block ages to the number of blocks with this age
	vector<Block*> all_blocks;
	int num_erases_up_to_date;
	Ssd* ssd;
	double average_erase_cycle_time;
	set<Block*> blocks_being_wl;
	set<Block*> blocks_to_wl;
	Migrator* migrator;
	int max_age;
	MTRand_int32 random_number_generator;
	struct Block_data {
		int age;
		double last_erase_time;
		Block_data() : age(0), last_erase_time(0) {}
	    friend class boost::serialization::access;
	    template<class Archive>
	    void serialize(Archive & ar, const unsigned int version)
	    {
	    	ar & age;
	    	ar & last_erase_time;
	    }
	};
	vector<Block_data> block_data;
};

// A BM that assigns each write to the die with the shortest queue. No hot-cold seperation
class Block_manager_parallel : public Block_manager_parent {
public:
	Block_manager_parallel();
	~Block_manager_parallel() {}
	void register_write_outcome(Event const& event, enum status status);
	void register_erase_outcome(Event const& event, enum status status);
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & boost::serialization::base_object<Block_manager_parent>(*this);
    }
protected:
	Address choose_best_address(Event& write);
	Address choose_any_address(Event const& write);
};

// A BM that assigns each write to the die with the shortest queue. No hot-cold seperation
class bm_gc_locality : public Block_manager_parent {
public:
	bm_gc_locality();
	~bm_gc_locality() {}
	void register_write_outcome(Event const& event, enum status status);
	void check_if_should_trigger_more_GC(Event const& event);
	void register_erase_outcome(Event const& event, enum status status);
	bool may_garbage_collect_this_block(Block* block, double current_time);
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & boost::serialization::base_object<Block_manager_parent>(*this);
    }
protected:
	Address choose_best_address(Event& write);
	Address choose_any_address(Event const& write);
	map<Block*, Address> pointers_for_ongoing_gc_operations;
	vector<vector<queue<Address> > > partially_used_blocks;
private:
	int get_num_partially_empty_blocks() const;
	Address get_block_for_gc(int package, int die, double current_time);
};

// A BM that seperates blocks based on tags
class Block_Manager_Tag_Groups : public Block_manager_parent {
public:
	Block_Manager_Tag_Groups();
	~Block_Manager_Tag_Groups() {}
	void register_write_arrival(Event const& e);
	void register_write_outcome(Event const& event, enum status status);
	void register_erase_outcome(Event const& event, enum status status);
	//void increment_pointer_and_find_free(Address& block, double time);
    friend class boost::serialization::access;
    void print();
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & boost::serialization::base_object<Block_manager_parent>(*this);
    	ar & free_block_pointers_tags;
    }
protected:
	Address choose_best_address(Event& write);
	Address choose_any_address(Event const& write);
private:
	map<int, vector<vector<Address> > > free_block_pointers_tags;  // tags, packages, dies
};

enum write_amp_choice {greedy, prob, opt};

struct pointers {
	pointers();
	pointers(Block_manager_parent* bm);
	void register_completion(Event const& e);
	Address get_best_block(Block_manager_parent* bm);
	void print();
	int get_num_free_blocks() const;
	Block_manager_parent* bm;
	vector<vector<Address> > blocks;
};

class group {
public:
	group(double prob, double size, Block_manager_parent* bm, Ssd* ssd, int id);
	group();
	void print() const;
	void print_die_spesific_info() const;
	void print_blocks_valid_pages_per_die() const;
	double get_prob_op(double PBA, double LBA);
	double get_greedy_op(double PBA, double LBA);
	double get_average_op(double PBA, double LBA);
	double get_write_amp(write_amp_choice choice) const;
	Block* get_gc_victim() const;
	Block* get_gc_victim(int package, int die) const;
	void register_write_outcome(Event const& event);
	void register_erase_outcome(Event const& event);
	bool is_starved() const;
	bool needs_more_blocks() const;
	bool in_equilbirium() const;
	static bool in_total_equilibrium(vector<group> const& groups, int group_id);
	static double get_average_write_amp(vector<group>& groups, write_amp_choice choice = opt);
	static vector<group> iterate(vector<group> const& groups);
	static void print(vector<group>& groups);
	static void init_stats(vector<group>& groups);
	static void count_num_groups_that_need_more_blocks(vector<group> const& groups);
	double get_avg_pages_per_die() const;
	double get_avg_blocks_per_die() const;
	double get_min_pages_per_die() const;
	double prob;
	double size;
	double offset;
	double OP;
	double OP_greedy;
	double OP_prob;
	double OP_average;
	pointers free_blocks;
	pointers next_free_blocks;
	set<Block*> block_ids;
	set<Block*> blocks_being_garbage_collected;
	struct group_stats {
		group_stats() : num_gc_in_group(0), num_writes_to_group(0), num_gc_writes_to_group(0),
				migrated_in(0), migrated_out(0) {}
		int num_gc_in_group, num_writes_to_group, num_gc_writes_to_group;
		int migrated_in, migrated_out;
		void print() const;
	};

	int num_pages;
	double actual_prob;
	group_stats stats;
	static vector<int> mapping_pages_to_groups;

	static int num_groups_that_need_more_blocks, num_groups_that_need_less_blocks;

	vector<vector<int> > num_pages_per_die, num_blocks_per_die, num_blocks_ever_given;
	vector<vector<vector<Block*> > > blocks_queue_per_die;
	StatisticsGatherer stats_gatherer;
	int id;
	Ssd* ssd;
};

// A temperature detector interface to be used by Block_Manager_Groups
class temperature_detector {
public:
	temperature_detector(vector<group>& groups) : groups(groups) {};
	virtual ~temperature_detector() {}
	virtual int which_group_should_this_page_belong_to(Event const& event) = 0;
	virtual void register_write_completed(Event const& event, int prior_group, int group_id) { }
	virtual void change_in_groups(vector<group> const& groups) {}
protected:
	vector<group>& groups;
};

// Conceptually and oracle that uses tags on a page to infer which group the page belongs to
class tag_detector : public temperature_detector {
public:
	tag_detector(vector<group>& groups) : temperature_detector(groups) {};
	int which_group_should_this_page_belong_to(Event const& event);
};



// A BM that seperates blocks based on tags
class Block_Manager_Groups : public Block_manager_parent {
public:
	Block_Manager_Groups();
	~Block_Manager_Groups();
	void init(Ssd*, FtlParent*, IOScheduler*, Garbage_Collector*, Wear_Leveling_Strategy*, Migrator*);
	void init_detector();
	void register_write_arrival(Event const& e);
	void register_write_outcome(Event const& event, enum status status);
	void register_erase_outcome(Event const& event, enum status status);
	void trigger_gc_in_same_lun_but_different_group(int package, int die, int group_id, double time);
	void handle_block_out_of_space(Event const& event, int group_id);
	void receive_message(Event const& message);
	void change_update_frequencies(Groups_Message const& message);
	void check_if_should_trigger_more_GC(Event const&);
	bool try_to_allocate_block_to_group(int group_id, int package, int die, double time);
	bool may_garbage_collect_this_block(Block* block, double current_time);
	void register_logical_address(Event const& event, int group_id);
    friend class boost::serialization::access;
    void print();
    bool is_in_equilibrium() const;
    void add_group(double starting_prob_val = 0);
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & boost::serialization::base_object<Block_manager_parent>(*this);
    }
    static int detector_type;
protected:
	Address choose_best_address(Event& write);
	Address choose_any_address(Event const& write);
private:
	void request_gc(int group_id, int package, int die, double time);
	vector<group> groups;
	struct stats {
		stats() : num_group_misses(0) {}
		int num_group_misses;
	};
	stats stats;
	temperature_detector* detector;
};

// Conceptually and oracle that uses tags on a page to infer which group the page belongs to
class bloom_detector : public temperature_detector {
public:
	bloom_detector(vector<group>& groups, Block_Manager_Groups* bm);
	virtual ~bloom_detector() {};
	int which_group_should_this_page_belong_to(Event const& event);
	void change_in_groups(vector<group> const& groups);
	virtual void register_write_completed(Event const& event, int prior_group, int new_group_id);

private:
	int get_interval_length() { return NUMBER_OF_ADDRESSABLE_PAGES() * OVER_PROVISIONING_FACTOR * interval_size_of_the_lba_space; }
	void update_probilities();
	void group_interval_finished(int group_id);
	struct group_data {
		group_data(group const& group_ref, vector<group> const& data);
		bloom_filter current_filter, filter2, filter3;
		int bloom_filter_hits;
		int interval_hit_count;
		double update_probability;
		inline double get_hits_per_page() const { return update_probability / groups[id].size; }
		inline group get_group() { return groups[id]; }
		const int id;
		int lower_group_id, upper_group_id;
		const vector<group>& groups;
	};
private:
	vector<group_data*> data;	// sorted by group update probability
	Block_Manager_Groups* bm;
	int current_interval_counter;
	const double interval_size_of_the_lba_space;
	group_data* highest_group, *lowest_group;
};


// A simple BM that assigns writes sequentially to dies in a round-robin fashion. No hot-cold separation or anything else intelligent
class Block_manager_roundrobin : public Block_manager_parent {
public:
	Block_manager_roundrobin(bool channel_alternation = true);
	~Block_manager_roundrobin();
	void register_write_outcome(Event const& event, enum status status);
	void register_erase_outcome(Event const& event, enum status status);
protected:
	Address choose_best_address(Event& write);
	Address choose_any_address(Event const& write);
private:
	void move_address_cursor();
	Address address_cursor;
	bool channel_alternation;
};

// A BM that assigns each write to the die with the shortest queue, as well as hot-cold seperation
class Shortest_Queue_Hot_Cold_BM : public Block_manager_parent {
public:
	Shortest_Queue_Hot_Cold_BM();
	~Shortest_Queue_Hot_Cold_BM();
	void register_write_outcome(Event const& event, enum status status);
	void register_read_command_outcome(Event const& event, enum status status);
	void register_erase_outcome(Event const& event, enum status status);
protected:
	Address choose_best_address(Event& write);
	virtual Address choose_any_address(Event const& write);
	void check_if_should_trigger_more_GC(Event const& event);
private:
	void handle_cold_pointer_out_of_space(double start_time);
	BloomFilter_Page_Hotness_Measurer page_hotness_measurer;
	Address cold_pointer;
};


class Wearwolf : public Block_manager_parent {
public:
	Wearwolf();
	~Wearwolf();
	virtual void register_write_outcome(Event const& event, enum status status);
	virtual void register_read_command_outcome(Event const& event, enum status status);
	virtual void register_erase_outcome(Event const& event, enum status status);
protected:
	virtual void check_if_should_trigger_more_GC(Event const&);
	virtual Address choose_best_address(Event& write);
	virtual Address choose_any_address(Event const& write);
	Page_Hotness_Measurer* page_hotness_measurer;
private:
	void handle_cold_pointer_out_of_space(enum read_hotness rh, double start_time);
	void reset_any_filled_pointers(Event const& event);
	Address wcrh_pointer;
	Address wcrc_pointer;
};

class Sequential_Pattern_Detector_Listener {
public:
	virtual ~Sequential_Pattern_Detector_Listener() {}
	virtual void sequential_event_metadata_removed(long key, double current_time) = 0;
};

struct sequential_writes_tracking {
	int counter, num_times_pattern_has_repeated;
	long key;
	double last_arrival_timestamp;
	double const init_timestamp;
	ulong last_io_num;
	sequential_writes_tracking(double time, long key);
};

class Sequential_Pattern_Detector {
public:
	typedef ulong logical_address;
	Sequential_Pattern_Detector(uint threshold);
	~Sequential_Pattern_Detector();
	sequential_writes_tracking const& register_event(logical_address lb, double time);
	void set_listener(Sequential_Pattern_Detector_Listener * listener);
	void remove_old_sequential_writes_metadata(double time);
private:
	map<logical_address, logical_address> sequential_writes_key_lookup;  // a map from the next expected LBA in a seqeuntial pattern to the first LBA, which is the key
	map<logical_address, sequential_writes_tracking*> sequential_writes_identification_and_data;	// a map from the first logical write of a sequential pattern to metadata about the pattern
	sequential_writes_tracking* restart_pattern(int key, double time);
	sequential_writes_tracking* process_next_write(int lb, double time);
	sequential_writes_tracking* init_pattern(int lb, double time);
	uint registration_counter;
	Sequential_Pattern_Detector_Listener* listener;
	uint threshold;
	ulong io_num;
};

class Sequential_Locality_BM : public Block_manager_parallel, public Sequential_Pattern_Detector_Listener {
public:
	Sequential_Locality_BM();
	~Sequential_Locality_BM();
	void register_write_arrival(Event const& write);
	void register_write_outcome(Event const& event, enum status status);
	void register_erase_outcome(Event const& event, enum status status);
	void sequential_event_metadata_removed(long key, double current_time);
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & boost::serialization::base_object<Block_manager_parent>(*this);
    }
protected:
	Address choose_best_address(Event& write);
	Address choose_any_address(Event const& write);
private:
	enum parallel_degree_for_sequential_files { ONE, LUN, CHANNEL };
	parallel_degree_for_sequential_files parallel_degree;

	struct sequential_writes_pointers {
		int num_pointers;
		vector<vector<Address> > pointers;
		uint cursor;
		int tag;
		sequential_writes_pointers();
	};

	struct tagged_sequential_write {
		int key, size, free_allocated_space, num_written;
		tagged_sequential_write() : key(-1), size(-1), free_allocated_space(0), num_written(0) {}
		tagged_sequential_write(int key, int size) : key(key), size(size), free_allocated_space(0), num_written(0) {}
		bool need_more_space() {	return is_finished() ? false : size > free_allocated_space; }
		bool is_finished() {	return num_written == size; }
	};

	map<long, sequential_writes_pointers> seq_write_key_to_pointers_mapping;

	void set_pointers_for_sequential_write(long key, double time);
	void set_pointers_for_tagged_sequential_write(int tag, double time);
	Address perform_sequential_write(Event& event, long key);
	Address perform_sequential_write_shortest_queue(sequential_writes_pointers& swp);
	Address perform_sequential_write_round_robin(sequential_writes_pointers& swp);
	void process_write_completion(Event const& event, long key, pair<long, long> index);

	Sequential_Pattern_Detector* detector;

	enum strategy {SHOREST_QUEUE, ROUND_ROBIN};
	strategy strat;

	map<long, tagged_sequential_write> tag_map; // maps from tags of sequential writes to the size of the sequential write
	map<long, long> arrived_writes_to_sequential_key_mapping;

	void print_tags(); // to be removed
	MTRand_int32 random_number_generator;
	int num_hits;
	int num_misses;
};

class Garbage_Collector {
public:
	Garbage_Collector() : ssd(NULL), bm(NULL), num_age_classes(1) {}
	Garbage_Collector(Ssd* ssd, Block_manager_parent* bm) : ssd(ssd), bm(bm), num_age_classes(bm->get_num_age_classes()) {}
	virtual ~Garbage_Collector() {}
	virtual void register_trim(Event const& event, uint age_class, int num_live_pages) = 0;
	virtual Block* choose_gc_victim(int package_id, int die_id, int klass) const = 0;
	virtual void remove_as_gc_candidate(Address const& phys_address) = 0;
	virtual void commit_choice_of_victim(Address const& phys_address) = 0;
	void set_block_manager(Block_manager_parent* b) { bm = b; }
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & ssd;
    	ar & bm;
    	ar & num_age_classes;
    }
protected:
	Ssd* ssd;
	Block_manager_parent* bm;
	int num_age_classes;
};

// The garbage collector organizes blocks in a data structure that is convenient for choosing which block to garbage-collect next
// This organization happens within the gc_candidates structure.
// Blocks that are candidates for garbage collection are first organized based on which package and die they belong to.
// Within the each die, they are further divided how old they are (i.e. how many erases they have experienced).
// The variable num_age_classes controls how many groups we use for blocks of different ages.
// Note that the block manager maintains a free block for every age class in every LUN. This allows us to implement efficient
// dynamic wear-leveling by putting pages of a certain temperature in blocks of a certain age.
class Garbage_Collector_Greedy : public Garbage_Collector {
public:
	Garbage_Collector_Greedy();
	Garbage_Collector_Greedy(Ssd* ssd, Block_manager_parent* bm);
	// Called by the block manager after any page in the SSD is invalidated, as a result of a trim or a write.
	// This is used to keep the gc_candidates structure updated.
	void register_trim(Event const& event, uint age_class, int num_live_pages);
	// Called by the block manager to ask the garbage-collector for a good block to garbage-collect in a given package, die, and with a certain age.
	Block* choose_gc_victim(int package_id, int die_id, int klass) const;
	// Called by the block manager when a GC operation for a certain block has been issued. This block is removed from the gc_candidates structure.
	void remove_as_gc_candidate(Address const& phys_address);
	void commit_choice_of_victim(Address const& phys_address) {remove_as_gc_candidate(phys_address);}
	friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & boost::serialization::base_object<Garbage_Collector>(*this);
    	ar & gc_candidates;
    }
private:
	vector<long> get_relevant_gc_candidates(int package_id, int die_id, int klass) const;
	vector<vector<vector<set<long> > > > gc_candidates;  // each age class has a vector of candidates for GC
};

class Garbage_Collector_LRU : public Garbage_Collector {
public:
	Garbage_Collector_LRU();
	Garbage_Collector_LRU(Ssd* ssd, Block_manager_parent* bm);
	// Called by the block manager after any page in the SSD is invalidated, as a result of a trim or a write.
	// This is used to keep the gc_candidates structure updated.
	void register_trim(Event const& event, uint age_class, int num_live_pages) {}
	// Called by the block manager to ask the garbage-collector for a good block to garbage-collect in a given package, die, and with a certain age.
	Block* choose_gc_victim(int package_id, int die_id, int klass) const;
	// Called by the block manager when a GC operation for a certain block has been issued. This block is removed from the gc_candidates structure.
	void remove_as_gc_candidate(Address const& phys_address) {}
	void commit_choice_of_victim(Address const& phys_address);
	friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	ar & boost::serialization::base_object<Garbage_Collector>(*this);
    	ar & gc_candidates;
    }
private:
	vector<vector<int> > gc_candidates;  // each age class has a vector of candidates for GC
};


}
#endif /* BLOCK_MANAGEMENT_H_ */
