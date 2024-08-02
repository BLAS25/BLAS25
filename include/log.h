
#ifndef _LOG_H_
#define _LOG_H_

#include <vector>
#include <string>
#include <sstream>
#include <bitset>
#include <algorithm>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>

#include "./util.h"
#include "./mbt.h"
#include "./merkle_tree.h"

using namespace std;

#define ONE_TX_ENTRY_NUM 2048

#define ONE_BLOCK_TX_NUM 256

#define ONE_WINDOW_BLOCK_NUM 1024

#define WINDOW_NUM 1

#define ALL_BLOCK_NUM (ONE_WINDOW_BLOCK_NUM * WINDOW_NUM)

#define ALL_LOG_NUM (ALL_BLOCK_NUM * ONE_BLOCK_TX_NUM * ONE_TX_ENTRY_NUM)
#define LG_NUM 4096

#define FAN_OUT 4

#define U_INT_MAX numeric_limits<unsigned int>::max()
#define U_INT_MIN numeric_limits<unsigned int>::min()
using namespace std;

void load_log_from_file(string entry_file_path, string timestamp_path, string key_path, unsigned int entry_number,
                        vector<struct Entry *> &entrys);
ostream &operator<<(ostream &os, const struct Entry &l);
string general_get_rimbt_VO(MerkleBTree *range_mbt, unsigned int left, unsigned int right, vector<string> &nodeJsons);
bool general_verify_rimbt_VO(MerkleBTree *range_mbt, string VO);
// VO
struct VO {
    // KB
    double block_VO_size;
    // ms
    double block_query_time;
    // KB
    double tx_VO_size;
    // ms
    double tx_query_time;
    // KB
    double entry_VO_size;
    // ms
    double entry_query_time;
    double all_VO_size;
    double all_query_time;
};

struct EntryKeyBitmap {
    string key;
    bitset<ONE_TX_ENTRY_NUM> bitmap;
    EntryKeyBitmap() {
    }
    EntryKeyBitmap(string _key, bitset<ONE_TX_ENTRY_NUM> _bitmap) {
        key = _key;
        bitmap = _bitmap;
    }
    template <typename Archive>
    void serialize(Archive &archive, const unsigned int version) {
        archive &BOOST_SERIALIZATION_NVP(key);
        archive &BOOST_SERIALIZATION_NVP(bitmap);
    }

    static string toString(const struct EntryKeyBitmap &setting) {
        std::ostringstream oss;
        boost::archive::text_oarchive oa(oss);
        oa << setting;
        return oss.str();
    }

    static struct EntryKeyBitmap fromString(const std::string &data) {
        struct EntryKeyBitmap setting;
        std::istringstream iss(data);
        boost::archive::text_iarchive ia(iss);
        ia >> setting;
        return setting;
    }
};
// EntryKeyBitmap::EntryKeyBitmap() {}
struct TxKeyBitmap {
    string key;
    bitset<ONE_BLOCK_TX_NUM> bitmap;
    TxKeyBitmap() {
    }
    TxKeyBitmap(string _key, bitset<ONE_BLOCK_TX_NUM> _bitmap) {
        key = _key;
        bitmap = _bitmap;
    }
    template <typename Archive>
    void serialize(Archive &archive, const unsigned int version) {
        archive &BOOST_SERIALIZATION_NVP(key);
        archive &BOOST_SERIALIZATION_NVP(bitmap);
    }
    static string toString(const struct TxKeyBitmap &setting) {
        std::ostringstream oss;
        boost::archive::text_oarchive oa(oss);
        oa << setting;
        return oss.str();
    }

    static struct TxKeyBitmap fromString(const std::string &data) {
        struct TxKeyBitmap setting;
        std::istringstream iss(data);
        boost::archive::text_iarchive ia(iss);
        ia >> setting;
        return setting;
    }
};
// TxKeyBitmap::TxKeyBitmap() {}

