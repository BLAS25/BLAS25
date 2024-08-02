#include <ctime>
#include <thread>
#include <sstream>
#include <algorithm>
#include <future>
#include <chrono>

#include "../include/log.h"

using namespace std;
ostream &operator<<(ostream &os, const struct Entry &e) {
    os << "sn: " << e.sn << "\ntimestamp: " << e.timestamp << "\nvalue: " << e.log_value << "\nkeywords: {";
    for (const auto &e : e.keywords) { os << e << ", "; }
    os << "}";
    return os;
}

vector<string> split(const string &strs, const string &delim) {
    vector<string> res;
    if (strs == "") return res;
    char *str = new char[strs.size() + 1];
    strcpy(str, strs.c_str());
    char *d = new char[delim.size() + 1];
    strcpy(d, delim.c_str());
    char *p = strtok(str, d);
    while (p) {
        string temp = p;
        res.push_back(temp);
        p = strtok(NULL, d);
    }
    delete[] str;
    delete[] d;
    return res;
}

struct Transaction *generate_tx_from_lf(unsigned int time_offset, vector<string> public_keys, LogFile *lf) {
    Transaction *tx = new Transaction(lf);
    // Change the time
    tx->t_min += time_offset;
    tx->t_max += time_offset;
    // Modify keywords
    tx->tx_public_keys = public_keys;
    return tx;
}

void load_log_from_file(string entry_file_path, string timestamp_path, string key_path, unsigned int entry_number,
                        vector<struct Entry *> &entrys) {
    using namespace std;
    ifstream entry_f(entry_file_path);
    ifstream ts_f(timestamp_path);
    ifstream key_f(key_path);
    string str_entry;
    string timestamp;
    string str_keys;
    vector<string> keys;

    for (int i = 0; i < entry_number; i++) {
        // log value
        getline(entry_f, str_entry);
        // timestamp
        getline(ts_f, timestamp);
        // keywords
        getline(key_f, str_keys);
        keys = split(str_keys, " ");

        struct Entry *entry = new struct Entry;
        entry->sn = i;
        entry->timestamp = stoi(timestamp);
        entry->keywords = keys;
        entry->log_value = str_entry;
        entrys.push_back(entry);
    }
}
// Construct log files based on entries
LogFile::LogFile(unsigned int lid, vector<string> tx_public_keywords, vector<struct Entry *> entrys) {
    this->lid = lid;
    this->tx_public_keywords = tx_public_keywords;
    this->t_min = U_INT_MAX;
    this->t_max = U_INT_MIN;

    for (const auto &e : entrys) {
        this->entrys.push_back(Entry::toString(*e));
        for (auto &k : e->keywords) { this->entry_keywords.push_back(k); }
        if (e->timestamp < this->t_min) this->t_min = e->timestamp;
        if (e->timestamp > this->t_max) this->t_max = e->timestamp;
    }

    // Remove duplicate elements
    set<string> s(this->entry_keywords.begin(), this->entry_keywords.end());
    this->entry_keywords.assign(s.begin(), s.end());
    sort(this->entry_keywords.begin(), this->entry_keywords.end());

    this->omt = new MerkleTree(this->entrys);
    // cout << "storage_mt root: " << storage_mt->root().to_string() << endl;

    struct EntryKeyBitmap tmp_bm;
    vector<string> tmp_keys;

    for (const auto &k : this->entry_keywords) {
        tmp_bm.key = k;
        for (int i = 0; i < entrys.size(); i++) {
            tmp_keys = entrys[i]->keywords;
            // This log has this keyword
            if (find(tmp_keys.begin(), tmp_keys.end(), tmp_bm.key) != tmp_keys.end()) {
                // Set the corresponding value to 1
                // cout << i << endl;
                tmp_bm.bitmap.set(i);
            }
        }
        // Converting a bitset to a string
        this->kbms.push_back(EntryKeyBitmap::toString(tmp_bm));
        tmp_bm.bitmap.reset();
    }
    // cout << keybm_jsons[0] << endl;
    // Construct keyword index tree
    this->iimt = new MerkleTree(this->kbms);
    // cout << "keybm_mt root: " << keywords_mt->root().to_string() << endl;

    // Constructing MBTree
    vector<Node *> nodes;
    for (int i = 0; i < entrys.size(); i++) {
        Node *node = new Node;
        node->key = entrys[i]->timestamp;
        node->value = to_string(entrys[i]->sn);
        nodes.push_back(node);
    }

    this->rimbt = new MerkleBTree(nodes, FAN_OUT);
    // cout << "range MBTree root: " << string2Hex(mbt.getRoot()->getDigest(0))
    // << endl;
}

FullBlock::FullBlock(unsigned int blockid, vector<struct Transaction *> txs, vector<string> tx_public_keywords) {
    this->global_lidmt = NULL;
    this->global_iimt = NULL;
    this->global_tmin_rimbt = NULL;
    this->global_tmax_rimbt = NULL;
    this->global_new_iimt = NULL;

    this->blockid = blockid;
    this->t_min = U_INT_MAX;
    this->t_max = U_INT_MIN;
    // Block-level keywords are all predefined keywords
    this->tx_public_keywords.assign(tx_public_keywords.begin(), tx_public_keywords.end());
    for (auto &tx : txs) {
        // this->tx_public_keywords.insert(this->tx_public_keywords.end(),
        // tx->tx_public_keys.begin(), tx->tx_public_keys.end());
        if (tx->t_min < this->t_min) this->t_min = tx->t_min;
        if (tx->t_max > this->t_max) this->t_max = tx->t_max;
        this->lids.push_back(tx->lid);
        this->txs.push_back(Transaction::toString(*tx));
    }

    sort(this->tx_public_keywords.begin(), this->tx_public_keywords.end());

    this->omt = new MerkleTree(this->txs);
    // cout << "omt root: " << string2Hex(this->omt->root->hash) << endl;

    struct TxKeyBitmap tx_kbm;
    vector<string> tmp_keys;
    // For each common keyword, construct a keyword index tree subnode
    for (const auto k : this->tx_public_keywords) {
        tx_kbm.key = k;
        for (int i = 0; i < txs.size(); i++) {
            tmp_keys = txs[i]->tx_public_keys;
            // This transaction has this common keyword
            if (find(tmp_keys.begin(), tmp_keys.end(), tx_kbm.key) != tmp_keys.end()) {
                // Set the corresponding value to 1
                tx_kbm.bitmap.set(i);
            }
        }
        // Converting a bitset to a string
        this->kbms.push_back(TxKeyBitmap::toString(tx_kbm));
        tx_kbm.bitmap.reset();
    }
    this->iimt = new MerkleTree(this->kbms);
    // cout << "iimt root: " << string2Hex(this->iimt->root->hash) << endl;

    // Construct t_min range to query the MBT tree
    unsigned int tmp_min, tmp_max;
    bitset<ONE_BLOCK_TX_NUM> tmin_bits("");
    bitset<ONE_BLOCK_TX_NUM> tmax_bits("");
    vector<Node *> tmin_nodes;
    vector<Node *> tmax_nodes;
    for (int i = 0; i < txs.size(); i++) {
        tmp_min = txs[i]->t_min;
        tmp_max = txs[i]->t_max;
        for (int j = 0; j < txs.size(); j++) {
            if (txs[j]->t_min <= tmp_min) { tmin_bits.set(j); }
            if (txs[j]->t_max >= tmp_max) { tmax_bits.set(j); }
        }

        Node *tmin_node = new Node;
        tmin_node->key = txs[i]->t_min;
        tmin_node->value = tmin_bits.to_string();
        tmin_nodes.push_back(tmin_node);
        Node *tmax_node = new Node;
        tmax_node->key = txs[i]->t_max;
        tmax_node->value = tmax_bits.to_string();
        tmax_nodes.push_back(tmax_node);
        tmin_bits.reset();
        tmax_bits.reset();
    }

    int fanout = FAN_OUT;
    MerkleBTree *tmin_mbt = new MerkleBTree(tmin_nodes, FAN_OUT);
    MerkleBTree *tmax_mbt = new MerkleBTree(tmax_nodes, FAN_OUT);
    this->tmin_rimbt = tmin_mbt;
    this->tmax_rimbt = tmax_mbt;
}

