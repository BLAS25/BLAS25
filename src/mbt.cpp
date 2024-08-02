#include <vector>
#include <string>
#include <iostream>
#include <queue>
#include <sstream>
#include <iomanip>
#include <set>
#include <ctime>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>

#include "../include/mbt.h"
#include "../include/util.h"

using namespace std;

MerkleBTree::MerkleBTree(vector<Node *> nodes, unsigned int fanout) {
    this->fanout = fanout;
    this->root = NULL;

    unsigned int numberOfNodes = nodes.size();
    for (int i = 0; i != numberOfNodes; i++) {
        // cout << nodes[i]->key << endl;
        this->insert(nodes[i]);
    }

    this->initializeDigest();
}

Node *MerkleBTree::search(unsigned int key) {
    if (this->root == NULL) {
        return NULL;
    } else {
        return this->search(key, this->root);
    }
}

Node *MerkleBTree::search(unsigned int key, TreeNode *treeNode) {
    if (treeNode->isLeaf() == true) {
        // Number of leaf nodes
        int l = treeNode->dataItemsSize();
        for (int i = 0; i != l; i++) {
            if (treeNode->getDataItem(i)->key == key) { return treeNode->getDataItem(i); }
        }
        return NULL;
    } else {
        // the tree node is not a leaf
        int l = treeNode->getKeys().size();
        int i = 0;
        for (; i != l; i++) {
            if (key < treeNode->getKey(i)) { return search(key, treeNode->getChildTreeNode(i)); }
        }
        return search(key, treeNode->getChildTreeNode(l));
    }
}

TreeNode *MerkleBTree::searchLeaf(unsigned int key) {
    if (this->root == NULL) {
        return NULL;
    } else {
        return this->searchLeaf(key, this->root);
    }
}
TreeNode *MerkleBTree::searchLeaf(unsigned int key, TreeNode *treeNode) {
    if (treeNode->isLeaf() == true) {
        return treeNode;
    } else {
        // if the tree node is not a leaf
        int l = treeNode->getKeys().size();
        int i = 0;
        for (; i != l; i++) {
            if (key < treeNode->getKey(i)) { return searchLeaf(key, treeNode->getChildTreeNode(i)); }
        }

        return searchLeaf(key, treeNode->getChildTreeNode(l));
    }
}

