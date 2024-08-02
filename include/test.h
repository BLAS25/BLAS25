
void get_test_blocks(vector<FullBlock *> &test_blocks);
void get_test_lfs(vector<LogFile *> &lfs);
void test_tx_and(const vector<FullBlock *> &test_blocks);
void test_tx_or(const vector<FullBlock *> &test_blocks);
void test_tx_not(const vector<FullBlock *> &test_blocks);
void test_tx_range(const vector<FullBlock *> &test_blocks);
void test_entry_and(vector<LogFile *> &test_lfs);
void test_entry_or(vector<LogFile *> &test_lfs);
void test_entry_not(vector<LogFile *> &test_lfs);
void test_entry_range(vector<LogFile *> &test_lfs);