void update_global_trees(FullBlock *&block, vector<FullBlock *> &blocks, vector<string> block_public_keywords,
                         unsigned int already_gen_blocks) {
    struct BlockKeyBitmap block_kbm;

    vector<struct BlockKeyBitmap> block_key_bms;
    vector<string> tmp_keys;

    sort(block_public_keywords.begin(), block_public_keywords.end());
    unsigned int start_block = already_gen_blocks - ONE_WINDOW_BLOCK_NUM;
    cout << "update start block: " << start_block << endl;
    cout << "update end block: " << already_gen_blocks << endl;
    block->global_t_min = U_INT_MAX;
    block->global_t_max = U_INT_MIN;
    for (int i = start_block; i < already_gen_blocks; i++) {
        if (blocks.at(i)->t_min < block->global_t_min) { block->global_t_min = blocks.at(i)->t_min; }
        if (blocks.at(i)->t_max > block->global_t_max) { block->global_t_max = blocks.at(i)->t_max; }
    }
    // global_iimt

    for (const auto &k : block_public_keywords) {
        block_kbm.key = k;
        for (int i = start_block; i < already_gen_blocks; i++) {
            tmp_keys = blocks.at(i)->tx_public_keywords;
            // This block contains this public keyword
            if (find(tmp_keys.begin(), tmp_keys.end(), block_kbm.key) != tmp_keys.end()) {
                block_kbm.bitmap.set(i - start_block);
            }
        }
        block_key_bms.push_back(block_kbm);
        block_kbm.bitmap.reset();
    }
    // cout << "1" << endl;
    // Serialization keyword bitmap
    vector<string> seri_block_kbms;
    string public_keybm_json;
    for (const auto &k_b : block_key_bms) { seri_block_kbms.push_back(BlockKeyBitmap::toString(k_b)); }
    block->global_kbms = seri_block_kbms;
    block->global_iimt = new MerkleTree(block->global_kbms);
    // cout << "global_iimt root: " << string2Hex(this->global_iimt->root->hash)
    // << endl;

    // global_lidmt
    struct LidKeyBitmap lid_kbm;
    vector<struct LidKeyBitmap> lid_bms;
    string tmp;
    // Node type is blockid || txid || tx
    struct LidTreeNode lid_node;
    vector<string> lid_nodes;

    for (int l = 1; l <= LG_NUM; l++) {
        lid_kbm.key = l; // key
        lid_kbm.bitmap.reset();
        // Iterate over all transactions
        for (int i = start_block; i < already_gen_blocks; i++) {
            for (int j = 0; j < blocks.at(i)->txs.size(); j++) {
                // The jth transaction in this block is constructed as l
                if (blocks.at(i)->lids.at(j) == l) {
                    // bitmap
                    lid_kbm.bitmap.set(i - start_block);
                    // Construct the leaf nodes required by mht_root
                    lid_node.blockid = blocks.at(i)->blockid;
                    lid_node.txid = j;
                    lid_node.tx = blocks.at(i)->txs.at(j);
                    lid_nodes.push_back(LidTreeNode::toString(lid_node));
                }
            }
        }

        lid_bms.push_back(lid_kbm);
        // assert(lid_nodes.size() > 0);

        if (lid_nodes.size() > 0) {
            MerkleTree *lid_tree = new MerkleTree(lid_nodes);
            lid_kbm.mht_root = lid_tree->root->hash;
            // if (l == 1)
            //     cout << string2Hex(lid_tree->root->hash) << endl;
            // Save lid_tree in order
            block->lid_trees.push_back(lid_tree);
        } else {
            lid_kbm.mht_root = "";
            block->lid_trees.push_back(NULL);
        }
        // cout << this->lid_trees.size() << endl;
        // Store lid_nodes in lid order
        block->lid_nodes.push_back(lid_nodes);
        lid_nodes.clear();
        // if (l < 10)
        //     cout << l << ": " << lid_kbm.bitmap.to_string() << endl;
        lid_kbm.bitmap.reset();
    }

    // All lid traversals are completed, and the entire tree is constructed
    vector<string> seri_lid_bms;
    for (const auto &l_b : lid_bms) { seri_lid_bms.push_back(LidKeyBitmap::toString(l_b)); }
    block->lidkbms = seri_lid_bms;
    block->global_lidmt = new MerkleTree(seri_lid_bms);

    // cout << "block_lidmt root: " << block_lidmt.root().to_string() << endl;

    GlobalKeyTreeNode global_key_node;
    vector<struct GlobalKeyTreeNode> global_key_nodes;

    bitset<ONE_BLOCK_TX_NUM> tmp_bm;
    vector<string> block_bm_per_key;
    // sort(block_public_keywords.begin(), block_public_keywords.end());
    // for (auto e : block_public_keywords) { cout << e << endl; }
    for (const auto &e : block_public_keywords) {
        global_key_node.key = e;
        for (int i = start_block; i < already_gen_blocks; i++) {
            for (int j = 0; j < blocks.at(i)->txs.size(); j++) {
                Transaction tx = Transaction::fromString(blocks.at(i)->txs.at(j));
                if (find(tx.tx_public_keys.begin(), tx.tx_public_keys.end(), e) != tx.tx_public_keys.end()) {
                    tmp_bm.set(j);
                }
            }

            // if (e == "xiaomi") cout << e << ": " << tmp_bm.to_string() <<
            // endl;
            block_bm_per_key.push_back(tmp_bm.to_string());
            tmp_bm.reset();
        }

        assert(block_bm_per_key.size() > 0);
        block->block_bm_keys.push_back(block_bm_per_key);
        MerkleTree *mt = new MerkleTree(block_bm_per_key);
        block->new_key_trees.push_back(mt);
        global_key_node.mht_root = mt->root->hash;
        global_key_nodes.push_back(global_key_node);
        block_bm_per_key.clear();
    }
    // All key traversals are completed, and the entire tree is constructed
    vector<string> seri_global_key_bms;
    for (const auto &e : global_key_nodes) { seri_global_key_bms.push_back(GlobalKeyTreeNode::toString(e)); }
    block->global_new_kbms = seri_global_key_bms;
    block->global_new_iimt = new MerkleTree(seri_global_key_bms);
    // Construct tmin-MBT
    // Construct tmax-MBT
    unsigned int tmp_min, tmp_max;
    bitset<ONE_WINDOW_BLOCK_NUM> tmin_bits("");
    bitset<ONE_WINDOW_BLOCK_NUM> tmax_bits("");
    vector<Node *> tmin_nodes;
    vector<Node *> tmax_nodes;
    for (int i = start_block; i < already_gen_blocks; i++) {
        // Traverse the blocks and construct the time tree for each block’s tmin and tmax
        tmp_min = blocks.at(i)->t_min;
        tmp_max = blocks.at(i)->t_max;
        for (int j = start_block; j < already_gen_blocks; j++) {
            // cout << j << endl;
            if (blocks.at(j)->t_min <= tmp_min) { tmin_bits.set(j - start_block); }
            if (blocks.at(j)->t_max >= tmp_max) { tmax_bits.set(j - start_block); }
            // cout << j - start_block << endl;
        }
        // Converting a bitset to a string
        Node *tmin_node = new Node;
        tmin_node->key = blocks.at(i)->t_min;
        tmin_node->value = tmin_bits.to_string();
        tmin_nodes.push_back(tmin_node);
        Node *tmax_node = new Node;
        tmax_node->key = blocks[i]->t_max;
        tmax_node->value = tmax_bits.to_string();
        tmax_nodes.push_back(tmax_node);
        tmin_bits.reset();
        tmax_bits.reset();
    }

    MerkleBTree *tmin_mbt = new MerkleBTree(tmin_nodes, FAN_OUT);
    MerkleBTree *tmax_mbt = new MerkleBTree(tmax_nodes, FAN_OUT);
    block->global_tmin_rimbt = tmin_mbt;
    block->global_tmax_rimbt = tmax_mbt;
}

string general_get_rimbt_VO(MerkleBTree *range_mbt, unsigned int left, unsigned int right, vector<string> &nodeJsons) {
    set<int> keySet;
    // range_mbt->printKeys();
    range_mbt->rangeSearch(left, right, true, keySet);
    // cout << "left: " << left << endl;
    // cout << "right: " << right << endl;
    // for (int e : keySet) { cout << e << " "; }
    // cout << endl;
    string VO = range_mbt->generateVO(range_mbt->getRoot(), keySet, nodeJsons);
    // string VO;
    return VO;
}

bool general_verify_rimbt_VO(MerkleBTree *range_mbt, string VO) {
    AuthenticationTree at;
    at.parseVO(VO);
    string mbt_root = range_mbt->calculateRootDigest();
    string at_root = at.getRootDigest();
    return (mbt_root == at_root);
}