bool MerkleBTree::insert(Node *node) {
    // insert the item(node) into the tree

    // Perform a search to determine what bucket the new record should go into.
    TreeNode *leafToInsert = NULL;
    if (this->root == NULL) {
        TreeNode *rootNode = new TreeNode(true, true, NULL);
        this->root = rootNode;
        leafToInsert = rootNode;
    } else {
        leafToInsert = this->searchLeaf(node->key);
    }

    // If the bucket is not full (at most b - 1 entries after the insertion),
    // add the record.
    if (leafToInsert->dataItemsSize() <= this->fanout - 2) {
        int position = leafToInsert->addKey(node->key);
        leafToInsert->addDataItem(position, node);
    } else {
        // Otherwise, split the bucket.
        // Allocate new leaf and move half the bucket's elements to the new
        // bucket.
        TreeNode *newLeaf = new TreeNode(true, false, NULL);
        leafToInsert->setNextLeaf(newLeaf);
        newLeaf->setFatherNode(leafToInsert->getFatherNode());
        int position = leafToInsert->addKey(node->key);
        leafToInsert->addDataItem(position, node);
        for (int i = this->fanout / 2; i != this->fanout; i++) {
            newLeaf->addKey(i - this->fanout / 2, leafToInsert->getKey(i));
            newLeaf->addDataItem(i - this->fanout / 2, leafToInsert->getDataItem(i));
        }
        for (int i = this->fanout / 2; i != this->fanout; i++) {
            leafToInsert->getKeys().pop_back();
            leafToInsert->getDataItems().pop_back();
        }
        // Insert the new leaf's smallest key and address into the parent.
        TreeNode *parent = leafToInsert->getFatherNode();
        // If the parent is full, split it too.
        // Add the middle key to the parent node.
        // Repeat until a parent is found that need not split.
        unsigned int tempKey = newLeaf->getKey(0);
        TreeNode *newTreeNode = newLeaf;
        while (parent != NULL) {
            leafToInsert = parent;
            if (parent->childTreeNodesSize() <= this->fanout - 1) {
                int position = parent->addKey(tempKey);
                parent->addChildTreeNode(position + 1, newTreeNode);
                break;
            } else { // the parent is full
                int position = parent->addKey(tempKey);
                parent->addChildTreeNode(position + 1, newTreeNode);
                newTreeNode = new TreeNode(false, false, NULL);
                newTreeNode->setFatherNode(parent->getFatherNode());
                for (int i = this->fanout / 2 + 1; i != this->fanout; i++) {
                    newTreeNode->getKeys().push_back(parent->getKey(i));
                    newTreeNode->getChildTreeNodes().push_back(parent->getChildTreeNode(i));
                    parent->getChildTreeNode(i)->setFatherNode(newTreeNode);
                }
                newTreeNode->getChildTreeNodes().push_back(parent->getChildTreeNode(this->fanout));
                parent->getChildTreeNode(this->fanout)->setFatherNode(newTreeNode);
                parent->getChildTreeNodes().pop_back();
                for (int i = this->fanout / 2 + 1; i != this->fanout; i++) {
                    parent->getKeys().pop_back();
                    parent->getChildTreeNodes().pop_back();
                }
                tempKey = parent->getKeys().back();
                parent->getKeys().pop_back();

                parent = parent->getFatherNode();
            }
        }
        // If the root splits, create a new root which has one key and two
        // pointers. (That is, the value that gets pushed to the new root gets
        // removed from the original node)
        if (parent == NULL) {
            leafToInsert->setIsRoot(false);
            TreeNode *newRoot = new TreeNode(false, true, NULL);
            this->root = newRoot;
            newRoot->getKeys().push_back(tempKey);
            newRoot->getChildTreeNodes().push_back(leafToInsert);
            newRoot->getChildTreeNodes().push_back(newTreeNode);
            leafToInsert->setFatherNode(newRoot);
            newTreeNode->setFatherNode(newRoot);
        }
    }
    return true;
}
bool MerkleBTree::insert(TreeNode *treeNode, Node *node) {
    return false;
}

void MerkleBTree::printKeys() {
    TreeNode *current = this->root;
    if (current == NULL) return;
    this->printKeys(current);
}

void MerkleBTree::printKeys(TreeNode *node) {
    unsigned int currentLevelTreeNodes = 1;
    unsigned int nextLevelTreeNodes = 0;
    queue<TreeNode *> q;
    q.push(node);
    while (q.empty() == false) {
        TreeNode *current = q.front();
        q.pop();
        // Space-separated keys are the same node
        for (int i = 0; i != current->getKeys().size(); i++) { cout << current->getKey(i) << " "; }
        // cout << "size: " << current->getChildTreeNodes().size();
        for (int i = 0; i != current->getChildTreeNodes().size(); i++) {
            q.push(current->getChildTreeNode(i));
            nextLevelTreeNodes++;
        }
        currentLevelTreeNodes--;
        // Output line break after each layer is printed
        if (currentLevelTreeNodes == 0) { // the current level is finished
            currentLevelTreeNodes = nextLevelTreeNodes;
            nextLevelTreeNodes = 0;
            cout << "\n";
        }
        // Other nodes in this layer
        else {
            cout << " @@ ";
        }
    }
}

