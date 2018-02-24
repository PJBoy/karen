#include "utility/array.h"
#include "utility/mismatches.h"
#include "utility/string.h"
#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>


class SuffixTree
{
public:
    struct Node
    {
        unsigned start, end;
        std::unique_ptr<Node> edges[ALPHABET_SIZE]{};
        Node* suffixLink{nullptr};

        Node() = default;

        Node(unsigned start, unsigned end)
            : start(start), end(end)
        {}

        unsigned edge_length() const
        {
            return end - start;
        }

        unsigned edge_length(unsigned pos) const
        {
            return std::min(end, pos + 1) - start;
        }
    };

private:
    void add_SL(Node*& suffixLinkSource, Node* node) const
    {
        if (suffixLinkSource != nullptr)
            suffixLinkSource->suffixLink = node;
        suffixLinkSource = node;
    }

    std::unique_ptr<Node> newEdge(unsigned begin, unsigned end)
    {
        ++n_nodes;
        return std::make_unique<Node>(begin, end);
    }

public:
    String string;
    std::unique_ptr<Node> root{std::make_unique<Node>(0, 0)};
    unsigned n_nodes{1};

    SuffixTree() = default;

    // Expect the caller to add the sentinel
    SuffixTree(const String& string)
        : string(string)
    {
        // Ukkonen's algorithm //

        Node* active_node(root.get());

        unsigned
            active_length = 0,
            remainder = 0,
            pos = 0;

        const char* active_edge = &string[pos];

        for (unsigned char character : string)
        {
            Node* suffixLinkSource(nullptr);
            ++remainder;

            while (remainder != 0)
            {
                if (active_length == 0)
                    active_edge = &string[pos];

                std::unique_ptr<Node>& edge(active_node->edges[*active_edge]);
                if (edge == nullptr)
                {
                    // If character is not an edge of the active node, add it to the tree
                    edge = newEdge(pos, std::size(string));
                    add_SL(suffixLinkSource, active_node);
                }
                else
                {
                    // Else character is on the edge of the active node, so move active point and delay insertion
                    
                    // If active point is beyond this edge, go to next node and start again
                    if (active_length >= edge->edge_length(pos))
                    {
                        active_edge += edge->edge_length(pos);
                        active_length -= edge->edge_length(pos);
                        active_node = edge.get();
                        continue;
                    }

                    // Active point matches the character, so increase suffix length and move on to next character
                    if (string[edge->start + active_length] == character)
                    {
                        active_length++;
                        add_SL(suffixLinkSource, active_node);
                        break;
                    }

                    // Active point doesn't match character, so split the tree here
                    // Add the active point to one branch, the new suffix into the other
                    
                    // The part of the edge before the split
                    std::unique_ptr<Node> split(newEdge(edge->start, edge->start + active_length));
                    
                    // The part of the existing edge after the split
                    edge->start += active_length;
                    split->edges[string[edge->start]] = std::move(edge);
                    
                    // The part of the new edge after the split
                    split->edges[character] = newEdge(pos, std::size(string));
                    add_SL(suffixLinkSource, split.get());

                    // Replace edge from active node with the shortened edge
                    edge = std::move(split);
                }

                --remainder;

                // If active point is on edge from root, move to next character in suffix
                if (active_node == root.get() && active_length != 0)
                {
                    --active_length;
                    active_edge = &string[pos - remainder + 1];
                    continue;
                }

                // Set active node to suffix link if it exists
                if (active_node->suffixLink != nullptr)
                    active_node = active_node->suffixLink;
                else
                    active_node = root.get();
            }

            ++pos;
        }
    }

    friend std::ostream& operator<<(std::ostream& stream, const SuffixTree& suffixTree)
    {
        suffixTree.debugPrint(stream);
        return stream;
    }

    void debugPrint(std::ostream& stream = std::cout, const Node* node = nullptr, unsigned depth = 0) const
    {
        if (node == nullptr)
            node = root.get();

        std::string indent;
        if (depth != 0)
        {
            for (unsigned i(depth - 1); i; --i)
                indent += "|   ";
            indent += "|___";
        }
        std::string suffix(std::cbegin(string) + node->start, std::cbegin(string) + node->end);
        std::replace(std::begin(suffix), std::end(suffix), '\0', '$');
        std::cout << indent << suffix << '\n';
        
        for (const std::unique_ptr<Node>& edge : node->edges)
            if (edge != nullptr)
                debugPrint(stream, edge.get(), depth + 1);
    }
};


class RMQ
{
    // The difference-of-one special case of RMQ
    unsigned n, n_bits;

    Array<unsigned> data;
    Array<unsigned> d;
    MultiArray<unsigned> RMQ_small;
    MultiArray<unsigned> RMQ_d;

public:
    RMQ() = default;

