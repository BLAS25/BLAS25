

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>

#include "../include/merkle_tree.h"

bool verify_VO(vector<bool> bitmap, vector<string> &path, string root_hash) {
    unsigned int max_level = log2(bitmap.size());
    // cout << "max_level: " << max_level << endl;
    vector<vector<string>> paths(max_level + 1);
    for (const auto &e : path) {
        // cout << string2Hex(e) << endl;
        // cout << stoi(e) << endl;
        size_t pos = 2;
        unsigned int level = stoi(e, &pos);
        // cout << level << endl;
        string h = e.substr(pos + 1);
        paths.at(level).push_back(h);
    }
    // Including the root node layer
    vector<vector<string>> verify_hashs(max_level + 1);
    verify_hashs[max_level] = paths[max_level];
    string hash;
    verify_path(bitmap, paths, hash, 1, max_level);
    // cout << string2Hex(hash) << endl;
    return (hash == root_hash);
}
void verify_path(vector<bool> bitmap, vector<vector<string>> &paths, string &hash, unsigned current_level,
                 unsigned int max_level) {
    // cout << "current_level: " << current_level << endl;
    // Reach the last floor
    if (current_level == max_level) {
        // cout << paths[current_level].size() << endl;
        string left = paths[current_level][0];
        paths[current_level].erase(paths[current_level].begin());
        string right = paths[current_level][0];
        // cout << "left: " << string2Hex(left) << endl;
        // cout << "right: " << string2Hex(left) << endl;
        paths[current_level].erase(paths[current_level].begin());
        hash = my_blakes(left + right);
        return;
    }
    int bm_len = bitmap.size();
    // cout << "bitmap len: " << bm_len << endl;
    unsigned int half_len = bm_len / 2;
    vector<bool> left_bm(bitmap.begin(), bitmap.begin() + half_len);
    vector<bool> right_bm(bitmap.begin() + half_len, bitmap.end());
    string left, right;
    // cout << current_level << endl;
    // All zeros on the left, get the hash from verify_hashs
    if (isAllFalse(left_bm)) {
        left = paths[current_level][0];

        paths[current_level].erase(paths[current_level].begin());
    } else {
        verify_path(left_bm, paths, left, current_level + 1, max_level);
    }

    if (isAllFalse(right_bm)) {
        right = paths[current_level][0];

        paths[current_level].erase(paths[current_level].begin());
    } else {
        verify_path(right_bm, paths, right, current_level + 1, max_level);
    }
    hash = my_blakes(left + right);
}

void test_mht() {
    unsigned int VO_size = 0;
    unsigned int size = 32;
    unsigned int target_size = size * 0.1;
    double build_tree_time = 0;
    double get_VO_time = 0;
    double verify_VO_time = 0;
    vector<string> leaves;
    for (int i = 0; i < size / 2; i++) {
        leaves.push_back(to_string(i));
        leaves.push_back(to_string(i));
    }
    for (int i = 0; i < 10; i++) {
        clock_t start = clock();
        MerkleTree *tree = new MerkleTree(leaves);
        clock_t end = clock();
        build_tree_time += (double)(end - start) / CLOCKS_PER_SEC;
        vector<string> target_hashs;
        vector<unsigned int> index_set;
        int _min = 0, _max = size - 1;
        for (int i = 0; i < target_size; i++) { index_set.push_back(get_random(0, size - 1)); }
        set<int> s(index_set.begin(), index_set.end());
        index_set.assign(s.begin(), s.end());
        sort(index_set.begin(), index_set.end());
        for (auto index : index_set) {
            // cout << index << endl;
            target_hashs.push_back(tree->node_hashs[index]);
        }
        // cout << target_hashs.size() << endl;
        vector<bool> bitmap;
        vector<string> path;
        start = clock();
        tree->generateVO(index_set, target_hashs, bitmap, path);
        end = clock();
        get_VO_time += (double)(end - start) / CLOCKS_PER_SEC;
        VO_size += bitmap.size();
        VO_size += path.size() * path[0].length();
        // cout << path.size() << endl;
        start = clock();
        assert(verify_VO(bitmap, path, tree->root->hash) == 1);
        end = clock();
        verify_VO_time += (double)(end - start) / CLOCKS_PER_SEC;
        path.clear();
        bitmap.clear();
        target_hashs.clear();
        s.clear();
        index_set.clear();
        if (i == 9) {
            ofstream ofs("./serilized_mht", ios::out | ios::trunc);
            boost::archive::text_oarchive oa(ofs);
            oa << tree;
            ofs.close();
        }
    }
    cout << "leaf size: " << size << endl;
    cout << "target size: " << target_size << endl;
    cout << "build_tree_time: " << build_tree_time * 1000 / 10 << "ms" << endl;
    cout << "get_VO_time: " << get_VO_time * 1000 / 10 << "ms" << endl;
    cout << "verify_VO_time: " << verify_VO_time * 1000 / 10 << "ms" << endl;
    cout << "VO size: " << VO_size / 10 << endl;
}
