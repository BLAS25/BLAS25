
#include "../include/log.h"
bool operator==(vector<LogFile *> vec1, vector<LogFile *> vec2) {
    vector<string> vec1_omt_hash;
    vector<string> vec2_omt_hash;
    if (vec1.size() != vec2.size()) { return false; }
    for (int i = 0; i < vec1.size(); i++) {
        vec1_omt_hash.push_back(string2Hex(vec1[i]->omt->root->hash));
        vec2_omt_hash.push_back(string2Hex(vec2[i]->omt->root->hash));
    }

    sort(vec1_omt_hash.begin(), vec1_omt_hash.end());
    sort(vec2_omt_hash.begin(), vec2_omt_hash.end());

    for (int i = 0; i < vec1_omt_hash.size(); i++) {
        if (vec1_omt_hash[i] != vec2_omt_hash[i]) { return false; }
    }
    return true;
}

bool operator==(vector<string> entrys1, vector<string> entrys2) {
    if (entrys1.size() != entrys2.size()) { return false; }
    sort(entrys1.begin(), entrys1.end());
    sort(entrys2.begin(), entrys2.end());

    for (int i = 0; i < entrys1.size(); i++) {
        if (entrys1[i] != entrys2[i]) { return false; }
    }
    return true;
}

void get_test_blocks(vector<FullBlock *> &test_blocks) {
    vector<string> public_keys;
    string reuse_block_path = "./db/reuse/blocks";
    string public_key_path = "./else/keys/lid_all_keys";
    read_public_key(public_key_path, public_keys);
    // cout << public_keys.size() << endl;
    string block_path = reuse_block_path + "/block_" + to_string(1);
    load_from_file(test_blocks, block_path);
    update_global_trees(test_blocks[test_blocks.size() - 1], test_blocks, public_keys, 32);
}

void get_test_lfs(vector<LogFile *> &lfs) {
    LogFile *lf = new LogFile();
    string lf_path = "./db/reuse/lfs/lfs_0";
    load_from_file(lf, lf_path);
    lfs.push_back(lf);
}
// Transaction-level logic and testing, 32 blocks
// ONE_WINDOW_BLOCK_NUM must be set to 32
void test_tx_and(const vector<FullBlock *> &test_blocks) {
    QueryStat qs;
    qs.start_block = 0;
    qs.end_block = ONE_WINDOW_BLOCK_NUM;
    qs.tx_operation = AND;
    qs.tx_keywords = {"xiaomi", "2210132C"};
    qs.entry_operation = AND;
    qs.entry_keywords = {"E", "HwSystemManager"};
    qs.lid = 1;
    qs.query_lid = true;
    qs.query_time = false;
    qs.tmin = U_INT_MIN;
    qs.tmax = U_INT_MAX;
    // Traverse each block to query transactions that meet the conditions
    vector<LogFile *> right_lfs;
    for (int i = 0; i < test_blocks.size(); i++) {
        for (int j = 0; j < test_blocks[i]->txs.size(); j++) {
            Transaction tx = Transaction::fromString(test_blocks[i]->txs[j]);
            bool flag = true;
            if (qs.query_lid == true) {
                if (tx.lid == qs.lid) {
                    for (auto k : qs.tx_keywords) {
                        auto it = find(tx.tx_public_keys.begin(), tx.tx_public_keys.end(), k);
                        if (it == tx.tx_public_keys.end()) {
                            flag = false;
                            break;
                        }
                    }
                } else {
                    flag = false;
                }
            } else {
                for (auto k : qs.tx_keywords) {
                    auto it = find(tx.tx_public_keys.begin(), tx.tx_public_keys.end(), k);
                    if (it == tx.tx_public_keys.end()) {
                        flag = false;
                        break;
                    }
                }
            }
            if (flag == true) {
                unsigned int lf_index = (i * ONE_BLOCK_TX_NUM + j) % 4096;
                string lf_path = "./db/reuse/lfs/lfs_" + to_string(lf_index);
                LogFile *lf = new LogFile;
                load_from_file(lf, lf_path);
                right_lfs.push_back(lf);
            }
        }
    }
    vector<unsigned int> time_offset;
    struct VO optim_vo;
    struct VO normal_vo;
    vector<LogFile *> optim_lfs = optim_query_one_window_lfs(test_blocks, time_offset, qs, optim_vo, 1);
    vector<LogFile *> normal_lfs = query_one_window_lfs(test_blocks, time_offset, qs, normal_vo, 1);
    // cout << optim_lfs.size() << endl;
    // cout << (*right_lfs[0] == *optim_lfs[0]) << endl;
    // cout << string2Hex(right_lfs[0]->omt->root->hash) << endl;
    // cout << string2Hex(optim_lfs[0]->omt->root->hash) << endl;

    if (right_lfs == optim_lfs) {
        cout << "optim tx boolean AND query test success" << endl;
    } else {
        cout << "optim tx boolean AND query test failed" << endl;
    }
    if (right_lfs == normal_lfs) {
        cout << "normal tx boolean AND query test success" << endl;
    } else {
        cout << "normal tx boolean AND query test failed" << endl;
    }
}

