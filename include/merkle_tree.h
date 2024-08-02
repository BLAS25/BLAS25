
#ifndef _MERKLR_H_
#define _MERKLR_H_

#include <string>
#include <iostream>
#include <cmath>

#include "crypto.h"
#include "util.h"

using namespace std;

bool verify_VO(vector<bool> bitmap, vector<string> &path, string root_hash);
void verify_path(vector<bool> bitmap, vector<vector<string>> &paths, string &hash, unsigned current_level,
                 unsigned int max_level);
class MerkleNode {
public:
    string data;
    string hash;
    MerkleNode *parent;
    MerkleNode *left_c;
    MerkleNode *right_c;

public:
    template <class Archive>
    void serialize(Archive &archive, const unsigned version) {
        archive & data & hash & parent & left_c & right_c;
    }
    MerkleNode() {
        parent = NULL;
        left_c = NULL;
        right_c = NULL;
    }
    ~MerkleNode() {
    }
};

class MerkleTree {
public:
    MerkleNode *root;

    vector<string> node_hashs;

public:
    template <class Archive>
    void serialize(Archive &archive, const unsigned version) {
        archive & root & node_hashs;
        // archive & root;
    }
    unsigned int get_size() {
        return get_size(this->root);
    }
    unsigned int get_size(MerkleNode *tree_node) {
        unsigned int size = 0;
        if (tree_node->left_c != NULL) { size = size + get_size(tree_node->left_c) + 32; }
        if (tree_node->right_c != NULL) { size = size + get_size(tree_node->right_c) + 32; }
        if (tree_node->left_c == NULL && tree_node->right_c == NULL) { return 32; }

        // vchain+
        // if (tree_node->left_c != NULL) { size = size + get_size(tree_node->left_c) + 32; }
        // if (tree_node->right_c != NULL) { size = size + get_size(tree_node->right_c) + 32; }
        // if (tree_node->left_c == NULL && tree_node->right_c == NULL) { return 416; }
        return size;
    }
    MerkleTree() {
    }
    MerkleTree(vector<string> leaves) {
        unsigned int level = 0;
        vector<MerkleNode *> nodes;
        vector<string> hs;
        if (leaves.empty()) return;
        for (const auto &leaf : leaves) {
            MerkleNode *new_MerkleNode = new MerkleNode();
            string h = my_blakes(leaf);
            new_MerkleNode->data = leaf;
            new_MerkleNode->hash = h;
            this->node_hashs.push_back(h);
            // std::cout << string2Hex(my_blakes(leaf)) << std::endl;
            nodes.push_back(new_MerkleNode);
            // cout << string2Hex(h) << endl;
        }

        while (nodes.size() > 1) {
            if (nodes.size() % 2 == 1) {
                MerkleNode *new_node = new MerkleNode();
                new_node->data = nodes.back()->data;
                new_node->hash = nodes.back()->hash;
                nodes.push_back(new_node);
            }
            vector<MerkleNode *> new_level;
            for (int i = 0; i < nodes.size(); i += 2) {
                string comblined_hash = my_blakes(nodes[i]->hash + nodes[i + 1]->hash);
                MerkleNode *parent_MerkleNode = new MerkleNode;
                parent_MerkleNode->hash = comblined_hash;
                parent_MerkleNode->left_c = nodes[i];
                parent_MerkleNode->right_c = nodes[i + 1];
                new_level.push_back(parent_MerkleNode);
                // cout << string2Hex(comblined_hash) << endl;
            }
            nodes = new_level;
        }
        this->root = nodes[0];
    }
    void generateVO(vector<unsigned int> indexs, vector<string> target_hashs, vector<bool> &bitmap,
                    vector<string> &path) {
        // cout << "target_hashs size: " << target_hashs.size() << endl;
        // If the node is a query node, the corresponding position is 1
        vector<string> temp(target_hashs.begin(), target_hashs.end());
        for (int i = 0; i < this->node_hashs.size(); i++) {
            // cout << string2Hex(this->node_hashs[i]) << endl;
            auto it = find(indexs.begin(), indexs.end(), i);
            if (it != indexs.end()) {
                bitmap.push_back(true);
            } else {
                bitmap.push_back(false);
            }
        }

        while (!isPowerOfTwo(bitmap.size())) { bitmap.push_back(false); }

        vector<MerkleNode *> nodes(2);
        nodes.at(0) = NULL;
        nodes.at(1) = NULL;
        if (this->root->left_c != NULL) nodes.at(0) = this->root->left_c;
        if (this->root->right_c != NULL) nodes.at(1) = this->root->right_c;
        // Records the number of hashes in target_hashs that have been added
        int num = 0;
        unsigned int level = 1;
        unsigned int max_level = int(log2(bitmap.size()));

        get_verify_path(nodes, bitmap, target_hashs, path, level, num, max_level);
        // cout << "bitmap size: " << bitmap.size() << endl;
    }
    // Divide the bitmap into two and see if it is all 0 or 1. If it is not all 0, continue to divide it.
    void get_verify_path(vector<MerkleNode *> &nodes, vector<bool> bitmap, vector<string> target_hashs,
                         vector<string> &path, unsigned int level, int &num, unsigned int max_level) {
        int bm_len = bitmap.size();
        unsigned int half_len = bm_len / 2;
        vector<bool> left_bm(bitmap.begin(), bitmap.begin() + half_len);
        vector<bool> right_bm(bitmap.begin() + half_len, bitmap.end());
        // cout << "bitmap len: " << bm_len << endl;
        // cout << left_bm.size() << " " << right_bm.size() << endl;
        // cout << isAllFalse(left_bm) << " " << isAllTrue(left_bm) << endl;
        // cout << num << " " << target_hashs.size() << endl;
        // all 0
        if (isAllFalse(left_bm)) {
            // cout << "a"
            //      << " ";
            assert(nodes.at(0) != NULL);
            path.push_back(to_string(level) + " " + nodes.at(0)->hash);
        }
        // all 1

        else if (isAllTrue(left_bm)) {
            // cout << "b"
            //      << " ";
            // cout << "left_num: " << num << endl;
            int ini_num = num;
            for (int i = ini_num; i < left_bm.size() + ini_num; i++) {
                // cout << i << " ";
                path.push_back(to_string(max_level) + " " + target_hashs.at(i));
                num++;
            }
        }
        // There are 0 and 1
        else {
            // cout << "c"
            //      << " ";
            assert(nodes.at(0) != NULL);
            vector<MerkleNode *> child_nodes(2);
            child_nodes.at(0) = NULL;
            child_nodes.at(1) = NULL;
            if (nodes.at(0)->left_c != NULL) { child_nodes.at(0) = nodes.at(0)->left_c; }
            if (nodes.at(0)->right_c != NULL) { child_nodes.at(1) = nodes.at(0)->right_c; }
            get_verify_path(child_nodes, left_bm, target_hashs, path, level + 1, num, max_level);
        }

        // all 0
        if (isAllFalse(right_bm)) {
            // cout << to_string(level) << endl;
            assert(nodes.at(1) != NULL);
            path.push_back(to_string(level) + " " + nodes.at(1)->hash);
        }
        // all 1
        else if (isAllTrue(right_bm)) {
            int ini_num = num;
            // cout << "right_num: " << num << endl;
            for (int i = ini_num; i < right_bm.size() + ini_num; i++) {
                // cout << i << " ";
                path.push_back(to_string(max_level) + " " + target_hashs.at(i));
                num++;
            }
        }
        // There are 0 and 1
        else {
            assert(nodes.at(1) != NULL);
            vector<MerkleNode *> child_nodes(2);
            child_nodes.at(0) = NULL;
            child_nodes.at(1) = NULL;
            if (nodes.at(1)->left_c != NULL) { child_nodes.at(0) = nodes.at(1)->left_c; }
            if (nodes.at(1)->right_c != NULL) { child_nodes.at(1) = nodes.at(1)->right_c; }
            get_verify_path(child_nodes, right_bm, target_hashs, path, level + 1, num, max_level);
        }
        // for (const auto &e : path)
        // {
        //     cout << string2Hex(e) << endl;
        // }
    }
};
void test_mht();
#endif