// Optimize, find transactions, query range is one window
vector<LogFile *> optim_query_one_window_lfs(const vector<FullBlock *> &blocks, vector<unsigned int> &time_offset,
                                             QueryStat qs, struct VO &vo, int window_num) {
    clock_t block_query_start = clock();
    vector<LogFile *> res_lfs;
    bitset<ONE_WINDOW_BLOCK_NUM> block_bits;
    block_bits.set();
    FullBlock *latest_block = blocks.at(blocks.size() - 1);

    vector<bool> lid_bitmap;
    vector<string> lid_path;
    if (qs.query_lid) {
        int lid = qs.lid;
        // cout << latest_block->lidbms.at(lid - 1) << endl;
        assert(latest_block->global_lidmt->node_hashs.at(lid - 1) == my_blakes(latest_block->lidkbms.at(lid - 1)));
        struct LidKeyBitmap lid_kbm = LidKeyBitmap::fromString(latest_block->lidkbms.at(lid - 1));
        vector<unsigned int> lid_indexs;
        lid_indexs.push_back(lid - 1);

        latest_block->global_get_lidmt_VO(lid_indexs, lid_bitmap, lid_path);
        if (latest_block->global_lidmt_verify_VO(lid_bitmap, lid_path) != 1) { cout << "lidmt verify failed" << endl; }
        block_bits = block_bits & lid_kbm.bitmap;
        // vo.block_VO_size += lid_bitmap.size();
        vo.block_VO_size += lid_path.size() * lid_path[0].length();
        cout << lid_kbm.key << " block bits: " << lid_kbm.bitmap.to_string() << endl;
    }

    // Time range query
    if (qs.query_time && (latest_block->global_t_min <= qs.tmax) && (latest_block->global_t_max >= qs.tmin)) {
        unsigned long long t_min = qs.tmin;
        unsigned long long t_max = qs.tmax;
        // Time range search
        vector<string> tmin_nodeJsons;
        vector<string> tmax_nodeJsons;
        // t_min, check for values ​​less than t_max
        // latest_block->global_tmin_rimbt->printKeys();
        string tmin_VO = latest_block->global_get_tmin_rimbt_VO(t_min, t_max, tmin_nodeJsons);
        // cout << "tmin_VO: " << tmin_VO << endl;
        vo.block_VO_size += tmin_VO.length();
        // t_max, check for values ​​greater than t_min
        string tmax_VO = latest_block->global_get_tmax_rimbt_VO(t_min, t_max, tmax_nodeJsons);

        vo.block_VO_size += tmax_VO.length();
        // cout << "tmin_VO size: " << tmin_VO.length() << endl;
        // cout << "tmax_VO size: " << tmax_VO.length() << endl;
        // cout << "tmax_VO: " << tmax_VO << endl;
        // Find the block that meets the conditions according to the returned node
        Node tmin_node, tmax_node;

        if (tmin_nodeJsons.size() >= 2) {
            string tmin_str = tmin_nodeJsons[tmin_nodeJsons.size() - 2];
            tmin_node = Node::fromString(tmin_str.c_str());
        }
        if (tmin_node.key < qs.tmax && tmin_nodeJsons.size() >= 1) {
            string tmin_str = tmin_nodeJsons[tmin_nodeJsons.size() - 1];
            tmin_node = Node::fromString(tmin_str.c_str());
        }
        // cout << "tmin_node key: " << tmin_node.key << endl;
        // First, make sure you don’t miss any
        tmax_node = Node::fromString(tmax_nodeJsons[0]);
        // Logically AND the bitmap obtained by block_bits with tmin and tmax
        bitset<ONE_WINDOW_BLOCK_NUM> tmin_bits(tmin_node.value);
        block_bits = block_bits & tmin_bits;
        bitset<ONE_WINDOW_BLOCK_NUM> tmax_bits(tmax_node.value);
        block_bits = block_bits & tmax_bits;
        // cout << "tmin_bits: " << tmin_bits.to_string() << endl;
        // cout << "tmax_bits: " << tmax_bits.to_string() << endl;
        // The block corresponding to block_bits being 1 may contain the required transaction
        cout << "range block_bits: " << block_bits.to_string() << endl;
    } else if (qs.query_time) {
        cout << "window tmin: " << latest_block->global_t_min << " qs.tmax: " << qs.tmax
             << " window tmax: " << latest_block->global_t_max << " qs.tmin: " << qs.tmin << endl;
        cout << "skip window" << endl;
        vector<LogFile *> n;
        return n;
    }
    unsigned int need_block_num = block_bits.count();
    // Keyword search
    // All about client_key block_bitmap
    struct GlobalKeyTreeNode tree_node;
    vector<string> seri_block_keybms;
    vector<string> tx_keywords(qs.tx_keywords);
    vector<unsigned int> indexs;

    // sort(tx_keywords.begin(), tx_keywords.end());
    sort(latest_block->tx_public_keywords.begin(), latest_block->tx_public_keywords.end());
    map<unsigned int, vector<bitset<ONE_BLOCK_TX_NUM>>> keybm_per_block;
    // Search without keywords
    if (tx_keywords.size() == 0) {
        // cout << 1 << endl;
        for (int i = 0; i < block_bits.size(); i++) {
            if (block_bits.test(i)) {
                bitset<ONE_BLOCK_TX_NUM> tmp;
                tmp.set();
                keybm_per_block[i].push_back(tmp);
            }
        }
    }
    // for (auto e : latest_block->tx_public_keywords) { cout << e << endl; }
    for (auto &client_tx_key : tx_keywords) {
        vector<unsigned int> down_indexs;
        tree_node.key = client_tx_key;

        auto it = find(latest_block->tx_public_keywords.begin(), latest_block->tx_public_keywords.end(), client_tx_key);
        unsigned int index = it - latest_block->tx_public_keywords.begin();
        // cout << index << endl;
        tree_node.mht_root = latest_block->new_key_trees.at(index)->root->hash;
        // block_bms.at(0) represents the transaction-level key-bitmap of blocks.at(0)
        // The bitmap of the index keyword
        vector<string> block_bms = latest_block->block_bm_keys.at(index);
        // cout << client_tx_key << ": " << block_bms[0] << endl;
        for (int i = 0; i < block_bits.size(); i++) {
            // The i-th block
            if (block_bits.test(i)) {
                bitset<ONE_BLOCK_TX_NUM> tmp(block_bms.at(i));
                keybm_per_block[i].push_back(tmp);
                down_indexs.push_back(i);
            }
        }
        // for (auto e : down_indexs) { cout << e << " "; }
        // cout << endl;
        // Verify the lower layer first
        vector<bool> down_bitmap;
        vector<string> down_path;
        // cout << "down_indexs size: " << down_indexs.size() << endl;
        // for (int x : down_indexs) { cout << x << " "; }
        // cout << endl;
        latest_block->global_get_down_iimt_VO(index, down_indexs, down_bitmap, down_path);
        assert(!isAllFalse(down_bitmap));

        if (latest_block->global_down_iimt_verify_VO(index, down_bitmap, down_path) != 1) {
            cout << "down iimt verify failed" << endl;
        }
        // for (auto e : down_path) { cout << e << " "; }
        // cout << endl;
        vo.block_VO_size += down_bitmap.size() / 8;
        vo.block_VO_size += down_path.size() * down_path[0].length();
        // cout << vo.block_VO_size << endl;
        down_bitmap.clear();
        down_path.clear();
        assert(latest_block->global_new_kbms[index] == GlobalKeyTreeNode::toString(tree_node));
        indexs.push_back(index);
    }

    // Verify global_iimt based on seri_block_keybms and indexes
    if (indexs.size() > 0) {
        vector<bool> bitmap;
        vector<string> path;
        latest_block->global_get_iimt_VO(indexs, bitmap, path);
        assert(!isAllFalse(bitmap));
        if (latest_block->global_iimt_verify_VO(bitmap, path) != 1) { cout << "global iimt verify failed" << endl; }
        clock_t block_query_end = clock();
        vo.block_query_time += double(block_query_end - block_query_start) / CLOCKS_PER_SEC;
        vo.block_VO_size += bitmap.size() / 8;
        if (path.size() > 0) { vo.block_VO_size += path.size() * path[0].length(); }
    }
    // cout << 1 << endl;
    // Get the transaction of each block according to the bitmap, and then check the transaction
    clock_t tx_query_start = clock();
    struct Transaction tx;
    for (auto &e : keybm_per_block) {
        // All keywords and or not, filter the bitmap
        if (!qs.complex_query) {
            for (auto c : e.second) {
                // cout << "block: " << e.first << " " << c << endl;
                if (qs.tx_operation == AND) e.second.at(0) &= c;
                if (qs.tx_operation == OR) e.second.at(0) |= c;
                if (qs.tx_operation == NOT) {
                    if (c == e.second.at(0)) {
                        e.second.at(0) = ~c;

                    } else {
                        e.second.at(0) &= (~c);
                    }
                }
            }
        } else {
            assert(e.second.size() == 3);
            // NOT
            e.second.at(0) = ~e.second.at(0);
            // AND
            e.second.at(0) &= e.second.at(1);
            // OR
            e.second.at(0) |= e.second.at(2);
        }
        // Check if the lid condition is met and filter the bitmap again
        if (qs.query_lid) {
            for (int i = 0; i < ONE_BLOCK_TX_NUM; i++) {
                if (e.second.at(0).test(i)) {
                    tx = Transaction::fromString(blocks[e.first]->txs.at(i));
                    if (tx.lid != qs.lid) {
                        // cout << tx.lid << endl;
                        e.second.at(0).reset(i);
                    }
                }
            }
        }
    }
    // clock_t tx_query_start = clock();
    // clock_t tx_query_end = clock();

    // Transaction Level Query
    //  Check lid
    vector<unsigned int> lid_node_indexs;
    vector<string> lid_nodes;
    MerkleTree *lid_tree = latest_block->lid_trees.at(qs.lid - 1);
    // Do not check lid
    vector<unsigned int> omt_indexs;
    vector<string> omt_nodes;
    double load_time = 0;
    // cout << "keybm_per_block size: " << keybm_per_block.size() << endl;
    for (auto &e : keybm_per_block) {
        // cout << "blockid: " << e.first << endl;
        // cout << e.second.at(0).to_string() << endl;
        // Transaction level, block-by-block time range search
        if (qs.query_time && !(blocks[e.first]->t_min >= qs.tmin && blocks[e.first]->t_max <= qs.tmax)) {
            if (blocks[e.first]->t_min > qs.tmax || blocks[e.first]->t_max < qs.tmin) {
                cout << "block tmin: " << blocks[e.first]->t_min << " qs.tmax: " << qs.tmax
                     << " block tmax: " << blocks[e.first]->t_max << " qs.tmin: " << qs.tmin << endl;
                cout << "block time out" << endl;
                continue;
            }
            vector<string> tmin_nodeJsons;
            vector<string> tmax_nodeJsons;
            // t_min, check for values ​​less than t_max
            string tmin_VO = blocks[e.first]->get_tmin_rimbt_VO(qs.tmin, qs.tmax, tmin_nodeJsons);
            vo.tx_VO_size += tmin_VO.length();
            // t_max, check for values ​​greater than t_min
            string tmax_VO = blocks[e.first]->get_tmax_rimbt_VO(qs.tmin, qs.tmax, tmax_nodeJsons);
            vo.tx_VO_size += tmax_VO.length();
            // Find the block that meets the conditions according to the returned node
            Node tmin_node, tmax_node;
            // The last one, make sure you don't miss it
            if (tmin_nodeJsons.size() >= 2) {
                string tmin_str = tmin_nodeJsons[tmin_nodeJsons.size() - 2];
                tmin_node = Node::fromString(tmin_str.c_str());
            }
            if (tmin_node.key < qs.tmax && tmin_nodeJsons.size() >= 1) {
                string tmin_str = tmin_nodeJsons[tmin_nodeJsons.size() - 1];
                tmin_node = Node::fromString(tmin_str.c_str());
            }
            tmax_node = Node::fromString(tmax_nodeJsons[0]);
            bitset<ONE_BLOCK_TX_NUM> tmin_bits(tmin_node.value);
            // cout << "tmin_bits: " << tmin_bits.to_string() << endl;
            e.second.at(0) = e.second.at(0) & tmin_bits;
            // cout << tn.key << endl;
            bitset<ONE_BLOCK_TX_NUM> tmax_bits(tmax_node.value);
            // cout << "tmax_bits: " << tmax_bits.to_string() << endl;
            e.second.at(0) = e.second.at(0) & tmax_bits;
            // cout << "tx bitmap: " << e.second.at(0).to_string() << endl;
            // cout << e.second.at(0).to_string() << endl;
        }
        // All Transactions
        for (int i = 0; i < e.second.at(0).size(); i++) {
            // This tx needs
            if (e.second.at(0).test(i)) {
                if ((i != e.second.at(0).size() - 1) && (!e.second.at(0).test(i + 1))
                    || (i == e.second.at(0).size() - 1)) {
                    Transaction t = Transaction::fromString(blocks[e.first]->txs.at(i));
                    // cout << t.t_min << " " << t.t_max << endl;
                    int x = ((window_num - 1) * (ONE_WINDOW_BLOCK_NUM / 16) + e.first / 16) * 100000;
                    if (t.t_min > qs.tmax || t.t_max < qs.tmin) {
                        e.second.at(0).reset(i);
                        continue;
                    }
                }
                if (qs.query_lid) {
                    struct LidTreeNode ltn;
                    ltn.blockid = blocks[e.first]->blockid;
                    ltn.txid = i;
                    ltn.tx = blocks[e.first]->txs.at(i);
                    lid_nodes.push_back(LidTreeNode::toString(ltn));
                    assert(latest_block->lid_trees.size() == LG_NUM);

                    string h = my_blakes(LidTreeNode::toString(ltn));
                    auto it = find(lid_tree->node_hashs.begin(), lid_tree->node_hashs.end(), h);
                    if (it != lid_tree->node_hashs.end()) {
                        lid_node_indexs.push_back(it - lid_tree->node_hashs.begin());
                    }
                } else {
                    string tx = blocks[e.first]->txs.at(i);
                    omt_indexs.push_back(i);
                    omt_nodes.push_back(tx);
                }
                unsigned int lf_index = (e.first * ONE_BLOCK_TX_NUM + i) % 4096;
                string lf_path = "./db/reuse/lfs/lfs_" + to_string(lf_index);
                LogFile *lf = new LogFile();
                clock_t load_lf_start = clock();
                load_from_file(lf, lf_path);
                clock_t load_lf_end = clock();
                load_time += double(load_lf_end - load_lf_start) / CLOCKS_PER_SEC;
                unsigned int b = 0;
                if (i != 0)
                    b = ((window_num - 1) * (ONE_WINDOW_BLOCK_NUM / 16) + (i - 1) / 16) * 100000;
                else
                    b = 0;
                time_offset.push_back(b);
                res_lfs.push_back(lf);
            }
        }
        // Block-by-block verification without checking lid
        assert(omt_indexs.size() == 0);
        if (omt_indexs.size() > 0) {
            vector<bool> omt_bitmap;
            vector<string> omt_path;
            blocks.at(e.first)->get_omt_VO(omt_indexs, omt_bitmap, omt_path);
            if (blocks.at(e.first)->omt_VO_verify(omt_bitmap, omt_path) != 1) { cout << "omt verify failed" << endl; }
            vo.tx_VO_size += omt_bitmap.size() / 8;
            vo.tx_VO_size += omt_path.size() * omt_path[0].length();
            omt_indexs.clear();
            omt_nodes.clear();
        }
        // cout << "block" << e.first << ": " << e.second.at(0).to_string() << " " << e.second.at(0).count() << endl;
    }
    // assert(lid_node_indexs.size() == 0);
    // assert(lid_node_indexs.size() != 0);
    // Check lid and verify together
    if (qs.query_lid && lid_node_indexs.size() > 0) {
        lid_bitmap.clear();
        lid_path.clear();
        latest_block->global_get_down_lidmt_VO(qs.lid, lid_node_indexs, lid_bitmap, lid_path);
        // assert(!isAllFalse(bitmap));
        if (latest_block->global_down_lidmt_verify_VO(qs.lid, lid_bitmap, lid_path) != 1) {
            cout << "optim query global lidmt verify failed" << endl;
        }
        vo.tx_VO_size += lid_bitmap.size() / 8;
        if (lid_path.size() > 0) vo.tx_VO_size += lid_path.size() * lid_path[0].length();
    }
    clock_t tx_query_end = clock();
    vo.tx_query_time += double(tx_query_end - tx_query_start) / CLOCKS_PER_SEC - load_time;
    // vo.tx_query_time += double(tx_query_end - tx_query_start) / CLOCKS_PER_SEC;
    return res_lfs;
}

