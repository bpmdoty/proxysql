#ifndef CLASS_PROXYSQL_CLUSTER_H
#define CLASS_PROXYSQL_CLUSTER_H
#include "proxysql.h"
#include "cpp.h"
#include "thread.h"
#include "wqueue.h"
#include <vector>

#include <prometheus/counter.h>
#include <prometheus/gauge.h>

#define PROXYSQL_NODE_METRICS_LEN	5

#define CLUSTER_QUERY_MYSQL_SERVERS "SELECT hostgroup_id, hostname, port, gtid_port, status, weight, compression, max_connections, max_replication_lag, use_ssl, max_latency_ms, comment FROM runtime_mysql_servers WHERE status<>'OFFLINE_HARD'"
#define CLUSTER_QUERY_MYSQL_REPLICATION_HOSTGROUPS "SELECT writer_hostgroup, reader_hostgroup, comment FROM runtime_mysql_replication_hostgroups"

class ProxySQL_Checksum_Value_2: public ProxySQL_Checksum_Value {
	public:
	time_t last_updated;
	time_t last_changed;
	unsigned int diff_check;
	ProxySQL_Checksum_Value_2() {
		ProxySQL_Checksum_Value();
		last_changed = 0;
		last_updated = 0;
		diff_check = 0;
	}
};

class ProxySQL_Node_Metrics {
	public:
	unsigned long long read_time_us;
	unsigned long long response_time_us;
	unsigned long long ProxySQL_Uptime;
	unsigned long long Questions;
	unsigned long long Client_Connections_created;
	unsigned long long Client_Connections_connected;
	unsigned long long Servers_table_version;
	void reset();
	ProxySQL_Node_Metrics() {
		reset();
	}
	~ProxySQL_Node_Metrics() {
		reset();
	}
};

class ProxySQL_Node_Address {
	public:
	pthread_t thrid;
	uint64_t hash; // unused for now
	char *uuid;
	char *hostname;
	char *admin_mysql_ifaces;
	uint16_t port;
	ProxySQL_Node_Address(char *h, uint16_t p) {
		hostname = strdup(h);
		admin_mysql_ifaces = NULL;
		port = p;
		uuid = NULL;
		hash = 0;
	}
	~ProxySQL_Node_Address() {
		if (hostname) free(hostname);
		if (uuid) free(uuid);
		if (admin_mysql_ifaces) free(admin_mysql_ifaces);
	}
};

class ProxySQL_Node_Entry {
	private:
	uint64_t hash;
	char *hostname;
	uint16_t port;
	uint64_t weight;
	char *comment;
	uint64_t generate_hash();
	bool active;
	int metrics_idx_prev;
	int metrics_idx;
	ProxySQL_Node_Metrics **metrics;
//	void pull_mysql_query_rules_from_peer();
//	void pull_mysql_servers_from_peer();
//	void pull_proxysql_servers_from_peer();

	public:
	uint64_t get_hash();
	ProxySQL_Node_Entry(char *_hostname, uint16_t _port, uint64_t _weight, char *_comment);
	~ProxySQL_Node_Entry();
	bool get_active();
	void set_active(bool a);
	uint64_t get_weight();
	void set_weight(uint64_t a);
	char * get_comment() { // note, NO strdup()
		return comment;
	}
	void set_comment(char *a); // note, this is strdup()
	void set_metrics(MYSQL_RES *_r, unsigned long long _response_time);
	void set_checksums(MYSQL_RES *_r);
	char *get_hostname() { // note, NO strdup()
		return hostname;
	}
	uint16_t get_port() {
		return port;
	}
	ProxySQL_Node_Metrics * get_metrics_curr();
	ProxySQL_Node_Metrics * get_metrics_prev();
	struct {
		ProxySQL_Checksum_Value_2 admin_variables;
		ProxySQL_Checksum_Value_2 mysql_variables;
		ProxySQL_Checksum_Value_2 ldap_variables;
		ProxySQL_Checksum_Value_2 mysql_query_rules;
		ProxySQL_Checksum_Value_2 mysql_servers;
		ProxySQL_Checksum_Value_2 mysql_users;
		ProxySQL_Checksum_Value_2 proxysql_servers;
	} checksums_values;
	uint64_t global_checksum;
};

