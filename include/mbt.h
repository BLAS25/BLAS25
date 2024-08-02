

#ifndef _MERKLEBTREE_H_
#define _MERKLEBTREE_H_

#include <vector>
#include <string>
#include <set>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "crypto.h"

#define DEFAULT_FANOUT 4

using namespace std;

struct Node {
    unsigned int key;
    string value;
    Node() {
    }
    template <typename Archive>
    void serialize(Archive &archive, const unsigned int version) {
        archive &BOOST_SERIALIZATION_NVP(key);
        archive &BOOST_SERIALIZATION_NVP(value);
    }
    unsigned int get_size() {
        return sizeof(key) + value.length();
    }
    static string toString(const struct Node &setting) {
        ostringstream oss;
        boost::archive::text_oarchive oa(oss);
        oa << setting;
        return oss.str();
    }

    static struct Node fromString(const string &data) {
        struct Node setting;
        istringstream iss(data);
        boost::archive::text_iarchive ia(iss);
        ia >> setting;
        return setting;
    }
};

// Node::Node(){}
class MerkleBTreeNode {
private:
    bool isLeaf_;                             // Is it a leaf node
    bool isRoot_;                             // Is it the root node
    MerkleBTreeNode *fatherNode;              // Parent Node
    vector<unsigned long long> keys;          // Keyword collection, stored in ascending order
    vector<MerkleBTreeNode *> childTreeNodes; // When the node is not a leaf node, store all child nodes
    vector<Node *> dataItems;                 // When the node is a leaf node, all data is stored
    MerkleBTreeNode *nextLeaf;                // When the node is a leaf node, store the sibling nodes
    vector<string> digests;                   // Hash of child nodes or data
public:
    template <class Archive>
    void serialize(Archive &archive, const unsigned version) {
        archive & isLeaf_ & isRoot_ & fatherNode & keys & childTreeNodes & dataItems & nextLeaf & digests;
    }

    MerkleBTreeNode(bool isLeaf = false, bool isRoot = false, MerkleBTreeNode *fatherNode = NULL) {
        this->isLeaf_ = isLeaf;
        this->isRoot_ = isRoot;
        this->fatherNode = fatherNode;
        this->nextLeaf = NULL;
    };
    ~MerkleBTreeNode() {};

    bool isLeaf() {
        return this->isLeaf_;
    }
    void setIsLeaf(bool value) {
        this->isLeaf_ = value;
    }
    bool isRoot() {
        return this->isRoot_;
    }
    void setIsRoot(bool value) {
        this->isRoot_ = value;
    }

    MerkleBTreeNode *getFatherNode() {
        return this->fatherNode;
    }
    void setFatherNode(MerkleBTreeNode *value) {
        this->fatherNode = value;
    }

    vector<unsigned long long> &getKeys() {
        return this->keys;
    }
    int getKey(unsigned int index) {
        if (index >= 0 && index < this->keys.size()) { return this->keys[index]; }
        return -1;
    }
    int addKey(unsigned int key) {
        // return the position to which the key is added
        if (this->keys.size() == 0) {
            this->keys.push_back(key);
            return 0;
        } else {
            int i = 0;
            for (; i != this->keys.size(); i++) {
                if (key < this->keys[i]) {
                    this->keys.insert(this->keys.begin() + i, key);
                    return i;
                }
            }
            this->keys.push_back(key);
            return i;
        }
    }
    void addKey(int index, unsigned int key) {
        this->keys.insert(this->keys.begin() + index, key);
    }

    MerkleBTreeNode *getNextLeaf() {
        return this->nextLeaf;
    }

    vector<Node *> &getDataItems() {
        return this->dataItems;
    }
    Node *getDataItem(unsigned int index) {
        if (index >= 0 && index < this->dataItems.size()) { return this->dataItems[index]; }
        return NULL;
    }
    unsigned int dataItemsSize() {
        return this->dataItems.size();
    }
    void addDataItem(int index, Node *node) {
        this->dataItems.insert(this->dataItems.begin() + index, node);
    }

    MerkleBTreeNode *getChildTreeNode(unsigned int index) {
        if (index >= 0 && index < this->childTreeNodes.size()) { return this->childTreeNodes[index]; }
        return NULL;
    }
    vector<MerkleBTreeNode *> &getChildTreeNodes() {
        return this->childTreeNodes;
    }
    unsigned int childTreeNodesSize() {
        return this->childTreeNodes.size();
    }
    void addChildTreeNode(int index, MerkleBTreeNode *treeNode) {
        this->childTreeNodes.insert(this->childTreeNodes.begin() + index, treeNode);
    }

    void setNextLeaf(MerkleBTreeNode *treeNode) {
        this->nextLeaf = treeNode;
    }

    void addDigest(string value) {
        this->digests.push_back(value);
    }
    const vector<string> &getDigests() {
        return this->digests;
    }
    string getDigest(int index) {
        return this->digests[index];
    }
};

typedef MerkleBTreeNode TreeNode;