// Found a transaction, the query range is a window
vector<LogFile *> query_one_window_lfs(const vector<FullBlock *> &blocks, vector<unsigned int> &time_offset,
                                       QueryStat qs, struct VO &vo, int window_num) {
    vector<LogFile *> res_lfs;
    LogFile *tmp;
    // cout << "window" << window_num << " start query" << endl;
    FullBlock *latest_block = blocks.at(blocks.size() - 1);
    assert(latest_block->global_lidmt != NULL);
    vector<vector<string>> res_entrys;
    bitset<ONE_WINDOW_BLOCK_NUM> block_bits;
    if (qs.tx_operation == AND || qs.tx_operation == NOT)
        block_bits.set();
    else
        block_bits.reset();
    // block_bits[i] is 1, which means there may be a transaction that meets the conditions in blocks[i].

    clock_t block_query_start = clock();
    block_bits = block_query(blocks, qs, vo);
    clock_t block_query_end = clock();
    vo.block_query_time += double(block_query_end - block_query_start) / CLOCKS_PER_SEC;
    // cout << "block_bits.count: " << block_bits.count() << endl;
    cout << "block_bits: " << block_bits.to_string() << endl;
    vector<unsigned int> lid_node_indexs;
    vector<unsigned int> indexs;
    vector<string> lid_nodes;
    MerkleTree *lid_tree = latest_block->lid_trees.at(qs.lid - 1);
    vector<unsigned int> omt_indexs;
    vector<string> omt_nodes;
    assert(lid_tree != NULL);
    vector<bitset<ONE_BLOCK_TX_NUM>> bitmaps;
    double load_time = 0;
    clock_t tx_query_start = clock();
    for (int i = 0; i < block_bits.size(); i++) {
        // Check block by block
        if (block_bits.test(i)) {
            // blocks1.push_back(blocks.at(i));
            // blocks1_lfs.push_back(all_lfs.at(i));
            bitset<ONE_BLOCK_TX_NUM> tx_bm;
            tx_bm.set();
            // cout << "tx bitmap: " << tx_bm.to_string() << endl;
            if (qs.tx_operation == AND)
                tx_bm.set();
            else if (qs.tx_operation == OR && qs.tx_keywords.size() > 0)
                tx_bm.reset();
            else if (qs.tx_operation == NOT)
                tx_bm.set();

            tx_query(blocks[i], qs, tx_bm, vo);
            // cout << "block" << i << ": " << tx_bm.to_string() << endl;
            // Get LogFile according to tx_bm
            struct LidTreeNode ltn;
            for (int j = 0; j < tx_bm.size(); j++) {
                if (tx_bm.test(j)) {
                    if ((j != tx_bm.size() - 1) && (!tx_bm.test(j + 1)) || (j == tx_bm.size() - 1)) {
                        Transaction t = Transaction::fromString(blocks[i]->txs.at(j));
                        if (t.t_min > qs.tmax || t.t_max < qs.tmin) {
                            tx_bm.reset(j);
                            continue;
                        }
                    }
                    struct Transaction tx = Transaction::fromString(blocks[i]->txs.at(j));
                    if (qs.query_lid) {
                        if (tx.lid != qs.lid) {
                            tx_bm.reset(j);
                            continue;
                        }
                    }

                    omt_indexs.push_back(j);
                    omt_nodes.push_back(blocks[i]->txs.at(j));

                    // Find lfs from reuse_lfs
                    // The jth transaction of the i-th block, 4096 transactions are used in a cycle
                    unsigned int lf_index = (i * ONE_BLOCK_TX_NUM + j) % 4096;
                    string lf_path = "./db/reuse/lfs/lfs_" + to_string(lf_index);
                    // cout << "lf_path: " << lf_path << endl;
                    LogFile *lf;
                    clock_t load_start = clock();
                    load_from_file(lf, lf_path);
                    clock_t load_end = clock();
                    load_time += double(load_end - load_start) / CLOCKS_PER_SEC;
                    res_lfs.push_back(lf);
                    // The jth transaction in the i-th block has a deviation of 100,000 every 16 blocks.
                    unsigned int b = 0;
                    if (i != 0)
                        b = ((window_num - 1) * (ONE_WINDOW_BLOCK_NUM / 16) + (i - 1) / 16) * 100000;
                    else
                        b = 0;
                    time_offset.push_back(b);
                }
            }
            if (omt_indexs.size() > 0) {
                // Block-by-block verification
                vector<bool> omt_bitmap;
                vector<string> omt_path;
                blocks.at(i)->get_omt_VO(omt_indexs, omt_bitmap, omt_path);
                if (blocks.at(i)->omt_VO_verify(omt_bitmap, omt_path) != 1) { cout << "omt verify failed" << endl; }
                // cout << "tx_VO_size: " << vo.tx_VO_size << endl;
                vo.tx_VO_size += omt_bitmap.size() / 8;
                vo.tx_VO_size += omt_path.size() * omt_path[0].length();
                // cout << "tx_VO_size: " << vo.tx_VO_size << endl;
                omt_indexs.clear();
                omt_nodes.clear();
            }
        }
    }
    if (lid_node_indexs.size() > 0) {
        vector<bool> lid_bitmap;
        vector<string> lid_path;
        latest_block->global_get_down_lidmt_VO(qs.lid, lid_node_indexs, lid_bitmap, lid_path);
        // assert(!isAllFalse(bitmap));
        if (latest_block->global_down_lidmt_verify_VO(qs.lid, lid_bitmap, lid_path) != 1) {
            cout << "optim query global lidmt verify failed" << endl;
        }
        vo.tx_VO_size += lid_bitmap.size();
        if (lid_path.size() > 0) vo.tx_VO_size += lid_path.size() * lid_path[0].length();
    }
    clock_t tx_query_end = clock();
    vo.tx_query_time += double(tx_query_end - tx_query_start) / CLOCKS_PER_SEC - load_time;
    return res_lfs;
}

