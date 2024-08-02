

#include <random>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <sstream>
#include <iomanip>
#include <chrono>

#include "../include/util.h"
#include "../include/crypto.h"
#include "../include/log.h"

using namespace std;
boost::property_tree::ptree read_config(string config_path) {
    boost::property_tree::ptree pt;

    boost::property_tree::ini_parser::read_ini(config_path, pt);
    return pt;
}
// Generate private keywords and write them into the file
void generate_private_key(string output_file, unsigned int key_num) {
    ofstream output_f(output_file);
    for (int i = 0; i < key_num; i++) { output_f << "ek" << to_string(i) << endl; }
    output_f.close();
}

unsigned long long parse_time(string time_string) {
    tm tm = {};
    tm.tm_year = 2018 - 1900;
    tm.tm_isdst = -1;
    cout << time_string << endl;
    istringstream ss(time_string);

    ss >> get_time(&tm, "%m-%d %H:%M:%S");
    if (ss.fail()) {
        // handle error
        std::cout << "Failed to parse time string" << std::endl;
        return -1;
    }

    time_t time_t_val = mktime(&tm);
    if (time_t_val == (time_t)(-1)) {
        // handle error
        cout << "Failed to convert tm to time_t" << endl;
        return -1;
    }
    chrono::system_clock::time_point tp = chrono::system_clock::from_time_t(time_t_val);
    unsigned long long timestamp = chrono::system_clock::to_time_t(tp);
    return timestamp;
}
void pre_process_entry(string entry_path) {
    ifstream input_f(entry_path);
    // ofstream output_f("./entry_time");
    ofstream output_f("./entry_keys");
    ofstream output_f1("./entry_all_keys");
    string line;
    int all_line_num = 2048 * 256 * 16;
    int line_num = 1;
    set<string> s_keys;
    while (getline(input_f, line)) {
        if (line_num > all_line_num) { break; }
        // if (line_num >= 10) { break; }
        vector<string> r = split(line, " ");
        vector<string> keys;
        keys.push_back(r[4]);
        s_keys.insert(r[4]);
        output_f << r[4] << " ";
        r[5].pop_back();
        output_f << r[5] << endl;
        keys.push_back(r[5]);
        s_keys.insert(r[5]);

        // cout << endl;
        // string time_string = r[0] + " " + r[1];

        // unsigned long long timestamp = parse_time(time_string);
        // output_f << timestamp << endl;

        line_num++;
    }
    for (auto e : s_keys) { output_f1 << e << endl; }
}
// Read private keywords from a file
void read_private_key(string input_file, vector<string> &private_keys) {
    ifstream input_f(input_file);
    string line;
    while (getline(input_f, line)) { private_keys.push_back(line); }
    input_f.close();
}

// Generate common keywords and write them into the file
void generate_public_key(string output_file, unsigned int key_num) {
    ofstream output_f(output_file);
    for (int i = 0; i < key_num; i++) { output_f << "pk" << to_string(i) << endl; }
    output_f.close();
}