// Transaction-level logic or testing, 32 blocks
// ONE_WINDOW_BLOCK_NUM must be set as 32
void test_tx_or(const vector<FullBlock *> &test_blocks) {
    QueryStat qs;
    qs.start_block = 0;
    qs.end_block = ONE_WINDOW_BLOCK_NUM;
    qs.tx_operation = OR;
    qs.tx_keywords = {"xiaomi", "2210132C"};
    qs.lid = 1;
    qs.query_lid = true;
    qs.query_time = false;
    qs.tmin = U_INT_MIN;
    qs.tmax = U_INT_MAX;
    // Traverse each block to query transactions that meet the conditions
    vector<LogFile *> right_lfs;
    for (int i = 0; i < test_blocks.size(); i++) {
        for (int j = 0; j < test_blocks[i]->txs.size(); j++) {
            Transaction tx = Transaction::fromString(test_blocks[i]->txs[j]);
            bool flag = false;
            if (qs.query_lid == true) {
                if ((tx.lid == qs.lid)) {
                    for (auto k : qs.tx_keywords) {
                        auto it = find(tx.tx_public_keys.begin(), tx.tx_public_keys.end(), k);
                        if (it != tx.tx_public_keys.end()) {
                            flag = true;
                            break;
                        }
                    }
                }
            } else {
                for (auto k : qs.tx_keywords) {
                    auto it = find(tx.tx_public_keys.begin(), tx.tx_public_keys.end(), k);
                    if (it != tx.tx_public_keys.end()) {
                        flag = true;
                        break;
                    }
                }
            }
            if (flag == true) {
                unsigned int lf_index = (i * ONE_BLOCK_TX_NUM + j) % 4096;
                string lf_path = "./db/reuse/lfs/lfs_" + to_string(lf_index);
                LogFile *lf = new LogFile;
                load_from_file(lf, lf_path);
                right_lfs.push_back(lf);
            }
        }
    }
    vector<unsigned int> time_offset;
    struct VO optim_vo;
    struct VO normal_vo;
    vector<LogFile *> optim_lfs = optim_query_one_window_lfs(test_blocks, time_offset, qs, optim_vo, 1);
    vector<LogFile *> normal_lfs = query_one_window_lfs(test_blocks, time_offset, qs, normal_vo, 1);
    // cout << right_lfs.size() << endl;
    // cout << optim_lfs.size() << endl;
    // cout << normal_lfs.size() << endl;

    if (right_lfs == optim_lfs) {
        cout << "optim tx boolean OR query test success" << endl;
    } else {
        cout << "optim tx boolean OR query test failed" << endl;
    }
    if (right_lfs == normal_lfs) {
        cout << "normal tx boolean OR query test success" << endl;
    } else {
        cout << "normal tx boolean OR query test failed" << endl;
    }
}
// Transaction level logical NOT test, 32 blocks
// ONE_WINDOW_BLOCK_NUM must be set to 32
void test_tx_not(const vector<FullBlock *> &test_blocks) {
    QueryStat qs;
    qs.start_block = 0;
    qs.end_block = ONE_WINDOW_BLOCK_NUM;
    qs.tx_operation = NOT;
    // qs.tx_keywords = {"pk110", "pk11", "pk1",  "pk15", "pk20", "pk2", "pk23",
    //                   "pk34",  "pk47", "pk80", "pk3",  "pk4",  "pk5", "pk6",
    //                   "pk7",   "pk8",  "pk9",  "pk10", "pk12", "pk13"};
    qs.tx_keywords = {"xiaomi", "2210132C"};
    qs.lid = 1;
    qs.query_lid = true;
    qs.query_time = false;
    qs.tmin = U_INT_MIN;
    qs.tmax = U_INT_MAX;
    // Traverse each block to query transactions that meet the conditions
    vector<LogFile *> right_lfs;
    for (int i = 0; i < test_blocks.size(); i++) {
        for (int j = 0; j < test_blocks[i]->txs.size(); j++) {
            Transaction tx = Transaction::fromString(test_blocks[i]->txs[j]);
            bool flag = true;
            if (qs.query_lid) {
                if (tx.lid == qs.lid) {
                    for (auto k : qs.tx_keywords) {
                        auto it = find(tx.tx_public_keys.begin(), tx.tx_public_keys.end(), k);
                        if (it != tx.tx_public_keys.end()) {
                            flag = false;
                            break;
                        }
                    }
                } else {
                    flag = false;
                }
            } else {
                for (auto k : qs.tx_keywords) {
                    auto it = find(tx.tx_public_keys.begin(), tx.tx_public_keys.end(), k);
                    if (it != tx.tx_public_keys.end()) {
                        flag = false;
                        break;
                    }
                }
            }

            if (flag == true) {
                unsigned int lf_index = (i * ONE_BLOCK_TX_NUM + j) % 4096;
                string lf_path = "./db/reuse/lfs/lfs_" + to_string(lf_index);
                LogFile *lf = new LogFile;
                load_from_file(lf, lf_path);
                right_lfs.push_back(lf);
            }
        }
    }
    vector<unsigned int> time_offset;
    struct VO optim_vo;
    struct VO normal_vo;
    vector<LogFile *> optim_lfs = optim_query_one_window_lfs(test_blocks, time_offset, qs, optim_vo, 1);
    vector<LogFile *> normal_lfs = query_one_window_lfs(test_blocks, time_offset, qs, normal_vo, 1);

    // cout << (*right_lfs[0] == *optim_lfs[0]) << endl;
    // cout << string2Hex(right_lfs[0]->omt->root->hash) << endl;
    // cout << string2Hex(optim_lfs[0]->omt->root->hash) << endl;
    cout << optim_lfs.size() << endl;
    if (right_lfs == optim_lfs) {
        cout << "optim tx boolean NOT query test success" << endl;
    } else {
        cout << "optim tx boolean NOT query test failed" << endl;
    }
    if (right_lfs == normal_lfs) {
        cout << "normal tx boolean NOT query test success" << endl;
    } else {
        cout << "normal tx boolean NOT query test failed" << endl;
    }
}
// Transaction-level range query test, 32 blocks
// ONE_WINDOW_BLOCK_NUM must be set to 32
void test_tx_range(const vector<FullBlock *> &test_blocks) {
    QueryStat qs;
    qs.start_block = 0;
    qs.end_block = ONE_WINDOW_BLOCK_NUM;
    qs.tx_operation = OR;
    qs.tx_keywords = {};
    qs.lid = 1;
    qs.query_lid = true;
    qs.query_time = true;
    qs.tmin = 1526849360;
    qs.tmax = 1545146912;

    // Traverse each block to query transactions that meet the conditions
    vector<LogFile *> right_lfs;
    unsigned long long time_offset = 0;
    for (int i = 0; i < test_blocks.size(); i++) {
        for (int j = 0; j < test_blocks[i]->txs.size(); j++) {
            Transaction tx = Transaction::fromString(test_blocks[i]->txs[j]);
            bool flag = false;
            if (qs.query_lid) {
                if (tx.lid == qs.lid) {
                    if (tx.t_min + time_offset <= qs.tmax && tx.t_max + time_offset >= qs.tmin) { flag = true; }
                }
            } else {
                if (tx.t_min + time_offset <= qs.tmax && tx.t_max + time_offset >= qs.tmin) {
                    // cout << i << " " << j << endl;
                    flag = true;
                }
            }
            if (flag == true) {
                unsigned int lf_index = (i * ONE_BLOCK_TX_NUM + j) % 4096;
                string lf_path = "./db/reuse/lfs/lfs_" + to_string(lf_index);
                LogFile *lf = new LogFile;
                load_from_file(lf, lf_path);
                right_lfs.push_back(lf);
            }
        }
        if (((i + 1) % 16 == 0)) { time_offset += 100000; }
    }
    vector<unsigned int> time_offset1;
    struct VO optim_vo;
    struct VO normal_vo;
    vector<LogFile *> optim_lfs = optim_query_one_window_lfs(test_blocks, time_offset1, qs, optim_vo, 1);
    vector<LogFile *> normal_lfs = query_one_window_lfs(test_blocks, time_offset1, qs, normal_vo, 1);
    // cout << optim_lfs.size() << endl;
    // cout << normal_lfs.size() << endl;
    // cout << right_lfs.size() << endl;
    if (right_lfs == optim_lfs) {
        cout << "optim range query test success" << endl;
    } else {
        cout << "optim range query test failed" << endl;
    }
    if (right_lfs == normal_lfs) {
        cout << "normal range query test success" << endl;
    } else {
        cout << "normal range query test failed" << endl;
    }
}