void MerkleBTree::printDigests() {
    if (this->root == NULL) { return; }
    this->printDigests(this->root);
}
void MerkleBTree::printDigests(TreeNode *node) {
    // Nodes of the current layer
    unsigned int currentLevelTreeNodes = 1;
    // The next layer of nodes
    unsigned int nextLevelTreeNodes = 0;
    queue<TreeNode *> q;
    q.push(node);
    while (q.empty() == false) {
        TreeNode *current = q.front();
        q.pop();
        for (int i = 0; i != current->getDigests().size(); i++) {
            cout << hex2String(current->getDigest(i));
            cout << " ";
        }
        for (int i = 0; i != current->getChildTreeNodes().size(); i++) {
            q.push(current->getChildTreeNode(i));
            nextLevelTreeNodes++;
        }
        currentLevelTreeNodes--;
        // The current layer output is completed
        if (currentLevelTreeNodes == 0) { // the current level is finished
            currentLevelTreeNodes = nextLevelTreeNodes;
            nextLevelTreeNodes = 0;
            cout << "\n";
        } else {
            cout << " @@ ";
        }
    }
}

void MerkleBTree::get_all_leaf(vector<TreeNode *> &res) {
    if (this->root != NULL) { get_all_leaf(this->root, res); }
}
void MerkleBTree::get_all_leaf(TreeNode *treeNode, vector<TreeNode *> &res) {
    if (treeNode->isLeaf()) {
        res.push_back(treeNode);
    } else {
        unsigned int child_size = treeNode->childTreeNodesSize();
        for (int i = 0; i < child_size; i++) { get_all_leaf(treeNode->getChildTreeNode(i), res); }
    }
}

void MerkleBTree::initializeDigest() {
    if (this->root == NULL) {
        return;
    } else {
        this->calculateDigest(this->root);
    }
}

// Calculate the hash summary of a node
void MerkleBTree::calculateDigest(TreeNode *treeNode) {
    // leaf node
    if (treeNode->isLeaf() == true) {
        unsigned int size = treeNode->dataItemsSize();
        for (int i = 0; i != size; i++) {
            // Calculate the hash of the data after Json serialization
            Node *current = treeNode->getDataItem(i);
            std::string strJson;
            strJson = Node::toString(*current);
            std::string strHash = my_blakes(strJson);
            treeNode->addDigest(strHash);
            strJson.clear();
        }
    } else {
        // not leaf node
        unsigned int size = treeNode->childTreeNodesSize();
        // First calculate the hash digest of the child node
        for (int i = 0; i != size; i++) { this->calculateDigest(treeNode->getChildTreeNode(i)); }
        // Then calculate the hash summary of the current node. Each child node has a corresponding hash, and the hash
        // value is the hash of the child node's hash value.
        for (int i = 0; i != size; i++) {
            ostringstream oss;
            oss.clear();
            TreeNode *currentChildNode = treeNode->getChildTreeNode(i);
            // Concatenate the hash values ​​of each child node
            for (int j = 0; j != currentChildNode->getDigests().size(); j++) { oss << currentChildNode->getDigest(j); }
            std::string strHash = my_blakes(oss.str());
            treeNode->addDigest(strHash);
        }
    }
}

// Return HASH (root node hash concatenation)
string MerkleBTree::calculateRootDigest() {
    if (this->root == NULL) return "";
    ostringstream oss;
    for (int j = 0; j != this->root->getDigests().size(); j++) { oss << this->root->getDigest(j); }
    std::string strHash = my_blakes(oss.str());
    return strHash;
}

TreeNode *MerkleBTree::findFirstLeafNode() {
    if (this->root == NULL) { return NULL; }
    return MerkleBTree::findFirstLeafNode(this->root);
}

TreeNode *MerkleBTree::findFirstLeafNode(TreeNode *treeNode) {
    unsigned int size = treeNode->childTreeNodesSize();
    for (int i = 0; i < size; i++) {
        // Find the first leaf node
        if (treeNode->getChildTreeNode(i)->isLeaf()) {
            return treeNode->getChildTreeNode(i);
        } else {
            return MerkleBTree::findFirstLeafNode(treeNode->getChildTreeNode(i));
        }
    }
    return NULL;
}

void MerkleBTree::rangeSearch(unsigned int left, unsigned int right, bool boundary, std::set<int> &keySet) {
    if (this->root == NULL) { return; }
    return MerkleBTree::rangeSearch(left, right, boundary, keySet, this->root);
}