struct BlockKeyBitmap {
    string key;
    bitset<ONE_WINDOW_BLOCK_NUM> bitmap;
    BlockKeyBitmap() {
    }
    BlockKeyBitmap(string _key, bitset<ONE_WINDOW_BLOCK_NUM> _bitmap) {
        key = _key;
        bitmap = _bitmap;
    }
    template <typename Archive>
    void serialize(Archive &archive, const unsigned int version) {
        archive &BOOST_SERIALIZATION_NVP(key);
        archive &BOOST_SERIALIZATION_NVP(bitmap);
    }

    static string toString(const struct BlockKeyBitmap &setting) {
        std::ostringstream oss;
        boost::archive::text_oarchive oa(oss);
        oa << setting;
        return oss.str();
    }

    static struct BlockKeyBitmap fromString(const std::string &data) {
        struct BlockKeyBitmap setting;
        std::istringstream iss(data);
        boost::archive::text_iarchive ia(iss);
        ia >> setting;
        return setting;
    }
};
// BlockKeyBitmap::BlockKeyBitmap() {}

struct Entry {
    unsigned int sn;              // serial number
    unsigned long long timestamp; // timestamp
    vector<string> keywords;      // keyword list
    string log_value;
    Entry() {
    }
    Entry(unsigned int _sn, unsigned int _timestamp, const vector<string> &_keywords, const string &_log_value) {
        sn = _sn;
        timestamp = _timestamp;
        keywords = _keywords;
        log_value = _log_value;
    }
    template <typename Archive>
    void serialize(Archive &archive, const unsigned int version) {
        archive &BOOST_SERIALIZATION_NVP(sn);
        archive &BOOST_SERIALIZATION_NVP(timestamp);
        archive &BOOST_SERIALIZATION_NVP(keywords);
        archive &BOOST_SERIALIZATION_NVP(log_value);
    }
    static string toString(const struct Entry &e) {
        std::ostringstream oss;
        boost::archive::text_oarchive oa(oss);
        oa << e;
        return oss.str();
    }

    static struct Entry fromString(const std::string &data) {
        struct Entry e;
        std::istringstream iss(data);
        boost::archive::text_iarchive ia(iss);
        ia >> e;
        return e;
    }
};

struct LidKeyBitmap {
    unsigned int key;
    bitset<ONE_WINDOW_BLOCK_NUM> bitmap;
    string mht_root;
    LidKeyBitmap() {
    }
    LidKeyBitmap(unsigned int _key, bitset<ONE_WINDOW_BLOCK_NUM> _bitmap) {
        key = _key;
        bitmap = _bitmap;
    }
    template <typename Archive>
    void serialize(Archive &archive, const unsigned int version) {
        archive &BOOST_SERIALIZATION_NVP(key);
        archive &BOOST_SERIALIZATION_NVP(bitmap);
        archive &BOOST_SERIALIZATION_NVP(mht_root);
    }

    static string toString(const struct LidKeyBitmap &setting) {
        std::ostringstream oss;
        boost::archive::text_oarchive oa(oss);
        oa << setting;
        return oss.str();
    }

    static struct LidKeyBitmap fromString(const std::string &data) {
        struct LidKeyBitmap setting;
        std::istringstream iss(data);
        boost::archive::text_iarchive ia(iss);
        ia >> setting;
        return setting;
    }
};

struct LidTreeNode {
    unsigned int blockid;
    unsigned int txid;
    string tx;

    LidTreeNode() {
    }
    template <typename Archive>
    void serialize(Archive &archive, const unsigned int version) {
        archive &BOOST_SERIALIZATION_NVP(blockid);
        archive &BOOST_SERIALIZATION_NVP(txid);
        archive &BOOST_SERIALIZATION_NVP(tx);
    }

    static string toString(const struct LidTreeNode &setting) {
        std::ostringstream oss;
        boost::archive::text_oarchive oa(oss);
        oa << setting;
        return oss.str();
    }

    static struct LidTreeNode fromString(const std::string &data) {
        struct LidTreeNode setting;
        std::istringstream iss(data);
        boost::archive::text_iarchive ia(iss);
        ia >> setting;
        return setting;
    }
};