class MerkleBTree {
private:
    unsigned int fanout;
    TreeNode *root;

public:
    template <class Archive>
    void serialize(Archive &archive, const unsigned version) {
        archive & fanout & root;
    }
    MerkleBTree(vector<Node *> nodes, unsigned int fanout);
    MerkleBTree() {
    }
    ~MerkleBTree() {
    }

    unsigned int get_size() {
        return get_size(this->root);
    }
    unsigned int get_size(TreeNode *tree_node) {
        unsigned int size = 0;
        if (tree_node->isLeaf()) {
            vector<string> digets = tree_node->getDigests();
            for (auto e : digets) { size += e.length(); }
            // vchain+
            // for (auto e : digets) { size += e.length() + 416; }
            return size;
        } else {
            for (int i = 0; i < tree_node->childTreeNodesSize(); i++) {
                vector<string> digets = tree_node->getDigests();
                for (auto e : digets) { size += e.length(); }
                // vchain+
                // for (auto e : digets) { size += e.length() + 416; }
                size = size + get_size(tree_node->getChildTreeNode(i));
            }
        }
        return size;
    }
    unsigned int getFanout() {
        return this->fanout;
    }
    TreeNode *getRoot() {
        return this->root;
    }
    TreeNode *findFirstLeafNode();
    TreeNode *findFirstLeafNode(TreeNode *treeNode);
    void rangeSearch(unsigned int left, unsigned int right, bool boundary, set<int> &keySet);
    void rangeSearch(unsigned int left, unsigned int right, bool boundary, set<int> &keySet, TreeNode *treeNode);
    Node *search(unsigned int key);
    Node *search(unsigned int key, TreeNode *treeNode);
    TreeNode *searchLeaf(unsigned int key); // search for the leaf that contains
                                            // the data which has the key
    TreeNode *searchLeaf(unsigned int key, TreeNode *treeNode);
    void get_all_leaf(vector<TreeNode *> &res);
    void get_all_leaf(TreeNode *treeNode, vector<TreeNode *> &res);
    bool insert(Node *);

    TreeNode *leftestLeafNode();
    unsigned int numberOfTreeNodes();
    unsigned int numberOfLeaves();
    unsigned int numberOfDataItems();

    void printKeys();
    void printKeys(TreeNode *treeNode);
    void printDigests();
    void printDigests(TreeNode *treeNode);

    string generateVO(vector<Node *> &nodes, vector<string> &nodeJsons);
    string generateVO(MerkleBTreeNode *treeNode, set<int> &keySet, vector<string> &nodeJsons);
    string calculateRootDigest();

private:
    bool insert(TreeNode *, Node *);
    void initializeDigest();
    void calculateDigest(MerkleBTreeNode *);
};

class AuthenticationTreeNode {
private:
    vector<AuthenticationTreeNode *> childNodes; // if childNodes is empty, it is a leaf
    AuthenticationTreeNode *parentNode;          // if it is NULL, it is the root
    bool isNode_;                                // Indicates whether this node stores data or hashes
    string value;

    string digest; // for authentication, contains the digest of all the
                   // information in the current node

public:
    AuthenticationTreeNode() {
        parentNode = NULL;
        isNode_ = false;
    };
    AuthenticationTreeNode(AuthenticationTreeNode *parentNode, bool isNode, string value) {
        this->parentNode = parentNode;
        this->isNode_ = isNode;
        this->value = value;
    }
    ~AuthenticationTreeNode() {
    }

    const vector<AuthenticationTreeNode *> &getChildNodes() {
        return this->childNodes;
    }
    AuthenticationTreeNode *getChildNode(unsigned int index) {
        if (index >= 0 && index < this->childNodes.size()) {
            return this->childNodes[index];
        } else {
            return NULL;
        }
    }
    void addChildNode(AuthenticationTreeNode *value) {
        this->childNodes.push_back(value);
    }

    AuthenticationTreeNode *getParentNode() {
        return this->parentNode;
    }
    void setParentNode(AuthenticationTreeNode *value) {
        this->parentNode = value;
    }

    bool isNode() {
        return this->isNode_;
    }
    void setIsNode(bool value) {
        this->isNode_ = value;
    }

    const string &getValue() {
        return this->value;
    }
    void setValue(string value) {
        this->value = value;
    }

    const string &getDigest() {
        return this->digest;
    }
    void setDigest(string value) {
        this->digest = value;
    }
};

class AuthenticationTree {
private:
    AuthenticationTreeNode *root;
    string hashMethodName;

public:
    AuthenticationTree() {
        this->root = NULL;
    };
    ~AuthenticationTree();

    AuthenticationTreeNode *getRoot() {
        return this->root;
    }
    void setRoot(AuthenticationTreeNode *value) {
        this->root = value;
    }

    bool parseVO(string s);

    string getRootDigest();

    void printDigests();
    void printDigests(AuthenticationTreeNode *);

private:
    void calculateDigest();
    string calculateDigest(AuthenticationTreeNode *);
};
void test_mbt();
#endif // _MERKLEBTREE_H_