    RMQ(const Array<unsigned>& data_in)
    {
        // Requires n >= 2
        n = std::size(data_in);
        n_bits = unsigned(std::log2(n)) / 2;
        data = Array<unsigned>(n);
        std::copy(std::cbegin(data_in), std::cend(data_in), std::begin(data));

        // Precompute the RMQ for all possible values of d and all possible queries
        const unsigned n_values = 1u << n_bits;
        RMQ_small = MultiArray<unsigned>{n_values, n_bits + 1, n_bits + 1};
        for (unsigned i_d = 0; i_d < n_values; ++i_d)
            for (unsigned i_l = 0; i_l <= n_bits; ++i_l)
            {
                unsigned i_min = i_l;
                signed value = 0, min = 0;
                RMQ_small[{i_d, i_l, i_l}] = i_min;
                for (unsigned i_r = i_l + 1; i_r <= n_bits; ++i_r)
                {
                    const bool bit = i_d >> i_r - 1 & 1;
                    value += bit * 2 - 1;
                    if (value < min)
                    {
                        min = value;
                        i_min = i_r;
                    }
                    RMQ_small[{i_d, i_l, i_r}] = i_min;
                }
            }

        // Construct d the array of adjacent differences for each unit (negative difference = 0, positive = 1)
        // Care is taken for the last unit that may have a size less than the others

        const unsigned
            n_units = n / n_bits,
            n_last = n % n_bits,
            n_d = n_units + (n_last != 0),
            n_y = unsigned(std::log2(n_d)) + 1;
        
        d = Array<unsigned>(n_d);
        RMQ_d = MultiArray<unsigned>{n_y, n_d};

        for (unsigned i = 0; i < n_units; ++i)
        {
            d[i] = 0;
            for (unsigned ii(0); ii < n_bits - 1; ++ii)
                d[i] |= (signed(data[i * n_bits + ii + 1] - data[i * n_bits + ii]) > 0) << ii;
            RMQ_d[{0, i}] = i * n_bits + RMQ_small[{d[i], 0, n_bits - 1}];
        }
        if (n_last != 0)
        {
            d[n_units] = 0;
            for (unsigned ii(0); ii < n_last - 1; ++ii)
                d[n_units] |= (signed(data[n_units * n_bits + ii + 1] - data[n_units * n_bits + ii]) > 0) << ii;
            RMQ_d[{0, n_units}] = n_units * n_bits + RMQ_small[{d[n_units], 0, n_last - 1}];
        }

        // Precompute the RMQs for d
        /*
            for y = 0 up to log_2(n) - 1:
                for x = 0 up to n - 2^y:
                    R_{y+1}[x] = argmin(data[R_y(x)], data[R_y(x+2^y)])

            R = [[0 for i in range(n + 1 - 2**y)] for y in range(l+1)]
            R[0] = d
            for y in range(l):
                for x in range(n + 1 - 2**(y+1)):
                        R[y+1][x] = R[y][x] if data[R[y][x]] < data[R[y][x+2**y]] else R[y][x+2**y]
        */

        for (unsigned y = 0; y < n_y - 1; ++y)
            for (unsigned x = 0; x <= n_d - (1 << y + 1); ++x)
                if (data[RMQ_d[{y, x}]] < data[RMQ_d[{y, x + (1 << y)}]])
                    RMQ_d[{y + 1, x}] = RMQ_d[{y, x}];
                else
                    RMQ_d[{y + 1, x}] = RMQ_d[{y, x + (1 << y)}];
    }

    unsigned operator()(unsigned i_l, unsigned i_r) const
    {
        std::tie(i_l, i_r) = std::minmax({i_l, i_r});
        ++i_r; // Transform the inclusive interval [i_l, i_r] -> exclusive [i_l, i_r + 1)

        const unsigned
            i_l_d = (i_l + n_bits - 1) / n_bits,
            i_r_d = i_r / n_bits;

        unsigned i_min = i_l;

        // Minimum in i_l_d * n_bits <= min < i_r_d * n_bits
        if (i_l_d < i_r_d)
        {
            const unsigned l = unsigned(std::log2(i_r_d - i_l_d));

            // Minimum in i_l_d * n_bits <= min < (i_l_d + 2^l) * n_bits
            unsigned i = RMQ_d[{l, i_l_d}];
            if (data[i] < data[i_min])
                i_min = i;

            // Minimum in i_r_d * n_bits - 2^l <= min < i_r_d * n_bits
            i = RMQ_d[{l, i_r_d - (1 << l)}];
            if (data[i] < data[i_min])
                i_min = i;
        }

        const unsigned
            i_l_small = i_l % n_bits,
            i_r_small = i_r % n_bits;

        if (i_r_d < i_l_d)
        {
            const unsigned i = i_r_d*n_bits + RMQ_small[{d[i_r_d], i_l_small, i_r_small}];
            if (data[i] < data[i_min])
                i_min = i;
        }
        else
        {
            if (i_l_d * n_bits != i_l)
            {
                const unsigned i = (i_l_d - 1)*n_bits + RMQ_small[{d[i_l_d - 1], i_l_small, n_bits - 1}];
                if (data[i] < data[i_min])
                    i_min = i;
            }
            if (i_r_d * n_bits != i_r)
            {
                const unsigned i = i_r_d*n_bits + RMQ_small[{d[i_r_d], 0, i_r_small}];
                if (data[i] < data[i_min])
                    i_min = i;
            }
        }

        return i_min;
    }
};