class ProxySQL_Cluster_Nodes {
	private:
	pthread_mutex_t mutex;
	std::unordered_map<uint64_t, ProxySQL_Node_Entry *> umap_proxy_nodes;
	void set_all_inactive();
	void remove_inactives();
	uint64_t generate_hash(char *_hostname, uint16_t _port);
	public:
	ProxySQL_Cluster_Nodes();
	~ProxySQL_Cluster_Nodes();
	void load_servers_list(SQLite3_result *, bool _lock);
	bool Update_Node_Metrics(char * _h, uint16_t _p, MYSQL_RES *_r, unsigned long long _response_time);
	bool Update_Global_Checksum(char * _h, uint16_t _p, MYSQL_RES *_r);
	bool Update_Node_Checksums(char * _h, uint16_t _p, MYSQL_RES *_r);
	SQLite3_result * dump_table_proxysql_servers();
	SQLite3_result * stats_proxysql_servers_checksums();
	SQLite3_result * stats_proxysql_servers_metrics();
	void get_peer_to_sync_mysql_query_rules(char **host, uint16_t *port);
	void get_peer_to_sync_mysql_servers(char **host, uint16_t *port, char **peer_checksum);
	void get_peer_to_sync_mysql_users(char **host, uint16_t *port);
	void get_peer_to_sync_mysql_variables(char **host, uint16_t *port);
	void get_peer_to_sync_admin_variables(char **host, uint16_t* port);
	void get_peer_to_sync_ldap_variables(char **host, uint16_t *port);
	void get_peer_to_sync_proxysql_servers(char **host, uint16_t *port);
};

struct p_cluster_counter {
	enum metric {
		pulled_mysql_query_rules_success = 0,
		pulled_mysql_query_rules_failure,

		pulled_mysql_servers_success,
		pulled_mysql_servers_failure,
		pulled_mysql_servers_replication_hostgroups_success,
		pulled_mysql_servers_replication_hostgroups_failure,
		pulled_mysql_servers_group_replication_hostgroups_success,
		pulled_mysql_servers_group_replication_hostgroups_failure,
		pulled_mysql_servers_galera_hostgroups_success,
		pulled_mysql_servers_galera_hostgroups_failure,
		pulled_mysql_servers_aws_aurora_hostgroups_success,
		pulled_mysql_servers_aws_aurora_hostgroups_failure,
		pulled_mysql_servers_runtime_checks_success,
		pulled_mysql_servers_runtime_checks_failure,

		pulled_mysql_users_success,
		pulled_mysql_users_failure,

		pulled_proxysql_servers_success,
		pulled_proxysql_servers_failure,

		pulled_mysql_variables_success,
		pulled_mysql_variables_failure,

		pulled_admin_variables_success,
		pulled_admin_variables_failure,

		pulled_ldap_variables_success,
		pulled_ldap_variables_failure,

		pulled_mysql_ldap_mapping_success,
		pulled_mysql_ldap_mapping_failure,

		sync_conflict_mysql_query_rules_share_epoch,
		sync_conflict_mysql_servers_share_epoch,
		sync_conflict_proxysql_servers_share_epoch,
		sync_conflict_mysql_users_share_epoch,
		sync_conflict_mysql_variables_share_epoch,
		sync_conflict_admin_variables_share_epoch,
		sync_conflict_ldap_variables_share_epoch,

		sync_delayed_mysql_query_rules_version_one,
		sync_delayed_mysql_servers_version_one,
		sync_delayed_mysql_users_version_one,
		sync_delayed_proxysql_servers_version_one,
		sync_delayed_mysql_variables_version_one,
		sync_delayed_admin_variables_version_one,
		sync_delayed_ldap_variables_version_one,

		__size
	};
};

struct p_cluster_gauge {
	enum metric {
		__size
	};
};

struct cluster_metrics_map_idx {
	enum index {
		counters = 0,
		gauges
	};
};

struct variable_type {
	enum type {
		mysql,
		admin,
		__size
	};
};

/**
 * @brief Simple struct for holding a query, and three messages to report
 *  the progress of the query execution.
 */
