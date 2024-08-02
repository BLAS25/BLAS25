#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <limits>
#include <ctime>
#include <iomanip>

#include "include/log.h"
#include "include/util.h"
#include "include/threadpool.h"
#include "include/test.h"

using namespace std;
int main() {
    string config_path = "./config.ini";
    boost::property_tree::ptree pt = read_config(config_path);
    string lf_key_path = pt.get<string>("Config.lf_all_key_path");
    string origin_entry_path = pt.get<string>("Config.origin_entry_path");
    string processed_entry_path = pt.get<string>("Config.processed_entry_path");
    string reuse_lfs_dir = pt.get<string>("Config.reuse_lfs_dir");
    string reuse_block_dir = pt.get<string>("Config.reuse_block_dir");
    string lfs_public_key_path = pt.get<string>("Config.lfs_public_key_path");
    bool gen_reuse_blocks = pt.get<bool>("Config.gen_reuse_blocks");
    bool gen_reuse_lfs = pt.get<bool>("Config.gen_reuse_lfs");
    bool test_gen_block_time = pt.get<bool>("Config.test_gen_block_time");
    bool test = pt.get<bool>("Config.test");
    bool gen_blockchain = pt.get<bool>("Config.gen_blockchain");
    bool gen_vchain_plus = pt.get<bool>("Config.gen_vchain_plus");
    bool optim = pt.get<bool>("Query.optim");
    bool normal = pt.get<bool>("Query.normal");
    string tx_op = pt.get<string>("Query.tx_op");
    string entry_op = pt.get<string>("Query.entry_op");
    int tx_keys_num = pt.get<int>("Query.tx_keys_num");
    int entry_keys_num = pt.get<int>("Query.entry_keys_num");
    bool query_lid = pt.get<bool>("Query.query_lid");
    bool query_range = pt.get<bool>("Range.query_range");
    bool only_query_range = pt.get<bool>("Range.only_query_range");
    bool only_range_multi_block = pt.get<bool>("Range.only_range_multi_block");
    bool complex_query = pt.get<bool>("Query.complex_query");
    double range = pt.get<double>("Range.range");

    vector<string> lf_keys;
    read_public_key(lf_key_path, lf_keys);
    vector<FullBlock *> blocks;
    // Generate a dataset for vchain_plus
    if (gen_vchain_plus) {
        gen_vchain_entry_dataset();
        gen_vchain_tx_dataset();
        return 0;
    }
    if (gen_reuse_lfs) {
        generate_reuse_lfs(reuse_lfs_dir, processed_entry_path, lfs_public_key_path, lf_keys);
        return 0;
    }
    if (gen_reuse_blocks) {
        generate_reuse_blocks_from_lfs(reuse_lfs_dir, reuse_block_dir, blocks, lfs_public_key_path, lf_keys);
        return 0;
    }

    // Test block build time
    if (test_gen_block_time) {
        LogFile *lf;
        load_from_file(lf, "./db/test_gen_block_time/lfs/lfs_0");
        cout << lf->get_size() / 1024 << endl;
        return 0;
        cout << 1 << endl;
        assert(ONE_WINDOW_BLOCK_NUM == 1024);
        assert(WINDOW_NUM == 1);
        // Generate 4096 log files
        clock_t start = clock();
        test_generate_reuse_lfs("./db/test_gen_block_time/lfs", processed_entry_path, lfs_public_key_path, lf_keys);
        clock_t end = clock();
        cout << "LG File Construction time: " << (double)(end - start) / CLOCKS_PER_SEC / 4096 << "s/lg_file" << endl;
        // Generate 1024 blocks and calculate the average time and ADS Size
        double time1 = test_generate_reuse_blocks_from_lfs(
            "./db/test_gen_block_time/lfs", "./db/test_gen_block_time/blocks", blocks, lfs_public_key_path, lf_keys);
        start = clock();
        update_global_trees(blocks[1023], blocks, lf_keys, 1024);
        end = clock();
        double time = time1 + double(end - start) / CLOCKS_PER_SEC;
        double per_block_time = time / 1024;
        cout << "construction time: " << per_block_time << "s/block" << endl;
        unsigned int size = 0;
        for (auto e : blocks) { size += e->get_size(); }
        cout << "ADS Size: " << double(size) / 1024 / 1024 << "KB/block" << endl;
        return 0;
    }
    // Perform traversal query and ADS query within 32 blocks and compare whether the results are consistent
    if (test) {
        vector<FullBlock *> test_blocks;
        vector<LogFile *> test_lfs;
        assert(ALL_BLOCK_NUM == 32);
        get_test_blocks(test_blocks);
        test_tx_and(test_blocks);
        test_tx_or(test_blocks);
        test_tx_not(test_blocks);
        test_tx_range(test_blocks);

        get_test_lfs(test_lfs);
        test_entry_and(test_lfs);
        test_entry_or(test_lfs);
        test_entry_not(test_lfs);
        test_entry_range(test_lfs);
        return 0;
    }
    // add_keyword_to_entry(origin_entry_path,
    // processed_entry_path, private_keys,
    // ONE_ENTRY_KEY_NUM); return 0;

    if (gen_blockchain) {
        vector<FullBlock *> reuse_blocks;
        ThreadPool pool(8);
        for (int i = 1; i <= ALL_BLOCK_NUM / 32; i++) {
            string block_path = reuse_block_dir + "/block_" + to_string(i);
            load_from_file(reuse_blocks, block_path);
            blocks.insert(blocks.end(), reuse_blocks.begin(), reuse_blocks.end());
            reuse_blocks.clear();
        }
        for (int i = 0; i < blocks.size(); i++) {
            if ((i + 1) % ONE_WINDOW_BLOCK_NUM == 0) {
                // pool.enqueue(update_global_trees, ref(blocks[i]), ref(blocks), public_keys, i + 1);
                update_global_trees(blocks[i], blocks, lf_keys, i + 1);
            }
        }
    }
    string blocks_128_1024 = "./db/test/blocks_128-1024";
    string blocks_256_1024 = "./db/test/blocks_256-1024";
    string blocks_512_1024 = "./db/test/blocks_512-1024";
    string blocks_1024_1024 = "./db/test/blocks_1024-1024";
    string blocks_1024_4096 = "./db/test/blocks_1024-4096";
    string blocks_1024_8192 = "./db/test/blocks_1024-8192";
    string normal_blocks_4096 = "./db/test/normal_blocks_4096";
    string normal_blocks_8192 = "./db/test/normal_blocks_8192";
    if (ONE_WINDOW_BLOCK_NUM == 128) {
        if (gen_blockchain) {
            store_to_file(blocks, blocks_128_1024);
        } else {
            load_from_file(blocks, blocks_128_1024);
        }
    } else if (ONE_WINDOW_BLOCK_NUM == 256) {
        if (gen_blockchain) {
            store_to_file(blocks, blocks_256_1024);
        } else {
            load_from_file(blocks, blocks_256_1024);
        }
    } else if (ONE_WINDOW_BLOCK_NUM == 512) {
        if (gen_blockchain) {
            store_to_file(blocks, blocks_512_1024);
        } else {
            load_from_file(blocks, blocks_512_1024);
        }
    } else if (ONE_WINDOW_BLOCK_NUM == 1024 && WINDOW_NUM == 1) {
        if (gen_blockchain) {
            store_to_file(blocks, blocks_1024_1024);
        } else {
            load_from_file(blocks, blocks_1024_1024);
        }
    } else if (ONE_WINDOW_BLOCK_NUM == 1024 && WINDOW_NUM == 4) {
        if (gen_blockchain) {
            store_to_file(blocks, blocks_1024_4096);
        } else {
            load_from_file(blocks, blocks_1024_4096);
        }
    } else if (ONE_WINDOW_BLOCK_NUM == 1024 && WINDOW_NUM == 8) {
        if (gen_blockchain) {
            store_to_file(blocks, blocks_1024_8192);
        } else {
            load_from_file(blocks, blocks_1024_8192);
        }
    } else if (ONE_WINDOW_BLOCK_NUM == 4096 && WINDOW_NUM == 1) {
        if (gen_blockchain) {
            store_to_file(blocks, normal_blocks_4096);
        } else {
            load_from_file(blocks, normal_blocks_4096);
        }
    } else if (ONE_WINDOW_BLOCK_NUM == 8192 && WINDOW_NUM == 1) {
        if (gen_blockchain) {
            store_to_file(blocks, normal_blocks_8192);
        } else {
            load_from_file(blocks, normal_blocks_8192);
        }
    }
    cout << "load block success!" << endl;

    QueryStat qs;
    if (tx_op == "AND")
        qs.tx_operation = AND;
    else if (tx_op == "OR")
        qs.tx_operation = OR;
    else { qs.tx_operation = NOT; }

    if (entry_op == "AND")
        qs.entry_operation = AND;
    else if (entry_op == "OR")
        qs.entry_operation = OR;
    else
        qs.entry_operation = NOT;
    if (qs.entry_operation != NOT) {
        if (entry_keys_num == 2)
            qs.entry_keywords = {"HwSystemManager", "E"};
        else if (entry_keys_num == 3)
            qs.entry_keywords = {"HwSystemManager", "E", "as"};
        else if (entry_keys_num == 4)
            qs.entry_keywords = {"HwSystemManager", "E", "as", "BetaClub"};
        else if (entry_keys_num == 5)
            qs.entry_keywords = {"HwSystemManager", "E", "as", "BetaClub", "ANYOFFICE"};
        else if (entry_keys_num == 6)
            qs.entry_keywords = {"HwSystemManager", "E", "as", "BetaClub", "ANYOFFICE", "APKCloudAccountImpl"};
        else if (entry_keys_num == 7)
            qs.entry_keywords = {"HwSystemManager",     "E",       "as", "BetaClub", "ANYOFFICE",
                                 "APKCloudAccountImpl", "AUTOHOME"};
        else if (entry_keys_num == 8)
            qs.entry_keywords = {"HwSystemManager",     "E",        "as",         "BetaClub", "ANYOFFICE",
                                 "APKCloudAccountImpl", "AUTOHOME", "CHR_ChrUtil"};
        else if (entry_keys_num == 9)
            qs.entry_keywords = {"HwSystemManager",     "E",        "as",          "BetaClub",           "ANYOFFICE",
                                 "APKCloudAccountImpl", "AUTOHOME", "CHR_ChrUtil", "DownloadLogicHelper"};
        else if (entry_keys_num == 10)
            qs.entry_keywords = {"HwSystemManager",
                                 "E",
                                 "as",
                                 "BetaClub",
                                 "ANYOFFICE",
                                 "APKCloudAccountImpl",
                                 "AUTOHOME",
                                 "CHR_ChrUtil",
                                 "DownloadLogicHelper",
                                 "DpmAp"};
    }
    qs.lid = 1;
    if (query_lid) qs.query_lid = true;
    qs.query_time = false;
    qs.tmin = U_INT_MIN;
    qs.tmax = U_INT_MAX;
    // Range query, set the time range to 20%
    if (query_range) {
        qs.query_time = true;
        qs.tmin = 1526849360;
        if (ALL_BLOCK_NUM == 128)
            // qs.tmax = 1527373894; // 0.5
            qs.tmax = 1527173894; // 0.2
        else if (ALL_BLOCK_NUM == 512)
            // qs.tmax = 1528573894; // 0.5
            qs.tmax = 1527673894; // 0.2
        else if (ALL_BLOCK_NUM == 1024)
            // qs.tmax = 1530173894; // 0.5
            qs.tmax = 1528273894; // 0.2
        else if (ALL_BLOCK_NUM == 4096)
            // qs.tmax = 1539773894; // 0.5
            qs.tmax = 1532073894; // 0.2
        else if (ALL_BLOCK_NUM == 8192)
            // qs.tmax = 1552573894; // 0.5
            qs.tmax = 1537273894; // 0.2
    }
    // Only query the range, fix the number of blocks, adjust the query range
    if (only_query_range) {
        assert(ALL_BLOCK_NUM == 1024);
        qs.tx_keywords = {};
        qs.tx_operation = OR;
        qs.entry_operation = OR;
        qs.entry_keywords = {};
        qs.query_time = true;
        qs.tmin = 1526849360;
        if (range == 0.1)
            qs.tmax = 1527673894; // 0.1
        else if (range == 0.2)
            qs.tmax = 1528273894; // 0.2
        else if (range == 0.3)
            qs.tmax = 1528973894; // 0.3
        else if (range == 0.4)
            qs.tmax = 1529573894; // 0.4
        else if (range == 0.5)
            qs.tmax = 1530173894; // 0.5
    }
    // Only query the range, fixed range
    if (only_range_multi_block) {
        qs.tx_keywords = {};
        qs.tx_operation = OR;
        qs.entry_operation = OR;
        qs.entry_keywords = {};
        qs.query_time = true;
        qs.tmin = 1526849360;
        if (ALL_BLOCK_NUM == 128)
            // qs.tmax = 1527073894;    //0.1
            qs.tmax = 1527173894; // 0.2
        // qs.tmax = 1527373894; // 0.5
        else if (ALL_BLOCK_NUM == 512)
            // qs.tmax = 1527373894;
            qs.tmax = 1527673894; // 0.2
        else if (ALL_BLOCK_NUM == 1024)
            // qs.tmax = 1527673894;
            qs.tmax = 1528273894; // 0.2
        else if (ALL_BLOCK_NUM == 4096)
            // qs.tmax = 1529573894;
            qs.tmax = 1532273894; // 0.2
        else if (ALL_BLOCK_NUM == 8192)
            // qs.tmax = 1532173894;
            qs.tmax = 1537173894;
    }
    qs.complex_query = complex_query;
    cout << "start query" << endl;
    // Optimization solution query
    if (optim) {
        if (!only_query_range && !only_range_multi_block) {
            cout << "optim query Boolean AND" << endl;
            if (qs.tx_operation != NOT) {
                if (tx_keys_num == 2)
                    qs.tx_keywords = {"xiaomi", "2211133C"};
                else if (tx_keys_num == 3)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9"};
                else if (tx_keys_num == 4)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10"};
                else if (tx_keys_num == 5)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G"};
                else if (tx_keys_num == 6)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G", "11"};
                else if (tx_keys_num == 7)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G", "11", "12"};
                else if (tx_keys_num == 8)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G", "11", "12", "13"};
                else if (tx_keys_num == 9)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G", "11", "12", "13", "14"};
                else if (tx_keys_num == 10)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G", "11", "12", "13", "14", "sony"};
            }
        } else
            cout << "only range query" << endl;
        // Hybrid Query

        if (qs.complex_query) {
            qs.tx_operation = OR;
            qs.tx_keywords = {"sony", "14", "xiaomi"};
            qs.entry_keywords = {"as", "HwSystemManager", "E"};
        }

        struct VO optim_vo = {0};
        vector<LogFile *> res_lfs;
        vector<string> res_entrys;
        for (int i = 0; i < WINDOW_NUM; i++) {
            vector<unsigned int> time_offset;
            vector<FullBlock *> one_winodw_blocks;
            one_winodw_blocks.assign(blocks.begin() + i * ONE_WINDOW_BLOCK_NUM,
                                     blocks.begin() + (i + 1) * ONE_WINDOW_BLOCK_NUM);
            vector<LogFile *> lfs = optim_query_one_window_lfs(one_winodw_blocks, time_offset, qs, optim_vo, i + 1);
            res_lfs.insert(res_lfs.end(), lfs.begin(), lfs.end());

            vector<string> entrys = entry_query(lfs, time_offset, qs, optim_vo);
            res_entrys.insert(res_entrys.end(), entrys.begin(), entrys.end());
        }
        optim_vo.all_VO_size = (optim_vo.block_VO_size + optim_vo.tx_VO_size + optim_vo.entry_VO_size);
        optim_vo.all_query_time = (optim_vo.block_query_time + optim_vo.tx_query_time + optim_vo.entry_query_time);
        cout << "some entrys: " << endl;
        for (int i = 0; i < res_entrys.size(); i++) {
            Entry e = Entry::fromString(res_entrys[i]);
            cout << e.timestamp << " " << e.log_value << endl;
            if (i == 4) break;
        }
        cout << "result LG_FILE number: " << res_lfs.size() << endl;
        cout << "result entrys number: " << res_entrys.size() << endl;
        cout << "block query time: " << optim_vo.block_query_time * 1000 << "ms" << endl;
        cout << "tx query time: " << optim_vo.tx_query_time * 1000 << "ms" << endl;
        cout << "entry query time: " << optim_vo.entry_query_time * 1000 << "ms" << endl;
        cout << "block VO size: " << optim_vo.block_VO_size / 1024 << "KB" << endl;
        cout << "tx VO size: " << optim_vo.tx_VO_size / 1024 << "KB" << endl;
        cout << "entry VO size: " << optim_vo.entry_VO_size / 1024 << "KB" << endl;
        cout << "all query time: " << fixed << setprecision(2) << optim_vo.all_query_time * 1000 << "ms" << endl;
        cout << "all VO size: " << fixed << setprecision(2) << optim_vo.all_VO_size / 1024 << "KB" << endl;
        res_lfs.clear();
        res_entrys.clear();
        cout << endl;

        if (only_query_range) { return 0; }
        if (only_range_multi_block) { return 0; }
        if (complex_query) { return 0; }
        // OR
        cout << "optim query Boolean OR" << endl;
        qs.tx_operation = OR;
        qs.entry_operation = OR;
        optim_vo = {0};
        for (int i = 0; i < WINDOW_NUM; i++) {
            vector<unsigned int> time_offset;
            vector<FullBlock *> one_winodw_blocks;
            one_winodw_blocks.assign(blocks.begin() + i * ONE_WINDOW_BLOCK_NUM,
                                     blocks.begin() + (i + 1) * ONE_WINDOW_BLOCK_NUM);
            vector<LogFile *> lfs = optim_query_one_window_lfs(one_winodw_blocks, time_offset, qs, optim_vo, i + 1);
            res_lfs.insert(res_lfs.end(), lfs.begin(), lfs.end());

            vector<string> entrys = entry_query(lfs, time_offset, qs, optim_vo);
            res_entrys.insert(res_entrys.end(), entrys.begin(), entrys.end());
        }
        optim_vo.all_VO_size = (optim_vo.block_VO_size + optim_vo.tx_VO_size + optim_vo.entry_VO_size);
        optim_vo.all_query_time = (optim_vo.block_query_time + optim_vo.tx_query_time + optim_vo.entry_query_time);
        cout << "some entrys: " << endl;
        for (int i = 0; i < res_entrys.size(); i++) {
            Entry e = Entry::fromString(res_entrys[i]);
            cout << e.timestamp << " " << e.log_value << endl;
            if (i == 4) break;
        }
        cout << "result LG_FILE number: " << res_lfs.size() << endl;
        cout << "result entrys number: " << res_entrys.size() << endl;
        cout << "block query time: " << optim_vo.block_query_time * 1000 << "ms" << endl;
        cout << "tx query time: " << optim_vo.tx_query_time * 1000 << "ms" << endl;
        cout << "entry query time: " << optim_vo.entry_query_time * 1000 << "ms" << endl;
        cout << "block VO size: " << optim_vo.block_VO_size / 1024 << "KB" << endl;
        cout << "tx VO size: " << optim_vo.tx_VO_size / 1024 << "KB" << endl;
        cout << "entry VO size: " << optim_vo.entry_VO_size / 1024 << "KB" << endl;
        cout << "all query time: " << fixed << setprecision(2) << optim_vo.all_query_time * 1000 << "ms" << endl;
        cout << "all VO size: " << fixed << setprecision(2) << optim_vo.all_VO_size / 1024 << "KB" << endl;
        res_lfs.clear();
        res_entrys.clear();
        cout << endl;
        // NOT
        qs.tx_operation = NOT;
        qs.entry_operation = NOT;

        if (tx_keys_num == 2)
            qs.tx_keywords = {"9", "2211133C"};
        else if (tx_keys_num == 3)
            qs.tx_keywords = {"9", "2211133C", "10"};
        else if (tx_keys_num == 4)
            qs.tx_keywords = {"9", "2211133C", "10", "11"};
        else if (tx_keys_num == 5)
            qs.tx_keywords = {"9", "2211133C", "10", "11", "12"};
        optim_vo = {0};
        cout << "optim query Boolean NOT" << endl;
        for (int i = 0; i < WINDOW_NUM; i++) {
            vector<unsigned int> time_offset;
            vector<FullBlock *> one_winodw_blocks;
            one_winodw_blocks.assign(blocks.begin() + i * ONE_WINDOW_BLOCK_NUM,
                                     blocks.begin() + (i + 1) * ONE_WINDOW_BLOCK_NUM);
            vector<LogFile *> lfs = optim_query_one_window_lfs(one_winodw_blocks, time_offset, qs, optim_vo, i + 1);
            res_lfs.insert(res_lfs.end(), lfs.begin(), lfs.end());

            vector<string> entrys = entry_query(lfs, time_offset, qs, optim_vo);
            res_entrys.insert(res_entrys.end(), entrys.begin(), entrys.end());
        }
        optim_vo.all_VO_size = (optim_vo.block_VO_size + optim_vo.tx_VO_size + optim_vo.entry_VO_size);
        optim_vo.all_query_time = (optim_vo.block_query_time + optim_vo.tx_query_time + optim_vo.entry_query_time);
        cout << "some entrys: " << endl;
        for (int i = 0; i < res_entrys.size(); i++) {
            Entry e = Entry::fromString(res_entrys[i]);
            cout << e.timestamp << " " << e.log_value << endl;
            if (i == 4) break;
        }
        cout << "result LG_FILE number: " << res_lfs.size() << endl;
        cout << "result entrys number: " << res_entrys.size() << endl;
        cout << "block query time: " << optim_vo.block_query_time * 1000 << "ms" << endl;
        cout << "tx query time: " << optim_vo.tx_query_time * 1000 << "ms" << endl;
        cout << "entry query time: " << optim_vo.entry_query_time * 1000 << "ms" << endl;
        cout << "block VO size: " << optim_vo.block_VO_size / 1024 << "KB" << endl;
        cout << "tx VO size: " << optim_vo.tx_VO_size / 1024 << "KB" << endl;
        cout << "entry VO size: " << optim_vo.entry_VO_size / 1024 << "KB" << endl;
        cout << "all query time: " << fixed << setprecision(2) << optim_vo.all_query_time * 1000 << "ms" << endl;
        cout << "all VO size: " << fixed << setprecision(2) << optim_vo.all_VO_size / 1024 << "KB" << endl;
        res_lfs.clear();
        res_entrys.clear();
        cout << endl;
    }

    // No optimization solution query
    if (normal) {
        if (!only_query_range && !only_range_multi_block) {
            cout << "normal query Boolean AND" << endl;
            qs.tx_operation = AND;
            qs.entry_operation = AND;
            if (qs.tx_operation != NOT) {
                if (tx_keys_num == 2)
                    qs.tx_keywords = {"xiaomi", "2211133C"};
                else if (tx_keys_num == 3)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9"};
                else if (tx_keys_num == 4)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10"};
                else if (tx_keys_num == 5)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G"};
                else if (tx_keys_num == 6)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G", "11"};
                else if (tx_keys_num == 7)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G", "11", "12"};
                else if (tx_keys_num == 8)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G", "11", "12", "13"};
                else if (tx_keys_num == 9)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G", "11", "12", "13", "14"};
                else if (tx_keys_num == 10)
                    qs.tx_keywords = {"xiaomi", "2211133C", "9", "10", "24030PN60G", "11", "12", "13", "14", "sony"};
            }
        } else {
            cout << "only range query" << endl;
        }

        if (qs.complex_query) {
            qs.tx_operation = OR;
            qs.tx_keywords = {"sony", "14", "xiaomi"};
            qs.entry_keywords = {"as", "HwSystemManager", "E"};
        }
        struct VO normal_vo = {0};
        vector<LogFile *> res_lfs;
        vector<string> res_entrys;
        for (int i = 0; i < WINDOW_NUM; i++) {
            vector<unsigned int> time_offset;
            vector<FullBlock *> one_winodw_blocks;
            one_winodw_blocks.assign(blocks.begin() + i * ONE_WINDOW_BLOCK_NUM,
                                     blocks.begin() + (i + 1) * ONE_WINDOW_BLOCK_NUM);
            vector<LogFile *> lfs = query_one_window_lfs(one_winodw_blocks, time_offset, qs, normal_vo, i + 1);
            res_lfs.insert(res_lfs.end(), lfs.begin(), lfs.end());

            vector<string> entrys = entry_query(lfs, time_offset, qs, normal_vo);
            res_entrys.insert(res_entrys.end(), entrys.begin(), entrys.end());
        }
        normal_vo.all_VO_size = (normal_vo.block_VO_size + normal_vo.tx_VO_size + normal_vo.entry_VO_size);
        normal_vo.all_query_time = (normal_vo.block_query_time + normal_vo.tx_query_time + normal_vo.entry_query_time);
        cout << "some entrys: " << endl;
        for (int i = 0; i < res_entrys.size(); i++) {
            Entry e = Entry::fromString(res_entrys[i]);
            cout << e.timestamp << " " << e.log_value << endl;
            if (i == 4) break;
        }
        cout << "result LG_FILE number: " << res_lfs.size() << endl;
        cout << "result entrys number: " << res_entrys.size() << endl;
        cout << "block query time: " << normal_vo.block_query_time * 1000 << "ms" << endl;
        cout << "tx query time: " << normal_vo.tx_query_time * 1000 << "ms" << endl;
        cout << "entry query time: " << normal_vo.entry_query_time * 1000 << "ms" << endl;
        cout << "block VO size: " << normal_vo.block_VO_size / 1024 << "KB" << endl;
        cout << "tx VO size: " << normal_vo.tx_VO_size / 1024 << "KB" << endl;
        cout << "entry VO size: " << normal_vo.entry_VO_size / 1024 << "KB" << endl;
        cout << "all query time: " << fixed << setprecision(2) << normal_vo.all_query_time * 1000 << "ms" << endl;
        cout << "all VO size: " << fixed << setprecision(2) << normal_vo.all_VO_size / 1024 << "KB" << endl;
        res_lfs.clear();
        res_entrys.clear();
        cout << endl;
        if (only_query_range) { return 0; }
        if (only_range_multi_block) { return 0; }
        if (complex_query) { return 0; }
        cout << "normal query Boolean OR" << endl;
        qs.tx_operation = OR;
        qs.entry_operation = OR;
        normal_vo = {0};
        for (int i = 0; i < WINDOW_NUM; i++) {
            vector<unsigned int> time_offset;
            vector<FullBlock *> one_winodw_blocks;
            one_winodw_blocks.assign(blocks.begin() + i * ONE_WINDOW_BLOCK_NUM,
                                     blocks.begin() + (i + 1) * ONE_WINDOW_BLOCK_NUM);
            vector<LogFile *> lfs = query_one_window_lfs(one_winodw_blocks, time_offset, qs, normal_vo, i + 1);
            res_lfs.insert(res_lfs.end(), lfs.begin(), lfs.end());
            vector<string> entrys = entry_query(lfs, time_offset, qs, normal_vo);
            res_entrys.insert(res_entrys.end(), entrys.begin(), entrys.end());
        }
        normal_vo.all_VO_size = (normal_vo.block_VO_size + normal_vo.tx_VO_size + normal_vo.entry_VO_size);
        normal_vo.all_query_time = (normal_vo.block_query_time + normal_vo.tx_query_time + normal_vo.entry_query_time);
        cout << "some entrys: " << endl;
        for (int i = 0; i < res_entrys.size(); i++) {
            Entry e = Entry::fromString(res_entrys[i]);
            cout << e.timestamp << " " << e.log_value << endl;
            if (i == 4) break;
        }
        cout << "result LG_FILE number: " << res_lfs.size() << endl;
        cout << "result entrys number: " << res_entrys.size() << endl;
        cout << "block query time: " << normal_vo.block_query_time * 1000 << "ms" << endl;
        cout << "tx query time: " << normal_vo.tx_query_time * 1000 << "ms" << endl;
        cout << "entry query time: " << normal_vo.entry_query_time * 1000 << "ms" << endl;
        cout << "block VO size: " << normal_vo.block_VO_size / 1024 << "KB" << endl;
        cout << "tx VO size: " << normal_vo.tx_VO_size / 1024 << "KB" << endl;
        cout << "entry VO size: " << normal_vo.entry_VO_size / 1024 << "KB" << endl;
        cout << "all query time: " << fixed << setprecision(2) << normal_vo.all_query_time * 1000 << "ms" << endl;
        cout << "all VO size: " << fixed << setprecision(2) << normal_vo.all_VO_size / 1024 << "KB" << endl;
        res_lfs.clear();
        res_entrys.clear();
        cout << endl;
        cout << "normal query Boolean NOT" << endl;
        qs.tx_operation = NOT;
        qs.entry_operation = NOT;

        if (tx_keys_num == 2)
            qs.tx_keywords = {"9", "2211133C"};
        else if (tx_keys_num == 3)
            qs.tx_keywords = {"9", "2211133C", "10"};
        else if (tx_keys_num == 4)
            qs.tx_keywords = {"9", "2211133C", "10", "24030PN60G"};
        else if (tx_keys_num == 5)
            qs.tx_keywords = {"9", "2211133C", "10", "24030PN60G", "sony"};
        normal_vo = {0};
        for (int i = 0; i < WINDOW_NUM; i++) {
            vector<unsigned int> time_offset;
            vector<FullBlock *> one_winodw_blocks;
            one_winodw_blocks.assign(blocks.begin() + i * ONE_WINDOW_BLOCK_NUM,
                                     blocks.begin() + (i + 1) * ONE_WINDOW_BLOCK_NUM);
            vector<LogFile *> lfs = query_one_window_lfs(one_winodw_blocks, time_offset, qs, normal_vo, i + 1);
            res_lfs.insert(res_lfs.end(), lfs.begin(), lfs.end());

            vector<string> entrys = entry_query(lfs, time_offset, qs, normal_vo);
            res_entrys.insert(res_entrys.end(), entrys.begin(), entrys.end());
        }
        normal_vo.all_VO_size = (normal_vo.block_VO_size + normal_vo.tx_VO_size + normal_vo.entry_VO_size);
        normal_vo.all_query_time = (normal_vo.block_query_time + normal_vo.tx_query_time + normal_vo.entry_query_time);
        cout << "some entrys: " << endl;
        for (int i = 0; i < res_entrys.size(); i++) {
            Entry e = Entry::fromString(res_entrys[i]);
            cout << e.timestamp << " " << e.log_value << endl;
            if (i == 4) break;
        }
        cout << "result LG_FILE number: " << res_lfs.size() << endl;
        cout << "result entrys number: " << res_entrys.size() << endl;
        cout << "block query time: " << normal_vo.block_query_time * 1000 << "ms" << endl;
        cout << "tx query time: " << normal_vo.tx_query_time * 1000 << "ms" << endl;
        cout << "entry query time: " << normal_vo.entry_query_time * 1000 << "ms" << endl;
        cout << "block VO size: " << normal_vo.block_VO_size / 1024 << "KB" << endl;
        cout << "tx VO size: " << normal_vo.tx_VO_size / 1024 << "KB" << endl;
        cout << "entry VO size: " << normal_vo.entry_VO_size / 1024 << "KB" << endl;
        cout << "all query time: " << fixed << setprecision(2) << normal_vo.all_query_time * 1000 << "ms" << endl;
        cout << "all VO size: " << fixed << setprecision(2) << normal_vo.all_VO_size / 1024 << "KB" << endl;
        res_lfs.clear();
        res_entrys.clear();
    }
}