struct GlobalKeyTreeNode {
    string key;
    string mht_root;
    GlobalKeyTreeNode() {
    }
    template <typename Archive>
    void serialize(Archive &archive, const unsigned int version) {
        archive &BOOST_SERIALIZATION_NVP(key);
        archive &BOOST_SERIALIZATION_NVP(mht_root);
    }

    static string toString(const struct GlobalKeyTreeNode &setting) {
        std::ostringstream oss;
        boost::archive::text_oarchive oa(oss);
        oa << setting;
        return oss.str();
    }

    static struct GlobalKeyTreeNode fromString(const std::string &data) {
        struct GlobalKeyTreeNode setting;
        std::istringstream iss(data);
        boost::archive::text_iarchive ia(iss);
        ia >> setting;
        return setting;
    }
};
// Entry::Entry() {}
// Log files, SP storage, not on-chain
class LogFile {
public:
    unsigned int lid;                  // Log generator ID
    unsigned long long t_min;          // Minimum time of all entries
    unsigned long long t_max;          // Maximum time of all entries
    vector<string> tx_public_keywords; // Log file common keywords
    vector<string> entry_keywords;     // The log file contains a list of private keywords for all logs.
    MerkleTree *omt;                   // MHT tree composed of all entries
    MerkleTree *iimt;                  // Keyword index tree
    MerkleBTree *rimbt;                // Time Range Query Tree
    vector<string> entrys;             // All log entries contained in the log file are not uploaded to the chain
    vector<string> kbms; // Keyword query key||bitmap list contained in the log file is not uploaded to the chain
public:
    bool operator==(const LogFile &lf) const {
        return (omt->root->hash == lf.omt->root->hash);
    }
    bool operator!=(const LogFile &lf) const {
        return (omt->root->hash != lf.omt->root->hash);
    }
    bool operator<(const LogFile &lf) {
        return (stoi(string2Hex(omt->root->hash), 0, 16) < stoi(string2Hex(lf.omt->root->hash), 0, 16));
    }
    bool operator>(const LogFile &lf) {
        return (stoi(string2Hex(omt->root->hash)) > stoi(string2Hex(lf.omt->root->hash)));
    }
    template <class Archive>
    void serialize(Archive &archive, const unsigned version) {
        archive & lid & t_min & t_max & tx_public_keywords & entry_keywords & iimt & omt & rimbt & entrys & kbms;
    }
    LogFile() {
    }
    LogFile(unsigned int lid, vector<string> tx_public_keys, vector<struct Entry *> entrys);
    ~LogFile() {
        delete this->omt;
        delete this->iimt;
        delete this->rimbt;
    }

    // storage_mt_VO
    void get_omt_VO(vector<unsigned int> indexs, vector<bool> &bitmap, vector<string> &path) {
        sort(indexs.begin(), indexs.end());
        vector<string> target_hashs;
        for (auto &e : indexs) { target_hashs.push_back(my_blakes(this->entrys[e])); }
        this->omt->generateVO(indexs, target_hashs, bitmap, path);
    }
    unsigned int get_size() {
        return (sizeof(lid) + sizeof(t_min) + sizeof(t_max) + omt->get_size() + iimt->get_size() + rimbt->get_size());
    }
    // Verify that the path is correct
    bool omt_VO_verify(vector<bool> &bitmap, vector<string> &path) {
        return verify_VO(bitmap, path, this->omt->root->hash);
    }
    // Get the keyword tree verification path from the element index
    void get_iimt_VO(vector<unsigned int> indexs, vector<bool> &bitmap, vector<string> &path) {
        sort(indexs.begin(), indexs.end());
        vector<string> target_hashs;
        for (auto &e : indexs) { target_hashs.push_back(my_blakes(this->kbms[e])); }
        this->iimt->generateVO(indexs, target_hashs, bitmap, path);
    }
    // Verify that the path is correct
    bool iimt_verify_VO(vector<bool> &bitmap, vector<string> &path) {
        return verify_VO(bitmap, path, this->iimt->root->hash);
    }