// Block-level search
bitset<ONE_WINDOW_BLOCK_NUM> block_query(const vector<FullBlock *> &blocks, QueryStat qs, struct VO &vo) {
    // A bit being 1 indicates that there may be a transaction that meets the conditions in the block.
    bitset<ONE_WINDOW_BLOCK_NUM> block_bits;
    block_bits.set();
    // Latest Block
    FullBlock *latest_block = blocks[blocks.size() - 1];
    assert(latest_block->global_new_iimt != NULL);
    assert(latest_block->global_iimt != NULL);
    assert(latest_block->global_lidmt != NULL);
    assert(latest_block->global_tmin_rimbt != NULL);
    assert(latest_block->global_tmax_rimbt != NULL);
    // Keyword search
    vector<string> seri_block_keybms;
    // Search all keywords at once
    vector<string> tx_keywords(qs.tx_keywords);
    vector<unsigned int> indexs;
    // sort(tx_keywords.begin(), tx_keywords.end());
    for (auto &client_tx_key : tx_keywords) {
        // cout << client_tx_key << " ";
        auto it = find(latest_block->tx_public_keywords.begin(), latest_block->tx_public_keywords.end(), client_tx_key);
        unsigned int index = it - latest_block->tx_public_keywords.begin();
        // cout << index << endl;
        // indexes stores which nodes need to be found from the keyword tree
        assert(latest_block->global_iimt->node_hashs.at(index) == my_blakes(latest_block->global_kbms[index]));
        indexs.push_back(index);
        // Stores the bitmap of each keyword in the query
        seri_block_keybms.push_back(latest_block->global_kbms[index]);
    }
    // cout << indexs.size() << endl;
    if (indexs.size() > 0) {
        vector<bool> bitmap;
        vector<string> path;
        // global iimt
        latest_block->global_get_iimt_VO(indexs, bitmap, path);
        assert(!isAllFalse(bitmap));
        if (latest_block->global_iimt_verify_VO(bitmap, path) != 1) { cout << "latest_block verify failed" << endl; }

        vo.block_VO_size += bitmap.size() / 8;
        // cout << "global_iimt bitmap size: " << bitmap.size() << endl;
        vo.block_VO_size += path.size() * path[0].length();
        // cout << "global_iimt path size: " << path.size() << endl;
        // cout << "global_iimt path length: " << path[0].length() << endl;
    }

    struct BlockKeyBitmap block_key_bm;
    BoolOp tx_op = qs.tx_operation;
    bool q_time = qs.query_time;
    // cout << "VO size: " << VO_size << endl;
    //  block_bits and each key-bitmap logical operation
    for (const auto &b : seri_block_keybms) {
        block_key_bm = BlockKeyBitmap::fromString(b);
        // cout << block_key_bm.key << endl;
        if (tx_op == AND) {
            block_bits = block_bits & block_key_bm.bitmap;
        } else if (tx_op == OR) {
            block_bits = block_bits | block_key_bm.bitmap;
        }
        // The presence of this keyword in a block does not mean that there are no transactions without this keyword in
        // the block.
        else if (tx_op == NOT) {
            block_bits.set();
        }
    }
    // cout << "block_bits: " << block_bits.to_string() << endl;

    if (qs.query_time && (latest_block->global_t_min < qs.tmax) && (latest_block->global_t_max > qs.tmin)) {
        unsigned t_min = qs.tmin;
        unsigned t_max = qs.tmax;

        vector<string> tmin_nodeJsons;
        vector<string> tmax_nodeJsons;

        string tmin_VO = latest_block->global_get_tmin_rimbt_VO(t_min, t_max, tmin_nodeJsons);
        vo.block_VO_size += tmin_VO.length();

        string tmax_VO = latest_block->global_get_tmax_rimbt_VO(t_min, t_max, tmax_nodeJsons);
        vo.block_VO_size += tmax_VO.length();

        Node tmin_node, tmax_node;

        if (tmin_nodeJsons.size() >= 2) {
            string tmin_str = tmin_nodeJsons[tmin_nodeJsons.size() - 2];
            tmin_node = Node::fromString(tmin_str.c_str());
        }
        if (tmin_node.key < qs.tmax && tmin_nodeJsons.size() >= 1) {
            string tmin_str = tmin_nodeJsons[tmin_nodeJsons.size() - 1];
            tmin_node = Node::fromString(tmin_str.c_str());
        }
        // cout << "tmin_node key: " << tmin_node.key << endl;
        tmax_node = Node::fromString(tmax_nodeJsons[0]);
        bitset<ONE_WINDOW_BLOCK_NUM> tmin_bits(tmin_node.value);
        block_bits = block_bits & tmin_bits;
        bitset<ONE_WINDOW_BLOCK_NUM> tmax_bits(tmax_node.value);
        block_bits = block_bits & tmax_bits;
        // cout << "tmin_bits: " << tmin_bits.to_string() << endl;
        // cout << "tmax_bits: " << tmax_bits.to_string() << endl;

        cout << "range block_bits: " << block_bits.to_string() << endl;
    } else if (qs.query_time) {
        cout << "window tmin: " << latest_block->global_t_min << " qs.tmax: " << qs.tmax
             << " window tmax: " << latest_block->global_t_max << " qs.tmin: " << qs.tmin << endl;
        cout << "skip window" << endl;
        block_bits.reset();
        return block_bits;
    }

    if (qs.query_lid) {
        int lid = qs.lid;
        // cout << latest_block->lidbms.at(lid - 1) << endl;
        assert(latest_block->global_lidmt->node_hashs.at(lid - 1) == my_blakes(latest_block->lidkbms.at(lid - 1)));
        struct LidKeyBitmap lid_kbm = LidKeyBitmap::fromString(latest_block->lidkbms.at(lid - 1));
        vector<unsigned int> lid_indexs;
        lid_indexs.push_back(lid - 1);
        vector<bool> lid_bitmap;
        vector<string> lid_path;
        latest_block->global_get_lidmt_VO(lid_indexs, lid_bitmap, lid_path);
        assert(!isAllFalse(lid_bitmap));
        if (latest_block->global_lidmt_verify_VO(lid_bitmap, lid_path) != 1) { cout << "lidmt verify failed" << endl; }
        block_bits = block_bits & lid_kbm.bitmap;
        // cout << "lid: " << lid_kbm.key << endl;
        // cout << lid_kbm.key << ": block_bits: " << block_bits.to_string() <<
        // endl; lid树VO VO_size += lid_bitmap.size();
        // The lid tree verification path has only one leaf node and does not require aggregation
        vo.block_VO_size += 1;
        // cout << "lid bitmap size: " << lid_bitmap.size() << endl;
        vo.block_VO_size += lid_path.size() * lid_path[0].length();
        // cout << "lid_path size: " << lid_path.size() << endl;
        // cout << "lid_path length: " << lid_path[0].length() << endl;
    }

    return block_bits;
}

