
#ifndef UDYNGRAPHMEMEFF_H_
#define UDYNGRAPHMEMEFF_H_

#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace std;

// O(deg(v)) update time per add/rem.

class UDynGraph {
public:
    // Returns true if the edge is added.
    bool add_edge(const int u, const int v);
    void remove_node(const int u);
    // Returns true if the edge is added.
    bool remove_edge(const int u, const int v);
    void neighbors(const int u, vector<int>* vec) const;
    int degree(const int u) const;
    void edges(vector<pair<int, int> >* vec) const;
    void nodes(vector<int>* vec) const;
    void clear();
    int num_nodes() const;
    int num_edges() const;

    UDynGraph();
    virtual ~UDynGraph();
private:
    unordered_map<int, vector<int> > node_map_;
    unordered_set<int> nodes_;

    int num_nodes_;
    int num_edges_;
};

#endif /* UDYNGRAPHMEMEFF_H_ */