    // Get time range query verification VO
    string get_rimbt_VO(unsigned int left, unsigned int right, vector<string> &nodeJsons) {
        // this->rimbt->printKeys();
        return general_get_rimbt_VO(this->rimbt, left, right, nodeJsons);
    }
    // Time range query verification VO
    bool timbt_VO_verify(string VO) {
        return general_verify_rimbt_VO(this->rimbt, VO);
    }
};

// The transaction data structure on the chain is constructed according to the log file
struct Transaction {
    unsigned int lid;              // Log generator ID
    unsigned long long t_min;      // Minimum time of all entries
    unsigned long long t_max;      // Maximum time of all entries
    vector<string> tx_public_keys; // The common keywords included in this transaction
    vector<string> entry_keywords; // List of all private keywords in the transaction corresponding log file
    string omt_root;               // omt root
    string iimt_root;              // iimt root
    string rimbt_root;             // rimbt root
    Transaction() {
    }
    Transaction(LogFile *lf) {
        lid = lf->lid;
        t_min = lf->t_min;
        t_max = lf->t_max;
        tx_public_keys = lf->tx_public_keywords;
        entry_keywords = lf->entry_keywords;
        omt_root = lf->omt->root->hash;
        iimt_root = lf->iimt->root->hash;
        rimbt_root = lf->rimbt->calculateRootDigest();
    }
    template <class Archive>
    void serialize(Archive &archive, const unsigned version) {
        archive & lid & t_min & t_max & tx_public_keys & entry_keywords & omt_root & iimt_root & rimbt_root;
    }
    static string toString(const struct Transaction &e) {
        std::ostringstream oss;
        boost::archive::text_oarchive oa(oss);
        oa << e;
        return oss.str();
    }

    static struct Transaction fromString(const std::string &data) {
        struct Transaction e;
        std::istringstream iss(data);
        boost::archive::text_iarchive ia(iss);
        ia >> e;
        return e;
    }
};

// The block containing all tree structures
class FullBlock {
public:
    unsigned int blockid;              // Block height, similar to block ID
    unsigned long long t_min;          // The min of all t_min in this block
    unsigned long long t_max;          // The max of all t_max in this block
    vector<string> tx_public_keywords; // List of common keywords contained in all transactions in the block
    vector<unsigned int> lids;         // A collection of all transaction lids in the block
    MerkleTree *omt;                   // MHT composed of all tx
    MerkleTree *iimt;                  // Transaction-level keyword index tree within a block
    MerkleBTree *tmin_rimbt;           // Minimum time range query tree
    MerkleBTree *tmax_rimbt;           // Maximum time range query tree

    vector<string> txs;  // All transactions in the block
    vector<string> kbms; // Keyword query key||bitmap list contained in the transaction