// Find out whether there is a transaction that meets the conditions in the block
bool tx_query(FullBlock *block, QueryStat qs, bitset<ONE_BLOCK_TX_NUM> &tx_bm, struct VO &vo) {
    unsigned int t_min = qs.tmin;
    unsigned int t_max = qs.tmax;
    BoolOp tx_op = qs.tx_operation;
    vector<string> tx_keywords(qs.tx_keywords);
    bool q_time = qs.query_time;
    // cout << "tx bitmap: " << tx_bm.to_string() << endl;
    if (q_time && (block->t_min > t_max || block->t_max < t_min)) {
        cout << "block tmin: " << block->t_min << " qs.tmax: " << qs.tmax << " block tmax: " << block->t_max
             << " qs.tmin: " << qs.tmin << endl;
        cout << "block time out" << endl;
        tx_bm.reset();
        return false;
    }

    if (qs.query_time && !(block->t_min >= qs.tmin && block->t_max <= qs.tmax)) {
        // cout << "tx bitmap: " << tx_bm.to_string() << endl;
        vector<string> tmin_nodeJsons;
        vector<string> tmax_nodeJsons;

        string tmin_VO = block->get_tmin_rimbt_VO(t_min, t_max, tmin_nodeJsons);
        vo.tx_VO_size += tmin_VO.length();

        string tmax_VO = block->get_tmax_rimbt_VO(t_min, t_max, tmax_nodeJsons);
        vo.tx_VO_size += tmax_VO.length();
        // block->tmax_range_mbt->printKeys();
        // cout << "tmin_VO: " << tmin_VO << endl;
        // cout << "tmax_VO: " << tmax_VO << endl;

        Node tmin_node, tmax_node;

        if (tmin_nodeJsons.size() >= 2) {
            string tmin_str = tmin_nodeJsons[tmin_nodeJsons.size() - 2];
            tmin_node = Node::fromString(tmin_str.c_str());
        }
        if (tmin_node.key < qs.tmax && tmin_nodeJsons.size() >= 1) {
            string tmin_str = tmin_nodeJsons[tmin_nodeJsons.size() - 1];
            tmin_node = Node::fromString(tmin_str.c_str());
        }
        tmax_node = Node::fromString(tmax_nodeJsons[0]);
        bitset<ONE_BLOCK_TX_NUM> tmin_bits(tmin_node.value);
        tx_bm = tx_bm & tmin_bits;
        bitset<ONE_BLOCK_TX_NUM> tmax_bits(tmax_node.value);
        tx_bm = tx_bm & tmax_bits;
    } else {
    }
    // cout << "after range tx bm: " << tx_bm.to_string() << endl;
    // First check the transaction-level keywords, return key||bitmap and verification path
    // Storing verification data
    vector<string> seri_txkeybms;
    // tx_keywords is the transaction keywords to be searched
    vector<unsigned int> indexs;
    // for (auto &e : blocks[i]->tx_public_keywords)
    // {
    //     cout << e << " ";
    // }
    // cout << endl;
    sort(block->tx_public_keywords.begin(), block->tx_public_keywords.end());
    for (auto &client_tx_key : tx_keywords) {
        // cout << client_tx_key << " ";
        auto it = find(block->tx_public_keywords.begin(), block->tx_public_keywords.end(), client_tx_key);
        unsigned int index = it - block->tx_public_keywords.begin();
        // cout << index << endl;
        assert(block->iimt->node_hashs.at(index) == my_blakes(block->kbms[index]));
        indexs.push_back(index);
        // cout << index << endl;
        seri_txkeybms.push_back(block->kbms[index]);
        Transaction tx = Transaction::fromString(block->txs[index]);
        // for (const auto e : tx.tx_public_keys)
        // {
        //     cout << e << " ";
        // }
        // cout << endl;
    }
    // Block iimt
    if (indexs.size() > 0) {
        vector<bool> bitmap;
        vector<string> path;
        // cout << indexs.size() << endl;
        // iimt
        block->get_iimt_VO(indexs, bitmap, path);
        assert(!isAllFalse(bitmap));
        if (block->iimt_verify_VO(bitmap, path) != 1) { cout << "block verify failed" << endl; }
        // iimt VO
        vo.tx_VO_size += bitmap.size() / 8;
        vo.tx_VO_size += path.size() * path[0].length();
        bitmap.clear();
        path.clear();
        // else
        // {
        //     cout << "block verify success" << endl;
        // }
        // cout << verify_mt_path(block->pubkey_mt, path) << endl;
    }

    // Perform logical operations on each keyword bitmap found
    struct TxKeyBitmap tx_keybm;
    if (!qs.complex_query) {
        for (const auto b : seri_txkeybms) {
            tx_keybm = TxKeyBitmap::fromString(b);
            // cout << tx_keybm.key << ": " << tx_keybm.bitmap.to_string() << endl;
            if (tx_op == AND) {
                tx_bm = tx_bm & tx_keybm.bitmap;
            } else if (tx_op == OR) {
                tx_bm = tx_bm | tx_keybm.bitmap;
            } else if (tx_op == NOT) {
                tx_bm = tx_bm & (~tx_keybm.bitmap);
            }
        }
    } else {
        assert(seri_txkeybms.size() == 3);
        struct TxKeyBitmap tx_keybm1 = TxKeyBitmap::fromString(seri_txkeybms.at(0));
        struct TxKeyBitmap tx_keybm2 = TxKeyBitmap::fromString(seri_txkeybms.at(1));
        struct TxKeyBitmap tx_keybm3 = TxKeyBitmap::fromString(seri_txkeybms.at(2));
        // NOT
        tx_bm &= (~tx_keybm1.bitmap);
        // AND
        tx_bm &= tx_keybm2.bitmap;
        // OR
        tx_bm |= tx_keybm3.bitmap;
    }

    // cout << tx_bm.to_string() << endl;
    return true;
}