void MerkleBTree::rangeSearch(unsigned int left, unsigned int right, bool boundary, std::set<int> &keySet,
                              TreeNode *treeNode) {
    TreeNode *firstLeafNode = MerkleBTree::findFirstLeafNode(treeNode);
    // First get all leaf nodes
    vector<TreeNode *> leaf_nodes;
    get_all_leaf(leaf_nodes);
    unsigned long long beforeLeft = -1, afterRight = -1;
    bool left_flag = false, right_flag = false;
    for (int i = 0; i < leaf_nodes.size(); i++) {
        unsigned int dataItemSize = leaf_nodes.at(i)->dataItemsSize();
        for (int j = 0; j < dataItemSize; j++) {
            unsigned long long key = leaf_nodes.at(i)->getDataItem(j)->key;
            // cout << key << endl;
            if (key < left) {
                beforeLeft = key;
            } else if (key == left) {
                left_flag = true;
                // There is a Node smaller than the left border
                if (beforeLeft != -1) { keySet.insert(beforeLeft); }
                keySet.insert(left);
            } else if (key > left && left_flag == false) {
                left_flag = true;
                if (beforeLeft != -1) { keySet.insert(beforeLeft); }
                keySet.insert(key);
            }
            if (key == right) {
                keySet.insert(right);
            } else if (right_flag != true && key > right) {
                right_flag = true;
                afterRight = key;
                if (afterRight != -1) { keySet.insert(afterRight); }
            }
        }
        if (left_flag == true && right_flag == true) { break; }
    }
    int size = leaf_nodes[leaf_nodes.size() - 1]->dataItemsSize();

    unsigned long long key = leaf_nodes.at(leaf_nodes.size() - 1)->getDataItem(size - 1)->key;
    // cout << "key: " << key << endl;
    if (key < right) { keySet.insert(key); }
}

// Generate node set proof VO
std::string MerkleBTree::generateVO(std::vector<Node *> &nodes, vector<string> &nodeJsons) {
    // indexSet stores the keyword keys of all the proof nodes to be generated
    set<int> keySet;
    for (vector<Node *>::const_iterator iter = nodes.begin(); iter != nodes.end(); iter++) {
        keySet.insert((*iter)->key);
    }
    return this->generateVO(this->root, keySet, nodeJsons);
}

std::string MerkleBTree::generateVO(MerkleBTreeNode *treeNode, set<int> &keySet, vector<string> &nodeJsons) {
    string s = "";
    // cout << "keySet: " << endl;
    // for (int k : keySet)
    // {
    //     cout << k << " ";
    // }
    // cout << endl;

    if (treeNode->isLeaf() == true) {
        s += "[";
        for (int i = 0; i != treeNode->dataItemsSize(); i++) {
            Node *currentNode = treeNode->getDataItem(i);
            // cout << "currentNode->key: " << currentNode->key << endl;
            if (keySet.find(currentNode->key) != keySet.end()) {
                // the node's index is found in the indexSet
                ostringstream oss;
                oss.clear();
                oss << setprecision(10);
                std::string strJson;
                strJson = Node::toString(*currentNode);
                // cout << "key: " << currentNode->key << endl;
                nodeJsons.push_back(strJson);
                oss << "[node," << strJson;
                oss << "]";
                s += oss.str();
            } else {
                // the node is not needed, so the hash value is includeed
                ostringstream oss;
                oss.clear();
                oss << "[hash," << string2Hex(treeNode->getDigest(i)) << "]";
                s += oss.str();
            }
        }
        // cout << endl;
        s += "]";
    } else {
        // the treeNode is not a leaf
        s += "[";
        int numberOfKeys = treeNode->getKeys().size();
        for (int i = 0; i != numberOfKeys; i++) {
            int key = treeNode->getKey(i);
            // cout << "key1: " << key << endl;
            set<int> originalIndexSet = keySet;
            set<int> newIndexSet;
            for (set<int>::const_iterator iter = originalIndexSet.begin(); iter != originalIndexSet.end(); iter++) {
                if ((*iter) < key) {
                    newIndexSet.insert((*iter));
                    keySet.erase(*iter);
                }
            }
            // cout << "newIndexSet: ";
            // for (auto e : newIndexSet)
            // {
            //     cout << e << " ";
            // }
            // cout << endl;
            if (newIndexSet.size() == 0) {
                ostringstream oss;
                oss.clear();
                oss << "[hash," << string2Hex(treeNode->getDigest(i)) << "]";
                s += oss.str();
            } else {
                s += this->generateVO(treeNode->getChildTreeNode(i), newIndexSet, nodeJsons);
            }
        }
        if (keySet.size() == 0) {
            ostringstream oss;
            oss.clear();
            oss << "[hash," << string2Hex(treeNode->getDigest(numberOfKeys)) << "]";
            s += oss.str();
        } else {
            s += this->generateVO(treeNode->getChildTreeNode(numberOfKeys), keySet, nodeJsons);
        }
        s += "]";
    }

    return s;
}