    // The following structures are only relevant to block truncation
    MerkleTree *global_lidmt;       // Global lid index tree
    MerkleTree *global_iimt;        // Global public keyword index tree
    MerkleTree *global_new_iimt;    // Global new public keyword index tree
    MerkleBTree *global_tmin_rimbt; // Global minimum time range query tree
    MerkleBTree *global_tmax_rimbt; // Global maximum time range query tree
    vector<string> global_new_kbms; // Result after GlobalKeyTreeNode serialization
    vector<string> global_kbms;     // Block-level keyword query key||bitmap list
    vector<string> lidkbms;
    vector<vector<string>> lid_nodes; // Each lid has a bunch of nodes
    vector<MerkleTree *> lid_trees;   // By lid order
    vector<MerkleTree *> new_key_trees;
    vector<vector<string>> block_bm_keys; // In keyword order, the first layer is key and the second layer is block
    unsigned long long global_t_min;      // The min of all t_min in this window
    unsigned long long global_t_max;

public:
    template <class Archive>
    void serialize(Archive &archive, const unsigned version) {
        archive & blockid & t_min & t_max & tx_public_keywords & lids & omt & iimt & tmin_rimbt & tmax_rimbt & txs
            & kbms & lidkbms & global_lidmt & global_iimt & global_new_iimt & global_new_kbms & global_tmin_rimbt
            & global_tmax_rimbt & global_kbms & lid_nodes & lid_trees & new_key_trees & block_bm_keys & global_t_min
            & global_t_max;
    }
    FullBlock() {
    }
    FullBlock(unsigned int blockid, vector<struct Transaction *> txs, vector<string> public_keywords);
    // Get the storage tree verification path from the element index
    unsigned int get_size() {
        unsigned int size = 0;
        size = size + sizeof(blockid) + sizeof(t_min) + sizeof(t_max) + sizeof(global_t_min) + sizeof(global_t_max);
        size = size + omt->get_size() + iimt->get_size() + tmin_rimbt->get_size() + tmax_rimbt->get_size();
        if (global_lidmt != NULL) size = size + global_lidmt->get_size();
        if (global_iimt != NULL) size = size + global_iimt->get_size();
        if (global_new_iimt != NULL) size = size + global_new_iimt->get_size();
        if (global_tmin_rimbt != NULL) size = size + global_tmin_rimbt->get_size();
        if (global_tmax_rimbt != NULL) size = size + global_tmax_rimbt->get_size();
        for (auto e : lid_trees) {
            if (e != NULL) { size = size + e->get_size(); }
        }
        for (auto e : new_key_trees) {
            if (e != NULL) { size = size + e->get_size(); }
        }
        return size;
    }
    void get_omt_VO(vector<unsigned int> indexs, vector<bool> &bitmap, vector<string> &path) {
        sort(indexs.begin(), indexs.end());
        vector<string> target_hashs;
        for (auto &e : indexs) { target_hashs.push_back(my_blakes(this->txs[e])); }
        this->omt->generateVO(indexs, target_hashs, bitmap, path);
    }

    // Verify that the path is correct
    bool omt_VO_verify(vector<bool> &bitmap, vector<string> &path) {
        return verify_VO(bitmap, path, this->omt->root->hash);
    }
    // Get the keyword tree verification path from the element index
    void get_iimt_VO(vector<unsigned int> indexs, vector<bool> &bitmap, vector<string> &path) {
        sort(indexs.begin(), indexs.end());
        vector<string> target_hashs;
        for (auto &e : indexs) { target_hashs.push_back(my_blakes(this->kbms[e])); }
        this->iimt->generateVO(indexs, target_hashs, bitmap, path);
    }
    // Verify that the path is correct
    bool iimt_verify_VO(vector<bool> &bitmap, vector<string> &path) {
        return verify_VO(bitmap, path, this->iimt->root->hash);
    }

    // Get the tmin time range query VO,
    string get_tmin_rimbt_VO(unsigned int left, unsigned int right, vector<string> &nodeJsons) {
        return general_get_rimbt_VO(this->tmin_rimbt, U_INT_MIN, right, nodeJsons);
    }
    // Verify tmin time range query VO
    bool tmin_rimbt_VO_verify(string VO) {
        return general_verify_rimbt_VO(this->tmin_rimbt, VO);
    }

    // Get tmax time range query VO，
    string get_tmax_rimbt_VO(unsigned int left, unsigned int right, vector<string> &nodeJsons) {
        return general_get_rimbt_VO(this->tmax_rimbt, left, U_INT_MAX, nodeJsons);
    }
    // Verify tmax time range query VO
    bool tmax_rimbt_VO_verify(string VO) {
        return general_verify_rimbt_VO(this->tmax_rimbt, VO);
    }