// Search entry based on the found transaction
vector<string> entry_query(vector<LogFile *> &lfs, vector<unsigned int> time_offset, QueryStat qs, struct VO &vo) {
    clock_t entry_start = clock();
    vector<string> res_entrys;
    bitset<ONE_TX_ENTRY_NUM> entry_bits;
    entry_bits.set();
    if (qs.entry_operation == AND)
        entry_bits.set();
    else if (qs.entry_operation == OR && (qs.entry_keywords.size() > 0))
        entry_bits.reset();
    else if (qs.entry_operation == NOT)
        entry_bits.set();
    bool q_time = qs.query_time;
    // Need to subtract time_offset
    unsigned long long t_min = qs.tmin;
    unsigned long long t_max = qs.tmax;
    // cout << "t_min: " << t_min << endl;
    // cout << "t_max: " << t_max << endl;
    vector<string> entry_keywords(qs.entry_keywords);
    BoolOp entry_op = qs.entry_operation;
    // Find each transaction in parallel
    for (int i = 0; i < lfs.size(); i++) {
        LogFile *lf = lfs[i];
        // cout << time_offset[i] << endl;
        vector<string> t_nodeJsons;
        Node n;
        unsigned long long key_min = U_INT_MAX;
        unsigned long long key_max = U_INT_MIN;
        // Whether to query time
        if (q_time) {
            // rimbt
            string t_VO = lf->get_rimbt_VO(t_min, t_max, t_nodeJsons);
            // cout << "t_VO: " << t_VO << endl;
            vo.entry_VO_size += t_VO.length();
            // vector<unsigned int> sns;
            // Get the sn range of logs that meet the time conditions
            for (const auto &_n : t_nodeJsons) {
                n = Node::fromString(_n);
                if (n.key + time_offset[i] >= t_min && n.key + time_offset[i] <= t_max) {
                    // value is the log sequence number
                    // cout << stoi(n.value) << endl;
                    if (n.key < key_min) key_min = n.key;
                    if (n.key > key_max) key_max = n.key;
                }
            }
        }
        // cout << "sn_min: " << sn_min << endl;
        // cout << "sn_max: " << sn_max << endl;
        // Check private keywords
        // Storing verification data
        vector<string> seri_entry_keybms;
        vector<unsigned int> indexs;
        // entry_keywords is the set of private keywords to be found
        for (auto &entry_key : entry_keywords) {
            auto it = find(lf->entry_keywords.begin(), lf->entry_keywords.end(), entry_key);
            // This transaction does not contain this keyword, so a bitmap of all zeros is constructed and added.
            if (it == lf->entry_keywords.end()) {
                struct EntryKeyBitmap tmp;
                tmp.key = entry_key;
                bitset<ONE_TX_ENTRY_NUM> _b;
                _b.reset();
                tmp.bitmap = _b;
                seri_entry_keybms.push_back(EntryKeyBitmap::toString(tmp));
                continue;
            } else {
                unsigned int index = it - lf->entry_keywords.begin();
                assert(lf->iimt->node_hashs.at(index) == my_blakes(lf->kbms[index]));
                indexs.push_back(index);
                seri_entry_keybms.push_back(lf->kbms[index]);
            }
        }
        if (indexs.size() > 0) {
            vector<bool> bitmap;
            vector<string> path;
            // iimt
            lf->get_iimt_VO(indexs, bitmap, path);
            assert(!isAllFalse(bitmap));
            if (lf->iimt_verify_VO(bitmap, path) != 1) { cout << "lf verify failed" << endl; }
            // tx_iimt
            vo.entry_VO_size += bitmap.size() / 8;
            vo.entry_VO_size += path.size() * path[0].length();
            // else
            // {
            //     cout << "lf verify success" << endl;
            // }
            // cout << lf->kbms[index] << endl;
        }

        struct EntryKeyBitmap entry_keybm;
        struct Entry e;

        if (!qs.complex_query) {
            for (const auto &b : seri_entry_keybms) {
                entry_keybm = EntryKeyBitmap::fromString(b);
                // cout << entry_keybm.key << ": " << entry_keybm.bitmap.to_string() << endl;
                if (entry_op == AND) {
                    entry_bits = entry_bits & entry_keybm.bitmap;
                } else if (entry_op == OR) {
                    entry_bits = entry_bits | entry_keybm.bitmap;
                } else if (entry_op == NOT) {
                    entry_bits = entry_bits & (~entry_keybm.bitmap);
                }
            }
        } else {
            assert(seri_entry_keybms.size() == 3);
            struct EntryKeyBitmap entry_keybm1 = EntryKeyBitmap::fromString(seri_entry_keybms.at(0));
            struct EntryKeyBitmap entry_keybm2 = EntryKeyBitmap::fromString(seri_entry_keybms.at(1));
            struct EntryKeyBitmap entry_keybm3 = EntryKeyBitmap::fromString(seri_entry_keybms.at(2));
            // cout << "1: " << entry_keybm1.bitmap.to_string() << endl;
            // cout << "2: " << entry_keybm2.bitmap.to_string() << endl;
            // cout << "3: " << entry_keybm3.bitmap.to_string() << endl;
            // NOT
            entry_bits &= (~entry_keybm1.bitmap);
            // AND
            entry_bits &= entry_keybm2.bitmap;
            // OR
            entry_bits |= entry_keybm3.bitmap;
        }

        // seri_entry_keybms.clear();
        // indexs.clear();
        // cout << "entry_bits: " << entry_bits.to_string() << endl;
        for (int k = 0; k < entry_bits.size(); k++) {
            if (entry_bits.test(k)) {
                e = Entry::fromString(lf->entrys[k].c_str());
                // cout << e.sn << endl;
                if ((q_time && e.timestamp >= key_min && e.timestamp <= key_max) || (!q_time)) {
                    // assert(lf->omt->node_hashs.at(k) ==
                    // my_blakes(lf->entrys[k])); indexs.push_back(k);
                    res_entrys.push_back(lf->entrys[k]);
                }
                // cout << lf->entrys[k] << endl;
            }
        }
        if (entry_op == AND) {
            entry_bits.set();
        } else if (entry_op == OR && qs.entry_keywords.size() > 0) {
            entry_bits.reset();
        } else if (entry_op == NOT) {
            entry_bits.set();
        }
    }
    clock_t entry_end = clock();
    vo.entry_query_time += double(entry_end - entry_start) / CLOCKS_PER_SEC;
    return res_entrys;
}

void store_to_file(vector<FullBlock *> &datas, string path) {
    // cout << 0 << endl;
    ofstream ofs(path, ios::out | ios::trunc);
    // cout << 1 << endl;
    boost::archive::text_oarchive oa(ofs);
    // cout << 2 << endl;
    int i = 0;
    oa << datas;
    // cout << 3 << endl;
    ofs.close();
}
void store_to_file(vector<vector<LogFile *>> &datas, string path) {
    ofstream ofs(path, ios::out | ios::trunc);
    boost::archive::text_oarchive oa(ofs);
    oa << datas;
    ofs.close();
}
void store_to_file(LogFile *&datas, string path) {
    ofstream ofs(path, ios::out | ios::trunc);
    boost::archive::text_oarchive oa(ofs);
    oa << datas;
    ofs.close();
}
void load_from_file(vector<FullBlock *> &datas, string path) {
    ifstream ifs(path);
    boost::archive::text_iarchive ia(ifs);
    ia >> datas;
    ifs.close();
}
void load_from_file(vector<vector<LogFile *>> &datas, string path) {
    ifstream ifs(path);
    boost::archive::text_iarchive ia(ifs);
    ia >> datas;
    ifs.close();
}

void load_from_file(LogFile *&datas, string path) {
    ifstream ifs(path);
    boost::archive::text_iarchive ia(ifs);
    ia >> datas;
    ifs.close();
}

