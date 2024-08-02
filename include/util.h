
#ifndef _UTIL_H_
#define _UTIL_H_

#include <chrono>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <bitset>
#include <map>
#include <set>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

using namespace std;

void hash_file(string input_file, string output_file);
bool isAllTrue(const vector<bool> &vec);
bool isAllFalse(const vector<bool> &vec);
bool isPowerOfTwo(int n);

vector<string> split(const string &strs, const string &delim);
int get_random(int left, int right);
string hex2String(string hex);
string string2Hex(string s);

void add_keyword_to_entry(string inputFile, string outputFile, vector<string> keywords, unsigned int num_per_line);

void add_tx_keywords_to_file(string key_file, vector<string> keywords, unsigned int num_per_line,
                             unsigned int all_line);
void read_tx_keywords(vector<vector<string>> &keys, string key_file, unsigned int line_n);
void generate_private_key(string output_file, unsigned int key_num);
void read_private_key(string input_file, vector<string> &private_keys);
void generate_public_key(string output_file, unsigned int key_num);
void read_public_key(string input_file, vector<string> &public_keys);
boost::property_tree::ptree read_config(string config_path);
void gen_vchain_tx_dataset();
void gen_vchain_entry_dataset();
void gen_all_lid_keywords();
void gen_lid_keywords(vector<string> keywords);
void read_lid_keywords(vector<vector<string>> &keys);
void pre_process_entry(string entry_path);
#endif