AuthenticationTree::~AuthenticationTree() {
    queue<AuthenticationTreeNode *> q;
    q.push(this->root);
    while (q.empty() != false) {
        AuthenticationTreeNode *current = q.front();
        q.pop();
        for (int i = 0; i != current->getChildNodes().size(); i++) { q.push(current->getChildNode(i)); }
        delete current;
    }
}

bool AuthenticationTree::parseVO(std::string s) {
    if (s.length() == 0) { return false; }

    // initially the root is NULL
    // the root will be created during the parse of "s"
    this->root = NULL;
    AuthenticationTreeNode *currentTreeNode = NULL;

    unsigned int index = 0;

    char beforeChar = s[index];
    char nextChar;
    while (index < s.length()) {
        nextChar = s[index];
        string tempValue = "";
        switch (nextChar) {
        case '[':
            if (currentTreeNode == NULL) {
                currentTreeNode = new AuthenticationTreeNode();
                this->root = currentTreeNode;
            } else {
                AuthenticationTreeNode *newNode = new AuthenticationTreeNode(currentTreeNode, false, "");
                currentTreeNode->addChildNode(newNode);
                currentTreeNode = newNode;
            }
            break;
        case ']': currentTreeNode = currentTreeNode->getParentNode(); break;
        case ',': break;
        case 'h':
            index += 5;
            if (index >= s.length()) {
                cout << "Authentication Failed.\n";
                return false;
            }
            while (index < s.length() && s[index] != ']') {
                tempValue += s[index];
                index++;
            }
            index--;
            // the current char is probably ']', so go back one char
            currentTreeNode->setValue(tempValue);
            break;
        case 'n':
            index += 5;
            if (index >= s.length()) {
                cout << "Authentication Failed.\n";
                return false;
            }
            while (index < s.length() && (s[index] != ']' || beforeChar == '"')) {
                tempValue += s[index];
                beforeChar = s[index];
                index++;
            }
            index--; // the current char is probably ']', so go back one char
            currentTreeNode->setValue(tempValue);
            currentTreeNode->setIsNode(true);
            break;
        default: cout << "The program should not go here.\n"; exit(1);
        }
        index++;
    }

    // the authentication tree is initialized
    // next, calculate the digests
    this->calculateDigest();
    return true;
}

std::string AuthenticationTree::getRootDigest() {
    return this->root->getDigest();
}

void AuthenticationTree::calculateDigest() {
    if (this->root == NULL) { return; }
    this->root->setDigest(this->calculateDigest(this->root));
}