void generate_reuse_blocks_from_lfs(string lfs_path, string blocks_path, vector<FullBlock *> &reuse_blocks,
                                    string public_key_path, vector<string> public_keys) {
    vector<vector<LogFile *>> reuse_lfs;

    unsigned int blockid = 1;
    unsigned int all_blocks_num = 8192;
    unsigned int lid = 1;
    unsigned int public_key_size = public_keys.size();
    vector<vector<string>> keys;
    read_tx_keywords(keys, public_key_path, 8192 * 256);
    cout << keys.size() << endl;
    vector<string> one_tx_keys;
    // Storing all log files in a block will release
    vector<Transaction *> one_block_txs;

    vector<LogFile *> all_reuse_lfs;
    ofstream out_f("./txs_time");
    ofstream out_f1("./blocks_time");
    for (int i = 0; i < 4096; i++) {
        string lf_path = lfs_path + "/lfs_" + to_string(i);
        LogFile *lf = new LogFile();
        load_from_file(lf, lf_path);
        // cout << "load LogFile " << i << " success" << endl;
        all_reuse_lfs.push_back(lf);
    }
    unsigned long long time_offset = 0;
    for (int i = 0; i < all_blocks_num; i++) {
        // Read ONE_BLOCK_TX_NUM transactions
        for (int j = 0; j < ONE_BLOCK_TX_NUM; j++) {
            // Read transaction keywords
            one_tx_keys = keys.at(i * ONE_BLOCK_TX_NUM + j);
            unsigned int lf_index = (i * ONE_BLOCK_TX_NUM + j) % 4096;
            LogFile *lf = all_reuse_lfs.at(lf_index);
            assert(lf != NULL);
            // Save LogFile to a file
            Transaction *tx = generate_tx_from_lf(time_offset, one_tx_keys, lf);
            out_f << to_string(tx->t_min) << " " << to_string(tx->t_max) << endl;
            one_block_txs.push_back(tx);
        }
        // The transactions required for a block are generated
        // Generate a block
        FullBlock *block = new FullBlock(blockid, one_block_txs, public_keys);
        assert(block != NULL);
        cout << "generate block " << i + 1 << "success" << endl;
        out_f1 << to_string(block->t_min) << " " << to_string(block->t_max) << endl;
        reuse_blocks.push_back(block);
        blockid++;

        one_block_txs.clear();
        if (((i + 1) % 16 == 0)) { time_offset += 100000; }
        // 32 blocks are stored at a time
        if ((i + 1) % 32 == 0) {
            string reuse_block_path = blocks_path + "/block_" + to_string((i + 1) / 32);
            store_to_file(reuse_blocks, reuse_block_path);
            reuse_blocks.clear();
        }
    }
    // store_to_file(reuse_lfs, lfs_path);
    // store_to_file(reuse_blocks, blocks_path);
}

double test_generate_reuse_blocks_from_lfs(string lfs_path, string blocks_path, vector<FullBlock *> &reuse_blocks,
                                           string public_key_path, vector<string> public_keys) {
    vector<vector<LogFile *>> reuse_lfs;

    // init blockid
    unsigned int blockid = 1;
    unsigned int all_blocks_num = 1024;
    unsigned int lid = 1;
    unsigned int public_key_size = public_keys.size();
    vector<vector<string>> keys;
    read_tx_keywords(keys, public_key_path, 256 * 1024);
    cout << keys.size() << endl;
    vector<string> one_tx_keys;

    vector<Transaction *> one_block_txs;

    vector<LogFile *> all_reuse_lfs;
    for (int i = 0; i < 4096; i++) {
        string lf_path = lfs_path + "/lfs_" + to_string(i);
        LogFile *lf = new LogFile();
        load_from_file(lf, lf_path);
        // cout << "load LogFile " << i << " success" << endl;
        all_reuse_lfs.push_back(lf);
    }
    unsigned long long time_offset = 0;
    clock_t start = clock();
    for (int i = 0; i < all_blocks_num; i++) {
        for (int j = 0; j < ONE_BLOCK_TX_NUM; j++) {
            one_tx_keys = keys.at(i * ONE_BLOCK_TX_NUM + j);
            unsigned int lf_index = (i * ONE_BLOCK_TX_NUM + j) % 4096;
            if ((i != 0) && (lf_index == 0)) { time_offset += 1; }
            LogFile *lf = all_reuse_lfs.at(lf_index);
            assert(lf != NULL);

            Transaction *tx = generate_tx_from_lf(time_offset, one_tx_keys, lf);
            one_block_txs.push_back(tx);
        }

        FullBlock *block = new FullBlock(blockid, one_block_txs, public_keys);
        assert(block != NULL);
        cout << "generate block " << i + 1 << "success" << endl;

        reuse_blocks.push_back(block);
        blockid++;

        one_block_txs.clear();
        if (((i + 1) % 16 == 0)) { time_offset += 7000; }
    }
    clock_t end = clock();
    double time = double(end - start) / CLOCKS_PER_SEC;
    return time;
}

void test_generate_reuse_lfs(string lfs_path, string entry_file_path, string public_key_path,
                             vector<string> public_keys) {
    vector<vector<LogFile *>> reuse_lfs;
    // init blockid
    unsigned int blockid = 1;
    unsigned int reuse_blocks_num = 16;
    unsigned int reuse_lfs_num = ONE_BLOCK_TX_NUM * reuse_blocks_num;

    unsigned int lid = 1;
    unsigned int public_key_size = public_keys.size();
    // Store all log entries read at once
    vector<struct Entry *> entrys;
    // Stores common keywords for a log file
    vector<string> one_lf_keys;
    vector<vector<string>> keys;
    read_tx_keywords(keys, public_key_path, reuse_lfs_num + 100);
    // Read all log lines at once
    load_log_from_file(entry_file_path, "./entry_time", "./entry_keys", 2048 * 256 * 16, entrys);
    cout << "reuse entry size: " << entrys.size() << endl;
    for (int i = 0; i < reuse_blocks_num; i++) {
        // Generate ONE_BLOCK_TX_NUM transactions
        for (int j = 0; j < ONE_BLOCK_TX_NUM; j++) {
            // Read transaction keywords
            one_lf_keys = keys.at(i * ONE_BLOCK_TX_NUM + j);
            unsigned int t = i * ONE_BLOCK_TX_NUM * ONE_TX_ENTRY_NUM + j * ONE_TX_ENTRY_NUM;
            // cout << t << endl;
            vector<Entry *> batch_entrys(entrys.begin() + t, entrys.begin() + t + ONE_TX_ENTRY_NUM);
            // cout << batch_entrys.size() << endl;
            LogFile *lf = new LogFile(lid, one_lf_keys, batch_entrys);
            assert(lf != NULL);
            lid++;
            if (lid > LG_NUM) { lid = 1; }
            // Save LogFile to a file
            string lf_store_path = lfs_path + "/lfs_" + to_string(i * ONE_BLOCK_TX_NUM + j);
            store_to_file(lf, lf_store_path);
        }
    }
}

void generate_reuse_lfs(string lfs_path, string entry_file_path, string public_key_path, vector<string> public_keys) {
    vector<vector<LogFile *>> reuse_lfs;
    // init blockid
    unsigned int blockid = 1;
    unsigned int reuse_blocks_num = 16;
    unsigned int reuse_lfs_num = ONE_BLOCK_TX_NUM * reuse_blocks_num;

    unsigned int lid = 1;
    unsigned int public_key_size = public_keys.size();

    vector<struct Entry *> entrys;

    vector<string> one_lf_keys;
    vector<vector<string>> keys;

    read_tx_keywords(keys, public_key_path, reuse_lfs_num + 100);

    load_log_from_file(entry_file_path, "./entry_time", "./entry_keys", 2048 * 256 * 16, entrys);
    cout << "reuse entry size: " << entrys.size() << endl;
    ofstream of("./lfs_time");
    for (int i = 0; i < reuse_blocks_num; i++) {
        for (int j = 0; j < ONE_BLOCK_TX_NUM; j++) {
            one_lf_keys = keys.at(i * ONE_BLOCK_TX_NUM + j);
            unsigned int t = i * ONE_BLOCK_TX_NUM * ONE_TX_ENTRY_NUM + j * ONE_TX_ENTRY_NUM;
            // cout << t << endl;
            vector<Entry *> batch_entrys(entrys.begin() + t, entrys.begin() + t + ONE_TX_ENTRY_NUM);
            // cout << batch_entrys.size() << endl;
            LogFile *lf = new LogFile(lid, one_lf_keys, batch_entrys);
            of << to_string(lf->t_min) << " " << to_string(lf->t_max) << " " << endl;
            // cout << "lf_tmin: " << lf->t_min << " lf_tmax: " << lf->t_max <<
            // endl;
            assert(lf != NULL);

            lid++;
            if (lid > LG_NUM) { lid = 1; }

            string lf_store_path = lfs_path + "/lfs_" + to_string(i * ONE_BLOCK_TX_NUM + j);
            store_to_file(lf, lf_store_path);
        }
    }
}

void load_reuse_blocks(string blocks_path, vector<FullBlock *> &blocks) {
    load_from_file(blocks, blocks_path);
}

void load_reuse_lfs(string lfs_path, LogFile *&lfs) {
    load_from_file(lfs, lfs_path);
}