struct fetch_query {
	const char* query;
	p_cluster_counter::metric success_counter;
	p_cluster_counter::metric failure_counter;
	std::string msgs[3];
};

class ProxySQL_Cluster {
	private:
	pthread_mutex_t mutex;
	std::vector<pthread_t> term_threads;
	ProxySQL_Cluster_Nodes nodes;
	char *cluster_username;
	char *cluster_password;
	struct {
		std::array<prometheus::Counter*, p_cluster_counter::__size> p_counter_array {};
		std::array<prometheus::Gauge*, p_cluster_gauge::__size> p_gauge_array {};
	} metrics;
	int fetch_and_store(MYSQL* conn, const fetch_query& f_query, MYSQL_RES** result);
	friend class ProxySQL_Node_Entry;
	public:
	pthread_mutex_t update_mysql_query_rules_mutex;
	pthread_mutex_t update_mysql_servers_mutex;
	pthread_mutex_t update_mysql_users_mutex;
	pthread_mutex_t update_mysql_variables_mutex;
	pthread_mutex_t update_proxysql_servers_mutex;
	// this records the interface that Admin is listening to
	pthread_mutex_t admin_mysql_ifaces_mutex;
	char *admin_mysql_ifaces;
	int cluster_check_interval_ms;
	int cluster_check_status_frequency;
	int cluster_mysql_query_rules_diffs_before_sync;
	int cluster_mysql_servers_diffs_before_sync;
	int cluster_mysql_users_diffs_before_sync;
	int cluster_proxysql_servers_diffs_before_sync;
	int cluster_mysql_variables_diffs_before_sync;
	int cluster_ldap_variables_diffs_before_sync;
	int cluster_admin_variables_diffs_before_sync;
	bool cluster_mysql_query_rules_save_to_disk;
	bool cluster_mysql_servers_save_to_disk;
	bool cluster_mysql_users_save_to_disk;
	bool cluster_proxysql_servers_save_to_disk;
	bool cluster_mysql_variables_save_to_disk;
	bool cluster_ldap_variables_save_to_disk;
	bool cluster_admin_variables_save_to_disk;
	ProxySQL_Cluster();
	~ProxySQL_Cluster();
	void init() {};
	void print_version();
	void load_servers_list(SQLite3_result *r, bool _lock = true) {
		nodes.load_servers_list(r, _lock);
	}
	void get_credentials(char **, char **);
	void set_username(char *);
	void set_password(char *);
	void set_admin_mysql_ifaces(char *);
	bool Update_Node_Metrics(char * _h, uint16_t _p, MYSQL_RES *_r, unsigned long long _response_time) {
		return nodes.Update_Node_Metrics(_h, _p, _r, _response_time);
	}
	bool Update_Global_Checksum(char * _h, uint16_t _p, MYSQL_RES *_r) {
		return nodes.Update_Global_Checksum(_h, _p, _r);
	}
	bool Update_Node_Checksums(char * _h, uint16_t _p, MYSQL_RES *_r = NULL) {
		return nodes.Update_Node_Checksums(_h, _p, _r);
	}
	SQLite3_result *dump_table_proxysql_servers() {
		return nodes.dump_table_proxysql_servers();
	}
	SQLite3_result * get_stats_proxysql_servers_checksums() {
		return nodes.stats_proxysql_servers_checksums();
	}
	SQLite3_result * get_stats_proxysql_servers_metrics() {
		return nodes.stats_proxysql_servers_metrics();
	}
	void thread_ending(pthread_t);
	void join_term_thread();
	void pull_mysql_query_rules_from_peer();
	void pull_mysql_servers_from_peer();
	void pull_mysql_users_from_peer();
	/**
	 * @brief Pulls from peer the specified global variables by the type parameter.
	 * @param type A string specifying the type of global variables to pull from the peer, supported
	 *  values right now are:
	 *    - 'mysql'.
     *    - 'admin'.
	 */
	void pull_global_variables_from_peer(const std::string& type);
	void pull_proxysql_servers_from_peer(const char *expected_checksum);
};
#endif /* CLASS_PROXYSQL_CLUSTER_H */