// The size of test_lfs is 1
void test_entry_and(vector<LogFile *> &test_lfs) {
    QueryStat qs;
    qs.start_block = 0;
    qs.end_block = ONE_WINDOW_BLOCK_NUM;
    qs.entry_operation = AND;
    qs.entry_keywords = {"E", "HwSystemManager"};
    qs.query_time = false;
    qs.tmin = U_INT_MIN;
    qs.tmax = U_INT_MAX;
    vector<unsigned int> time_offset;
    time_offset.push_back(0);
    struct VO optim_vo;
    vector<string> right_entrys;
    vector<string> all_entrys = test_lfs.at(0)->entrys;

    for (int i = 0; i < all_entrys.size(); i++) {
        bool flag = true;
        Entry e = Entry::fromString(all_entrys[i]);
        for (auto k : qs.entry_keywords) {
            auto it = find(e.keywords.begin(), e.keywords.end(), k);
            if (it == e.keywords.end()) {
                flag = false;
                break;
            }
        }
        if (flag == true) { right_entrys.push_back(all_entrys[i]); }
    }
    vector<string> res_entrys = entry_query(test_lfs, time_offset, qs, optim_vo);
    // cout << right_entrys.size() << endl;
    // cout << res_entrys.size() << endl;
    if (res_entrys == right_entrys) {
        cout << "entry boolean AND query test success" << endl;
    } else {
        cout << "entry boolean AND query test failed" << endl;
    }
}
void test_entry_or(vector<LogFile *> &test_lfs) {
    QueryStat qs;
    qs.start_block = 0;
    qs.end_block = ONE_WINDOW_BLOCK_NUM;
    qs.entry_operation = OR;
    qs.entry_keywords = {"E", "HwSystemManager"};
    qs.lid = 1;
    qs.query_lid = false;
    qs.query_time = false;
    qs.tmin = U_INT_MIN;
    qs.tmax = U_INT_MAX;
    vector<unsigned int> time_offset;
    time_offset.push_back(0);
    struct VO optim_vo;
    vector<string> right_entrys;
    vector<string> all_entrys = test_lfs.at(0)->entrys;

    for (int i = 0; i < all_entrys.size(); i++) {
        bool flag = false;
        Entry e = Entry::fromString(all_entrys[i]);
        for (auto k : qs.entry_keywords) {
            auto it = find(e.keywords.begin(), e.keywords.end(), k);
            if (it != e.keywords.end()) {
                flag = true;
                break;
            }
        }
        if (flag == true) { right_entrys.push_back(all_entrys[i]); }
    }
    vector<string> res_entrys = entry_query(test_lfs, time_offset, qs, optim_vo);
    // cout << right_entrys.size() << endl;
    if (res_entrys == right_entrys) {
        cout << "entry boolean OR query test success" << endl;
    } else {
        cout << "entry boolean OR query test failed" << endl;
    }
}
void test_entry_not(vector<LogFile *> &test_lfs) {
    QueryStat qs;
    qs.start_block = 0;
    qs.end_block = ONE_WINDOW_BLOCK_NUM;
    qs.entry_operation = NOT;
    qs.entry_keywords = {"E", "HwSystemManager"};
    qs.lid = 1;
    qs.query_lid = false;
    qs.query_time = false;
    qs.tmin = U_INT_MIN;
    qs.tmax = U_INT_MAX;
    vector<unsigned int> time_offset;
    time_offset.push_back(0);
    struct VO optim_vo;
    vector<string> right_entrys;
    vector<string> all_entrys = test_lfs.at(0)->entrys;

    for (int i = 0; i < all_entrys.size(); i++) {
        bool flag = true;
        Entry e = Entry::fromString(all_entrys[i]);
        for (auto k : qs.entry_keywords) {
            auto it = find(e.keywords.begin(), e.keywords.end(), k);
            if (it != e.keywords.end()) {
                flag = false;
                break;
            }
        }
        if (flag == true) { right_entrys.push_back(all_entrys[i]); }
    }
    vector<string> res_entrys = entry_query(test_lfs, time_offset, qs, optim_vo);
    // cout << res_entrys.size() << endl;
    // cout << right_entrys.size() << endl;
    if (res_entrys == right_entrys) {
        cout << "entry boolean NOT query test success" << endl;
    } else {
        cout << "entry boolean NOT query test failed" << endl;
    }
}

void test_entry_range(vector<LogFile *> &test_lfs) {
    QueryStat qs;
    qs.start_block = 0;
    qs.end_block = ONE_WINDOW_BLOCK_NUM;
    qs.entry_operation = OR;
    qs.entry_keywords = {};
    qs.query_time = true;
    qs.tmin = 1;
    qs.tmax = 512;
    vector<unsigned int> time_offset;
    time_offset.push_back(0);
    struct VO optim_vo;
    vector<string> right_entrys;
    vector<string> all_entrys = test_lfs.at(0)->entrys;

    for (int i = 0; i < all_entrys.size(); i++) {
        Entry e = Entry::fromString(all_entrys[i]);
        if (e.timestamp >= qs.tmin && e.timestamp <= qs.tmax) { right_entrys.push_back(all_entrys[i]); }
    }
    vector<string> res_entrys = entry_query(test_lfs, time_offset, qs, optim_vo);
    // cout << right_entrys.size() << endl;
    // cout << res_entrys.size() << endl;
    if (res_entrys == right_entrys) {
        cout << "entry range query test success" << endl;
    } else {
        cout << "entry range AND query test failed" << endl;
    }
}