class LCA
{
    // Array of nodes and the depth of the node in the tree in a depth first traversal of the tree
    Array<unsigned> N, D;

    // Map from node (a value in N) to an index in N; basically N^{-1}
    Array<unsigned> I;

    // Map from node to length of prefix of suffix (length of suffix for leaves)
    Array<unsigned> lengths;

    // Map from index of suffix to node
    Array<unsigned> leaves;

    RMQ rmq;

    void depthFirstTraversal(const SuffixTree::Node* node, unsigned depth = 0, unsigned length = 0)
    {
        const unsigned nodeId = lengths.back_i();
        N.push_back(nodeId);
        D.push_back(depth);
        lengths.push_back(length);

        bool isLeaf(true);
        for (const std::unique_ptr<SuffixTree::Node>& edge : node->edges)
            if (edge != nullptr)
            {
                isLeaf = false;
                depthFirstTraversal(edge.get(), depth + 1, length + edge->edge_length());
                N.push_back(nodeId);
                D.push_back(depth);
            }

        if (isLeaf)
        {
            const unsigned suffixIndex(std::size(leaves) - length);
            leaves[suffixIndex] = nodeId;
        }
    }

public:
    LCA() = default;

    LCA(const SuffixTree& tree)
    {
        /*
            Construct arrays N and D from an Eulerian tour of the tree.
            D[i] is the depth of node N[i] at point i of the tour.
            Constuct an array I such that I[i] = j where i = N[j] for some j.
            Preprocess D for range minimum queries.
        */

        const unsigned n = tree.n_nodes;

        lengths = Array<unsigned>(n);
        N = Array<unsigned>(n * 2 - 1);
        D = Array<unsigned>(n * 2 - 1);
        I = Array<unsigned>(n);
        leaves = Array<unsigned>(std::size(tree.string));

        depthFirstTraversal(tree.root.get());

        for (unsigned i = 0; i < std::size(N); ++i)
            I[N[i]] = i;

        // Preprocess D for range minimum queries
        rmq = RMQ(D);
    }

    unsigned operator()(unsigned i_l, unsigned i_r) const
    {
        return lengths[N[rmq(I[leaves[i_l]], I[leaves[i_r]])]];
    }
};


class LCP
{
    Array<unsigned> lcp;
    LCA lca;
    unsigned n_P, n_T;
    std::string string;

public:
    LCP(const String& P, const String& T)
    {
        n_P = std::size(P);
        n_T = std::size(T);

        // Get the concatenation of the strings with terminator symbol
        const unsigned n = n_P + n_T + 1;

        string.reserve(n);
        string.append(std::cbegin(P), std::cend(P));
        string.append(std::cbegin(T), std::cend(T));
        string.push_back('\0');

        // Process for LCA...
        lca = LCA(SuffixTree(string));
    }

    unsigned operator()(unsigned i_P, unsigned i_T) const
    {
        assert("LCP::operator(): i_P >= n_P || i_T >= n_T" && i_P < n_P && i_T < n_T);
        return lca(i_P, n_P + i_T);
    }
};


// Landau-Vishkin k-mismatch
Mismatches minKangaroo(unsigned k, const String& P, const String& T)
{
    /*
        Preprocessing T and P for LCP queries is preprocessing the LCA of the suffix tree of T concatenated with P.

        Preprocess T and P for LCP queries
        for i = 0 up to n - m:
            Let s = 0 be the number of mismatches
            j = 0
            while j < m and s <= k:
                j += LCP(i+j, j) + 1
                if j < m:
                    s += 1
            if s <= k:
                Ham_k(i) = s
            else
                Ham_k(i) = X
    */

    const unsigned
        m = std::size(P),
        n = std::size(T);

    if (n < m)
        return Mismatches{};

    LCP lcp(P, T);
    Mismatches minMismatches(k, k + 1);
    
    for (unsigned i = 0; i < n - m + 1; ++i)
    {
        unsigned mismatches = 0;
        for (unsigned j = 0; j < m;)
        {
            // A good optimisation here would be to check if the next two characters mismatch and only otherwise do the lcp query
            j += lcp(j, i + j) + 1;
            if (j <= m)
            {
                ++mismatches;
                if (mismatches > k)
                    break;
            }
        }

        minMismatches = std::min(minMismatches, Mismatches(k, mismatches));
    }

    return minMismatches;
}