    void global_get_iimt_VO(vector<unsigned int> indexs, vector<bool> &bitmap, vector<string> &path) {
        sort(indexs.begin(), indexs.end());
        vector<string> target_hashs;
        for (auto &e : indexs) {
            // cout << this->global_kbms[e].length() << endl;
            target_hashs.push_back(my_blakes(this->global_kbms[e]));
        }
        // cout << 1 << endl;
        this->global_iimt->generateVO(indexs, target_hashs, bitmap, path);
    }
    bool global_iimt_verify_VO(vector<bool> &bitmap, vector<string> &path) {
        return verify_VO(bitmap, path, this->global_iimt->root->hash);
    }
    void global_get_lidmt_VO(vector<unsigned int> indexs, vector<bool> &bitmap, vector<string> &path) {
        sort(indexs.begin(), indexs.end());
        vector<string> target_hashs;
        for (auto &e : indexs) {
            // cout << this->global_kbms[e].length() << endl;
            target_hashs.push_back(my_blakes(this->lidkbms[e]));
        }
        // cout << 1 << endl;
        this->global_lidmt->generateVO(indexs, target_hashs, bitmap, path);
    }
    bool global_lidmt_verify_VO(vector<bool> &bitmap, vector<string> &path) {
        return verify_VO(bitmap, path, this->global_lidmt->root->hash);
    }
    void global_get_down_lidmt_VO(unsigned int lid, vector<unsigned int> indexs, vector<bool> &bitmap,
                                  vector<string> &path) {
        sort(indexs.begin(), indexs.end());
        vector<string> target_hashs;
        for (auto &e : indexs) {
            // cout << this->global_kbms[e].length() << endl;
            target_hashs.push_back(my_blakes(this->lid_nodes.at(lid - 1)[e]));
        }
        // cout << 1 << endl;
        this->lid_trees.at(lid - 1)->generateVO(indexs, target_hashs, bitmap, path);
    }
    bool global_down_lidmt_verify_VO(unsigned int lid, vector<bool> &bitmap, vector<string> &path) {
        return verify_VO(bitmap, path, this->lid_trees.at(lid - 1)->root->hash);
    }
    void global_get_down_iimt_VO(unsigned int index, vector<unsigned int> indexs, vector<bool> &bitmap,
                                 vector<string> &path) {
        sort(indexs.begin(), indexs.end());
        vector<string> target_hashs;
        for (auto &e : indexs) {
            // cout << this->global_kbms[e].length() << endl;
            target_hashs.push_back(my_blakes(this->block_bm_keys.at(index)[e]));
        }
        this->new_key_trees.at(index)->generateVO(indexs, target_hashs, bitmap, path);
    }
    bool global_down_iimt_verify_VO(unsigned int index, vector<bool> &bitmap, vector<string> &path) {
        return verify_VO(bitmap, path, this->new_key_trees.at(index)->root->hash);
    }
    // Get tmin time range query VO
    string global_get_tmin_rimbt_VO(unsigned int left, unsigned int right, vector<string> &nodeJsons) {
        return general_get_rimbt_VO(this->global_tmin_rimbt, U_INT_MIN, right, nodeJsons);
    }
    // Verify tmin time range query VO
    bool global_tmin_rimbt_verify_VO(string VO) {
        return general_verify_rimbt_VO(this->global_tmin_rimbt, VO);
    }

    // Get tmax time range query VO
    string global_get_tmax_rimbt_VO(unsigned int left, unsigned int right, vector<string> &nodeJsons) {
        return general_get_rimbt_VO(this->global_tmax_rimbt, left, U_INT_MAX, nodeJsons);
    }
    // Verify tmax time range query VO
    bool global_tmax_rimbt_verify_VO(string VO) {
        return general_verify_rimbt_VO(this->global_tmax_rimbt, VO);
    }
};

// The blocks stored on the chain are constructed according to FullBlock
struct Block {
    unsigned int blockid;          // Block height, similar to block ID
    unsigned long long t_min;      // The min of all t_min in this block
    unsigned long long t_max;      // The maximum value of all t_max in this block
    vector<string> tx_public_keys; // The common keywords included in this transaction
    string omt_root;               // omt tree root
    string iimt_root;              // iimt tree root
    string tmin_rimbt_root;        // tmin_rimbt tree root
    string tmax_rimbt_root;        // tmax_rimbt tree root

