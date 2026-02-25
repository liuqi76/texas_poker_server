/*
这是5张牌组合素数积id-牌型排名预计算器

已从输出.txt转为输出.bin，便于初始化读取加速
*/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>

// 52张牌编号：i/4 = 点数(0-12, 0=2, 12=A), i%4 = 花色(0-3)
// 每张牌对应唯一素数，花色信息编码在素数本身中
static constexpr long long oddList[52] = {
    2,   3,   5,   7,   // 2  (花色0-3)
    11,  13,  17,  19,  // 3
    23,  29,  31,  37,  // 4
    41,  43,  47,  53,  // 5
    59,  61,  67,  71,  // 6
    73,  79,  83,  89,  // 7
    97,  101, 103, 107, // 8
    109, 113, 127, 131, // 9
    137, 139, 149, 151, // 10
    157, 163, 167, 173, // J
    179, 181, 191, 193, // Q
    197, 199, 211, 223, // K
    227, 229, 233, 239  // A
};

bool isStraight(int a, int b, int c, int d, int e) {
    std::vector<int> ranks = {a, b, c, d, e};
    std::sort(ranks.begin(), ranks.end());
    if (ranks[0]==0 && ranks[1]==1 && ranks[2]==2 && ranks[3]==3 && ranks[4]==12)
        return true;
    for (int i = 0; i < 4; i++)
        if (ranks[i] != ranks[i+1] - 1) return false;
    return true;
}

struct HandStrength {
    int category;
    std::vector<int> tiebreakers;

    bool operator<(const HandStrength& o) const {
        if (category != o.category) return category < o.category;
        return tiebreakers < o.tiebreakers;
    }
    bool operator==(const HandStrength& o) const {
        return category == o.category && tiebreakers == o.tiebreakers;
    }
};

HandStrength evaluate(int a, int b, int c, int d, int e) {
    int ra = a/4, rb = b/4, rc = c/4, rd = d/4, re = e/4;
    int fa = a%4, fb = b%4, fc = c%4, fd = d%4, fe = e%4;

    bool flush = (fa==fb && fb==fc && fc==fd && fd==fe);

    std::map<int,int> cnt;
    cnt[ra]++; cnt[rb]++; cnt[rc]++; cnt[rd]++; cnt[re]++;

    std::vector<int> ranks = {ra, rb, rc, rd, re};
    std::sort(ranks.begin(), ranks.end());
    bool straight = isStraight(ra, rb, rc, rd, re);

    std::vector<std::pair<int,int>> groups;
    for (auto& kv : cnt) groups.push_back({kv.second, kv.first});
    std::sort(groups.begin(), groups.end(), [](const std::pair<int,int>& x, const std::pair<int,int>& y){
        if (x.first != y.first) return x.first > y.first;
        return x.second > y.second;
    });

    std::vector<int> tb;
    for (auto& g : groups)
        for (int i = 0; i < g.first; i++)
            tb.push_back(g.second);

    auto straight_tb = [&]() -> std::vector<int> {
        if (ranks[0]==0 && ranks[1]==1 && ranks[2]==2 && ranks[3]==3 && ranks[4]==12)
            return {3};
        return {ranks[4]};
    };

    if (flush && straight) return {9, straight_tb()};
    if (groups[0].first == 4) return {8, tb};
    if (groups[0].first == 3 && groups[1].first == 2) return {7, tb};
    if (flush) return {6, {ranks[4], ranks[3], ranks[2], ranks[1], ranks[0]}};
    if (straight) return {5, straight_tb()};
    if (groups[0].first == 3) return {4, tb};
    if (groups[0].first == 2 && groups[1].first == 2) return {3, tb};
    if (groups[0].first == 2) return {2, tb};
    return {1, {ranks[4], ranks[3], ranks[2], ranks[1], ranks[0]}};
}

// 写入二进制文件的条目结构（12字节，按key排序后写入，服务器侧二分查找）
struct BinEntry {
    long long key;  // 8字节
    int rank;       // 4字节
};

int main() {
    std::cout << "Start generating(It will cost some time, please wait)..." << std::endl;

    struct Entry {
        long long key;
        HandStrength strength;
    };
    std::vector<Entry> entries;
    entries.reserve(2598960);

    for (int a = 0;  a < 52; a++)
    for (int b = a+1; b < 52; b++)
    for (int c = b+1; c < 52; c++)
    for (int d = c+1; d < 52; d++)
    for (int e = d+1; e < 52; e++) {
        long long key = oddList[a]*oddList[b]*oddList[c]*oddList[d]*oddList[e];
        entries.push_back({key, evaluate(a, b, c, d, e)});
    }

    std::cout << "Sorting and ranking..." << std::endl;

    std::vector<HandStrength> unique_strengths;
    unique_strengths.reserve(entries.size());
    for (auto& entry : entries)
        unique_strengths.push_back(entry.strength);
    std::sort(unique_strengths.begin(), unique_strengths.end());
    unique_strengths.erase(
        std::unique(unique_strengths.begin(), unique_strengths.end()),
        unique_strengths.end()
    );

    std::map<std::vector<int>, int> rank_map;
    for (int i = 0; i < (int)unique_strengths.size(); i++) {
        auto& s = unique_strengths[i];
        std::vector<int> k;
        k.push_back(s.category);
        for (int v : s.tiebreakers) k.push_back(v);
        rank_map[k] = i + 1;
    }

    std::cout << "Writing table.bin..." << std::endl;

    // 构造 BinEntry，按 key 排序，顺序写入二进制文件
    // 服务器侧：fread 一次载入，std::lower_bound 二分查找，O(log n)
    std::vector<BinEntry> bin_entries;
    bin_entries.reserve(entries.size());
    for (auto& entry : entries) {
        std::vector<int> k;
        k.push_back(entry.strength.category);
        for (int v : entry.strength.tiebreakers) k.push_back(v);
        bin_entries.push_back({entry.key, rank_map[k]});
    }
    std::sort(bin_entries.begin(), bin_entries.end(),
        [](const BinEntry& a, const BinEntry& b){ return a.key < b.key; });

    std::ofstream outFile("table.bin", std::ios::binary);
    outFile.write((char*)bin_entries.data(), (long long)bin_entries.size() * sizeof(BinEntry));
    outFile.close();

    std::cout << "Done." << std::endl;
    return 0;
}