void read_public_key(string input_file, vector<string> &public_keys) {
    ifstream input_f(input_file);
    string line;
    while (getline(input_f, line)) { public_keys.push_back(line); }
    input_f.close();
}
// Hash each line of input_file and store it in output_file
void hash_file(string input_file, string output_file) {
    ifstream input_f(input_file);
    ofstream output_f(output_file);
    string line;
    string hash;
    unsigned int line_num = 1;
    if (input_f.is_open()) {
        while (getline(input_f, line)) {
            hash = my_blakes(line);
            // hash = to_string(line_num);
            output_f << hash << endl;
            line_num++;
        }
    }
    input_f.close();
    output_f.close();
}
// Get a random number, including left and right boundaries
bool isAllTrue(const vector<bool> &vec) {
    return all_of(vec.begin(), vec.end(), [](bool v) { return v; });
}
bool isAllFalse(const vector<bool> &vec) {
    return all_of(vec.begin(), vec.end(), [](bool v) { return !v; });
}
bool isPowerOfTwo(int n) {
    if (n <= 0) return false;
    return (n & (n - 1)) == 0;
}
int get_random(int left, int right) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(left, right);

    int random_number = distrib(gen);

    return random_number;
}
// Generate 4096 lines of keywords and write them into the file
void gen_all_lid_keywords() {
    ofstream output_f("./all_lid_keywords");
    ofstream out_f("./keys/public_keys");

    vector<string> manu = {"xiaomi", "oppo", "vivo", "samsung", "sony", "honor", "motorola"};
    vector<vector<string>> phone = {
        // xiaomi
        {"2211133C", "2211133G", "2210132C", "2210132G", "2304FPN6DC", "2304FPN6DG", "23127PN0CC", "23127PN0CG",
         "23116PN5BC", "24031PN0DC", "24030PN60G", "2407FPN8EG", "23078RKD5C", "23113RKC6C", "23117RK66C", "2311DRK48C",
         "M1903F11G"},
        // oppo
        {"PGFM10", "PGEM10", "PHZ110", "PHY110", "PHY120", "PHN110", "PHT110", "PHV110", "PHU110", "PJH110", "PJJ110",
         "PJV110", "PJW110", "PDHM00", "PCLM10", "PBEM00"},
        // vivo
        {"V1821A", "V1821T", "V1923A", "V1923T", "V1924A", "V1924T", "V1950A", "V2241A", "V2241HA", "V2242A", "V2227A",
         "V2266A", "V2256A", "V2309A", "V2324A", "V2303A"},
        // samsung
        {"SM-S9010", "SM-S9060", "SM-S9080", "SM-S9110", "SM-S9160", "SM-S9180", "SM-S7110", "SM-S9210", "SM-S9260",
         "SM-S9280", "SM-N9600", "SM-N9608", "SM-N9700", "SM-N9760", "SM-N9810", "SM-N9860"},
        // sony
        {"H9493", "H8166", "H8296", "H4233", "G8342", "G8232", "J9110", "J9180", "J9210", "I4293", "XQ-AT72", "XQ-AS72",
         "XQ-BC72", "XQ-BQ72", "E6683", "XQ-BE72"},
        // honor
        {"BVL-AN00", "BVL-AN16", "BVL-AN20", "BVL-AN20", "BVL-AN20", "FCP-AN10", "FLC-AN00", "COL-TL10", "COL-TL00",
         "HRY-AL00a", "HRY-AL00a", "HRY-TL00T", "MAA-AN00", "MAA-AN10", "ELI-AN00", "ELP-AN00"},
        // motorola
        {"XT1970-5", "XT2071-4", "XT2125-4", "XT2143-1", "XT2153-1", "XT2137-2", "XT2171-3", "XT2169-2", "XT2225-2",
         "XT2175-2", "XT2363-4", "XT2401-2", "XT2427-4", "XT2453-2", "XT2451-4", "XT1965-6"}};
    // Android versions
    vector<string> android_vers = {"8.0", "8.1", "9", "10", "11", "12", "13", "14"};
    for (auto e : manu) { out_f << e << endl; }
    for (auto e : phone) {
        for (auto g : e) { out_f << g << endl; }
    }
    for (auto e : android_vers) { out_f << e << endl; }
    for (int j = 0; j < 8192 / 16; j++) {
        for (int i = 0; i < 4096; i++) {
            string line;
            int index = i % 7;
            line = line + manu[index] + " ";
            int start = i % 16;
            int end = (i + 4) % 16;
            assert(start != end);
            vector<string> i_phone;
            if (start <= end)
                i_phone.assign(phone[index].begin() + start, phone[index].begin() + end);
            else
                i_phone.assign(phone[index].begin() + end, phone[index].begin() + start);
            int x = get_random(0, 3);
            // cout << i_phone.size() << endl;
            assert(x < i_phone.size());
            line = line + i_phone[x] + " ";
            if (i == 0) { cout << i_phone[x] << endl; }
            x = get_random(0, 7);
            line = line + android_vers[x];
            // cout << line << endl;
            output_f << line << endl;
            line.clear();
        }
    }

    output_f.close();
}

// The i-th row indicates the keywords of the device with lid = 1.
void gen_lid_keywords(vector<string> keywords) {
    ofstream out_f("./lid_keywords");
    vector<int> temp;
    for (int i = 0; i < keywords.size(); i++) { temp.push_back(i); }
    unsigned int line_num = 1;
    string line;
    while (line_num <= LG_NUM) {
        random_shuffle(temp.begin(), temp.end());
        for (int i = 0; i < 16; i++) {
            line += keywords[temp[i]];
            line += ",";
        }
        out_f << line << endl;
        line_num++;
        line.clear();
    }
}