    // The following structures are only relevant to block truncation
    string global_lidmt_root;      // Global lid index tree root
    string global_iimt_root;       // Global public keyword index tree root
    string global_tmin_rimbt_root; // Global minimum time range query tree root
    string global_tmax_rimbt_root; // Global maximum time range query tree root
    Block(FullBlock *fb) {
        blockid = fb->blockid;
        t_min = fb->t_min;
        t_max = fb->t_max;
        tx_public_keys = fb->tx_public_keywords;
        iimt_root = fb->iimt->root->hash;
        omt_root = fb->omt->root->hash;
        tmin_rimbt_root = fb->tmin_rimbt->calculateRootDigest();
        tmax_rimbt_root = fb->tmax_rimbt->calculateRootDigest();

        // The following structures are only relevant to block truncation
        iimt_root = fb->global_iimt->root->hash;
        tmin_rimbt_root = fb->tmin_rimbt->calculateRootDigest();
        tmax_rimbt_root = fb->tmax_rimbt->calculateRootDigest();
    }
    template <class Archive>
    void serialize(Archive &archive, const unsigned version) {
        archive & blockid & t_min & t_max & tx_public_keys & omt_root & iimt_root & tmin_rimbt_root & tmax_rimbt_root
            & global_lidmt_root & global_iimt_root & global_tmin_rimbt_root & global_tmax_rimbt_root;
    }
    Block() {
    }
};
enum BoolOp { AND, OR, NOT };

struct QueryStat {
    unsigned int start_block;
    unsigned int end_block;
    int lid;
    // Check lid
    bool query_lid;
    // Transaction level operations
    BoolOp tx_operation;
    // Transaction Level Keywords
    vector<string> tx_keywords;
    // Entry level operations
    BoolOp entry_operation;
    // Entry Level Keywords
    vector<string> entry_keywords;
    // True means time query is required
    bool query_time;
    unsigned long long tmin;
    unsigned long long tmax;

    //! A && B || C
    bool complex_query;
};

// Update the global tree
void update_global_trees(FullBlock *&block, vector<FullBlock *> &blocks, vector<string> block_public_keywords,
                         unsigned int already_gen_blocks);
struct Transaction *generate_tx_from_lf(unsigned int time_offset, vector<string> public_keys, LogFile *lf);

bitset<ONE_WINDOW_BLOCK_NUM> block_query(const vector<FullBlock *> &blocks, QueryStat qs, struct VO &vo);
bool tx_query(FullBlock *block, QueryStat qs, bitset<ONE_BLOCK_TX_NUM> &tx_bm, struct VO &vo);
vector<string> entry_query(vector<LogFile *> &lfs, vector<unsigned int> time_offset, QueryStat qs, struct VO &vo);
void store_to_file(vector<FullBlock *> &datas, string path);
void store_to_file(vector<vector<LogFile *>> &datas, string path);
void store_to_file(LogFile *&datas, string path);
void load_from_file(vector<FullBlock *> &datas, string path);
void load_from_file(vector<vector<LogFile *>> &datas, string path);
void load_from_file(LogFile *&datas, string path);
void generate_reuse_blocks_from_lfs(string lfs_path, string blocks_path, vector<FullBlock *> &reuse_blocks,
                                    string public_key_path, vector<string> public_keys);
double test_generate_reuse_blocks_from_lfs(string lfs_path, string blocks_path, vector<FullBlock *> &reuse_blocks,
                                           string public_key_path, vector<string> public_keys);
void generate_reuse_lfs(string lfs_path, string entry_file_path, string public_key_path, vector<string> public_keys);
void test_generate_reuse_lfs(string lfs_path, string entry_file_path, string public_key_path,
                             vector<string> public_keys);
void load_reuse_blocks(string blocks_path, vector<FullBlock *> &blocks);
void load_reuse_lfs(string lfs_path, LogFile *&lfs);

vector<LogFile *> optim_query_one_window_lfs(const vector<FullBlock *> &blocks, vector<unsigned int> &time_offset,
                                             QueryStat qs, struct VO &vo, int window_num);
vector<LogFile *> query_one_window_lfs(const vector<FullBlock *> &blocks, vector<unsigned int> &time_offset,
                                       QueryStat qs, struct VO &vo, int window_num);

#endif