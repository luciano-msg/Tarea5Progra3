#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <queue>
#include <stack>
#include <set>
#include <stdexcept>

class Node {
    int id;
    std::vector<Node*> neighbors;
public:
    explicit Node(int id) : id(id) {}
    int getId() const { return id; }
    friend class Graph;
    friend class BFSIterator;
    friend class DFSIterator;
};

class Graph {
    std::map<int, std::unique_ptr<Node>> nodes;

    Node* getOrCreate(int id) {
        auto& slot = nodes[id];
        if (!slot) slot = std::make_unique<Node>(id);
        return slot.get();
    }

    Node* find(int id) const {
        auto it = nodes.find(id);
        if (it == nodes.end()) throw std::invalid_argument("Nodo inexistente: " + std::to_string(id));
        return it->second.get();
    }
public:
    void addNode(int id) { getOrCreate(id); }

    void addEdge(int from, int to) {
        Node* a = getOrCreate(from);
        Node* b = getOrCreate(to);
        a->neighbors.push_back(b);
    }

    std::size_t size() const { return nodes.size(); }

    friend class BFSIterator;
    friend class DFSIterator;
};

class GraphIterator {
public:
    virtual ~GraphIterator() = default;
    virtual bool hasNext() const = 0;
    virtual int next() = 0;
    virtual void reset() = 0;
};

class BFSIterator : public GraphIterator {
    Node* start;
    std::queue<Node*> pending;
    std::set<int> visited;
public:
    BFSIterator(const Graph& g, int startId) : start(g.find(startId)) { reset(); }

    void reset() override {
        pending = {};
        visited.clear();
        pending.push(start);
        visited.insert(start->id);
    }

    bool hasNext() const override { return !pending.empty(); }

    int next() override {
        if (!hasNext()) throw std::out_of_range("BFSIterator agotado");
        Node* cur = pending.front();
        pending.pop();
        for (Node* n : cur->neighbors) {
            if (visited.insert(n->id).second) pending.push(n);
        }
        return cur->id;
    }
};

class DFSIterator : public GraphIterator {
    Node* start;
    std::stack<Node*> pending;
    std::set<int> visited;

    // Descarta de la cima los nodos ya visitados para que hasNext sea exacto
    void skipVisited() {
        while (!pending.empty() && visited.count(pending.top()->id)) pending.pop();
    }
public:
    DFSIterator(const Graph& g, int startId) : start(g.find(startId)) { reset(); }

    void reset() override {
        pending = {};
        visited.clear();
        pending.push(start);
    }

    bool hasNext() const override { return !pending.empty(); }

    int next() override {
        if (!hasNext()) throw std::out_of_range("DFSIterator agotado");
        Node* cur = pending.top();
        pending.pop();
        visited.insert(cur->id);
        // En orden inverso para visitar los vecinos en el orden en que se agregaron
        for (auto it = cur->neighbors.rbegin(); it != cur->neighbors.rend(); ++it) {
            if (!visited.count((*it)->id)) pending.push(*it);
        }
        skipVisited();
        return cur->id;
    }
};

void traverse(const std::string& label, GraphIterator& it) {
    std::cout << label << ": ";
    while (it.hasNext()) std::cout << it.next() << " ";
    std::cout << std::endl;
}

int main() {
    Graph g;
    g.addEdge(1,2);
    g.addEdge(1,3);
    g.addEdge(2,4);

    std::unique_ptr<GraphIterator> it =
        std::make_unique<BFSIterator>(g, 1);

    while(it->hasNext()) {
        std::cout << it->next() << " ";
    }
    std::cout << std::endl;

    // Cambio dinámico de estrategia sobre el mismo puntero
    it = std::make_unique<DFSIterator>(g, 1);
    traverse("DFS", *it);

    // Grafo con ciclos: no debe haber bucles infinitos
    Graph c;
    c.addEdge(1,2);
    c.addEdge(1,3);
    c.addEdge(2,4);
    c.addEdge(3,4);
    c.addEdge(4,5);
    c.addEdge(4,1);
    c.addEdge(5,2);
    c.addEdge(5,5);
    c.addNode(99);   // nodo aislado, no alcanzable

    std::vector<std::unique_ptr<GraphIterator>> strategies;
    strategies.push_back(std::make_unique<BFSIterator>(c, 1));
    strategies.push_back(std::make_unique<DFSIterator>(c, 1));

    const char* names[] = {"BFS con ciclos", "DFS con ciclos"};
    for (std::size_t i = 0; i < strategies.size(); ++i) traverse(names[i], *strategies[i]);

    // reset permite volver a recorrer
    strategies[1]->reset();
    traverse("DFS tras reset", *strategies[1]);

    traverse("BFS desde 4", *std::make_unique<BFSIterator>(c, 4));

    try {
        BFSIterator bad(c, 42);
    } catch (const std::exception& e) {
        std::cout << "Error esperado: " << e.what() << std::endl;
    }
}