void read_lid_keywords(vector<vector<string>> &keys) {
    ifstream input_f("./lid_keywords");
    string line;
    unsigned int line_number = 1;
    while (getline(input_f, line)) {
        if (line_number <= LG_NUM) {
            vector<string> v = split(line, ",");
            keys.push_back(v);
        } else {
            break;
        }
        line_number++;
    }
}
// Modify the entry file
void add_keyword_to_entry(string inputFile, string outputFile, vector<string> keywords, unsigned int num_per_line) {
    ifstream input_f(inputFile);
    ofstream output_f(outputFile);
    string line;
    string log;
    int k = 1;
    vector<int> temp;
    for (int i = 0; i < keywords.size(); i++) { temp.push_back(i); }
    while (getline(input_f, line)) {
        // if (k >= ALL_LOG_NUM)
        // {
        //     break;
        // }
        log += "[";
        random_shuffle(temp.begin(), temp.end());
        for (int i = 0; i < num_per_line; i++) {
            int index = temp[i];
            log = log + keywords[index] + ",";
        }
        log = log + "] ";
        k++;
        log += line;
        output_f << log << endl;
        log.clear();
    }
    input_f.close();
    output_f.close();
}

// Generate common keywords and save them in files
void add_tx_keywords_to_file(string key_file, vector<string> keywords, unsigned int num_per_line,
                             unsigned int all_line) {
    ofstream out_f(key_file, ios::app);
    string line;
    vector<int> temp;
    vector<vector<string>> lid_keys;
    read_lid_keywords(lid_keys);
    int lid = 1;
    for (int i = 0; i < lid_keys[lid - 1].size(); i++) { temp.push_back(i); }
    unsigned int line_num = 1;
    while (line_num <= all_line) {
        random_shuffle(temp.begin(), temp.end());
        for (int i = 0; i < num_per_line; i++) {
            line += lid_keys[lid - 1][temp[i]];
            line += ",";
        }
        out_f << line << endl;
        line_num++;
        line.clear();
        lid++;
        if (lid > LG_NUM) { lid = 1; }
    }
    out_f.close();
}

// Read common keywords from the file, one line at a time
void read_tx_keywords(vector<vector<string>> &keys, string key_file, unsigned int line_n) {
    ifstream input_f(key_file);
    string line;
    unsigned int line_number = 0;
    while (getline(input_f, line)) {
        line_number++;
        if (line_number <= line_n) {
            vector<string> v = split(line, " ");
            keys.push_back(v);
        } else {
            break;
        }
    }
}

// Generate a transaction data set in vchain format based on the block
void gen_vchain_tx_dataset() {
    string vchain_tx_path = "./vchain_tx";
    ofstream vchain_tx(vchain_tx_path);

    unsigned int lid = 1;

    string blocks_path = "./db/reuse/blocks";

    vector<FullBlock *> blocks;

    for (int i = 1; i <= 256; i++) {
        string block_path = blocks_path + "/block_" + to_string(i);
        load_from_file(blocks, block_path);
        for (int j = 0; j < blocks.size(); j++) {
            for (auto t : blocks[j]->txs) {
                Transaction tx = Transaction::fromString(t);
                string line = "";
                // Block number
                line = line + to_string((i - 1) * 32 + j + 1) + " ";
                // time range
                line = line + "[" + to_string(tx.t_min) + ", " + to_string(tx.t_max) + "] ";
                // keywords
                line = line + "{";
                // Lid is the keyword, put it first
                line = line + "'" + to_string(lid) + "'" + ", ";
                // line = line + to_string(lid) + ", ";
                lid++;
                if (lid > LG_NUM) { lid = 1; }
                for (int k = 0; k < tx.tx_public_keys.size(); k++) {
                    string kk = tx.tx_public_keys[k];
                    if (k != tx.tx_public_keys.size() - 1)
                        line = line + "'" + kk + "'" + ", ";
                    else { line = line + "'" + kk + "'" + "}"; }
                }
                vchain_tx << line << endl;
            }
        }
        blocks.clear();
    }
}