string AuthenticationTree::calculateDigest(AuthenticationTreeNode *treeNode) {
    // if the treeNode contains hash value(not a node and has
    // value), then just return the value elseif the treeNode is a
    // node, then calculate the digest and return the value else,
    // calculate the digest recursively

    // The node contains the hash value
    if (treeNode->isNode() == false && treeNode->getValue() != "") {
        treeNode->setDigest(hex2String(treeNode->getValue()));
        return treeNode->getDigest();
    }
    // The node contains the original data
    else if (treeNode->isNode() == true) {
        string nodeInformation = treeNode->getValue();
        string strHash = my_blakes(nodeInformation);
        treeNode->setDigest(strHash);
        return treeNode->getDigest();
    } else
    // treeNode->isNode() == false && treeNode->getValue() !=
    // ""，Non-leaf nodes and nodes do not store any values
    //  Calculate the hash values ​​of all child nodes, and then concatenate them to calculate the hash values
    {
        // cout << treeNode->getChildNodes().size() << endl;
        string contentToDigest = "";
        for (int i = 0; i != treeNode->getChildNodes().size(); i++) {
            contentToDigest += this->calculateDigest(treeNode->getChildNode(i));
        }
        string strHash = my_blakes(contentToDigest);
        treeNode->setDigest(strHash);
        return treeNode->getDigest();
    }
}

void AuthenticationTree::printDigests() {
    if (this->root == NULL) { return; }
    this->printDigests(this->root);
}
void AuthenticationTree::printDigests(AuthenticationTreeNode *node) {
    unsigned int currentLevelTreeNodes = 1;
    unsigned int nextLevelTreeNodes = 0;
    queue<AuthenticationTreeNode *> q;
    q.push(node);
    while (q.empty() == false) {
        AuthenticationTreeNode *current = q.front();
        q.pop();
        cout << hex2String(current->getDigest());
        cout << " ";
        for (int i = 0; i != current->getChildNodes().size(); i++) {
            q.push(current->getChildNode(i));
            nextLevelTreeNodes++;
        }
        currentLevelTreeNodes--;
        if (currentLevelTreeNodes == 0) {
            // the current level is finished
            currentLevelTreeNodes = nextLevelTreeNodes;
            nextLevelTreeNodes = 0;
            cout << "\n";
        } else {
            cout << " @@ ";
        }
    }
}

void test_mbt() {
    unsigned int VO_size = 0;
    unsigned int size = 32;
    unsigned int target_size = size * 0.1;
    unsigned int left = 0;
    unsigned int right = left + target_size;
    double build_tree_time = 0;
    double get_VO_time = 0;
    double verify_VO_time = 0;
    clock_t start, end;
    unsigned int fan_out = 4;
    vector<Node *> nodes;
    vector<string> str_nodes;
    for (int i = 0; i < size; i++) {
        Node *node = new Node();
        node->key = i;
        node->value = to_string(i);
        nodes.push_back(node);
    }
    for (int i = 0; i < size; i++) { str_nodes.push_back(Node::toString(*nodes[i])); }
    for (int i = 0; i < 1; i++) {
        start = clock();
        MerkleBTree *mbt = new MerkleBTree(nodes, fan_out);
        // mbt->printKeys();
        end = clock();
        build_tree_time += (double)(end - start) / CLOCKS_PER_SEC;
        start = clock();
        set<int> keySet;
        cout << "left: " << left << " right: " << right << endl;
        mbt->rangeSearch(left, right, true, keySet);
        keySet = {0, 2, 4, 6, 8};
        cout << "keySet: ";
        for (auto e : keySet) { cout << e << " "; }

        cout << endl;
        mbt->printKeys();
        string VO = mbt->generateVO(mbt->getRoot(), keySet, str_nodes);
        cout << "VO: " << VO << endl;
        end = clock();
        get_VO_time += (double)(end - start) / CLOCKS_PER_SEC;
        VO_size += VO.length();
        start = clock();
        AuthenticationTree at;
        at.parseVO(VO);
        string mbt_root = mbt->calculateRootDigest();
        string at_root = at.getRootDigest();
        assert(mbt_root == at_root);
        end = clock();
        verify_VO_time += (double)(end - start) / CLOCKS_PER_SEC;
        if (i == 9) {
            ofstream ofs("./serilized_mbt", ios::out | ios::trunc);
            boost::archive::text_oarchive oa(ofs);
            oa << mbt;
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