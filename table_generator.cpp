/*
这是5张牌组合素数积id-牌型排名预计算器
*/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>

// 52张牌编号：i/4 = 点数(0-12, 0=2, 12=A), i%4 = 花色(0-3)
// 每张牌对应唯一素数，花色信息编码在素数本身中，无需额外判断
static constexpr long long oddList[52] = {
    2,   3,   5,   7,   // 2  (????)
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
    // 传入的是点数(0-12)
    std::vector<int> ranks = {a, b, c, d, e};
    std::sort(ranks.begin(), ranks.end());
    // 特判 A2345: 点数为 0,1,2,3,12
    if (ranks[0]==0 && ranks[1]==1 && ranks[2]==2 && ranks[3]==3 && ranks[4]==12)
        return true;
    for (int i = 0; i < 4; i++)
        if (ranks[i] != ranks[i+1] - 1) return false;
    return true;
}

// 牌型强度结构，用于排序
// 数字越大越强，与服务器侧 rank5in7 的 > 逻辑对齐
struct HandStrength {
    int category;       // 牌型大类 1-9
    std::vector<int> tiebreakers; // 用于同牌型内比较，从高到低

    bool operator<(const HandStrength& o) const {
        if (category != o.category) return category < o.category;
        return tiebreakers < o.tiebreakers;
    }
    bool operator==(const HandStrength& o) const {
        return category == o.category && tiebreakers == o.tiebreakers;
    }
};

HandStrength evaluate(int a, int b, int c, int d, int e) {
    // a~e 是牌编号(0-51)
    int ra = a/4, rb = b/4, rc = c/4, rd = d/4, re = e/4; // 点数
    int fa = a%4, fb = b%4, fc = c%4, fd = d%4, fe = e%4; // 花色

    bool flush = (fa == fb && fb == fc && fc == fd && fd == fe);

    // 统计各点数出现次数
    std::map<int, int> cnt;
    cnt[ra]++; cnt[rb]++; cnt[rc]++; cnt[rd]++; cnt[re]++;

    std::vector<int> ranks = {ra, rb, rc, rd, re};
    std::sort(ranks.begin(), ranks.end());
    bool straight = isStraight(ra, rb, rc, rd, re);

    // 按出现次数分组，用于 tiebreaker
    // groups: 按(次数desc, 点数desc)排序
    std::vector<std::pair<int,int>> groups; // (count, rank)
    for (auto& kv : cnt) groups.push_back({kv.second, kv.first});
    std::sort(groups.begin(), groups.end(), [](const std::pair<int,int>& x, const std::pair<int,int>& y){
        if (x.first != y.first) return x.first > y.first;
        return x.second > y.second;
    });

    // tiebreaker串：按groups顺序展开点数
    std::vector<int> tb;
    for (auto& g : groups)
        for (int i = 0; i < g.first; i++)
            tb.push_back(g.second);

    // 顺子的 tiebreaker 用最高牌（A2345特判最高牌为3）
    auto straight_tb = [&]() -> std::vector<int> {
        if (ranks[0]==0 && ranks[1]==1 && ranks[2]==2 && ranks[3]==3 && ranks[4]==12)
            return {3};
        return {ranks[4]};
    };

    if (flush && straight) return {9, straight_tb()};  // 同花顺（含皇家，排名按牌型内点数）
    if (groups[0].first == 4) return {8, tb};          // 四条
    if (groups[0].first == 3 && groups[1].first == 2) return {7, tb}; // 葫芦
    if (flush) return {6, {ranks[4], ranks[3], ranks[2], ranks[1], ranks[0]}}; // 同花
    if (straight) return {5, straight_tb()};           // 顺子
    if (groups[0].first == 3) return {4, tb};          // 三条
    if (groups[0].first == 2 && groups[1].first == 2) return {3, tb}; // 两对
    if (groups[0].first == 2) return {2, tb};          // 一对
    return {1, {ranks[4], ranks[3], ranks[2], ranks[1], ranks[0]}};   // 高牌
}

int main() {
    std::cout << "Start generating..." << std::endl;

    // 枚举所有合法5张牌组合,52选5无重复
    // 计算强度分
    struct Entry {
        long long key;        // 素数积
        HandStrength strength;
    };
    std::vector<Entry> entries;
    entries.reserve(2598960);

    for (int a = 0;  a < 52; a++)
    for (int b = a+1; b < 52; b++)
    for (int c = b+1; c < 52; c++)
    for (int d = c+1; d < 52; d++)
    for (int e = d+1; e < 52; e++) {
        long long key = oddList[a] * oddList[b] * oddList[c] * oddList[d] * oddList[e];
        HandStrength strength = evaluate(a, b, c, d, e);
        entries.push_back({key, strength});
    }

    // 收集所有不同强度并排序，分配排名
    std::vector<HandStrength> unique_strengths;
    for (auto& entry : entries)
        unique_strengths.push_back(entry.strength);
    std::sort(unique_strengths.begin(), unique_strengths.end());
    unique_strengths.erase(std::unique(unique_strengths.begin(), unique_strengths.end()), unique_strengths.end());

    // unique_strengths[0] 是最弱，排名1；最强排名为 unique_strengths.size()
    // 用map快速查排名
    std::map<std::vector<int>, int> rank_map; // 用 category+tiebreakers 做key
    for (int i = 0; i < (int)unique_strengths.size(); i++) {
        auto& s = unique_strengths[i];
        std::vector<int> k;
        k.push_back(s.category);
        for (int v : s.tiebreakers) k.push_back(v);
        rank_map[k] = i + 1; // 排名从1开始
    }

    std::cout << "Writing table..." << std::endl;

    std::ofstream outFile("table.txt");
    for (auto& entry : entries) {
        std::vector<int> k;
        k.push_back(entry.strength.category);
        for (int v : entry.strength.tiebreakers) k.push_back(v);
        int r = rank_map[k];
        outFile << entry.key << " " << r << "\n";
    }
    outFile.close();

    std::cout << "Done." << std::endl;
    return 0;
}