void gen_vchain_entry_dataset() {
    string lf_path = "./db/reuse/lfs/lfs_0";
    LogFile *lf = new LogFile();
    load_from_file(lf, lf_path);
    string vchain_entry_path = "./vchain_entry";
    ofstream vchain_entry(vchain_entry_path);
    vector<Entry> all_entrys;
    for (int i = 0; i < lf->entrys.size(); i++) { all_entrys.push_back(Entry::fromString(lf->entrys[i])); }
    for (auto e : all_entrys) {
        string line = "1 ";
        line = line + "[" + to_string(e.timestamp) + "] ";
        line = line + "{ ";
        for (int k = 0; k < e.keywords.size(); k++) {
            string kk = e.keywords[k];
            if (k != e.keywords.size() - 1)
                line = line + "'" + kk + "'" + ", ";
            else { line = line + "'" + kk + "'" + "}"; }
        }
        vchain_entry << line << endl;
    }
}
// Convert string to hexadecimal
string string2Hex(string s) {
    string hex = "";
    for (int i = 0; i != s.size(); i++) {
        unsigned char ch = s[i];
        unsigned char more = ch / 16;
        unsigned char less = ch % 16;
        switch (more) {
        case 0: hex += "0"; break;
        case 1: hex += "1"; break;
        case 2: hex += "2"; break;
        case 3: hex += "3"; break;
        case 4: hex += "4"; break;
        case 5: hex += "5"; break;
        case 6: hex += "6"; break;
        case 7: hex += "7"; break;
        case 8: hex += "8"; break;
        case 9: hex += "9"; break;
        case 10: hex += "a"; break;
        case 11: hex += "b"; break;
        case 12: hex += "c"; break;
        case 13: hex += "d"; break;
        case 14: hex += "e"; break;
        case 15: hex += "f"; break;
        default: cout << "The program should not be here. Check stringToHex().\n"; exit(1);
        }
        switch (less) {
        case 0: hex += "0"; break;
        case 1: hex += "1"; break;
        case 2: hex += "2"; break;
        case 3: hex += "3"; break;
        case 4: hex += "4"; break;
        case 5: hex += "5"; break;
        case 6: hex += "6"; break;
        case 7: hex += "7"; break;
        case 8: hex += "8"; break;
        case 9: hex += "9"; break;
        case 10: hex += "a"; break;
        case 11: hex += "b"; break;
        case 12: hex += "c"; break;
        case 13: hex += "d"; break;
        case 14: hex += "e"; break;
        case 15: hex += "f"; break;
        default: cout << "The program should not be here. Check stringToHex().\n"; exit(1);
        }
    }
    return hex;
}
// Hexadecimal to string
string hex2String(string hex) {
    if (hex.length() == 0 || hex.length() % 2 == 1) { cout << "The Program Should NOT Go Here.\n"; }
    string s;
    s.resize(hex.length() / 2);
    int index = 0;
    char more, less;
    while (index < hex.length()) {
        more = hex[index];
        less = hex[index + 1];
        index += 2;
        unsigned char ch;
        switch (more) {
        case '0': ch = 0; break;
        case '1': ch = 16; break;
        case '2': ch = 32; break;
        case '3': ch = 48; break;
        case '4': ch = 64; break;
        case '5': ch = 80; break;
        case '6': ch = 96; break;
        case '7': ch = 112; break;
        case '8': ch = 128; break;
        case '9': ch = 144; break;
        case 'a':
        case 'A': ch = 160; break;
        case 'b':
        case 'B': ch = 176; break;
        case 'c':
        case 'C': ch = 192; break;
        case 'd':
        case 'D': ch = 208; break;
        case 'e':
        case 'E': ch = 224; break;
        case 'f':
        case 'F': ch = 240; break;
        default: cout << "The Program Should NOT Go Here!\n"; exit(1);
        }
        switch (less) {
        case '0': ch += 0; break;
        case '1': ch += 1; break;
        case '2': ch += 2; break;
        case '3': ch += 3; break;
        case '4': ch += 4; break;
        case '5': ch += 5; break;
        case '6': ch += 6; break;
        case '7': ch += 7; break;
        case '8': ch += 8; break;
        case '9': ch += 9; break;
        case 'a':
        case 'A': ch += 10; break;
        case 'b':
        case 'B': ch += 11; break;
        case 'c':
        case 'C': ch += 12; break;
        case 'd':
        case 'D': ch += 13; break;
        case 'e':
        case 'E': ch += 14; break;
        case 'f':
        case 'F': ch += 15; break;
        default: cout << "The Program Should NOT Go Here!\n"; exit(1);
        }
        s[index / 2 - 1] = ch;
    }
    return s;
}